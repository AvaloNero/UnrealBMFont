// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#include "Widgets/SBMFontText.h"

#include "BMFontAsset.h"
#include "Engine/Texture2D.h"
#include "Rendering/DrawElements.h"
#if WITH_ACCESSIBILITY
#include "Widgets/Accessibility/SlateCoreAccessibleWidgets.h"
#endif

void SBMFontText::Construct(const FArguments& InArgs)
{
	Text = InArgs._Text;
	FontAsset = InArgs._FontAsset;
	ColorAndOpacity = InArgs._ColorAndOpacity;
	FontScale = FMath::Max(InArgs._FontScale, 0.001f);
	LetterSpacing = InArgs._LetterSpacing;
	FallbackCodepoint = InArgs._FallbackCodepoint;
	Justification = InArgs._Justification;
	WrappingPolicy = InArgs._WrappingPolicy;
	bAutoWrapText = InArgs._AutoWrapText;
	WrapTextAt = InArgs._WrapTextAt;
	LineHeightPercentage = FMath::Max(InArgs._LineHeightPercentage, 0.001f);
	bApplyLineHeightToBottomLine = InArgs._ApplyLineHeightToBottomLine;
	Margin = InArgs._Margin;
	ShadowOffset = InArgs._ShadowOffset;
	ShadowColorAndOpacity = InArgs._ShadowColorAndOpacity;
	bPixelSnapping = InArgs._PixelSnapping;
#if WITH_ACCESSIBILITY
	AccessibleBehavior = EAccessibleBehavior::Auto;
	bCanChildrenBeAccessible = false;
#endif
}

void SBMFontText::SetText(TAttribute<FText> InText)
{
	Text = MoveTemp(InText);
	InvalidateLayout();
}

void SBMFontText::SetFontAsset(UBMFontAsset* InFontAsset)
{
	if (FontAsset.Get() != InFontAsset)
	{
		FontAsset = InFontAsset;
		InvalidateLayout();
		BrushFontAsset.Reset();
		GlyphBrushes.Reset();
	}
}

void SBMFontText::SetColorAndOpacity(TAttribute<FSlateColor> InColorAndOpacity)
{
	ColorAndOpacity = MoveTemp(InColorAndOpacity);
	Invalidate(EInvalidateWidgetReason::PaintAndVolatility);
}

void SBMFontText::SetFontScale(const float InFontScale)
{
	const float NewScale = FMath::Max(InFontScale, 0.001f);
	if (!FMath::IsNearlyEqual(FontScale, NewScale))
	{
		FontScale = NewScale;
		InvalidateLayout();
	}
}

void SBMFontText::SetLetterSpacing(const float InLetterSpacing)
{
	if (!FMath::IsNearlyEqual(LetterSpacing, InLetterSpacing))
	{
		LetterSpacing = InLetterSpacing;
		InvalidateLayout();
	}
}

void SBMFontText::SetFallbackCodepoint(const int32 InFallbackCodepoint)
{
	if (FallbackCodepoint != InFallbackCodepoint)
	{
		FallbackCodepoint = InFallbackCodepoint;
		InvalidateLayout();
	}
}

void SBMFontText::SetJustification(const ETextJustify::Type InJustification)
{
	if (Justification != InJustification)
	{
		Justification = InJustification;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void SBMFontText::SetWrappingPolicy(const ETextWrappingPolicy InWrappingPolicy)
{
	if (WrappingPolicy != InWrappingPolicy)
	{
		WrappingPolicy = InWrappingPolicy;
		InvalidateLayout();
	}
}

void SBMFontText::SetAutoWrapText(const bool bInAutoWrapText)
{
	if (bAutoWrapText != bInAutoWrapText)
	{
		bAutoWrapText = bInAutoWrapText;
		InvalidateLayout();
	}
}

void SBMFontText::SetWrapTextAt(const float InWrapTextAt)
{
	if (!FMath::IsNearlyEqual(WrapTextAt, InWrapTextAt))
	{
		WrapTextAt = InWrapTextAt;
		InvalidateLayout();
	}
}

void SBMFontText::SetLineHeightPercentage(const float InLineHeightPercentage)
{
	const float NewPercentage = FMath::Max(InLineHeightPercentage, 0.001f);
	if (!FMath::IsNearlyEqual(LineHeightPercentage, NewPercentage))
	{
		LineHeightPercentage = NewPercentage;
		InvalidateLayout();
	}
}

void SBMFontText::SetApplyLineHeightToBottomLine(const bool bInApplyLineHeightToBottomLine)
{
	if (bApplyLineHeightToBottomLine != bInApplyLineHeightToBottomLine)
	{
		bApplyLineHeightToBottomLine = bInApplyLineHeightToBottomLine;
		InvalidateLayout();
	}
}

void SBMFontText::SetMargin(const FMargin& InMargin)
{
	if (Margin != InMargin)
	{
		Margin = InMargin;
		InvalidateLayout();
	}
}

void SBMFontText::SetShadowOffset(const FVector2D InShadowOffset)
{
	if (!ShadowOffset.Equals(InShadowOffset))
	{
		ShadowOffset = InShadowOffset;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void SBMFontText::SetShadowColorAndOpacity(TAttribute<FLinearColor> InShadowColorAndOpacity)
{
	ShadowColorAndOpacity = MoveTemp(InShadowColorAndOpacity);
	Invalidate(EInvalidateWidgetReason::PaintAndVolatility);
}

void SBMFontText::SetPixelSnapping(const bool bInPixelSnapping)
{
	if (bPixelSnapping != bInPixelSnapping)
	{
		bPixelSnapping = bInPixelSnapping;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

FText SBMFontText::GetText() const
{
	return Text.Get(FText::GetEmpty());
}

const FBMFontLayoutResult& SBMFontText::GetCachedLayout(const float AvailableWidth) const
{
	EnsureLayout(AvailableWidth);
	return CachedLayout;
}

FVector2D SBMFontText::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	const float AvailableWidth = WrapTextAt > 0.0f ? WrapTextAt : LastAllottedWidth;
	EnsureLayout(AvailableWidth);
	return FVector2D(
		CachedLayout.Size.X + Margin.Left + Margin.Right,
		CachedLayout.Size.Y + Margin.Top + Margin.Bottom
	);
}

int32 SBMFontText::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	const int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	const bool bParentEnabled) const
{
	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	if (bAutoWrapText && !FMath::IsNearlyEqual(LastAllottedWidth, LocalSize.X))
	{
		LastAllottedWidth = LocalSize.X;
		bLayoutDirty = true;
		const_cast<SBMFontText*>(this)->Invalidate(EInvalidateWidgetReason::Layout);
	}

	EnsureLayout(LocalSize.X);
	EnsureBrushes();
	if (CachedLayout.Glyphs.IsEmpty() || GlyphBrushes.IsEmpty())
	{
		return LayerId;
	}

	const ESlateDrawEffect DrawEffects = bParentEnabled
		? (bPixelSnapping ? ESlateDrawEffect::None : ESlateDrawEffect::NoPixelSnapping)
		: ESlateDrawEffect::DisabledEffect;
	const float InnerWidth = FMath::Max(0.0f, static_cast<float>(LocalSize.X) - Margin.Left - Margin.Right);
	const FLinearColor Foreground = ColorAndOpacity.Get(FSlateColor::UseForeground()).GetColor(InWidgetStyle)
		* InWidgetStyle.GetColorAndOpacityTint();
	const FLinearColor ShadowColor = ShadowColorAndOpacity.Get(FLinearColor::Transparent);

	const auto PaintPass = [&](const int32 PaintLayer, const FVector2D Offset, const FLinearColor& Tint)
	{
		if (Tint.A <= 0.0f)
		{
			return;
		}

		for (const FBMFontLayoutLine& Line : CachedLayout.Lines)
		{
			if (Line.GlyphCount <= 0)
			{
				continue;
			}

			FSlateRect LineRect = AllottedGeometry.GetRenderBoundingRect(FSlateRect(
				0.0f,
				Margin.Top + Line.DrawTop + Offset.Y,
				static_cast<float>(LocalSize.X),
				Margin.Top + Line.DrawBottom + Offset.Y
			));
			LineRect.Left = MyCullingRect.Left;
			LineRect.Right = MyCullingRect.Right;
			if (!FSlateRect::DoRectanglesIntersect(LineRect, MyCullingRect))
			{
				continue;
			}

			const float JustificationOffset = GetLineJustificationOffset(Line, InnerWidth);
			for (int32 Index = Line.FirstGlyphIndex; Index < Line.FirstGlyphIndex + Line.GlyphCount; ++Index)
			{
				const FBMFontLayoutGlyph& Glyph = CachedLayout.Glyphs[Index];
				const FSlateBrush* Brush = GlyphBrushes.Find(Glyph.GlyphCodepoint);
				if (Brush == nullptr || Brush->GetResourceObject() == nullptr)
				{
					continue;
				}

				const FVector2D Position(
					Margin.Left + JustificationOffset + Glyph.Position.X + Offset.X,
					Margin.Top + Glyph.Position.Y + Offset.Y
				);
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					PaintLayer,
					AllottedGeometry.ToPaintGeometry(
						FVector2D(Glyph.Size.X, Glyph.Size.Y),
						FSlateLayoutTransform(Position)
					),
					Brush,
					DrawEffects,
					Tint
				);
			}
		}
	};

	const bool bHasShadow = !ShadowOffset.IsNearlyZero() && ShadowColor.A > 0.0f;
	if (bHasShadow)
	{
		PaintPass(LayerId, ShadowOffset, ShadowColor * InWidgetStyle.GetColorAndOpacityTint());
	}
	PaintPass(LayerId + (bHasShadow ? 1 : 0), FVector2D::ZeroVector, Foreground);
	return LayerId + (bHasShadow ? 1 : 0);
}

bool SBMFontText::ComputeVolatility() const
{
	return SLeafWidget::ComputeVolatility()
		|| Text.IsBound()
		|| ColorAndOpacity.IsBound()
		|| ShadowColorAndOpacity.IsBound();
}

#if WITH_ACCESSIBILITY
TSharedRef<FSlateAccessibleWidget> SBMFontText::CreateAccessibleWidget()
{
	return MakeShareable<FSlateAccessibleWidget>(
		new FSlateAccessibleWidget(SharedThis(this), EAccessibleWidgetType::Text)
	);
}

TOptional<FText> SBMFontText::GetDefaultAccessibleText(const EAccessibleType AccessibleType) const
{
	return Text.Get(FText::GetEmpty());
}
#endif

void SBMFontText::InvalidateLayout()
{
	bLayoutDirty = true;
	Invalidate(EInvalidateWidgetReason::LayoutAndVolatility);
}

void SBMFontText::EnsureLayout(const float AvailableWidth) const
{
	const UBMFontAsset* Font = FontAsset.Get();
	const FString CurrentText = Text.Get(FText::GetEmpty()).ToString();
	const uint32 CurrentRevision = Font != nullptr ? Font->GetDataRevision() : 0;
	const float CurrentWrapWidth = ResolveWrapWidth(AvailableWidth);
	if (!bLayoutDirty
		&& CurrentText == CachedText
		&& CurrentRevision == CachedDataRevision
		&& FMath::IsNearlyEqual(CurrentWrapWidth, CachedWrapWidth))
	{
		return;
	}

	CachedText = CurrentText;
	CachedDataRevision = CurrentRevision;
	CachedWrapWidth = CurrentWrapWidth;
	CachedLayout.Reset();
	if (Font != nullptr)
	{
		FBMFontLayoutSettings Settings;
		Settings.FontScale = FontScale;
		Settings.LetterSpacing = LetterSpacing;
		Settings.WrapWidth = CurrentWrapWidth;
		Settings.LineHeightScale = LineHeightPercentage;
		Settings.bApplyLineHeightToBottomLine = bApplyLineHeightToBottomLine;
		Settings.FallbackCodepoint = FallbackCodepoint;
		Settings.WrappingPolicy = WrappingPolicy;
		FBMFontLayout::Build(*Font, CachedText, Settings, CachedLayout);
	}
	bLayoutDirty = false;
}

void SBMFontText::EnsureBrushes() const
{
	UBMFontAsset* Font = FontAsset.Get();
	const uint32 CurrentRevision = Font != nullptr ? Font->GetDataRevision() : 0;
	if (BrushFontAsset.Get() == Font && BrushDataRevision == CurrentRevision)
	{
		return;
	}

	GlyphBrushes.Reset();
	BrushFontAsset = Font;
	BrushDataRevision = CurrentRevision;
	if (Font == nullptr || !Font->FontData.IsValid())
	{
		return;
	}

	TMap<int32, UTexture2D*> PageTextures;
	for (const FBMFontPage& Page : Font->FontData.Pages)
	{
		if (Page.Texture != nullptr)
		{
			PageTextures.Add(Page.Id, Page.Texture);
		}
	}

	for (const TPair<int32, FBMFontGlyph>& Entry : Font->FontData.Glyphs)
	{
		UTexture2D* const* Texture = PageTextures.Find(Entry.Value.Page);
		if (Texture == nullptr || *Texture == nullptr)
		{
			continue;
		}

		FSlateBrush& Brush = GlyphBrushes.Add(Entry.Key);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.SetResourceObject(*Texture);
		Brush.SetImageSize(FVector2D(Entry.Value.Width, Entry.Value.Height));
		Brush.SetUVRegion(Entry.Value.GetUvRegion(Font->FontData.Common));
	}
}

float SBMFontText::ResolveWrapWidth(const float AvailableWidth) const
{
	const float HorizontalMargin = Margin.Left + Margin.Right;
	if (WrapTextAt > 0.0f)
	{
		return FMath::Max(0.0f, WrapTextAt - HorizontalMargin);
	}
	if (bAutoWrapText && AvailableWidth > 0.0f)
	{
		return FMath::Max(0.0f, AvailableWidth - HorizontalMargin);
	}
	return 0.0f;
}

float SBMFontText::GetLineJustificationOffset(const FBMFontLayoutLine& Line, const float InnerWidth) const
{
	switch (Justification)
	{
	case ETextJustify::Center:
		return FMath::Max(0.0f, (InnerWidth - Line.Width) * 0.5f);
	case ETextJustify::Right:
	case ETextJustify::InvariantRight:
		return FMath::Max(0.0f, InnerWidth - Line.Width);
	case ETextJustify::Left:
	case ETextJustify::InvariantLeft:
	default:
		return 0.0f;
	}
}
