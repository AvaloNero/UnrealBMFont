// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#include "BMFontFactory.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "BMFontAsset.h"
#include "BMFontParser.h"
#include "EditorFramework/AssetImportData.h"
#include "Engine/Texture2D.h"
#include "Factories/TextureFactory.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ObjectTools.h"
#include "UnrealBMFontModule.h"
#include "UObject/UObjectGlobals.h"

#define LOCTEXT_NAMESPACE "UnrealBMFontFactory"

namespace
{
	/** Deep, single-layer snapshot used both for staging and rollback. */
	struct FPageTextureSourceSnapshot
	{
		int32 SizeX = 0;
		int32 SizeY = 0;
		int32 NumMips = 0;
		ETextureSourceFormat Format = TSF_Invalid;
		TArray<TArray64<uint8>> MipData;

		bool Capture(UTexture2D* Texture)
		{
			if (Texture == nullptr)
			{
				return false;
			}

			Texture->WaitForPendingInitOrStreaming();
			FTextureSource& Source = Texture->Source;
			if (!Source.IsValid()
				|| Source.GetNumBlocks() != 1
				|| Source.GetNumLayers() != 1
				|| Source.GetNumSlices() != 1
				|| Source.GetNumMips() <= 0
				|| Source.GetFormat() == TSF_Invalid)
			{
				return false;
			}

			SizeX = Source.GetSizeX();
			SizeY = Source.GetSizeY();
			NumMips = Source.GetNumMips();
			Format = Source.GetFormat();
			MipData.SetNum(NumMips);
			for (int32 MipIndex = 0; MipIndex < NumMips; ++MipIndex)
			{
				if (!Source.GetMipData(MipData[MipIndex], 0, 0, MipIndex)
					|| MipData[MipIndex].Num() <= 0)
				{
					return false;
				}
			}
			return true;
		}

		bool ApplyTo(UTexture2D* Texture) const
		{
			if (Texture == nullptr
				|| SizeX <= 0
				|| SizeY <= 0
				|| NumMips <= 0
				|| Format == TSF_Invalid
				|| MipData.Num() != NumMips)
			{
				return false;
			}

			Texture->WaitForPendingInitOrStreaming();
			Texture->Modify();
			Texture->PreEditChange(nullptr);
			Texture->Source.Init(SizeX, SizeY, 1, NumMips, Format, nullptr);
			bool bSuccess = true;
			for (int32 MipIndex = 0; MipIndex < NumMips; ++MipIndex)
			{
				const TArray64<uint8>& Bytes = MipData[MipIndex];
				if (Texture->Source.CalcMipSize(0, 0, MipIndex) != Bytes.Num())
				{
					bSuccess = false;
					break;
				}
				uint8* DestMip = Texture->Source.LockMip(0, 0, MipIndex);
				if (DestMip == nullptr)
				{
					bSuccess = false;
					break;
				}
				FMemory::Memcpy(DestMip, Bytes.GetData(), Bytes.Num());
				Texture->Source.UnlockMip(0, 0, MipIndex);
			}
			Texture->PostEditChange();
			return bSuccess;
		}
	};

	struct FExistingPageTextureBackup
	{
		UTexture2D* Texture = nullptr;
		FPageTextureSourceSnapshot Source;

		bool Capture(UTexture2D* InTexture)
		{
			Texture = InTexture;
			return Source.Capture(InTexture);
		}

		bool Restore() const
		{
			return Source.ApplyTo(Texture);
		}
	};

	struct FStagedPageTexture
	{
		int32 PageId = INDEX_NONE;
		FString SourceFilename;
		FName TargetName;
		UObject* TargetParent = nullptr;
		UTexture2D* ExistingTexture = nullptr;
		UTexture2D* StagedTexture = nullptr;
		UTexture2D* FinalTexture = nullptr;
		FPageTextureSourceSnapshot StagedSource;
	};

	void DiscardTextureObject(UTexture2D* Texture)
	{
		if (Texture == nullptr)
		{
			return;
		}

		Texture->ClearFlags(RF_Public | RF_Standalone | RF_Transactional);
		Texture->Rename(
			nullptr,
			GetTransientPackage(),
			REN_DontCreateRedirectors | REN_DoNotDirty | REN_NonTransactional
		);
		Texture->MarkAsGarbage();
	}

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
	TArray<UTexture2D*> ObsoleteGeneratedTextures;
	TSet<int32> IncomingPageIds;
	for (const FBMFontPage& Page : ParseResult.Data.Pages)
	{
		IncomingPageIds.Add(Page.Id);
	}
	if (ExistingAsset != nullptr)
	{
		for (const FBMFontPage& Page : ExistingAsset->FontData.Pages)
		{
			ExistingTextures.Add(Page.Id, Page.Texture);
			const FName GeneratedTextureName(*ObjectTools::SanitizeObjectName(
				FString::Printf(TEXT("%s_Page_%d"), *InName.ToString(), Page.Id)
			));
			if (!IncomingPageIds.Contains(Page.Id)
				&& Page.Texture != nullptr
				&& Page.Texture->GetOuter() == InParent
				&& Page.Texture->GetFName() == GeneratedTextureName)
			{
				ObsoleteGeneratedTextures.AddUnique(Page.Texture);
			}
		}
	}

	const EObjectFlags TextureFlags = static_cast<EObjectFlags>(
		(Flags & (RF_Public | RF_Standalone | RF_Transient)) | RF_Transactional
	);
	TArray<FStagedPageTexture> StagedPages;
	StagedPages.Reserve(ParseResult.Data.Pages.Num());

	const auto DiscardStagedPages = [&StagedPages]()
	{
		for (FStagedPageTexture& StagedPage : StagedPages)
		{
			DiscardTextureObject(StagedPage.StagedTexture);
			StagedPage.StagedTexture = nullptr;
		}
	};

	// Stage and decode every page without touching the destination package. This also
	// establishes that every staged source can be copied before the commit phase begins.
	for (const FBMFontPage& Page : ParseResult.Data.Pages)
	{
		const FString* PageFilename = PageFiles.Find(Page.Id);
		if (PageFilename == nullptr)
		{
			DiscardStagedPages();
			return nullptr;
		}

		UTexture2D* ExistingTexture = ExistingTextures.FindRef(Page.Id);
		const FName GeneratedTextureName(*ObjectTools::SanitizeObjectName(
			FString::Printf(TEXT("%s_Page_%d"), *InName.ToString(), Page.Id)
		));
		if (ExistingTexture == nullptr)
		{
			if (UObject* ExistingObject = StaticFindObjectFast(UObject::StaticClass(), InParent, GeneratedTextureName))
			{
				ExistingTexture = Cast<UTexture2D>(ExistingObject);
				if (ExistingTexture == nullptr)
				{
					DiscardStagedPages();
					UE_LOG(
						LogUnrealBMFont,
						Error,
						TEXT("Cannot import BMFont page %d because '%s' is already used by a non-texture object."),
						Page.Id,
						*ExistingObject->GetPathName()
					);
					return nullptr;
				}
			}
		}

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

		FStagedPageTexture& StagedPage = StagedPages.AddDefaulted_GetRef();
		StagedPage.PageId = Page.Id;
		StagedPage.SourceFilename = *PageFilename;
		StagedPage.TargetParent = ExistingTexture != nullptr ? ExistingTexture->GetOuter() : InParent;
		StagedPage.TargetName = ExistingTexture != nullptr ? ExistingTexture->GetFName() : GeneratedTextureName;
		StagedPage.ExistingTexture = ExistingTexture;

		const FName StageName = MakeUniqueObjectName(
			GetTransientPackage(),
			UTexture2D::StaticClass(),
			TEXT("BMFontPageStage")
		);
		StagedPage.StagedTexture = ImportPageTexture(
			GetTransientPackage(),
			StageName,
			TextureFlags | RF_Transient,
			*PageFilename,
			Warn,
			bOutOperationCanceled,
			true,
			ParseResult.Data.Common.bPacked
		);
		if (bOutOperationCanceled
			|| StagedPage.StagedTexture == nullptr
			|| !StagedPage.StagedSource.Capture(StagedPage.StagedTexture))
		{
			DiscardStagedPages();
			UE_LOG(LogUnrealBMFont, Error, TEXT("Failed to import BMFont page texture: %s"), **PageFilename);
			return nullptr;
		}
	}

	// Snapshot every existing destination before committing even the first page. A bad
	// later destination can therefore never leave earlier pages overwritten.
	TArray<FExistingPageTextureBackup> TextureBackups;
	for (const FStagedPageTexture& StagedPage : StagedPages)
	{
		if (StagedPage.ExistingTexture == nullptr)
		{
			continue;
		}

		FExistingPageTextureBackup& Backup = TextureBackups.AddDefaulted_GetRef();
		if (!Backup.Capture(StagedPage.ExistingTexture))
		{
			DiscardStagedPages();
			UE_LOG(
				LogUnrealBMFont,
				Error,
				TEXT("Failed to snapshot BMFont page texture %s before reimport; no destination texture was changed."),
				*StagedPage.ExistingTexture->GetName()
			);
			return nullptr;
		}
	}

	const auto RestoreTextureBackups = [&TextureBackups]()
	{
		bool bRestoredAll = true;
		for (const FExistingPageTextureBackup& Backup : TextureBackups)
		{
			bRestoredAll = Backup.Restore() && bRestoredAll;
		}
		return bRestoredAll;
	};
	const auto DiscardNewTextures = [&StagedPages]()
	{
		for (FStagedPageTexture& StagedPage : StagedPages)
		{
			if (StagedPage.ExistingTexture == nullptr)
			{
				DiscardTextureObject(StagedPage.FinalTexture);
				StagedPage.FinalTexture = nullptr;
			}
		}
	};
	const auto RollBackCommit = [&]()
	{
		const bool bRestoredAll = RestoreTextureBackups();
		DiscardNewTextures();
		DiscardStagedPages();
		if (!bRestoredAll)
		{
			UE_LOG(LogUnrealBMFont, Error, TEXT("BMFont reimport rollback could not restore every page texture."));
		}
	};

	// Create all new destination objects before modifying existing ones. Any failure here
	// can still be cleaned up without a source rollback.
	for (FStagedPageTexture& StagedPage : StagedPages)
	{
		if (StagedPage.ExistingTexture != nullptr)
		{
			StagedPage.FinalTexture = StagedPage.ExistingTexture;
			continue;
		}

		StagedPage.FinalTexture = Cast<UTexture2D>(StaticDuplicateObject(
			StagedPage.StagedTexture,
			StagedPage.TargetParent,
			StagedPage.TargetName,
			static_cast<EObjectFlags>(RF_AllFlags & ~RF_Transient)
		));
		if (StagedPage.FinalTexture == nullptr)
		{
			DiscardNewTextures();
			DiscardStagedPages();
			UE_LOG(
				LogUnrealBMFont,
				Error,
				TEXT("Failed to create BMFont page texture: %s"),
				*StagedPage.TargetName.ToString()
			);
			return nullptr;
		}
		StagedPage.FinalTexture->SetFlags(TextureFlags);
	}

	for (FStagedPageTexture& StagedPage : StagedPages)
	{
		if (StagedPage.ExistingTexture != nullptr
			&& !StagedPage.StagedSource.ApplyTo(StagedPage.ExistingTexture))
		{
			RollBackCommit();
			UE_LOG(
				LogUnrealBMFont,
				Error,
				TEXT("Failed to commit BMFont page texture: %s"),
				*StagedPage.TargetName.ToString()
			);
			return nullptr;
		}
	}

	for (int32 PageIndex = 0; PageIndex < ParseResult.Data.Pages.Num(); ++PageIndex)
	{
		FStagedPageTexture& StagedPage = StagedPages[PageIndex];
		UTexture2D* Texture = StagedPage.FinalTexture;
		if (Texture->AssetImportData == nullptr)
		{
			Texture->AssetImportData = NewObject<UAssetImportData>(Texture, TEXT("AssetImportData"));
		}
		Texture->AssetImportData->Update(StagedPage.SourceFilename);
		if (StagedPage.ExistingTexture == nullptr)
		{
			Texture->PostEditChange();
		}
		ParseResult.Data.Pages[PageIndex].Texture = Texture;
	}

	UBMFontAsset* Asset = ExistingAsset;
	if (Asset == nullptr)
	{
		Asset = NewObject<UBMFontAsset>(InParent, InName, Flags | RF_Transactional);
	}
	if (Asset == nullptr)
	{
		RollBackCommit();
		return nullptr;
	}

	Asset->Modify();
	if (Asset->AssetImportData == nullptr)
	{
		Asset->AssetImportData = NewObject<UAssetImportData>(Asset, TEXT("AssetImportData"));
	}
	Asset->AssetImportData->Update(Filename);
	Asset->SetFontData(MoveTemp(ParseResult.Data));
	Asset->MarkPackageDirty();
	Asset->PostEditChange();

	for (UTexture2D* ObsoleteTexture : ObsoleteGeneratedTextures)
	{
		bool bIsReferenced = false;
		bool bIsReferencedByUndo = false;
		ObjectTools::GatherObjectReferencersForDeletion(
			ObsoleteTexture,
			bIsReferenced,
			bIsReferencedByUndo,
			nullptr,
			true
		);
		if (!bIsReferenced && !bIsReferencedByUndo)
		{
			ObsoleteTexture->MarkPackageDirty();
			if (!ObsoleteTexture->HasAnyFlags(RF_Transient))
			{
				FAssetRegistryModule::AssetDeleted(ObsoleteTexture);
			}
			DiscardTextureObject(ObsoleteTexture);
		}
		else
		{
			UE_LOG(
				LogUnrealBMFont,
				Display,
				TEXT("Retained obsolete generated page texture %s because it is still referenced%s."),
				*ObsoleteTexture->GetPathName(),
				bIsReferencedByUndo ? TEXT(" by the undo buffer") : TEXT("")
			);
		}
	}

	for (FStagedPageTexture& StagedPage : StagedPages)
	{
		if (StagedPage.ExistingTexture == nullptr && !StagedPage.FinalTexture->HasAnyFlags(RF_Transient))
		{
			FAssetRegistryModule::AssetCreated(StagedPage.FinalTexture);
		}
	}
	DiscardStagedPages();
	return Asset;
}

#undef LOCTEXT_NAMESPACE
