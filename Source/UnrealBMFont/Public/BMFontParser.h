// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#pragma once

#include "BMFontTypes.h"
#include "Containers/ArrayView.h"

/**
 * Resource limits applied before and while parsing untrusted descriptor data.
 * The defaults are deliberately generous enough for full Unicode atlases while
 * bounding memory growth, record processing, and imported texture dimensions.
 */
struct UNREALBMFONT_API FBMFontParserLimits
{
	int64 MaxDescriptorBytes = 64LL * 1024LL * 1024LL;
	int32 MaxDescriptorCharacters = 64 * 1024 * 1024;
	int32 MaxTextLines = 2250000;
	int32 MaxTextLineCharacters = 64 * 1024;
	int32 MaxXmlElements = 1200000;
	int32 MaxXmlAttributes = 5000000;
	int32 MaxPages = 256;
	int32 MaxGlyphs = 1112064;
	int32 MaxKerningPairs = 1000000;
	int32 MaxPageFileCharacters = 1024;
	int32 MaxAtlasDimension = 16384;
	int64 MaxAtlasPixelsPerPage = 64LL * 1024LL * 1024LL;
	int64 MaxTotalAtlasPixels = 256LL * 1024LL * 1024LL;

	bool IsValid() const;
};

enum class EBMFontParseMessageSeverity : uint8
{
	Warning,
	Error
};

/** One actionable parser diagnostic. Line is unset for XML and binary input. */
struct UNREALBMFONT_API FBMFontParseMessage
{
	EBMFontParseMessageSeverity Severity = EBMFontParseMessageSeverity::Error;
	int32 Line = INDEX_NONE;
	FString Message;
};

/** Parsed data plus every warning and error emitted while reading it. */
struct UNREALBMFONT_API FBMFontParseResult
{
	FBMFontData Data;
	TArray<FBMFontParseMessage> Messages;

	bool HasErrors() const;
	bool IsSuccess() const;
};

/** Stateless parser for AngelCode BMFont text, XML, and binary v3 descriptors. */
class UNREALBMFONT_API FBMFontParser
{
public:
	static FBMFontParseResult Parse(TArrayView<const uint8> Bytes);
	static FBMFontParseResult Parse(TArrayView<const uint8> Bytes, const FBMFontParserLimits& Limits);
	static FBMFontParseResult ParseText(const FString& DescriptorText);
	static FBMFontParseResult ParseText(const FString& DescriptorText, const FBMFontParserLimits& Limits);
	static FBMFontParseResult ParseXml(const FString& DescriptorXml);
	static FBMFontParseResult ParseXml(const FString& DescriptorXml, const FBMFontParserLimits& Limits);
	static FBMFontParseResult ParseBinary(TArrayView<const uint8> Bytes);
	static FBMFontParseResult ParseBinary(TArrayView<const uint8> Bytes, const FBMFontParserLimits& Limits);
};
