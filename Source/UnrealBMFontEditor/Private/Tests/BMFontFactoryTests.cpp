// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#if WITH_DEV_AUTOMATION_TESTS

#include "BMFontFactory.h"

#include "BMFontAsset.h"
#include "EditorFramework/AssetImportData.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontFactoryImportAndReimportTest,
	"UnrealBMFont.Editor.FactoryImportAndReimport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontFactoryImportAndReimportTest::RunTest(const FString& Parameters)
{
	const FString UniqueId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString TempDirectory = FPaths::Combine(
		FPaths::ProjectIntermediateDir(),
		TEXT("UnrealBMFontTests"),
		UniqueId
	);
	TestTrue(TEXT("Temporary source directory can be created"), IFileManager::Get().MakeDirectory(*TempDirectory, true));
	ON_SCOPE_EXIT
	{
		IFileManager::Get().DeleteDirectory(*TempDirectory, false, true);
	};

	const FString AtlasFilename = FPaths::Combine(TempDirectory, TEXT("atlas.png"));
	const auto WriteAtlas = [&AtlasFilename](const int32 Width, const int32 Height)
	{
		TArray<FColor> Pixels;
		Pixels.Init(FColor::White, Width * Height);
		TArray<uint8> PngBytes;
		FImageUtils::ThumbnailCompressImageArray(Width, Height, Pixels, PngBytes);
		return FFileHelper::SaveArrayToFile(PngBytes, *AtlasFilename);
	};
	if (!TestTrue(TEXT("PNG fixture can be written"), WriteAtlas(4, 4)))
	{
		return false;
	}

	const FString DescriptorFilename = FPaths::Combine(TempDirectory, TEXT("factory-test.fnt"));
	const auto WriteDescriptor = [&DescriptorFilename](const int32 Advance)
	{
		const FString Descriptor = FString::Printf(
			TEXT("info face=\"Factory Test\" size=4 unicode=1 smooth=0 padding=0,0,0,0 spacing=0,0\n")
			TEXT("common lineHeight=4 base=4 scaleW=4 scaleH=4 pages=1 packed=0 alphaChnl=0 redChnl=4 greenChnl=4 blueChnl=4\n")
			TEXT("page id=0 file=\"atlas.png\"\n")
			TEXT("chars count=1\n")
			TEXT("char id=65 x=0 y=0 width=4 height=4 xoffset=0 yoffset=0 xadvance=%d page=0 chnl=15\n"),
			Advance
		);
		return FFileHelper::SaveStringToFile(
			Descriptor,
			*DescriptorFilename,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM
		);
	};
	if (!TestTrue(TEXT("Descriptor fixture can be written"), WriteDescriptor(4)))
	{
		return false;
	}

	const FString PackageName = FString::Printf(TEXT("/Engine/Transient/UnrealBMFontFactory_%s"), *UniqueId);
	UPackage* Package = CreatePackage(*PackageName);
	Package->SetFlags(RF_Transient);
	UBMFontFactory* Factory = NewObject<UBMFontFactory>();
	bool bOperationCanceled = false;
	UBMFontAsset* Asset = Cast<UBMFontAsset>(Factory->FactoryCreateFile(
		UBMFontAsset::StaticClass(),
		Package,
		TEXT("FactoryTest"),
		RF_Public | RF_Standalone | RF_Transient,
		DescriptorFilename,
		TEXT(""),
		GWarn,
		bOperationCanceled
	));
	TestFalse(TEXT("Import is not cancelled"), bOperationCanceled);
	if (!TestNotNull(TEXT("Factory imports a BMFont asset"), Asset))
	{
		return false;
	}

	TestEqual(TEXT("Imported glyph count"), Asset->FontData.Glyphs.Num(), 1);
	TestEqual(TEXT("Imported page count"), Asset->FontData.Pages.Num(), 1);
	const FBMFontPage* Page = Asset->FindPage(0);
	if (!TestNotNull(TEXT("Imported page exists"), Page)
		|| !TestNotNull(TEXT("Imported page has a texture"), Page != nullptr ? Page->Texture.Get() : nullptr))
	{
		return false;
	}

	UTexture2D* Texture = Page->Texture;
	TestEqual(TEXT("Texture source width matches descriptor"), Texture->Source.GetSizeX(), int64{4});
	TestEqual(TEXT("Texture source height matches descriptor"), Texture->Source.GetSizeY(), int64{4});
	TestEqual(TEXT("Texture uses the UI LOD group"), Texture->LODGroup, TEXTUREGROUP_UI);
	TestEqual(TEXT("Texture has no generated mips"), Texture->MipGenSettings, TMGS_NoMipmaps);
	TestTrue(TEXT("Texture streaming is disabled"), Texture->NeverStream);
	TestEqual(TEXT("Texture defaults to bilinear filtering"), Texture->Filter, TF_Bilinear);

	TArray<FString> ReimportFilenames;
	TestTrue(TEXT("Imported asset supports reimport"), Factory->CanReimport(Asset, ReimportFilenames));
	TestEqual(TEXT("Reimport tracks one descriptor"), ReimportFilenames.Num(), 1);
	if (!ReimportFilenames.IsEmpty())
	{
		TestEqual(
			TEXT("Reimport tracks the descriptor path"),
			FPaths::ConvertRelativePathToFull(ReimportFilenames[0]),
			FPaths::ConvertRelativePathToFull(DescriptorFilename)
		);
	}

	Texture->Filter = TF_Nearest;
	TestTrue(TEXT("Updated descriptor fixture can be written"), WriteDescriptor(3));
	TestEqual(TEXT("Reimport succeeds"), Factory->Reimport(Asset), EReimportResult::Succeeded);
	const FBMFontGlyph* ReimportedGlyph = Asset->FindGlyph(65);
	if (TestNotNull(TEXT("Reimported glyph exists"), ReimportedGlyph))
	{
		TestEqual(TEXT("Reimport refreshes glyph metrics"), ReimportedGlyph->XAdvance, 3);
	}
	const FBMFontPage* ReimportedPage = Asset->FindPage(0);
	if (TestNotNull(TEXT("Reimported page exists"), ReimportedPage)
		&& TestNotNull(TEXT("Reimported page keeps a texture"), ReimportedPage->Texture.Get()))
	{
		TestEqual(TEXT("Reimport preserves edited texture filtering"), ReimportedPage->Texture->Filter, TF_Nearest);
	}

	if (!TestTrue(TEXT("Mismatched PNG fixture can be written"), WriteAtlas(8, 4)))
	{
		return false;
	}
	AddExpectedError(TEXT("BMFont page 0 is 8x4"), EAutomationExpectedErrorFlags::Contains, 1);
	TestEqual(TEXT("Dimension-mismatched reimport fails"), Factory->Reimport(Asset), EReimportResult::Failed);

	const FBMFontGlyph* PreservedGlyph = Asset->FindGlyph(65);
	if (TestNotNull(TEXT("Failed reimport preserves the existing glyph"), PreservedGlyph))
	{
		TestEqual(TEXT("Failed reimport preserves glyph metrics"), PreservedGlyph->XAdvance, 3);
	}
	const FBMFontPage* PreservedPage = Asset->FindPage(0);
	if (TestNotNull(TEXT("Failed reimport preserves the existing page"), PreservedPage)
		&& TestNotNull(TEXT("Failed reimport preserves its texture"), PreservedPage->Texture.Get()))
	{
		TestEqual(TEXT("Failed reimport preserves the texture object"), PreservedPage->Texture.Get(), Texture);
		TestEqual(TEXT("Failed reimport preserves source width"), PreservedPage->Texture->Source.GetSizeX(), int64{4});
		TestEqual(TEXT("Failed reimport preserves source height"), PreservedPage->Texture->Source.GetSizeY(), int64{4});
		TestEqual(TEXT("Failed reimport preserves texture filtering"), PreservedPage->Texture->Filter, TF_Nearest);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontFactoryShowcaseFixtureTest,
	"UnrealBMFont.Editor.ShowcaseFixtureImport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontFactoryShowcaseFixtureTest::RunTest(const FString& Parameters)
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("UnrealBMFont"));
	if (!TestTrue(TEXT("UnrealBMFont plugin is discoverable"), Plugin.IsValid()))
	{
		return false;
	}

	const FString DescriptorFilename = FPaths::Combine(
		Plugin->GetBaseDir(),
		TEXT("Samples/Showcase/Showcase.fnt")
	);
	if (!TestTrue(TEXT("Packaged Showcase descriptor exists"), FPaths::FileExists(DescriptorFilename)))
	{
		return false;
	}

	const FString UniqueId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	UPackage* Package = CreatePackage(*FString::Printf(TEXT("/Engine/Transient/UnrealBMFontShowcase_%s"), *UniqueId));
	Package->SetFlags(RF_Transient);
	UBMFontFactory* Factory = NewObject<UBMFontFactory>();
	bool bOperationCanceled = false;
	UBMFontAsset* Asset = Cast<UBMFontAsset>(Factory->FactoryCreateFile(
		UBMFontAsset::StaticClass(),
		Package,
		TEXT("ShowcaseFixture"),
		RF_Public | RF_Standalone | RF_Transient,
		DescriptorFilename,
		TEXT(""),
		GWarn,
		bOperationCanceled
	));
	TestFalse(TEXT("Showcase import is not cancelled"), bOperationCanceled);
	if (!TestNotNull(TEXT("Showcase fixture imports"), Asset))
	{
		return false;
	}

	TestEqual(TEXT("Showcase glyph count"), Asset->FontData.Glyphs.Num(), 135);
	const int32 RequiredCodepoints[] = {
		TEXT('0'), TEXT('9'), TEXT('A'), TEXT('Z'),
		0x3041, 0x3096, 0x30FC, 0x96F6, 0x4E5D, 0xFFFD
	};
	for (const int32 Codepoint : RequiredCodepoints)
	{
		TestTrue(
			*FString::Printf(TEXT("Showcase contains U+%04X"), Codepoint),
			Asset->HasGlyph(Codepoint)
		);
	}

	const FBMFontPage* Page = Asset->FindPage(0);
	if (TestNotNull(TEXT("Showcase page exists"), Page)
		&& TestNotNull(TEXT("Showcase page texture imports"), Page->Texture.Get()))
	{
		TestEqual(TEXT("Showcase atlas width"), Page->Texture->Source.GetSizeX(), int64{1024});
		TestEqual(TEXT("Showcase atlas height"), Page->Texture->Source.GetSizeY(), int64{512});
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontFactoryPackedImportTest,
	"UnrealBMFont.Editor.PackedFixtureImport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontFactoryPackedImportTest::RunTest(const FString& Parameters)
{
	const FString UniqueId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString TempDirectory = FPaths::Combine(
		FPaths::ProjectIntermediateDir(),
		TEXT("UnrealBMFontTests"),
		UniqueId
	);
	TestTrue(TEXT("Temporary source directory can be created"), IFileManager::Get().MakeDirectory(*TempDirectory, true));
	ON_SCOPE_EXIT
	{
		IFileManager::Get().DeleteDirectory(*TempDirectory, false, true);
	};

	const FString AtlasFilename = FPaths::Combine(TempDirectory, TEXT("packed-atlas.png"));
	{
		TArray<FColor> Pixels;
		Pixels.Init(FColor::White, 4 * 4);
		TArray<uint8> PngBytes;
		FImageUtils::ThumbnailCompressImageArray(4, 4, Pixels, PngBytes);
		if (!TestTrue(TEXT("Packed PNG fixture can be written"), FFileHelper::SaveArrayToFile(PngBytes, *AtlasFilename)))
		{
			return false;
		}
	}

	const FString DescriptorFilename = FPaths::Combine(TempDirectory, TEXT("packed-test.fnt"));
	const FString Descriptor =
		FString(TEXT("info face=\"Packed Test\" size=4 unicode=1 smooth=0 padding=0,0,0,0 spacing=0,0\n"))
		+ TEXT("common lineHeight=4 base=4 scaleW=4 scaleH=4 pages=1 packed=1 alphaChnl=3 redChnl=0 greenChnl=3 blueChnl=3\n")
		+ TEXT("page id=0 file=\"packed-atlas.png\"\n")
		+ TEXT("chars count=1\n")
		+ TEXT("char id=65 x=0 y=0 width=4 height=4 xoffset=0 yoffset=0 xadvance=4 page=0 chnl=4\n");
	if (!TestTrue(
		TEXT("Packed descriptor fixture can be written"),
		FFileHelper::SaveStringToFile(Descriptor, *DescriptorFilename, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)))
	{
		return false;
	}

	const FString PackageName = FString::Printf(TEXT("/Engine/Transient/UnrealBMFontPacked_%s"), *UniqueId);
	UPackage* Package = CreatePackage(*PackageName);
	Package->SetFlags(RF_Transient);
	UBMFontFactory* Factory = NewObject<UBMFontFactory>();

	// Packed imports announce the channel-extraction path; expecting the message keeps
	// the run at zero warning results while asserting the diagnostic still fires.
	AddExpectedMessage(
		TEXT("The descriptor uses packed texture channels"),
		ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains,
		1,
		false
	);

	bool bOperationCanceled = false;
	UBMFontAsset* Asset = Cast<UBMFontAsset>(Factory->FactoryCreateFile(
		UBMFontAsset::StaticClass(),
		Package,
		TEXT("PackedTest"),
		RF_Public | RF_Standalone | RF_Transient,
		DescriptorFilename,
		TEXT(""),
		GWarn,
		bOperationCanceled
	));
	TestFalse(TEXT("Packed import is not cancelled"), bOperationCanceled);
	if (!TestNotNull(TEXT("Factory imports a packed BMFont asset"), Asset))
	{
		return false;
	}

	TestTrue(TEXT("Packed flag survives import"), Asset->FontData.Common.bPacked);
	TestEqual(
		TEXT("Packed glyph channel metadata survives import"),
		Asset->FontData.Common.RedChannel,
		EBMFontChannelContent::Glyph
	);

	const FBMFontPage* Page = Asset->FindPage(0);
	if (!TestNotNull(TEXT("Packed page exists"), Page)
		|| !TestNotNull(TEXT("Packed page has a texture"), Page != nullptr ? Page->Texture.Get() : nullptr))
	{
		return false;
	}
	TestFalse(TEXT("Packed page texture disables sRGB"), Page->Texture->SRGB);

	TestNotNull(
		TEXT("Packed page resolves a render resource"),
		Asset->GetPageRenderResource(0)
	);
	return true;
}

#endif
