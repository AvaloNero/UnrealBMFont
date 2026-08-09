// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#if WITH_DEV_AUTOMATION_TESTS

#include "BMFontAsset.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "RHI.h"
#include "ShaderCompiler.h"
#include "Slate/WidgetRenderer.h"
#include "Styling/StyleDefaults.h"
#include "Widgets/BMFontRichTextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/SBMFontText.h"

namespace
{
	constexpr float MaxChannelDelta = 8.0f;
	constexpr double MaxMismatchRatio = 0.005;

	bool IsNullRHI()
	{
		return GUsingNullRHI;
	}

	bool IsGroundTruthUpdateRequested()
	{
		return FParse::Param(FCommandLine::Get(), TEXT("UpdateBMFontGroundTruth"));
	}

	FString GetGroundTruthFilename(const FString& Name)
	{
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("UnrealBMFont"));
		return Plugin.IsValid()
			? FPaths::Combine(Plugin->GetBaseDir(), TEXT("Samples/GroundTruth"), Name + TEXT(".png"))
			: FString();
	}

	/**
	 * Plain: solid white 8x10 glyphs at x=0/8/16.
	 * Packed: A and V overlap at x=0, with complementary coverage in green and red;
	 * the replacement glyph occupies green at x=8. This catches page-wide channel mixing.
	 */
	UTexture2D* CreateGlyphAtlas(UObject* Outer, const bool bPacked)
	{
		constexpr int32 AtlasSize = 32;
		UTexture2D* Texture = UTexture2D::CreateTransient(AtlasSize, AtlasSize, PF_B8G8R8A8);
		Texture->Filter = TF_Nearest;
		Texture->SRGB = !bPacked;

		FTexturePlatformData* PlatformData = Texture->GetPlatformData();
		FColor* Pixels = static_cast<FColor*>(PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
		for (int32 Index = 0; Index < AtlasSize * AtlasSize; ++Index)
		{
			Pixels[Index] = FColor::Transparent;
		}

		const auto FillGlyphRect = [Pixels, AtlasSize](const int32 GlyphX)
		{
			for (int32 Y = 0; Y < 10; ++Y)
			{
				for (int32 X = 0; X < 8; ++X)
				{
					FColor& Pixel = Pixels[Y * AtlasSize + GlyphX + X];
					Pixel = FColor::White;
				}
			}
		};
		if (bPacked)
		{
			for (int32 Y = 0; Y < 10; ++Y)
			{
				for (int32 X = 0; X < 8; ++X)
				{
					FColor& Pixel = Pixels[Y * AtlasSize + X];
					Pixel = X < 4
						? FColor(0, 255, 0, 0)
						: FColor(255, 0, 0, 0);
				}
			}
			for (int32 Y = 0; Y < 10; ++Y)
			{
				for (int32 X = 8; X < 16; ++X)
				{
					Pixels[Y * AtlasSize + X] = FColor(0, 255, 0, 0);
				}
			}
		}
		else
		{
			FillGlyphRect(0);
			FillGlyphRect(8);
			FillGlyphRect(16);
			for (int32 Y = 8; Y < 10; ++Y)
			{
				for (int32 X = 24; X < 26; ++X)
				{
					Pixels[Y * AtlasSize + X] = FColor::White;
				}
			}
		}

		PlatformData->Mips[0].BulkData.Unlock();
		Texture->UpdateResource();
		return Texture;
	}

	UBMFontAsset* CreateRenderFontAsset(UObject* Outer, const bool bPacked)
	{
		UBMFontAsset* Asset = NewObject<UBMFontAsset>(Outer);
		FBMFontData Data;
		Data.DescriptorFormat = EBMFontDescriptorFormat::Text;
		Data.Common.LineHeight = 20;
		Data.Common.Base = 15;
		Data.Common.ScaleWidth = 32;
		Data.Common.ScaleHeight = 32;
		Data.Common.PageCount = 1;
		Data.Common.bPacked = bPacked;
		if (bPacked)
		{
			Data.Common.AlphaChannel = EBMFontChannelContent::Zero;
			Data.Common.RedChannel = EBMFontChannelContent::Glyph;
			Data.Common.GreenChannel = EBMFontChannelContent::Glyph;
			Data.Common.BlueChannel = EBMFontChannelContent::Zero;
		}
		FBMFontPage& Page = Data.Pages.AddDefaulted_GetRef();
		Page.Id = 0;
		Page.File = TEXT("atlas.png");
		Page.Texture = CreateGlyphAtlas(Asset, bPacked);

		const auto AddGlyph = [&Data](
			const int32 Codepoint,
			const int32 AtlasX,
			const int32 Advance,
			const int32 Channel = 15)
		{
			FBMFontGlyph& Glyph = Data.Glyphs.Add(Codepoint);
			Glyph.Codepoint = Codepoint;
			Glyph.X = AtlasX;
			Glyph.Y = 0;
			Glyph.Width = 8;
			Glyph.Height = 10;
			Glyph.XAdvance = Advance;
			Glyph.Page = 0;
			Glyph.Channel = Channel;
		};
		if (bPacked)
		{
			AddGlyph(TEXT('A'), 0, 10, 2);
			AddGlyph(TEXT('V'), 0, 10, 4);
			AddGlyph(0xFFFD, 8, 6, 2);
		}
		else
		{
			AddGlyph(TEXT('A'), 0, 10);
			AddGlyph(TEXT('V'), 8, 10);
			AddGlyph(0xFFFD, 16, 6);

			FBMFontGlyph& DotGlyph = Data.Glyphs.Add(TEXT('.'));
			DotGlyph.Codepoint = TEXT('.');
			DotGlyph.X = 24;
			DotGlyph.Y = 8;
			DotGlyph.Width = 2;
			DotGlyph.Height = 2;
			DotGlyph.YOffset = 8;
			DotGlyph.XAdvance = 2;
			DotGlyph.Page = 0;
		}

		FBMFontKerningPair& Pair = Data.KerningPairs.AddDefaulted_GetRef();
		Pair.First = TEXT('A');
		Pair.Second = TEXT('V');
		Pair.Amount = -2;

		Asset->SetFontData(MoveTemp(Data));
		return Asset;
	}

	void RenderWidgetPixels(
		const TSharedRef<SWidget>& Widget,
		const FIntPoint Size,
		TArray<FColor>& OutPixels)
	{
		FWidgetRenderer Renderer(false, true);

		// The first draw of a fresh material resource only registers it; Slate resolves
		// new shader resources during its tick, and material shaders compile async.
		// Warm both before the authoritative draw so the packed material path is stable.
		Renderer.DrawWidget(Widget, FVector2D(Size));
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().Tick();
		}
		if (GShaderCompilingManager != nullptr)
		{
			GShaderCompilingManager->FinishAllCompilation();
		}
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().Tick();
		}

		UTextureRenderTarget2D* RenderTarget = Renderer.DrawWidget(Widget, FVector2D(Size));
		if (RenderTarget != nullptr)
		{
			FTextureRenderTargetResource* Resource = RenderTarget->GameThread_GetRenderTargetResource();
			if (Resource != nullptr)
			{
				Resource->ReadPixels(OutPixels);
			}
		}
	}

	bool VerifyAgainstGroundTruth(
		FAutomationTestBase& Test,
		const FString& Name,
		const TArray<FColor>& Pixels,
		const FIntPoint Size)
	{
		const FString Filename = GetGroundTruthFilename(Name);
		if (Filename.IsEmpty())
		{
			Test.AddError(TEXT("UnrealBMFont plugin is not discoverable for ground truth lookup."));
			return false;
		}

		if (IsGroundTruthUpdateRequested())
		{
			IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
			TArray64<uint8> PngBytes;
			FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, PngBytes);
			const bool bSaved = FFileHelper::SaveArrayToFile(PngBytes, *Filename);
			Test.AddInfo(FString::Printf(TEXT("Ground truth regenerated: %s"), *Filename));
			return bSaved;
		}

		if (!FPaths::FileExists(Filename))
		{
			Test.AddError(FString::Printf(
				TEXT("Ground truth is missing: %s. Regenerate with -UpdateBMFontGroundTruth on a GPU run."),
				*Filename
			));
			return false;
		}

		FImage GroundTruth;
		if (!FImageUtils::LoadImage(*Filename, GroundTruth))
		{
			Test.AddError(FString::Printf(TEXT("Ground truth failed to decode: %s"), *Filename));
			return false;
		}
		if (GroundTruth.SizeX != Size.X || GroundTruth.SizeY != Size.Y)
		{
			Test.AddError(FString::Printf(
				TEXT("Ground truth size mismatch for %s: expected %dx%d, got %dx%d."),
				*Name, Size.X, Size.Y, GroundTruth.SizeX, GroundTruth.SizeY
			));
			return false;
		}

		if (GroundTruth.Format != ERawImageFormat::BGRA8)
		{
			Test.AddError(FString::Printf(TEXT("Ground truth is not BGRA8: %s."), *Filename));
			return false;
		}

		const TArrayView64<const FColor> ExpectedPixels = GroundTruth.AsBGRA8();
		if (Pixels.Num() != ExpectedPixels.Num())
		{
			Test.AddError(FString::Printf(TEXT("Ground truth pixel count mismatch for %s."), *Name));
			return false;
		}

		int64 MismatchCount = 0;
		for (int32 Index = 0; Index < Pixels.Num(); ++Index)
		{
			const FColor& Actual = Pixels[Index];
			const FColor& Expected = ExpectedPixels[Index];
			const int32 DeltaB = FMath::Abs(static_cast<int32>(Actual.B) - Expected.B);
			const int32 DeltaG = FMath::Abs(static_cast<int32>(Actual.G) - Expected.G);
			const int32 DeltaR = FMath::Abs(static_cast<int32>(Actual.R) - Expected.R);
			const int32 DeltaA = FMath::Abs(static_cast<int32>(Actual.A) - Expected.A);
			if (DeltaB > MaxChannelDelta || DeltaG > MaxChannelDelta || DeltaR > MaxChannelDelta || DeltaA > MaxChannelDelta)
			{
				++MismatchCount;
			}
		}

		const double MismatchRatio = static_cast<double>(MismatchCount) / static_cast<double>(Pixels.Num());
		if (MismatchRatio > MaxMismatchRatio)
		{
			// Persist the actual render beside the report so ground-truth drift is debuggable.
			const FString DebugFilename = FPaths::Combine(
				FPaths::ProjectSavedDir(),
				TEXT("AutomationReports/Actual"),
				Name + TEXT(".actual.png")
			);
			IFileManager::Get().MakeDirectory(*FPaths::GetPath(DebugFilename), true);
			TArray64<uint8> DebugBytes;
			FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, DebugBytes);
			FFileHelper::SaveArrayToFile(DebugBytes, *DebugFilename);

			Test.AddError(FString::Printf(
				TEXT("%s differs from ground truth on %lld of %d pixels (ratio %.4f > %.4f). Actual render: %s"),
				*Name, MismatchCount, Pixels.Num(), MismatchRatio, MaxMismatchRatio, *DebugFilename
			));
			return false;
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontRenderPlainGlyphsTest,
	"UnrealBMFont.Render.PlainGlyphs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontRenderPlainGlyphsTest::RunTest(const FString& Parameters)
{
	if (IsNullRHI())
	{
		AddInfo(TEXT("Render tests require a GPU run without -NullRHI; skipping."));
		return true;
	}

	UBMFontAsset* Asset = CreateRenderFontAsset(GetTransientPackage(), false);
	TSharedRef<SBMFontText> Widget = SNew(SBMFontText)
		.Text(FText::FromString(TEXT("AV")))
		.FontAsset(Asset)
		.ColorAndOpacity(FSlateColor(FLinearColor::White))
		.PixelSnapping(true);

	const FIntPoint Size(64, 32);
	TArray<FColor> Pixels;
	RenderWidgetPixels(Widget, Size, Pixels);
	if (!TestEqual(TEXT("Render produced a full pixel buffer"), Pixels.Num(), Size.X * Size.Y))
	{
		return false;
	}

	TestTrue(
		TEXT("Plain glyph rendering matches ground truth"),
		VerifyAgainstGroundTruth(*this, TEXT("PlainGlyphs"), Pixels, Size)
	);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontRenderTintShadowTest,
	"UnrealBMFont.Render.TintShadowWrap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontRenderTintShadowTest::RunTest(const FString& Parameters)
{
	if (IsNullRHI())
	{
		AddInfo(TEXT("Render tests require a GPU run without -NullRHI; skipping."));
		return true;
	}

	UBMFontAsset* Asset = CreateRenderFontAsset(GetTransientPackage(), false);
	TSharedRef<SBMFontText> Widget = SNew(SBMFontText)
		.Text(FText::FromString(TEXT("AVVA")))
		.FontAsset(Asset)
		.ColorAndOpacity(FSlateColor(FLinearColor::Red))
		.ShadowOffset(FVector2D(1.0, 1.0))
		.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f))
		.WrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping)
		.AutoWrapText(true)
		.WrapTextAt(20.0f)
		.PixelSnapping(true);

	const FIntPoint Size(64, 48);
	TArray<FColor> Pixels;
	RenderWidgetPixels(Widget, Size, Pixels);
	if (!TestEqual(TEXT("Render produced a full pixel buffer"), Pixels.Num(), Size.X * Size.Y))
	{
		return false;
	}

	TestTrue(
		TEXT("Tinted, shadowed, wrapped rendering matches ground truth"),
		VerifyAgainstGroundTruth(*this, TEXT("TintShadowWrap"), Pixels, Size)
	);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontRenderPackedAtlasTest,
	"UnrealBMFont.Render.PackedAtlas",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontRenderPackedAtlasTest::RunTest(const FString& Parameters)
{
	if (IsNullRHI())
	{
		AddInfo(TEXT("Render tests require a GPU run without -NullRHI; skipping."));
		return true;
	}

	UBMFontAsset* Asset = CreateRenderFontAsset(GetTransientPackage(), true);
	UObject* Resource = Asset->GetPageRenderResource(0, 2);
	if (!TestNotNull(TEXT("Packed font resolves a render resource"), Resource))
	{
		return false;
	}


	TSharedRef<SBMFontText> Widget = SNew(SBMFontText)
		.Text(FText::FromString(TEXT("AV")))
		.FontAsset(Asset)
		.ColorAndOpacity(FSlateColor(FLinearColor(0.0f, 0.5f, 1.0f, 1.0f)))
		.PixelSnapping(true);

	const FIntPoint Size(64, 32);
	TArray<FColor> Pixels;
	RenderWidgetPixels(Widget, Size, Pixels);
	if (!TestEqual(TEXT("Render produced a full pixel buffer"), Pixels.Num(), Size.X * Size.Y))
	{
		return false;
	}

	TestTrue(
		TEXT("Packed material-path rendering matches ground truth"),
		VerifyAgainstGroundTruth(*this, TEXT("PackedAtlas"), Pixels, Size)
	);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontRenderRichTextForegroundTest,
	"UnrealBMFont.Render.RichTextForeground",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontRenderRichTextForegroundTest::RunTest(const FString& Parameters)
{
	if (IsNullRHI())
	{
		AddInfo(TEXT("Render tests require a GPU run without -NullRHI; skipping."));
		return true;
	}

	UBMFontAsset* Asset = CreateRenderFontAsset(GetTransientPackage(), false);
	UBMFontRichTextBlock* RichText = NewObject<UBMFontRichTextBlock>();
	RichText->SetFontAsset(Asset);
	RichText->SetText(FText::FromString(TEXT("A")));
	RichText->SetColorAndOpacity(FSlateColor::UseForeground());

	TSharedRef<SWidget> Widget = SNew(SBorder)
		.Padding(0.0f)
		.BorderImage(FStyleDefaults::GetNoBrush())
		.ForegroundColor(FLinearColor::Green)
		[
			RichText->TakeWidget()
		];

	const FIntPoint Size(32, 24);
	TArray<FColor> Pixels;
	RenderWidgetPixels(Widget, Size, Pixels);
	if (!TestEqual(TEXT("Render produced a full pixel buffer"), Pixels.Num(), Size.X * Size.Y))
	{
		return false;
	}

	int32 GreenPixelCount = 0;
	for (const FColor& Pixel : Pixels)
	{
		if (Pixel.G > 128 && Pixel.R < 32 && Pixel.B < 32)
		{
			++GreenPixelCount;
		}
	}
	TestTrue(TEXT("UseForeground resolves through the parent widget style"), GreenPixelCount >= 60);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontRenderRichTextEllipsisTest,
	"UnrealBMFont.Render.RichTextEllipsis",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontRenderRichTextEllipsisTest::RunTest(const FString& Parameters)
{
	if (IsNullRHI())
	{
		AddInfo(TEXT("Render tests require a GPU run without -NullRHI; skipping."));
		return true;
	}

	UBMFontAsset* Asset = CreateRenderFontAsset(GetTransientPackage(), false);
	UBMFontRichTextBlock* RichText = NewObject<UBMFontRichTextBlock>();
	RichText->SetFontAsset(Asset);
	RichText->SetText(FText::FromString(TEXT("AVAV")));
	RichText->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
	RichText->SetClipping(EWidgetClipping::ClipToBounds);

	// The run is deliberately wider than its clipping parent. Its own allotted width is
	// 64 px, so ellipsis must derive the visible range from MyCullingRect, not local size.
	TSharedRef<SWidget> Widget = SNew(SBox)
		.WidthOverride(24.0f)
		.HeightOverride(32.0f)
		.Clipping(EWidgetClipping::ClipToBounds)
		[
			SNew(SConstraintCanvas)
			+ SConstraintCanvas::Slot()
			.Offset(FMargin(0.0f, 0.0f, 64.0f, 32.0f))
			.Anchors(FAnchors(0.0f, 0.0f))
			.Alignment(FVector2D::ZeroVector)
			[
				RichText->TakeWidget()
			]
		];

	const FIntPoint Size(24, 32);
	TArray<FColor> Pixels;
	RenderWidgetPixels(Widget, Size, Pixels);
	if (!TestEqual(TEXT("Render produced a full pixel buffer"), Pixels.Num(), Size.X * Size.Y))
	{
		return false;
	}

	TestTrue(
		TEXT("Rich-text ellipsis rendering matches ground truth"),
		VerifyAgainstGroundTruth(*this, TEXT("RichTextEllipsis"), Pixels, Size)
	);
	return true;
}

#endif
