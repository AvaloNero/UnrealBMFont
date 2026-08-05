// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#include "BMFontAsset.h"

#if WITH_EDITORONLY_DATA
#include "EditorFramework/AssetImportData.h"
#endif

UBMFontAsset::UBMFontAsset()
{
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
