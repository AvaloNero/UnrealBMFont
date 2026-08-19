// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#include "RichText/BMFontSlateRun.h"

#if WITH_FANCY_TEXT

#include "BMFontAsset.h"
#include "Containers/StringConv.h"
#include "Framework/Text/DefaultLayoutBlock.h"
#include "Framework/Text/RunUtils.h"
#include "Framework/Text/SlateTextUtils.h"
#include "Rendering/DrawElements.h"
#include "Rendering/DrawElementTextOverflowArgs.h"

namespace
{
	UTF32CHAR ReadRunCodepoint(const FString& Text, const int32 TextIndex, const int32 TextEnd, int32& OutCodeUnitCount)
	{
		const uint32 FirstCodeUnit = static_cast<uint32>(Text[TextIndex]);
		OutCodeUnitCount = 1;
		if constexpr (sizeof(TCHAR) == 2)
		{
			if (StringConv::IsHighSurrogate(FirstCodeUnit) && TextIndex + 1 < TextEnd)
			{
				const uint32 SecondCodeUnit = static_cast<uint32>(Text[TextIndex + 1]);
				if (StringConv::IsLowSurrogate(SecondCodeUnit))
				{
					OutCodeUnitCount = 2;
					return static_cast<UTF32CHAR>(StringConv::EncodeSurrogate(
						static_cast<uint16>(FirstCodeUnit),
						static_cast<uint16>(SecondCodeUnit)
					));
				}
			}
		}
		return static_cast<UTF32CHAR>(FirstCodeUnit);
	}
}

TSharedRef<FBMFontSlateRun> FBMFontSlateRun::Create(
	const FRunInfo& InRunInfo,
	const TSharedRef<const FString>& InText,
	const FTextRange& InRange,
	const TSharedPtr<FBMFontRunStyleBlock>& InStyleBlock)
{
	return MakeShareable(new FBMFontSlateRun(InRunInfo, InText, InRange, InStyleBlock));
}

FBMFontSlateRun::FBMFontSlateRun(
	const FRunInfo& InRunInfo,
	const TSharedRef<const FString>& InText,
	const FTextRange& InRange,
	const TSharedPtr<FBMFontRunStyleBlock>& InStyleBlock)
	: RunInfo(InRunInfo)
	, Text(InText)
	, Range(InRange)
	, StyleBlock(InStyleBlock)
{
}

FTextRange FBMFontSlateRun::GetTextRange() const
{
	return Range;
}

void FBMFontSlateRun::SetTextRange(const FTextRange& Value)
{
	Range = Value;
	bItemsDirty = true;
}

int16 FBMFontSlateRun::GetBaseLine(const float Scale) const
{
	// The text layout derives line height from GetMaxHeight + GetBaseLine and anchors
	// blocks from the line top. BMFont glyph offsets are already relative to the line
	// top, so the run reports no separate baseline: line height stays lineHeight and
	// pure BMFont lines keep their block at the line top. Mixed runs with taller font
	// blocks bottom-align this run, which the adapter documents as its approximation.
	return 0;
}

int16 FBMFontSlateRun::GetMaxHeight(const float Scale) const
{
	const UBMFontAsset* Font = StyleBlock.IsValid() ? StyleBlock->FontAsset.Get() : nullptr;
	if (Font == nullptr)
	{
		return 0;
	}
	return static_cast<int16>(FMath::Clamp(
		FMath::RoundToInt(
			Font->FontData.Common.LineHeight * StyleBlock->Style.FontScale * Scale
			+ FMath::Abs(StyleBlock->Style.ShadowOffset.Y * Scale)
		),
		0,
		MAX_int16
	));
}

FVector2D FBMFontSlateRun::Measure(
	const int32 StartIndex,
	const int32 EndIndex,
	const float Scale,
	const FRunTextContext& TextContext) const
{
	EnsureItems();
	const int32 BeginItem = FindItemIndex(StartIndex);
	const int32 EndItem = FindItemIndex(EndIndex);
	return FVector2D(
		MeasureItems(BeginItem, EndItem) * Scale + GetShadowSize(StartIndex, EndIndex, Scale).X,
		GetMaxHeight(Scale)
	);
}

int8 FBMFontSlateRun::GetKerning(const int32 CurrentIndex, const float Scale, const FRunTextContext& TextContext) const
{
	EnsureItems();
	const int32 ItemIndex = FindItemIndex(CurrentIndex);
	if (ItemIndex >= Items.Num() || Items[ItemIndex].TextStart != CurrentIndex || ItemIndex == 0)
	{
		return 0;
	}
	return static_cast<int8>(FMath::Clamp(
		FMath::RoundToInt(Items[ItemIndex].KerningBefore * Scale),
		MIN_int8,
		MAX_int8
	));
}

FVector2D FBMFontSlateRun::GetShadowSize(
	const int32 StartIndex,
	const int32 EndIndex,
	const float Scale) const
{
	if (!StyleBlock.IsValid())
	{
		return FVector2D::ZeroVector;
	}

	const FVector2D ShadowOffset = StyleBlock->Style.ShadowOffset * Scale;
	const bool bMeasureHorizontalShadow =
		(ShadowOffset.X > 0.0f && EndIndex == Range.EndIndex)
		|| (ShadowOffset.X < 0.0f && StartIndex == Range.BeginIndex);
	return FVector2D(
		bMeasureHorizontalShadow ? FMath::Abs(ShadowOffset.X) : 0.0f,
		FMath::Abs(ShadowOffset.Y)
	);
}

TSharedRef<ILayoutBlock> FBMFontSlateRun::CreateBlock(
	const int32 StartIndex,
	const int32 EndIndex,
	const FVector2D Size,
	const FLayoutBlockTextContext& TextContext,
	const TSharedPtr<IRunRenderer>& Renderer)
{
	return FDefaultLayoutBlock::Create(SharedThis(this), FTextRange(StartIndex, EndIndex), Size, TextContext, Renderer);
}

int32 FBMFontSlateRun::GetTextIndexAt(
	const TSharedRef<ILayoutBlock>& Block,
	const FVector2D& Location,
	const float Scale,
	ETextHitPoint* const OutHitPoint) const
{
	EnsureItems();
	const FBMFontRunStyle& Style = StyleBlock->Style;
	const FVector2D BlockOffset = Block->GetLocationOffset();
	const FVector2D BlockSize = Block->GetSize();
	const bool bContainsPoint =
		Location.X >= BlockOffset.X
		&& Location.X < BlockOffset.X + BlockSize.X
		&& Location.Y >= BlockOffset.Y
		&& Location.Y < BlockOffset.Y + BlockSize.Y;
	if (!bContainsPoint)
	{
		return INDEX_NONE;
	}

	const FTextRange BlockRange = Block->GetTextRange();
	const int32 BeginItem = FindItemIndex(BlockRange.BeginIndex);
	const int32 EndItem = FindItemIndex(BlockRange.EndIndex);

	const float LocalX = Location.X - BlockOffset.X;
	int32 HitIndex = BlockRange.EndIndex;
	float PenX = 0.0f;
	for (int32 Index = BeginItem; Index < EndItem; ++Index)
	{
		if (Index > BeginItem)
		{
			PenX += (Style.LetterSpacing + Items[Index].KerningBefore) * Scale;
		}
		const float ItemWidth = Items[Index].Advance * Scale;
		if (LocalX < PenX + ItemWidth * 0.5f)
		{
			HitIndex = Items[Index].TextStart;
			break;
		}
		PenX += ItemWidth;
		if (LocalX < PenX)
		{
			HitIndex = Items[Index].TextEnd;
			break;
		}
	}

	if (OutHitPoint != nullptr)
	{
		const FLayoutBlockTextContext BlockTextContext = Block->GetTextContext();
		*OutHitPoint = RunUtils::CalculateTextHitPoint(HitIndex, BlockRange, BlockTextContext.TextDirection);
	}
	return HitIndex;
}

FVector2D FBMFontSlateRun::GetLocationAt(const TSharedRef<ILayoutBlock>& Block, const int32 Offset, const float Scale) const
{
	EnsureItems();
	const FTextRange BlockRange = Block->GetTextRange();
	const int32 BeginItem = FindItemIndex(BlockRange.BeginIndex);
	const int32 OffsetItem = FindItemIndex(Offset);
	return Block->GetLocationOffset() + FVector2D(MeasureItems(BeginItem, OffsetItem) * Scale, 0.0);
}

void FBMFontSlateRun::Move(const TSharedRef<FString>& NewText, const FTextRange& NewRange)
{
	Text = NewText;
	Range = NewRange;
	bItemsDirty = true;
}

TSharedRef<IRun> FBMFontSlateRun::Clone() const
{
	return Create(RunInfo, Text, Range, StyleBlock);
}

void FBMFontSlateRun::AppendTextTo(FString& AppendToText) const
{
	AppendToText.Append(**Text + Range.BeginIndex, Range.Len());
}

void FBMFontSlateRun::AppendTextTo(FString& AppendToText, const FTextRange& PartialRange) const
{
	check(Range.BeginIndex <= PartialRange.BeginIndex);
	check(Range.EndIndex >= PartialRange.EndIndex);
	AppendToText.Append(**Text + PartialRange.BeginIndex, PartialRange.Len());
}

const FRunInfo& FBMFontSlateRun::GetRunInfo() const
{
	return RunInfo;
}

ERunAttributes FBMFontSlateRun::GetRunAttributes() const
{
	return ERunAttributes::None;
}

int32 FBMFontSlateRun::OnPaint(
	const FPaintArgs& PaintArgs,
	const FTextArgs& TextArgs,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	const int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	const bool bParentEnabled) const
{
	EnsureItems();
	EnsureBrushes();
	if (Items.IsEmpty())
	{
		return LayerId;
	}

	const FBMFontRunStyle& Style = StyleBlock->Style;

	// Block offsets arrive pre-scaled; glyph metrics are layout units, so convert once.
	const float InverseScale = AllottedGeometry.Scale > SMALL_NUMBER ? 1.0f / AllottedGeometry.Scale : 1.0f;
	const FTextRange BlockRange = TextArgs.Block->GetTextRange();
	const FVector2D BlockOffset = TextArgs.Block->GetLocationOffset();

	const int32 BeginItem = FindItemIndex(BlockRange.BeginIndex);
	const int32 EndItem = FindItemIndex(BlockRange.EndIndex);
	const float ContentWidth = MeasureItems(BeginItem, EndItem);
	const float BlockLocalX = BlockOffset.X * InverseScale;
	const float WidgetWidth = AllottedGeometry.GetLocalSize().X;
	const FVector2D DrawShadowOffset(
		FMath::Max(0.0f, Style.ShadowOffset.X),
		FMath::Max(0.0f, Style.ShadowOffset.Y)
	);
	const FVector2D DrawTextOffset(
		FMath::Max(0.0f, -Style.ShadowOffset.X),
		FMath::Max(0.0f, -Style.ShadowOffset.Y)
	);
	const float ContentLocalX = BlockLocalX + DrawTextOffset.X;
	const FSlateRect LocalCullingRect = TransformRect(
		AllottedGeometry.GetAccumulatedRenderTransform().Inverse(),
		MyCullingRect
	);
	const float VisibleWidgetLeft = FMath::Clamp(LocalCullingRect.Left, 0.0f, WidgetWidth);
	const float VisibleWidgetRight = FMath::Clamp(
		LocalCullingRect.Right,
		VisibleWidgetLeft,
		WidgetWidth
	);

	TArray<FGlyphItem, TInlineAllocator<3>> EllipsisItems;
	const auto AddEllipsisGlyph = [this, &EllipsisItems, &Style](
		UBMFontAsset& Font,
		const int32 Codepoint,
		const FBMFontGlyph& Glyph)
	{
		FGlyphItem& Item = EllipsisItems.AddDefaulted_GetRef();
		Item.GlyphCodepoint = Codepoint;
		Item.Glyph = &Glyph;
		Item.Advance = Glyph.XAdvance * Style.FontScale;
		if (EllipsisItems.Num() > 1)
		{
			Item.KerningBefore = Font.GetKerning(
				EllipsisItems[EllipsisItems.Num() - 2].GlyphCodepoint,
				Codepoint
			) * Style.FontScale;
		}
		EnsureGlyphBrush(Font, Codepoint, Glyph);
	};

	UBMFontAsset* Font = StyleBlock.IsValid() ? StyleBlock->FontAsset.Get() : nullptr;
	if (Font != nullptr)
	{
		if (const FBMFontGlyph* EllipsisGlyph = Font->FontData.Glyphs.Find(0x2026))
		{
			AddEllipsisGlyph(*Font, 0x2026, *EllipsisGlyph);
		}
		else if (const FBMFontGlyph* DotGlyph = Font->FontData.Glyphs.Find(TEXT('.')))
		{
			AddEllipsisGlyph(*Font, TEXT('.'), *DotGlyph);
			AddEllipsisGlyph(*Font, TEXT('.'), *DotGlyph);
			AddEllipsisGlyph(*Font, TEXT('.'), *DotGlyph);
		}
		else if (const FBMFontGlyph* FallbackGlyph = Font->FontData.Glyphs.Find(Style.FallbackCodepoint))
		{
			AddEllipsisGlyph(*Font, Style.FallbackCodepoint, *FallbackGlyph);
		}
	}

	const auto MeasureEllipsis = [&EllipsisItems, &Style]()
	{
		float Width = 0.0f;
		for (int32 Index = 0; Index < EllipsisItems.Num(); ++Index)
		{
			if (Index > 0)
			{
				Width += Style.LetterSpacing + EllipsisItems[Index].KerningBefore;
			}
			Width += EllipsisItems[Index].Advance;
		}
		return Width;
	};

	const float EllipsisWidth = MeasureEllipsis();
	const bool bUsesEllipsis = SlateTextUtils::IsEllipsisPolicy(TextArgs.OverflowPolicy)
		&& TextArgs.OverflowDirection != ETextOverflowDirection::NoOverflow
		&& TextArgs.bIsLastVisibleBlock;
	const bool bOverflows = TextArgs.OverflowDirection == ETextOverflowDirection::LeftToRight
		? ContentLocalX + ContentWidth > VisibleWidgetRight + KINDA_SMALL_NUMBER
		: ContentLocalX < VisibleWidgetLeft - KINDA_SMALL_NUMBER;
	const bool bPaintEllipsis = bUsesEllipsis && (bOverflows || TextArgs.bIsNextBlockClipped);

	int32 PrefixEndItem = EndItem;
	int32 SuffixBeginItem = EndItem;
	float PrefixX = 0.0f;
	float PrefixWidth = ContentWidth;
	float SuffixX = 0.0f;
	float SuffixWidth = 0.0f;
	float EllipsisX = 0.0f;

	const auto FitPrefix = [this, &Style](
		const int32 FirstItem,
		const int32 LastItem,
		const float WidthBudget,
		int32& OutEndItem,
		float& OutWidth)
	{
		OutEndItem = FirstItem;
		OutWidth = 0.0f;
		for (int32 Index = FirstItem; Index < LastItem; ++Index)
		{
			const float Leading = Index > FirstItem ? Style.LetterSpacing + Items[Index].KerningBefore : 0.0f;
			const float CandidateWidth = OutWidth + Leading + Items[Index].Advance;
			if (CandidateWidth > WidthBudget + KINDA_SMALL_NUMBER)
			{
				break;
			}
			OutWidth = CandidateWidth;
			OutEndItem = Index + 1;
		}
	};

	const auto FitSuffix = [this, &Style](
		const int32 FirstItem,
		const int32 LastItem,
		const float WidthBudget,
		int32& OutBeginItem,
		float& OutWidth)
	{
		OutBeginItem = LastItem;
		OutWidth = 0.0f;
		for (int32 Index = LastItem - 1; Index >= FirstItem; --Index)
		{
			const float Trailing = Index + 1 < LastItem
				? Style.LetterSpacing + Items[Index + 1].KerningBefore
				: 0.0f;
			const float CandidateWidth = Items[Index].Advance + Trailing + OutWidth;
			if (CandidateWidth > WidthBudget + KINDA_SMALL_NUMBER)
			{
				break;
			}
			OutWidth = CandidateWidth;
			OutBeginItem = Index;
		}
	};

	if (bPaintEllipsis)
	{
		const float VisibleLeft = FMath::Max(0.0f, VisibleWidgetLeft - ContentLocalX);
		const float VisibleRight = FMath::Max(VisibleLeft, VisibleWidgetRight - ContentLocalX);
		const float AvailableWidth = FMath::Max(0.0f, VisibleRight - VisibleLeft);
		const float ContentBudget = FMath::Max(0.0f, AvailableWidth - EllipsisWidth);

		if (TextArgs.OverflowPolicy == ETextOverflowPolicy::MiddleEllipsis)
		{
			FitPrefix(BeginItem, EndItem, ContentBudget * 0.5f, PrefixEndItem, PrefixWidth);
			FitSuffix(PrefixEndItem, EndItem, ContentBudget - PrefixWidth, SuffixBeginItem, SuffixWidth);
			PrefixX = VisibleLeft;
			EllipsisX = PrefixX + PrefixWidth;
			SuffixX = EllipsisX + EllipsisWidth;
		}
		else if (TextArgs.OverflowDirection == ETextOverflowDirection::RightToLeft)
		{
			PrefixEndItem = BeginItem;
			PrefixWidth = 0.0f;
			FitSuffix(BeginItem, EndItem, ContentBudget, SuffixBeginItem, SuffixWidth);
			SuffixX = VisibleRight - SuffixWidth;
			EllipsisX = SuffixX - EllipsisWidth;
		}
		else
		{
			SuffixBeginItem = EndItem;
			FitPrefix(BeginItem, EndItem, ContentBudget, PrefixEndItem, PrefixWidth);
			PrefixX = VisibleLeft;
			EllipsisX = PrefixX + PrefixWidth;
		}
	}

	if (GlyphBrushes.IsEmpty())
	{
		return LayerId;
	}

	const ESlateDrawEffect DrawEffects = bParentEnabled
		? ESlateDrawEffect::None
		: ESlateDrawEffect::DisabledEffect;

	const auto PaintPass = [&](const int32 PaintLayer, const FVector2D Offset, const FLinearColor& Tint)
	{
		if (Tint.A <= 0.0f)
		{
			return;
		}

		const auto PaintGlyph = [&](const FGlyphItem& Item, const float PenX)
		{
			if (Item.Glyph != nullptr && Item.Glyph->Width > 0 && Item.Glyph->Height > 0)
			{
				if (const FSlateBrush* Brush = GlyphBrushes.Find(Item.GlyphCodepoint))
				{
					if (Brush->GetResourceObject() != nullptr)
					{
						const FVector2D LocalPosition(
							BlockOffset.X * InverseScale + PenX + Item.Glyph->XOffset * Style.FontScale + Offset.X,
							BlockOffset.Y * InverseScale + Item.Glyph->YOffset * Style.FontScale + Offset.Y
						);
						FSlateDrawElement::MakeBox(
							OutDrawElements,
							PaintLayer,
							AllottedGeometry.ToPaintGeometry(
								FVector2D(Item.Glyph->Width, Item.Glyph->Height) * Style.FontScale,
								FSlateLayoutTransform(LocalPosition)
							),
							Brush,
							DrawEffects,
							Tint
						);
					}
				}
			}
		};

		const auto PaintRange = [&](const int32 FirstItem, const int32 LastItem, const float StartX)
		{
			float PenX = StartX;
			for (int32 Index = FirstItem; Index < LastItem; ++Index)
			{
				const FGlyphItem& Item = Items[Index];
				if (Index > FirstItem)
				{
					PenX += Style.LetterSpacing + Item.KerningBefore;
				}
				PaintGlyph(Item, PenX);
				PenX += Item.Advance;
			}
		};

		PaintRange(BeginItem, PrefixEndItem, PrefixX);
		if (bPaintEllipsis)
		{
			float PenX = EllipsisX;
			for (int32 Index = 0; Index < EllipsisItems.Num(); ++Index)
			{
				const FGlyphItem& Item = EllipsisItems[Index];
				if (Index > 0)
				{
					PenX += Style.LetterSpacing + Item.KerningBefore;
				}
				PaintGlyph(Item, PenX);
				PenX += Item.Advance;
			}
			PaintRange(SuffixBeginItem, EndItem, SuffixX);
		}
	};

	const FLinearColor Foreground = Style.Color.GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint();
	const bool bHasShadow = !Style.ShadowOffset.IsNearlyZero() && Style.ShadowColor.A > 0.0f;
	if (bHasShadow)
	{
		PaintPass(LayerId, DrawShadowOffset, Style.ShadowColor * InWidgetStyle.GetColorAndOpacityTint());
	}
	PaintPass(LayerId + (bHasShadow ? 1 : 0), DrawTextOffset, Foreground);
	return LayerId + (bHasShadow ? 1 : 0);
}

const TArray<TSharedRef<SWidget>>& FBMFontSlateRun::GetChildren()
{
	static const TArray<TSharedRef<SWidget>> NoChildren;
	return NoChildren;
}

void FBMFontSlateRun::ArrangeChildren(
	const TSharedRef<ILayoutBlock>& Block,
	const FGeometry& AllottedGeometry,
	FArrangedChildren& ArrangedChildren) const
{
	// Leaf run: no child widgets.
}

void FBMFontSlateRun::EnsureItems() const
{
	UBMFontAsset* Font = StyleBlock.IsValid() ? StyleBlock->FontAsset.Get() : nullptr;
	const uint32 CurrentRevision = Font != nullptr ? Font->GetDataRevision() : 0;
	const uint32 CurrentGeneration = StyleBlock.IsValid() ? StyleBlock->Generation : 0;
	if (!bItemsDirty && ItemsGeneration == CurrentGeneration && ItemsDataRevision == CurrentRevision)
	{
		return;
	}

	Items.Reset();
	bItemsDirty = false;
	ItemsGeneration = CurrentGeneration;
	ItemsDataRevision = CurrentRevision;
	++ItemsRevision;
	if (Font == nullptr || !Font->FontData.IsValid())
	{
		return;
	}

	const FBMFontRunStyle& Style = StyleBlock->Style;
	const FBMFontData& FontData = Font->FontData;
	int32 PreviousGlyphCodepoint = INDEX_NONE;
	int32 TextIndex = Range.BeginIndex;
	while (TextIndex < Range.EndIndex)
	{
		int32 CodeUnitCount = 0;
		const UTF32CHAR Codepoint = ReadRunCodepoint(*Text, TextIndex, Range.EndIndex, CodeUnitCount);
		if (Codepoint == 0)
		{
			break;
		}

		FGlyphItem& Item = Items.AddDefaulted_GetRef();
		Item.TextStart = TextIndex;
		Item.TextEnd = TextIndex + CodeUnitCount;

		if (Codepoint == TEXT('\n') || Codepoint == TEXT('\r'))
		{
			// The rich text marshaller splits lines; a stray break occupies no width.
			Item.Advance = 0.0f;
		}
		else if (Codepoint == TEXT('\t'))
		{
			const FBMFontGlyph* SpaceGlyph = FontData.Glyphs.Find(TEXT(' '));
			Item.GlyphCodepoint = SpaceGlyph != nullptr ? TEXT(' ') : INDEX_NONE;
			Item.Glyph = nullptr;
			Item.Advance = SpaceGlyph != nullptr
				? SpaceGlyph->XAdvance * Style.FontScale * 4.0f
				: FontData.Common.LineHeight * Style.FontScale * 2.0f;
		}
		else
		{
			const FBMFontGlyph* Glyph = FontData.Glyphs.Find(Codepoint);
			Item.GlyphCodepoint = Glyph != nullptr ? static_cast<int32>(Codepoint) : INDEX_NONE;
			if (Glyph == nullptr && Style.FallbackCodepoint != static_cast<int32>(Codepoint))
			{
				Glyph = FontData.Glyphs.Find(Style.FallbackCodepoint);
				if (Glyph != nullptr)
				{
					Item.GlyphCodepoint = Style.FallbackCodepoint;
				}
			}
			Item.Glyph = Glyph;
			Item.Advance = Glyph != nullptr
				? Glyph->XAdvance * Style.FontScale
				: FontData.Common.LineHeight * Style.FontScale * 0.5f;
		}

		if (PreviousGlyphCodepoint != INDEX_NONE && Item.GlyphCodepoint != INDEX_NONE)
		{
			Item.KerningBefore = Font->GetKerning(PreviousGlyphCodepoint, Item.GlyphCodepoint) * Style.FontScale;
		}

		PreviousGlyphCodepoint = Item.GlyphCodepoint;
		TextIndex += CodeUnitCount;
	}
}

void FBMFontSlateRun::EnsureBrushes() const
{
	EnsureItems();
	UBMFontAsset* Font = StyleBlock.IsValid() ? StyleBlock->FontAsset.Get() : nullptr;
	const uint32 CurrentRevision = Font != nullptr ? Font->GetDataRevision() : 0;
	const bool bFontCacheChanged = BrushFontAsset.Get() != Font || BrushDataRevision != CurrentRevision;
	if (!bFontCacheChanged && BrushItemsRevision == ItemsRevision)
	{
		return;
	}

	if (bFontCacheChanged)
	{
		GlyphBrushes.Reset();
		BrushFontAsset = Font;
		BrushDataRevision = CurrentRevision;
	}
	BrushItemsRevision = ItemsRevision;
	if (Font == nullptr || !Font->FontData.IsValid())
	{
		return;
	}

	for (const FGlyphItem& Item : Items)
	{
		if (Item.Glyph != nullptr)
		{
			EnsureGlyphBrush(*Font, Item.GlyphCodepoint, *Item.Glyph);
		}
	}
}

void FBMFontSlateRun::EnsureGlyphBrush(
	UBMFontAsset& Font,
	const int32 GlyphCodepoint,
	const FBMFontGlyph& Glyph) const
{
	if (GlyphBrushes.Contains(GlyphCodepoint))
	{
		return;
	}

	UObject* Resource = Font.GetPageRenderResource(Glyph.Page, Glyph.Channel);
	if (Resource == nullptr)
	{
		return;
	}

	FSlateBrush& Brush = GlyphBrushes.Add(GlyphCodepoint);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.SetResourceObject(Resource);
	Brush.SetImageSize(FVector2D(Glyph.Width, Glyph.Height));
	Brush.SetUVRegion(Glyph.GetUvRegion(Font.FontData.Common));
}

int32 FBMFontSlateRun::FindItemIndex(const int32 TextIndex) const
{
	int32 Low = 0;
	int32 High = Items.Num();
	while (Low < High)
	{
		const int32 Mid = (Low + High) / 2;
		if (Items[Mid].TextStart < TextIndex)
		{
			Low = Mid + 1;
		}
		else
		{
			High = Mid;
		}
	}
	return Low;
}

float FBMFontSlateRun::MeasureItems(const int32 BeginItem, const int32 EndItem) const
{
	const float LetterSpacing = StyleBlock.IsValid() ? StyleBlock->Style.LetterSpacing : 0.0f;
	float Width = 0.0f;
	for (int32 Index = BeginItem; Index < EndItem; ++Index)
	{
		if (Index > BeginItem)
		{
			Width += LetterSpacing + Items[Index].KerningBefore;
		}
		Width += Items[Index].Advance;
	}
	return Width;
}

#endif
