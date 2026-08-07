// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#include "RichText/BMFontTextDecorator.h"

#if WITH_FANCY_TEXT

FBMFontTextDecorator::FBMFontTextDecorator(
	UBMFontAsset* InFontAsset,
	const FBMFontRunStyle& InStyle,
	FString InTagName,
	const bool bInClaimPlainTextRuns)
	: StyleBlock(MakeShared<FBMFontRunStyleBlock>())
	, TagName(MoveTemp(InTagName))
	, bClaimPlainTextRuns(bInClaimPlainTextRuns)
{
	StyleBlock->FontAsset = InFontAsset;
	StyleBlock->Style = InStyle;
}

void FBMFontTextDecorator::ApplyStyle(UBMFontAsset* InFontAsset, const FBMFontRunStyle& InStyle)
{
	StyleBlock->FontAsset = InFontAsset;
	StyleBlock->Style = InStyle;
	++StyleBlock->Generation;
}

void FBMFontTextDecorator::SetMatching(FString InTagName, const bool bInClaimPlainTextRuns)
{
	TagName = MoveTemp(InTagName);
	bClaimPlainTextRuns = bInClaimPlainTextRuns;
}

bool FBMFontTextDecorator::Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const
{
	if (StyleBlock->FontAsset.Get() == nullptr)
	{
		return false;
	}
	if (RunParseResult.Name.IsEmpty())
	{
		return bClaimPlainTextRuns;
	}
	return RunParseResult.Name == TagName;
}

TSharedRef<ISlateRun> FBMFontTextDecorator::Create(
	const TSharedRef<FTextLayout>& TextLayout,
	const FTextRunParseResults& RunParseResult,
	const FString& OriginalText,
	const TSharedRef<FString>& InOutModelText,
	const ISlateStyle* Style)
{
	FRunInfo RunInfo(RunParseResult.Name);
	for (const TPair<FString, FTextRange>& Pair : RunParseResult.MetaData)
	{
		const int32 Length = FMath::Max(0, Pair.Value.EndIndex - Pair.Value.BeginIndex);
		RunInfo.MetaData.Add(Pair.Key, OriginalText.Mid(Pair.Value.BeginIndex, Length));
	}

	const FTextRange SourceRange = RunParseResult.ContentRange.Len() > 0
		? RunParseResult.ContentRange
		: RunParseResult.OriginalRange;

	FTextRange ModelRange;
	ModelRange.BeginIndex = InOutModelText->Len();
	InOutModelText->Append(*OriginalText + SourceRange.BeginIndex, SourceRange.Len());
	ModelRange.EndIndex = InOutModelText->Len();

	return FBMFontSlateRun::Create(RunInfo, InOutModelText, ModelRange, StyleBlock);
}

#endif
