// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#include "BMFontAsset.h"

#include "BMFontRendering.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UnrealBMFontModule.h"

#if WITH_EDITORONLY_DATA
#include "EditorFramework/AssetImportData.h"
#endif

namespace
{
	uint64 MakePageMaterialKey(const int32 PageId, const int32 GlyphChannel)
	{
		return (static_cast<uint64>(static_cast<uint32>(PageId)) << 32)
			| static_cast<uint32>(GlyphChannel & 0x0F);
	}
}

UBMFontAsset::UBMFontAsset()
{
	// Deliberately empty: a CDO-default soft reference is delta-serialized away on save,
	// so it never reaches the asset registry and the material would not cook. The factory
	// assigns the default path at import; GetPageRenderResource falls back to it at runtime.
#if WITH_EDITORONLY_DATA
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		AssetImportData = CreateDefaultSubobject<UAssetImportData>(TEXT("AssetImportData"));
	}
#endif
}

bool UBMFontAsset::HasGlyph(const int32 Codepoint) const
{
	return FontData.Glyphs.Contains(Codepoint);
}

int32 UBMFontAsset::GetLineHeight() const
{
	return FontData.Common.LineHeight;
}

int32 UBMFontAsset::GetPageCount() const
{
	return FontData.Pages.Num();
}

bool UBMFontAsset::GetGlyph(const int32 Codepoint, FBMFontGlyph& OutGlyph) const
{
	if (const FBMFontGlyph* Glyph = FindGlyph(Codepoint))
	{
		OutGlyph = *Glyph;
		return true;
	}

	OutGlyph = FBMFontGlyph();
	return false;
}

UTexture2D* UBMFontAsset::GetPageTexture(const int32 PageId) const
{
	if (const FBMFontPage* Page = FindPage(PageId))
	{
		return Page->Texture;
	}
	return nullptr;
}

UObject* UBMFontAsset::GetPageRenderResource(const int32 PageId, const int32 GlyphChannel)
{
	const FBMFontPage* Page = FindPage(PageId);
	if (Page == nullptr || Page->Texture == nullptr)
	{
		return nullptr;
	}
	if (!FontData.Common.bPacked)
	{
		return Page->Texture;
	}

	const uint64 MaterialKey = MakePageMaterialKey(PageId, GlyphChannel);
	if (const TObjectPtr<UMaterialInstanceDynamic>* Cached = PageMaterialCache.Find(MaterialKey))
	{
		if (*Cached != nullptr && PageMaterialSources.FindRef(MaterialKey).Get() == Page->Texture)
		{
			return *Cached;
		}
	}

	UMaterialInterface* BaseMaterial = PackedRenderMaterial.IsNull()
		? LoadObject<UMaterialInterface>(nullptr, BMFontRendering::GetDefaultPackedMaterialPath())
		: PackedRenderMaterial.LoadSynchronous();
	if (BaseMaterial == nullptr)
	{
		if (!bWarnedMissingPackedMaterial)
		{
			bWarnedMissingPackedMaterial = true;
			UE_LOG(
				LogUnrealBMFont,
				Warning,
				TEXT("BMFont asset '%s' uses packed channels but no packed render material could be loaded; "
					"pages fall back to raw textures and will render incorrectly."),
				*GetName()
			);
		}
		return Page->Texture;
	}

	UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	const FBMFontPackedChannelMapping Mapping = BMFontRendering::ComputePackedChannelMapping(
		FontData.Common,
		GlyphChannel
	);
	MaterialInstance->SetTextureParameterValue(TEXT("FontAtlas"), Page->Texture);
	MaterialInstance->SetVectorParameterValue(TEXT("ChannelWeights"), Mapping.ChannelWeights);
	MaterialInstance->SetScalarParameterValue(TEXT("ChannelBias"), Mapping.ConstantBias);

	PageMaterialCache.Add(MaterialKey, MaterialInstance);
	PageMaterialSources.Add(MaterialKey, Page->Texture);
	return MaterialInstance;
}

const FBMFontGlyph* UBMFontAsset::FindGlyph(const int32 Codepoint) const
{
	return FontData.Glyphs.Find(Codepoint);
}

const FBMFontPage* UBMFontAsset::FindPage(const int32 PageId) const
{
	return FontData.Pages.FindByPredicate(
		[PageId](const FBMFontPage& Page)
		{
			return Page.Id == PageId;
		}
	);
}

int32 UBMFontAsset::GetKerning(const int32 FirstCodepoint, const int32 SecondCodepoint) const
{
	if (const int32* Amount = KerningLookup.Find(FBMFontKerningPair::MakeKey(FirstCodepoint, SecondCodepoint)))
	{
		return *Amount;
	}

	return 0;
}

uint32 UBMFontAsset::GetDataRevision() const
{
	return DataRevision;
}

void UBMFontAsset::SetFontData(FBMFontData InFontData)
{
	FontData = MoveTemp(InFontData);
	ClearRenderResourceCache();
	RebuildLookup();
	++DataRevision;
}

void UBMFontAsset::PostLoad()
{
	Super::PostLoad();
	RebuildLookup();
}

#if WITH_EDITOR
void UBMFontAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ClearRenderResourceCache();
	RebuildLookup();
	++DataRevision;
}
#endif

void UBMFontAsset::RebuildLookup()
{
	KerningLookup.Reset();
	KerningLookup.Reserve(FontData.KerningPairs.Num());
	for (const FBMFontKerningPair& Pair : FontData.KerningPairs)
	{
		KerningLookup.Add(FBMFontKerningPair::MakeKey(Pair.First, Pair.Second), Pair.Amount);
	}
}

void UBMFontAsset::ClearRenderResourceCache()
{
	PageMaterialCache.Reset();
	PageMaterialSources.Reset();
	bWarnedMissingPackedMaterial = false;
}
