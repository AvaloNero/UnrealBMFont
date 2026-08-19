// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Text/ITextDecorator.h"
#include "RichText/BMFontSlateRun.h"
#include "SlateGlobals.h"

class UBMFontAsset;

#if WITH_FANCY_TEXT

/**
 * Slate decorator that converts matching rich-text runs into FBMFontSlateRun instances.
 * Matches the configured tag name, and optionally plain-text runs so an entire block
 * can render through BMFont without markup.
 *
 * Style changes apply to existing runs through the shared style block on the next
 * layout invalidate. Matching-rule changes (tag name, plain-text claiming) only affect
 * runs created by later re-marshals.
 */
class FBMFontTextDecorator final : public ITextDecorator
{
public:
	FBMFontTextDecorator(
		UBMFontAsset* InFontAsset,
		const FBMFontRunStyle& InStyle,
		FString InTagName,
		bool bInClaimPlainTextRuns
	);

	/** Updates the shared style block; existing runs observe the new values. */
	void ApplyStyle(UBMFontAsset* InFontAsset, const FBMFontRunStyle& InStyle);

	/** Updates which runs get claimed; affects runs created from now on. */
	void SetMatching(FString InTagName, bool bInClaimPlainTextRuns);

	//~ ITextDecorator
	virtual bool Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const override;
	virtual TSharedRef<ISlateRun> Create(
		const TSharedRef<FTextLayout>& TextLayout,
		const FTextRunParseResults& RunParseResult,
		const FString& OriginalText,
		const TSharedRef<FString>& InOutModelText,
		const ISlateStyle* Style
	) override;

private:
	TSharedPtr<FBMFontRunStyleBlock> StyleBlock;
	FString TagName;
	bool bClaimPlainTextRuns = false;
};

#endif
