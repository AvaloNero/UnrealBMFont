// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#pragma once

#include "BMFontLayout.h"
#include "Layout/Margin.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"
#include "Widgets/SLeafWidget.h"

class UBMFontAsset;

/** Cache-aware Slate leaf widget used by UBMFontText and C++ Slate callers. */
class UNREALBMFONT_API SBMFontText final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SBMFontText)
		: _Text(FText::GetEmpty())
		, _FontAsset(nullptr)
		, _ColorAndOpacity(FLinearColor::White)
		, _FontScale(1.0f)
		, _LetterSpacing(0.0f)
		, _FallbackCodepoint(0xFFFD)
		, _Justification(ETextJustify::Left)
		, _WrappingPolicy(ETextWrappingPolicy::DefaultWrapping)
		, _AutoWrapText(false)
		, _WrapTextAt(0.0f)
		, _LineHeightPercentage(1.0f)
		, _ApplyLineHeightToBottomLine(true)
		, _Margin(FMargin(0.0f))
		, _ShadowOffset(FVector2D::ZeroVector)
		, _ShadowColorAndOpacity(FLinearColor::Transparent)
		, _PixelSnapping(true)
	{
	}

		SLATE_ATTRIBUTE(FText, Text)
		SLATE_ARGUMENT(UBMFontAsset*, FontAsset)
		SLATE_ATTRIBUTE(FSlateColor, ColorAndOpacity)
		SLATE_ARGUMENT(float, FontScale)
		SLATE_ARGUMENT(float, LetterSpacing)
		SLATE_ARGUMENT(int32, FallbackCodepoint)
		SLATE_ARGUMENT(ETextJustify::Type, Justification)
		SLATE_ARGUMENT(ETextWrappingPolicy, WrappingPolicy)
		SLATE_ARGUMENT(bool, AutoWrapText)
		SLATE_ARGUMENT(float, WrapTextAt)
		SLATE_ARGUMENT(float, LineHeightPercentage)
		SLATE_ARGUMENT(bool, ApplyLineHeightToBottomLine)
		SLATE_ARGUMENT(FMargin, Margin)
		SLATE_ARGUMENT(FVector2D, ShadowOffset)
		SLATE_ATTRIBUTE(FLinearColor, ShadowColorAndOpacity)
		SLATE_ARGUMENT(bool, PixelSnapping)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetText(TAttribute<FText> InText);
	void SetFontAsset(UBMFontAsset* InFontAsset);
	void SetColorAndOpacity(TAttribute<FSlateColor> InColorAndOpacity);
	void SetFontScale(float InFontScale);
	void SetLetterSpacing(float InLetterSpacing);
	void SetFallbackCodepoint(int32 InFallbackCodepoint);
	void SetJustification(ETextJustify::Type InJustification);
	void SetWrappingPolicy(ETextWrappingPolicy InWrappingPolicy);
	void SetAutoWrapText(bool bInAutoWrapText);
	void SetWrapTextAt(float InWrapTextAt);
	void SetLineHeightPercentage(float InLineHeightPercentage);
	void SetApplyLineHeightToBottomLine(bool bInApplyLineHeightToBottomLine);
	void SetMargin(const FMargin& InMargin);
	void SetShadowOffset(FVector2D InShadowOffset);
	void SetShadowColorAndOpacity(TAttribute<FLinearColor> InShadowColorAndOpacity);
	void SetPixelSnapping(bool bInPixelSnapping);
	FText GetText() const;

	const FBMFontLayoutResult& GetCachedLayout(float AvailableWidth) const;
#if WITH_DEV_AUTOMATION_TESTS
	int32 GetCachedGlyphBrushCountForTesting(float AvailableWidth) const;
#endif

protected:
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;
	virtual bool ComputeVolatility() const override;
#if WITH_ACCESSIBILITY
	virtual TSharedRef<FSlateAccessibleWidget> CreateAccessibleWidget() override;
	virtual TOptional<FText> GetDefaultAccessibleText(EAccessibleType AccessibleType) const override;
#endif

private:
	void InvalidateLayout();
	void EnsureLayout(float AvailableWidth) const;
	void EnsureBrushes() const;
	float ResolveWrapWidth(float AvailableWidth) const;
	float GetLineJustificationOffset(const FBMFontLayoutLine& Line, float InnerWidth) const;

	TAttribute<FText> Text;
	TWeakObjectPtr<UBMFontAsset> FontAsset;
	TAttribute<FSlateColor> ColorAndOpacity;
	float FontScale = 1.0f;
	float LetterSpacing = 0.0f;
	int32 FallbackCodepoint = 0xFFFD;
	ETextJustify::Type Justification = ETextJustify::Left;
	ETextWrappingPolicy WrappingPolicy = ETextWrappingPolicy::DefaultWrapping;
	bool bAutoWrapText = false;
	float WrapTextAt = 0.0f;
	float LineHeightPercentage = 1.0f;
	bool bApplyLineHeightToBottomLine = true;
	FMargin Margin;
	FVector2D ShadowOffset = FVector2D::ZeroVector;
	TAttribute<FLinearColor> ShadowColorAndOpacity = FLinearColor::Transparent;
	bool bPixelSnapping = true;

	mutable bool bLayoutDirty = true;
	mutable FString CachedText;
	mutable float CachedWrapWidth = -1.0f;
	mutable uint32 CachedDataRevision = 0;
	mutable FBMFontLayoutResult CachedLayout;
	mutable float LastAllottedWidth = 0.0f;
	mutable uint64 LayoutRevision = 0;

	mutable TMap<int32, FSlateBrush> GlyphBrushes;
	mutable TWeakObjectPtr<UBMFontAsset> BrushFontAsset;
	mutable uint32 BrushDataRevision = 0;
	mutable uint64 BrushLayoutRevision = MAX_uint64;
};
