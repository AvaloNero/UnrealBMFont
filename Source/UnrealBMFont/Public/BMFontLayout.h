// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#pragma once

#include "BMFontTypes.h"
#include "Framework/Text/TextLayout.h"

class UBMFontAsset;

/** Inputs controlling deterministic bitmap-glyph layout. */
struct UNREALBMFONT_API FBMFontLayoutSettings
{
	float FontScale = 1.0f;
	float LetterSpacing = 0.0f;
	float WrapWidth = 0.0f;
	float LineHeightScale = 1.0f;
	bool bApplyLineHeightToBottomLine = true;
	int32 FallbackCodepoint = 0xFFFD;
	ETextWrappingPolicy WrappingPolicy = ETextWrappingPolicy::DefaultWrapping;
};

/** One visible glyph quad after fallback, kerning, wrapping, and offsets. */
struct UNREALBMFONT_API FBMFontLayoutGlyph
{
	int32 SourceCodepoint = INDEX_NONE;
	int32 GlyphCodepoint = INDEX_NONE;
	int32 Page = INDEX_NONE;
	int32 Channel = 15;
	FVector2f Position = FVector2f::ZeroVector;
	FVector2f Size = FVector2f::ZeroVector;
};

/** Range and metrics for one visual line. */
struct UNREALBMFONT_API FBMFontLayoutLine
{
	int32 FirstGlyphIndex = 0;
	int32 GlyphCount = 0;
	float Width = 0.0f;
	float Top = 0.0f;
	float DrawTop = 0.0f;
	float DrawBottom = 0.0f;
};

/** Cached output consumed by the Slate renderer. */
struct UNREALBMFONT_API FBMFontLayoutResult
{
	TArray<FBMFontLayoutGlyph> Glyphs;
	TArray<FBMFontLayoutLine> Lines;
	FVector2f Size = FVector2f::ZeroVector;
	int32 MissingGlyphCount = 0;

	void Reset();
};

/** Converts a Unicode string and BMFont metrics into positioned quads. */
class UNREALBMFONT_API FBMFontLayout
{
public:
	static void Build(
		const FBMFontData& FontData,
		FStringView Text,
		const FBMFontLayoutSettings& Settings,
		FBMFontLayoutResult& OutResult
	);

	/** Uses the asset's prebuilt kerning lookup for cache-friendly widget relayout. */
	static void Build(
		const UBMFontAsset& FontAsset,
		FStringView Text,
		const FBMFontLayoutSettings& Settings,
		FBMFontLayoutResult& OutResult
	);

private:
	static void BuildWithKerningLookup(
		const FBMFontData& FontData,
		const TMap<uint64, int32>& KerningLookup,
		FStringView Text,
		const FBMFontLayoutSettings& Settings,
		FBMFontLayoutResult& OutResult
	);
};
