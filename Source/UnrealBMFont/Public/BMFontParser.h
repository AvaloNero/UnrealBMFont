// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#pragma once

#include "BMFontTypes.h"
#include "Containers/ArrayView.h"

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
	static FBMFontParseResult ParseText(const FString& DescriptorText);
	static FBMFontParseResult ParseXml(const FString& DescriptorXml);
	static FBMFontParseResult ParseBinary(TArrayView<const uint8> Bytes);
};
