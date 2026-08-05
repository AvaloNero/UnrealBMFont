// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "BMFontTypes.generated.h"

class UTexture2D;

/** On-disk representation used by an AngelCode BMFont descriptor. */
UENUM(BlueprintType)
enum class EBMFontDescriptorFormat : uint8
{
	Unknown,
	Text,
	Xml,
	Binary
};

/** Meaning assigned to one channel by a BMFont common record. */
UENUM(BlueprintType)
enum class EBMFontChannelContent : uint8
{
	Glyph = 0,
	Outline = 1,
	GlyphAndOutline = 2,
	Zero = 3,
	One = 4,
	Unknown = 255
};

/** Generator metadata from the descriptor's info record. */
USTRUCT(BlueprintType)
struct UNREALBMFONT_API FBMFontInfo
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	FString Face;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 Size = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	bool bBold = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	bool bItalic = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	bool bUnicode = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	bool bSmooth = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	bool bFixedHeight = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	FString CharacterSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 CharacterSetId = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 StretchHeight = 100;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 Supersampling = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 PaddingUp = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 PaddingRight = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 PaddingDown = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 PaddingLeft = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	FIntPoint Spacing = FIntPoint::ZeroValue;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 Outline = 0;
};

/** Shared line and atlas metrics from the descriptor's common record. */
USTRUCT(BlueprintType)
struct UNREALBMFONT_API FBMFontCommon
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 LineHeight = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 Base = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 ScaleWidth = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 ScaleHeight = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 PageCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	bool bPacked = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	EBMFontChannelContent AlphaChannel = EBMFontChannelContent::Unknown;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	EBMFontChannelContent RedChannel = EBMFontChannelContent::Unknown;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	EBMFontChannelContent GreenChannel = EBMFontChannelContent::Unknown;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	EBMFontChannelContent BlueChannel = EBMFontChannelContent::Unknown;
};

/** One texture page declared by the descriptor. */
USTRUCT(BlueprintType)
struct UNREALBMFONT_API FBMFontPage
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 Id = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	FString File;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BMFont")
	TObjectPtr<UTexture2D> Texture = nullptr;
};

/** Metrics and atlas rectangle for one Unicode code point. */
USTRUCT(BlueprintType)
struct UNREALBMFONT_API FBMFontGlyph
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 Codepoint = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 X = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 Y = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 Width = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 Height = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 XOffset = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 YOffset = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 XAdvance = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 Page = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 Channel = 15;

	FBox2D GetUvRegion(const FBMFontCommon& Common) const;
};

/** Pair adjustment applied before letter spacing. */
USTRUCT(BlueprintType)
struct UNREALBMFONT_API FBMFontKerningPair
{
	GENERATED_BODY()

	static uint64 MakeKey(int32 FirstCodepoint, int32 SecondCodepoint);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 First = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 Second = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	int32 Amount = 0;
};

/** Complete serializable data model produced by the parser and importer. */
USTRUCT(BlueprintType)
struct UNREALBMFONT_API FBMFontData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	EBMFontDescriptorFormat DescriptorFormat = EBMFontDescriptorFormat::Unknown;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	FBMFontInfo Info;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	FBMFontCommon Common;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	TArray<FBMFontPage> Pages;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	TMap<int32, FBMFontGlyph> Glyphs;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	TArray<FBMFontKerningPair> KerningPairs;

	bool IsValid() const;
};
