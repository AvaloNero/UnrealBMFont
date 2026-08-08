// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#include "BMFontFactory.h"

#include "BMFontAsset.h"
#include "BMFontParser.h"
#include "BMFontRendering.h"
#include "EditorFramework/AssetImportData.h"
#include "Engine/Texture2D.h"
#include "Factories/TextureFactory.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ObjectTools.h"
#include "UnrealBMFontModule.h"

#define LOCTEXT_NAMESPACE "UnrealBMFontFactory"

namespace
{
	/** Deep snapshot of an existing page texture's source, used to roll back a failed reimport. */
	struct FPageTextureBackup
	{
		UTexture2D* Texture = nullptr;
		int32 SizeX = 0;
		int32 SizeY = 0;
		int32 NumMips = 0;
		ETextureSourceFormat Format = TSF_Invalid;
		TArray<TArray64<uint8>> MipData;

		bool Capture(UTexture2D* InTexture)
		{
			if (InTexture == nullptr)
			{
				return false;
			}
			FTextureSource& Source = InTexture->Source;
			Texture = InTexture;
			SizeX = Source.GetSizeX();
			SizeY = Source.GetSizeY();
			NumMips = Source.GetNumMips();
			Format = Source.GetFormat();
			MipData.SetNum(NumMips);
			for (int32 MipIndex = 0; MipIndex < NumMips; ++MipIndex)
			{
				if (!Source.GetMipData(MipData[MipIndex], 0, 0, MipIndex))
				{
					return false;
				}
			}
			return true;
		}

		void Restore() const
		{
			if (Texture == nullptr)
			{
				return;
			}
			Texture->Modify();
			Texture->Source.Init(SizeX, SizeY, 1, NumMips, Format, nullptr);
			for (int32 MipIndex = 0; MipIndex < NumMips; ++MipIndex)
			{
				uint8* DestMip = Texture->Source.LockMip(0, 0, MipIndex);
				if (DestMip != nullptr && MipData.IsValidIndex(MipIndex))
				{
					const TArray64<uint8>& Bytes = MipData[MipIndex];
					FMemory::Memcpy(DestMip, Bytes.GetData(), Bytes.Num());
				}
				Texture->Source.UnlockMip(0, 0, MipIndex);
			}
			Texture->PostEditChange();
		}
	};

	void ReportParseMessages(const FBMFontParseResult& ParseResult)
	{
		for (const FBMFontParseMessage& Message : ParseResult.Messages)
		{
			const FString Location = Message.Line != INDEX_NONE
				? FString::Printf(TEXT("line %d: "), Message.Line)
				: FString();
			if (Message.Severity == EBMFontParseMessageSeverity::Error)
			{
				UE_LOG(LogUnrealBMFont, Error, TEXT("BMFont import %s%s"), *Location, *Message.Message);
			}
			else
			{
				UE_LOG(LogUnrealBMFont, Warning, TEXT("BMFont import %s%s"), *Location, *Message.Message);
			}
		}
	}

	bool ResolvePageFiles(
		const FString& DescriptorFilename,
		const TArray<FBMFontPage>& Pages,
		TMap<int32, FString>& OutPageFiles)
	{
		FString SourceDirectory = FPaths::ConvertRelativePathToFull(FPaths::GetPath(DescriptorFilename));
		FPaths::NormalizeDirectoryName(SourceDirectory);
		OutPageFiles.Reset();

		for (const FBMFontPage& Page : Pages)
		{
			FString FullPagePath = FPaths::IsRelative(Page.File)
				? FPaths::Combine(SourceDirectory, Page.File)
				: Page.File;
			FullPagePath = FPaths::ConvertRelativePathToFull(FullPagePath);
			FPaths::NormalizeFilename(FullPagePath);

			if (!FPaths::IsUnderDirectory(FullPagePath, SourceDirectory))
			{
				UE_LOG(
					LogUnrealBMFont,
					Error,
					TEXT("BMFont page '%s' resolves outside the descriptor directory."),
					*Page.File
				);
				return false;
			}
			if (!FPaths::FileExists(FullPagePath))
			{
				UE_LOG(LogUnrealBMFont, Error, TEXT("BMFont page file does not exist: %s"), *FullPagePath);
				return false;
			}

			OutPageFiles.Add(Page.Id, MoveTemp(FullPagePath));
		}
		return true;
	}

	bool ValidatePageImages(
		const TArray<FBMFontPage>& Pages,
		const TMap<int32, FString>& PageFiles,
		const FBMFontCommon& Common)
	{
		for (const FBMFontPage& Page : Pages)
		{
			const FString* PageFilename = PageFiles.Find(Page.Id);
			if (PageFilename == nullptr)
			{
				return false;
			}

			FImage Image;
			if (!FImageUtils::LoadImage(**PageFilename, Image))
			{
				UE_LOG(LogUnrealBMFont, Error, TEXT("Failed to decode BMFont page image: %s"), **PageFilename);
				return false;
			}
			if (Image.SizeX != Common.ScaleWidth || Image.SizeY != Common.ScaleHeight)
			{
				UE_LOG(
					LogUnrealBMFont,
					Error,
					TEXT("BMFont page %d is %dx%d, but the descriptor declares %dx%d: %s"),
					Page.Id,
					Image.SizeX,
					Image.SizeY,
					Common.ScaleWidth,
					Common.ScaleHeight,
					**PageFilename
				);
				return false;
			}
		}
		return true;
	}

	UTexture2D* ImportPageTexture(
		UObject* Parent,
		const FName TextureName,
		const EObjectFlags Flags,
		const FString& Filename,
		FFeedbackContext* Warn,
		bool& bOutOperationCanceled,
		const bool bApplyImportDefaults,
		const bool bPackedPage)
	{
		UTextureFactory::SuppressImportOverwriteDialog(false);
		UTextureFactory* TextureFactory = NewObject<UTextureFactory>();
		if (bPackedPage)
		{
			// Packed channels encode glyph coverage, not display color. Force linear BEFORE
			// the factory builds the texture source; flipping SRGB after FactoryCreateFile
			// races the async derived-data build and can trip its gamma-space assertion.
			TextureFactory->ColorSpaceMode = ETextureSourceColorSpace::Linear;
		}
		UObject* ImportedObject = TextureFactory->FactoryCreateFile(
			UTexture2D::StaticClass(),
			Parent,
			TextureName,
			Flags | RF_Transactional,
			Filename,
			TEXT(""),
			Warn,
			bOutOperationCanceled
		);

		UTexture2D* Texture = Cast<UTexture2D>(ImportedObject);
		if (Texture != nullptr && bApplyImportDefaults)
		{
			Texture->LODGroup = TEXTUREGROUP_UI;
			Texture->MipGenSettings = TMGS_NoMipmaps;
			Texture->NeverStream = true;
			Texture->Filter = TF_Bilinear;
			Texture->PostEditChange();
		}
		return Texture;
	}
}

UBMFontFactory::UBMFontFactory()
{
	bCreateNew = false;
	bEditAfterNew = true;
	bEditorImport = true;
	bText = false;
	SupportedClass = UBMFontAsset::StaticClass();
	Formats.Add(TEXT("fnt;AngelCode BMFont descriptor"));
}

FText UBMFontFactory::GetDisplayName() const
{
	return LOCTEXT("DisplayName", "AngelCode BMFont");
}

bool UBMFontFactory::FactoryCanImport(const FString& Filename)
{
	return FPaths::GetExtension(Filename).Equals(TEXT("fnt"), ESearchCase::IgnoreCase);
}

UObject* UBMFontFactory::FactoryCreateFile(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	const FString& Filename,
	const TCHAR* Parms,
	FFeedbackContext* Warn,
	bool& bOutOperationCanceled)
{
	bOutOperationCanceled = false;
	return ImportFromFile(nullptr, InParent, InName, Flags, Filename, Warn, bOutOperationCanceled);
}

bool UBMFontFactory::CanReimport(UObject* Object, TArray<FString>& OutFilenames)
{
	const UBMFontAsset* Asset = Cast<UBMFontAsset>(Object);
	if (Asset == nullptr || Asset->AssetImportData == nullptr)
	{
		return false;
	}

	Asset->AssetImportData->ExtractFilenames(OutFilenames);
	return !OutFilenames.IsEmpty();
}

void UBMFontFactory::SetReimportPaths(UObject* Object, const TArray<FString>& NewReimportPaths)
{
	UBMFontAsset* Asset = Cast<UBMFontAsset>(Object);
	if (Asset != nullptr && Asset->AssetImportData != nullptr && NewReimportPaths.Num() == 1)
	{
		Asset->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
	}
}

EReimportResult::Type UBMFontFactory::Reimport(UObject* Object)
{
	UBMFontAsset* Asset = Cast<UBMFontAsset>(Object);
	if (Asset == nullptr || Asset->AssetImportData == nullptr)
	{
		return EReimportResult::Failed;
	}

	const FString Filename = Asset->AssetImportData->GetFirstFilename();
	if (Filename.IsEmpty() || !FPaths::FileExists(Filename))
	{
		UE_LOG(LogUnrealBMFont, Error, TEXT("Cannot reimport BMFont: source descriptor is missing: %s"), *Filename);
		return EReimportResult::Failed;
	}

	bool bOperationCanceled = false;
	UBMFontAsset* Result = ImportFromFile(
		Asset,
		Asset->GetOuter(),
		Asset->GetFName(),
		Asset->GetFlags(),
		Filename,
		GWarn,
		bOperationCanceled
	);
	if (bOperationCanceled)
	{
		return EReimportResult::Cancelled;
	}
	return Result != nullptr ? EReimportResult::Succeeded : EReimportResult::Failed;
}

int32 UBMFontFactory::GetPriority() const
{
	return ImportPriority;
}

UBMFontAsset* UBMFontFactory::ImportFromFile(
	UBMFontAsset* ExistingAsset,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	const FString& Filename,
	FFeedbackContext* Warn,
	bool& bOutOperationCanceled) const
{
	TArray<uint8> DescriptorBytes;
	if (!FFileHelper::LoadFileToArray(DescriptorBytes, *Filename))
	{
		UE_LOG(LogUnrealBMFont, Error, TEXT("Failed to read BMFont descriptor: %s"), *Filename);
		return nullptr;
	}

	FBMFontParseResult ParseResult = FBMFontParser::Parse(DescriptorBytes);
	ReportParseMessages(ParseResult);
	if (!ParseResult.IsSuccess())
	{
		return nullptr;
	}
	if (ParseResult.Data.Common.bPacked)
	{
		UE_LOG(
			LogUnrealBMFont,
			Display,
			TEXT("BMFont descriptor uses packed channels; pages render through the channel-extraction material.")
		);
	}

	TMap<int32, FString> PageFiles;
	if (!ResolvePageFiles(Filename, ParseResult.Data.Pages, PageFiles))
	{
		return nullptr;
	}
	if (!ValidatePageImages(ParseResult.Data.Pages, PageFiles, ParseResult.Data.Common))
	{
		return nullptr;
	}

	TMap<int32, TObjectPtr<UTexture2D>> ExistingTextures;
	if (ExistingAsset != nullptr)
	{
		for (const FBMFontPage& Page : ExistingAsset->FontData.Pages)
		{
			ExistingTextures.Add(Page.Id, Page.Texture);
		}
	}

	TArray<FPageTextureBackup> TextureBackups;
	const auto RestoreTextureBackups = [&TextureBackups]()
	{
		for (const FPageTextureBackup& Backup : TextureBackups)
		{
			Backup.Restore();
		}
	};

	for (FBMFontPage& Page : ParseResult.Data.Pages)
	{
		const FString* PageFilename = PageFiles.Find(Page.Id);
		if (PageFilename == nullptr)
		{
			return nullptr;
		}

		UTexture2D* ExistingTexture = ExistingTextures.FindRef(Page.Id);
		if (ParseResult.Data.Common.bPacked && ExistingTexture != nullptr && ExistingTexture->SRGB)
		{
			UE_LOG(
				LogUnrealBMFont,
				Warning,
				TEXT("BMFont page %d is packed but the existing texture has sRGB enabled; "
					"disable sRGB on the page texture for correct coverage extraction."),
				Page.Id
			);
		}
		UObject* TextureParent = ExistingTexture != nullptr ? ExistingTexture->GetOuter() : InParent;
		const FName TextureName = ExistingTexture != nullptr
			? ExistingTexture->GetFName()
			: FName(*ObjectTools::SanitizeObjectName(
				FString::Printf(TEXT("%s_Page_%d"), *InName.ToString(), Page.Id)
			));

		// Overwriting an existing texture is destructive; snapshot its source first so a
		// later page failure can roll back instead of leaving mixed texture/glyph state.
		if (ExistingTexture != nullptr)
		{
			FPageTextureBackup& Backup = TextureBackups.AddDefaulted_GetRef();
			if (!Backup.Capture(ExistingTexture))
			{
				TextureBackups.Reset();
				UE_LOG(
					LogUnrealBMFont,
					Error,
					TEXT("Failed to snapshot BMFont page texture %s before reimport; aborting to keep the asset consistent."),
					*ExistingTexture->GetName()
				);
				return nullptr;
			}
		}

		const EObjectFlags TextureFlags = static_cast<EObjectFlags>(
			(Flags & (RF_Public | RF_Standalone | RF_Transient)) | RF_Transactional
		);
		Page.Texture = ImportPageTexture(
			TextureParent,
			TextureName,
			TextureFlags,
			*PageFilename,
			Warn,
			bOutOperationCanceled,
			ExistingTexture == nullptr,
			ParseResult.Data.Common.bPacked
		);
		if (bOutOperationCanceled || Page.Texture == nullptr)
		{
			RestoreTextureBackups();
			UE_LOG(LogUnrealBMFont, Error, TEXT("Failed to import BMFont page texture: %s"), **PageFilename);
			return nullptr;
		}
	}

	UBMFontAsset* Asset = ExistingAsset;
	if (Asset == nullptr)
	{
		Asset = NewObject<UBMFontAsset>(InParent, InName, Flags | RF_Transactional);
	}
	if (Asset == nullptr)
	{
		return nullptr;
	}

	Asset->Modify();
	if (Asset->AssetImportData == nullptr)
	{
		Asset->AssetImportData = NewObject<UAssetImportData>(Asset, TEXT("AssetImportData"));
	}
	Asset->AssetImportData->Update(Filename);
	if (Asset->PackedRenderMaterial.IsNull())
	{
		// Serialize the default material path so the reference enters the asset registry;
		// a CDO-default value would be delta-serialized away and never cook.
		Asset->PackedRenderMaterial = TSoftObjectPtr<UMaterialInterface>(
			FSoftObjectPath(BMFontRendering::GetDefaultPackedMaterialPath())
		);
	}
	Asset->SetFontData(MoveTemp(ParseResult.Data));
	Asset->MarkPackageDirty();
	Asset->PostEditChange();
	return Asset;
}

#undef LOCTEXT_NAMESPACE
