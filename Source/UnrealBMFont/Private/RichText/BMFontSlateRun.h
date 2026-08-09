// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#pragma once

#include "BMFontTypes.h"
#include "CoreMinimal.h"
#include "Framework/Text/ILayoutBlock.h"
#include "Framework/Text/IRun.h"
#include "Framework/Text/ISlateRun.h"
#include "Framework/Text/TextLayout.h"
#include "SlateGlobals.h"
#include "Styling/SlateBrush.h"
#include "Widgets/SWidget.h"

class UBMFontAsset;

#if WITH_FANCY_TEXT

/** Appearance and layout inputs shared by all runs of one decorator. */
struct FBMFontRunStyle
{
	FSlateColor Color = FSlateColor(FLinearColor::White);
	FLinearColor ShadowColor = FLinearColor::Transparent;
	FVector2D ShadowOffset = FVector2D::ZeroVector;
	float FontScale = 1.0f;
	float LetterSpacing = 0.0f;
	int32 FallbackCodepoint = 0xFFFD;
};

/**
 * Mutable configuration shared by the runs a decorator creates. Widget property changes
 * update the block and bump Generation; existing runs observe it on the next layout
 * invalidate, so runtime updates do not need to re-parse the markup.
 */
struct FBMFontRunStyleBlock
{
	TWeakObjectPtr<UBMFontAsset> FontAsset;
	FBMFontRunStyle Style;
	uint32 Generation = 0;
};

/**
 * Slate run that measures and paints a rich-text run with BMFont glyphs.
 * Layout semantics mirror FBMFontLayout for a single unwrapped line: kerning applies
 * between resolved glyph pairs inside the run only, and missing glyphs advance by
 * half the line height. Created and configured by FBMFontTextDecorator.
 */
class FBMFontSlateRun final : public ISlateRun, public TSharedFromThis<FBMFontSlateRun>
{
public:
	static TSharedRef<FBMFontSlateRun> Create(
		const FRunInfo& InRunInfo,
		const TSharedRef<const FString>& InText,
		const FTextRange& InRange,
		const TSharedPtr<FBMFontRunStyleBlock>& InStyleBlock
	);

	//~ IRun
	virtual FTextRange GetTextRange() const override;
	virtual void SetTextRange(const FTextRange& Value) override;
	virtual int16 GetBaseLine(float Scale) const override;
	virtual int16 GetMaxHeight(float Scale) const override;
	virtual FVector2D Measure(int32 StartIndex, int32 EndIndex, float Scale, const FRunTextContext& TextContext) const override;
	virtual int8 GetKerning(int32 CurrentIndex, float Scale, const FRunTextContext& TextContext) const override;
	virtual FVector2D GetShadowSize(int32 StartIndex, int32 EndIndex, float Scale) const override;
	virtual TSharedRef<ILayoutBlock> CreateBlock(
		int32 StartIndex,
		int32 EndIndex,
		FVector2D Size,
		const FLayoutBlockTextContext& TextContext,
		const TSharedPtr<IRunRenderer>& Renderer
	) override;
	virtual int32 GetTextIndexAt(
		const TSharedRef<ILayoutBlock>& Block,
		const FVector2D& Location,
		float Scale,
		ETextHitPoint* const OutHitPoint = nullptr
	) const override;
	virtual FVector2D GetLocationAt(const TSharedRef<ILayoutBlock>& Block, int32 Offset, float Scale) const override;
	virtual void BeginLayout() override {}
	virtual void EndLayout() override {}
	virtual void Move(const TSharedRef<FString>& NewText, const FTextRange& NewRange) override;
	virtual TSharedRef<IRun> Clone() const override;
	virtual void AppendTextTo(FString& AppendToText) const override;
	virtual void AppendTextTo(FString& AppendToText, const FTextRange& PartialRange) const override;
	virtual const FRunInfo& GetRunInfo() const override;
	virtual ERunAttributes GetRunAttributes() const override;

	//~ ISlateRun
	virtual int32 OnPaint(
		const FPaintArgs& PaintArgs,
		const FTextArgs& TextArgs,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled
	) const override;
	virtual const TArray<TSharedRef<SWidget>>& GetChildren() override;
	virtual void ArrangeChildren(
		const TSharedRef<ILayoutBlock>& Block,
		const FGeometry& AllottedGeometry,
		FArrangedChildren& ArrangedChildren
	) const override;

private:
	FBMFontSlateRun(
		const FRunInfo& InRunInfo,
		const TSharedRef<const FString>& InText,
		const FTextRange& InRange,
		const TSharedPtr<FBMFontRunStyleBlock>& InStyleBlock
	);

	/** Per-codepoint metrics covering Range, rebuilt when the style block or asset changes. */
	struct FGlyphItem
	{
		int32 TextStart = 0;
		int32 TextEnd = 0;
		int32 GlyphCodepoint = INDEX_NONE;
		const FBMFontGlyph* Glyph = nullptr;
		/** XAdvance with FontScale applied; mirrors FBMFontLayout advance rules. */
		float Advance = 0.0f;
		/** Pair kerning with the previous in-run codepoint, FontScale applied. */
		float KerningBefore = 0.0f;
	};

	void EnsureItems() const;
	void EnsureBrushes() const;
	void EnsureGlyphBrush(UBMFontAsset& Font, int32 GlyphCodepoint, const FBMFontGlyph& Glyph) const;

	/** First item whose TextStart is not before TextIndex, or Items.Num(). */
	int32 FindItemIndex(int32 TextIndex) const;

	/** Width of items [BeginItem, EndItem) in layout units; inner kerning and spacing only. */
	float MeasureItems(int32 BeginItem, int32 EndItem) const;

	FRunInfo RunInfo;
	TSharedRef<const FString> Text;
	FTextRange Range;
	TSharedPtr<FBMFontRunStyleBlock> StyleBlock;

	mutable uint32 ItemsGeneration = 0;
	mutable uint32 ItemsDataRevision = 0;
	mutable uint32 ItemsRevision = 0;
	mutable bool bItemsDirty = true;
	mutable TArray<FGlyphItem> Items;

	mutable TWeakObjectPtr<UBMFontAsset> BrushFontAsset;
	mutable uint32 BrushDataRevision = 0;
	mutable uint32 BrushItemsRevision = MAX_uint32;
	mutable TMap<int32, FSlateBrush> GlyphBrushes;
};

#endif
