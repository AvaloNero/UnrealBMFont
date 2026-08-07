// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#pragma once

#include "Styling/SlateBrush.h"
#include "Widgets/SCompoundWidget.h"

class UBMFontAsset;
class UTexture2D;

DECLARE_DELEGATE_OneParam(FOnBMFontGlyphSelected, int32 /*Codepoint*/);

/**
 * Read-only atlas preview for the BMFont asset editor: draws the selected page texture,
 * outlines every glyph rectangle, and reports glyph clicks.
 */
class SBMFontAtlasPreview final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBMFontAtlasPreview)
		: _FontAsset(nullptr)
		, _PageId(0)
	{
	}

		SLATE_ARGUMENT(UBMFontAsset*, FontAsset)
		SLATE_ARGUMENT(int32, PageId)
		SLATE_EVENT(FOnBMFontGlyphSelected, OnGlyphSelected)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetPageId(int32 InPageId);
	void SetSelectedCodepoint(int32 InCodepoint);

protected:
	//~ SWidget
	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled
	) const override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return false; }

private:
	/** Atlas-rect geometry: fit scale and top-left offset for the current geometry size. */
	void ComputeViewTransform(const FVector2D& LocalSize, float& OutScale, FVector2D& OutOffset) const;

	TWeakObjectPtr<UBMFontAsset> FontAsset;
	int32 PageId = 0;
	int32 SelectedCodepoint = INDEX_NONE;
	FOnBMFontGlyphSelected OnGlyphSelected;

	mutable FSlateBrush PageBrush;
	mutable TWeakObjectPtr<UTexture2D> CachedPageTexture;
};
