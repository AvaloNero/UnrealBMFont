// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#include "BMFontLayout.h"

#include "BMFontAsset.h"
#include "Containers/StringConv.h"
#include "Internationalization/BreakIterator.h"
#include "Internationalization/IBreakIterator.h"
#include "Internationalization/TextChar.h"

namespace
{
	struct FBMFontLayoutItem
	{
		int32 SourceCodepoint = INDEX_NONE;
		int32 GlyphCodepoint = INDEX_NONE;
		int32 TextStart = 0;
		int32 TextEnd = 0;
		const FBMFontGlyph* Glyph = nullptr;
		float Advance = 0.0f;
		bool bNewLine = false;
		bool bWhitespace = false;
		bool bSoftBreakAfter = false;
		bool bMissingGlyph = false;
	};

	int32 GetKerning(
		const TMap<uint64, int32>& KerningLookup,
		const int32 First,
		const int32 Second)
	{
		if (const int32* Amount = KerningLookup.Find(FBMFontKerningPair::MakeKey(First, Second)))
		{
			return *Amount;
		}
		return 0;
	}

	UTF32CHAR ReadCodepoint(const FStringView Text, const int32 TextIndex, int32& OutCodeUnitCount)
	{
		const uint32 FirstCodeUnit = static_cast<uint32>(Text[TextIndex]);
		OutCodeUnitCount = 1;
		if constexpr (sizeof(TCHAR) == 2)
		{
			if (StringConv::IsHighSurrogate(FirstCodeUnit) && TextIndex + 1 < Text.Len())
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

	const FBMFontGlyph* ResolveGlyph(
		const FBMFontData& FontData,
		const int32 SourceCodepoint,
		const int32 FallbackCodepoint,
		int32& OutGlyphCodepoint)
	{
		if (const FBMFontGlyph* Glyph = FontData.Glyphs.Find(SourceCodepoint))
		{
			OutGlyphCodepoint = SourceCodepoint;
			return Glyph;
		}

		if (FallbackCodepoint != SourceCodepoint)
		{
			if (const FBMFontGlyph* FallbackGlyph = FontData.Glyphs.Find(FallbackCodepoint))
			{
				OutGlyphCodepoint = FallbackCodepoint;
				return FallbackGlyph;
			}
		}

		OutGlyphCodepoint = INDEX_NONE;
		return nullptr;
	}

	void AppendLine(
		const FBMFontData& FontData,
		const TArray<FBMFontLayoutItem>& Items,
		const int32 Begin,
		const int32 End,
		const TMap<uint64, int32>& KerningLookup,
		const FBMFontLayoutSettings& Settings,
		const float Top,
		FBMFontLayoutResult& OutResult)
	{
		FBMFontLayoutLine& Line = OutResult.Lines.AddDefaulted_GetRef();
		Line.FirstGlyphIndex = OutResult.Glyphs.Num();
		Line.Top = Top;
		Line.DrawTop = Top;
		Line.DrawBottom = Top;

		float PenX = 0.0f;
		int32 PreviousCodepoint = INDEX_NONE;
		bool bHasPrevious = false;
		for (int32 Index = Begin; Index < End; ++Index)
		{
			const FBMFontLayoutItem& Item = Items[Index];
			if (bHasPrevious)
			{
				if (PreviousCodepoint != INDEX_NONE && Item.GlyphCodepoint != INDEX_NONE)
				{
					PenX += GetKerning(KerningLookup, PreviousCodepoint, Item.GlyphCodepoint) * Settings.FontScale;
				}
				PenX += Settings.LetterSpacing;
			}

			if (Item.Glyph != nullptr && Item.Glyph->Width > 0 && Item.Glyph->Height > 0)
			{
				FBMFontLayoutGlyph& PlacedGlyph = OutResult.Glyphs.AddDefaulted_GetRef();
				PlacedGlyph.SourceCodepoint = Item.SourceCodepoint;
				PlacedGlyph.GlyphCodepoint = Item.GlyphCodepoint;
				PlacedGlyph.Page = Item.Glyph->Page;
				PlacedGlyph.Channel = Item.Glyph->Channel;
				PlacedGlyph.Position = FVector2f(
					PenX + Item.Glyph->XOffset * Settings.FontScale,
					Top + Item.Glyph->YOffset * Settings.FontScale
				);
				PlacedGlyph.Size = FVector2f(
					Item.Glyph->Width * Settings.FontScale,
					Item.Glyph->Height * Settings.FontScale
				);
				Line.DrawTop = FMath::Min(Line.DrawTop, PlacedGlyph.Position.Y);
				Line.DrawBottom = FMath::Max(Line.DrawBottom, PlacedGlyph.Position.Y + PlacedGlyph.Size.Y);
			}
			else if (Item.bMissingGlyph)
			{
				++OutResult.MissingGlyphCount;
			}

			PenX += Item.Advance;
			PreviousCodepoint = Item.GlyphCodepoint;
			bHasPrevious = true;
		}

		Line.GlyphCount = OutResult.Glyphs.Num() - Line.FirstGlyphIndex;
		Line.Width = PenX;
		OutResult.Size.X = FMath::Max(OutResult.Size.X, PenX);
	}

	int32 TrimTrailingWhitespace(const TArray<FBMFontLayoutItem>& Items, const int32 Begin, int32 End)
	{
		while (End > Begin && Items[End - 1].bWhitespace)
		{
			--End;
		}
		return End;
	}
}

void FBMFontLayoutResult::Reset()
{
	Glyphs.Reset();
	Lines.Reset();
	Size = FVector2f::ZeroVector;
	MissingGlyphCount = 0;
}

void FBMFontLayout::Build(
	const FBMFontData& FontData,
	const FStringView Text,
	const FBMFontLayoutSettings& InSettings,
	FBMFontLayoutResult& OutResult)
{
	TMap<uint64, int32> KerningLookup;
	KerningLookup.Reserve(FontData.KerningPairs.Num());
	for (const FBMFontKerningPair& Pair : FontData.KerningPairs)
	{
		KerningLookup.Add(FBMFontKerningPair::MakeKey(Pair.First, Pair.Second), Pair.Amount);
	}
	BuildWithKerningLookup(FontData, KerningLookup, Text, InSettings, OutResult);
}

void FBMFontLayout::Build(
	const UBMFontAsset& FontAsset,
	const FStringView Text,
	const FBMFontLayoutSettings& InSettings,
	FBMFontLayoutResult& OutResult)
{
	BuildWithKerningLookup(FontAsset.FontData, FontAsset.KerningLookup, Text, InSettings, OutResult);
}

void FBMFontLayout::BuildWithKerningLookup(
	const FBMFontData& FontData,
	const TMap<uint64, int32>& KerningLookup,
	const FStringView Text,
	const FBMFontLayoutSettings& InSettings,
	FBMFontLayoutResult& OutResult)
{
	OutResult.Reset();
	if (!FontData.IsValid() || Text.IsEmpty())
	{
		return;
	}

	FBMFontLayoutSettings Settings = InSettings;
	Settings.FontScale = FMath::Max(Settings.FontScale, 0.001f);
	Settings.LineHeightScale = FMath::Max(Settings.LineHeightScale, 0.001f);

	TSet<int32> SoftBreakPositions;
	TSharedRef<IBreakIterator> BreakIterator = FBreakIterator::CreateLineBreakIterator();
	BreakIterator->SetStringRef(Text);
	for (int32 Position = BreakIterator->ResetToBeginning(); Position != INDEX_NONE; Position = BreakIterator->MoveToNext())
	{
		SoftBreakPositions.Add(Position);
	}

	TArray<FBMFontLayoutItem> Items;
	Items.Reserve(Text.Len());
	const TCHAR* Buffer = Text.GetData();
	int32 TextIndex = 0;
	while (TextIndex < Text.Len())
	{
		int32 CodeUnitCount = 0;
		UTF32CHAR Codepoint = ReadCodepoint(Text, TextIndex, CodeUnitCount);

		if (Codepoint == TEXT('\r'))
		{
			if (TextIndex + CodeUnitCount < Text.Len() && Buffer[TextIndex + CodeUnitCount] == TEXT('\n'))
			{
				++CodeUnitCount;
			}
			Codepoint = TEXT('\n');
		}

		FBMFontLayoutItem& Item = Items.AddDefaulted_GetRef();
		Item.SourceCodepoint = static_cast<int32>(Codepoint);
		Item.TextStart = TextIndex;
		Item.TextEnd = TextIndex + CodeUnitCount;
		Item.bNewLine = Codepoint == TEXT('\n');
		Item.bWhitespace = !Item.bNewLine && FTextChar::IsWhitespace(Codepoint);
		Item.bSoftBreakAfter = SoftBreakPositions.Contains(Item.TextEnd);

		if (!Item.bNewLine)
		{
			if (Item.SourceCodepoint == TEXT('\t'))
			{
				int32 SpaceCodepoint = INDEX_NONE;
				if (const FBMFontGlyph* SpaceGlyph = ResolveGlyph(FontData, TEXT(' '), Settings.FallbackCodepoint, SpaceCodepoint))
				{
					Item.GlyphCodepoint = SpaceCodepoint;
					Item.Advance = SpaceGlyph->XAdvance * Settings.FontScale * 4.0f;
				}
				else
				{
					Item.Advance = FontData.Common.LineHeight * Settings.FontScale * 2.0f;
				}
			}
			else
			{
				Item.Glyph = ResolveGlyph(
					FontData,
					Item.SourceCodepoint,
					Settings.FallbackCodepoint,
					Item.GlyphCodepoint
				);
				Item.bMissingGlyph = Item.Glyph == nullptr;
			}
			if (Item.Advance <= 0.0f)
			{
				Item.Advance = Item.Glyph != nullptr
					? Item.Glyph->XAdvance * Settings.FontScale
					: FontData.Common.LineHeight * Settings.FontScale * 0.5f;
			}
		}

		TextIndex += CodeUnitCount;
	}

	const float LineAdvance = FontData.Common.LineHeight * Settings.FontScale * Settings.LineHeightScale;
	float LineTop = 0.0f;
	const auto EmitLine = [&](const int32 Begin, const int32 End)
	{
		AppendLine(FontData, Items, Begin, End, KerningLookup, Settings, LineTop, OutResult);
		LineTop += LineAdvance;
	};

	const auto LayoutParagraph = [&](const int32 ParagraphBegin, const int32 ParagraphEnd)
	{
		if (ParagraphBegin == ParagraphEnd)
		{
			EmitLine(ParagraphBegin, ParagraphEnd);
			return;
		}

		if (Settings.WrapWidth <= 0.0f)
		{
			EmitLine(ParagraphBegin, TrimTrailingWhitespace(Items, ParagraphBegin, ParagraphEnd));
			return;
		}

		int32 LineBegin = ParagraphBegin;
		while (LineBegin < ParagraphEnd)
		{
			int32 LastSoftBreak = INDEX_NONE;
			bool bEmitted = false;
			float CurrentWidth = 0.0f;
			int32 PreviousCodepoint = INDEX_NONE;
			for (int32 Scan = LineBegin; Scan < ParagraphEnd; ++Scan)
			{
				const FBMFontLayoutItem& Item = Items[Scan];
				if (Scan > LineBegin)
				{
					if (PreviousCodepoint != INDEX_NONE && Item.GlyphCodepoint != INDEX_NONE)
					{
						CurrentWidth += GetKerning(KerningLookup, PreviousCodepoint, Item.GlyphCodepoint)
							* Settings.FontScale;
					}
					CurrentWidth += Settings.LetterSpacing;
				}
				CurrentWidth += Item.Advance;
				PreviousCodepoint = Item.GlyphCodepoint;

				if (CurrentWidth > Settings.WrapWidth && Scan > LineBegin)
				{
					int32 LineEnd = INDEX_NONE;
					int32 NextLineBegin = INDEX_NONE;
					if (LastSoftBreak > LineBegin)
					{
						LineEnd = TrimTrailingWhitespace(Items, LineBegin, LastSoftBreak);
						NextLineBegin = LastSoftBreak;
					}
					else if (Settings.WrappingPolicy == ETextWrappingPolicy::AllowPerCharacterWrapping)
					{
						LineEnd = Scan;
						NextLineBegin = Scan;
					}

					if (LineEnd != INDEX_NONE)
					{
						if (LineEnd > LineBegin)
						{
							EmitLine(LineBegin, LineEnd);
						}
						while (NextLineBegin < ParagraphEnd && Items[NextLineBegin].bWhitespace)
						{
							++NextLineBegin;
						}
						LineBegin = NextLineBegin;
						bEmitted = true;
						break;
					}
				}

				if (Items[Scan].bSoftBreakAfter)
				{
					LastSoftBreak = Scan + 1;
				}
			}

			if (!bEmitted)
			{
				EmitLine(LineBegin, TrimTrailingWhitespace(Items, LineBegin, ParagraphEnd));
				break;
			}
		}
	};

	int32 ParagraphBegin = 0;
	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		if (Items[Index].bNewLine)
		{
			LayoutParagraph(ParagraphBegin, Index);
			ParagraphBegin = Index + 1;
		}
	}
	LayoutParagraph(ParagraphBegin, Items.Num());

	OutResult.Size.Y = OutResult.Lines.Num() * LineAdvance;
	if (!Settings.bApplyLineHeightToBottomLine && !OutResult.Lines.IsEmpty())
	{
		const float UnscaledLineAdvance = FontData.Common.LineHeight * Settings.FontScale;
		OutResult.Size.Y -= LineAdvance - UnscaledLineAdvance;
	}
}
