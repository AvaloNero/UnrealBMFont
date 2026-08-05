// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#if WITH_DEV_AUTOMATION_TESTS

#include "BMFontLayout.h"
#include "Internationalization/TextChar.h"
#include "Misc/AutomationTest.h"

namespace
{
	FBMFontGlyph MakeGlyph(
		const int32 Codepoint,
		const int32 Width,
		const int32 Height,
		const int32 XOffset,
		const int32 YOffset,
		const int32 XAdvance)
	{
		FBMFontGlyph Glyph;
		Glyph.Codepoint = Codepoint;
		Glyph.Width = Width;
		Glyph.Height = Height;
		Glyph.XOffset = XOffset;
		Glyph.YOffset = YOffset;
		Glyph.XAdvance = XAdvance;
		Glyph.Page = 0;
		return Glyph;
	}

	FBMFontData MakeLayoutFont()
	{
		FBMFontData Data;
		Data.DescriptorFormat = EBMFontDescriptorFormat::Text;
		Data.Common.LineHeight = 20;
		Data.Common.Base = 15;
		Data.Common.ScaleWidth = 64;
		Data.Common.ScaleHeight = 64;
		Data.Common.PageCount = 1;
		FBMFontPage& Page = Data.Pages.AddDefaulted_GetRef();
		Page.Id = 0;
		Page.File = TEXT("atlas.png");
		Data.Glyphs.Add(32, MakeGlyph(32, 0, 0, 0, 0, 5));
		Data.Glyphs.Add(65, MakeGlyph(65, 8, 10, 1, 2, 10));
		Data.Glyphs.Add(0xFFFD, MakeGlyph(0xFFFD, 8, 10, 0, 1, 9));
		Data.Glyphs.Add(0x1F600, MakeGlyph(0x1F600, 12, 12, 0, 1, 13));
		FBMFontKerningPair& Kerning = Data.KerningPairs.AddDefaulted_GetRef();
		Kerning.First = 65;
		Kerning.Second = 65;
		Kerning.Amount = -2;
		return Data;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontMetricsLayoutTest,
	"UnrealBMFont.Layout.MetricsAndKerning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontMetricsLayoutTest::RunTest(const FString& Parameters)
{
	const FBMFontData Data = MakeLayoutFont();
	FBMFontLayoutSettings Settings;
	Settings.LetterSpacing = 1.0f;
	FBMFontLayoutResult Layout;
	FBMFontLayout::Build(Data, TEXTVIEW("AA"), Settings, Layout);

	TestEqual(TEXT("One line"), Layout.Lines.Num(), 1);
	TestEqual(TEXT("Two visible glyphs"), Layout.Glyphs.Num(), 2);
	TestEqual(TEXT("Kerning and tracking affect width"), Layout.Size.X, 19.0f);
	TestEqual(TEXT("First glyph xoffset"), Layout.Glyphs[0].Position.X, 1.0f);
	TestEqual(TEXT("Second glyph position"), Layout.Glyphs[1].Position.X, 10.0f);
	TestEqual(TEXT("Glyph yoffset"), Layout.Glyphs[0].Position.Y, 2.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontWrappingLayoutTest,
	"UnrealBMFont.Layout.Wrapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontWrappingLayoutTest::RunTest(const FString& Parameters)
{
	const FBMFontData Data = MakeLayoutFont();
	FBMFontLayoutSettings Settings;
	Settings.WrapWidth = 15.0f;
	FBMFontLayoutResult Layout;
	FBMFontLayout::Build(Data, TEXTVIEW("A A"), Settings, Layout);

	TestEqual(TEXT("Wraps at the soft break"), Layout.Lines.Num(), 2);
	TestEqual(TEXT("First line width excludes trailing space"), Layout.Lines[0].Width, 10.0f);
	TestEqual(TEXT("Second line starts at next line"), Layout.Glyphs[1].Position.Y, 22.0f);
	TestEqual(TEXT("Total height"), Layout.Size.Y, 40.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontUnicodeAndFallbackLayoutTest,
	"UnrealBMFont.Layout.UnicodeAndFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontUnicodeAndFallbackLayoutTest::RunTest(const FString& Parameters)
{
	const FBMFontData Data = MakeLayoutFont();
	FString Text;
	TestTrue(TEXT("Supplementary codepoint can be encoded"), FTextChar::AppendCodepointToString(0x1F600, Text));
	Text.AppendChar(TEXT('B'));

	FBMFontLayoutSettings Settings;
	FBMFontLayoutResult Layout;
	FBMFontLayout::Build(Data, Text, Settings, Layout);
	TestEqual(TEXT("Supplementary character is one glyph"), Layout.Glyphs.Num(), 2);
	TestEqual(TEXT("Supplementary codepoint is preserved"), Layout.Glyphs[0].GlyphCodepoint, 0x1F600);
	TestEqual(TEXT("Missing character uses fallback"), Layout.Glyphs[1].GlyphCodepoint, 0xFFFD);
	TestEqual(TEXT("Fallback is not counted as missing"), Layout.MissingGlyphCount, 0);

	Settings.FallbackCodepoint = 0;
	FBMFontLayout::Build(Data, TEXTVIEW("B"), Settings, Layout);
	TestEqual(TEXT("Unresolved glyph is counted"), Layout.MissingGlyphCount, 1);
	TestEqual(TEXT("Unresolved glyph still advances"), Layout.Size.X, 10.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontWhitespaceAndLineHeightLayoutTest,
	"UnrealBMFont.Layout.WhitespaceAndLineHeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontWhitespaceAndLineHeightLayoutTest::RunTest(const FString& Parameters)
{
	const FBMFontData Data = MakeLayoutFont();
	FBMFontLayoutSettings Settings;
	Settings.LineHeightScale = 1.5f;
	Settings.bApplyLineHeightToBottomLine = false;

	FBMFontLayoutResult Layout;
	FBMFontLayout::Build(Data, TEXTVIEW("A\tA\nA"), Settings, Layout);
	TestEqual(TEXT("Tab does not produce a visible glyph"), Layout.Glyphs.Num(), 3);
	TestEqual(TEXT("Tab is not reported as a missing glyph"), Layout.MissingGlyphCount, 0);
	TestEqual(TEXT("Two explicit lines"), Layout.Lines.Num(), 2);
	TestEqual(TEXT("Line-height spacing is used between lines"), Layout.Glyphs[2].Position.Y, 32.0f);
	TestEqual(TEXT("Bottom line uses the unscaled height"), Layout.Size.Y, 50.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontTrailingWhitespaceLayoutTest,
	"UnrealBMFont.Layout.TrailingWhitespace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontTrailingWhitespaceLayoutTest::RunTest(const FString& Parameters)
{
	const FBMFontData Data = MakeLayoutFont();
	FBMFontLayoutSettings Settings;
	FBMFontLayoutResult UnwrappedLayout;
	FBMFontLayout::Build(Data, TEXTVIEW("A "), Settings, UnwrappedLayout);

	Settings.WrapWidth = 100.0f;
	FBMFontLayoutResult WrappedLayout;
	FBMFontLayout::Build(Data, TEXTVIEW("A "), Settings, WrappedLayout);

	TestEqual(TEXT("Unwrapped width excludes trailing space"), UnwrappedLayout.Size.X, 10.0f);
	TestEqual(TEXT("Wrapped width excludes trailing space"), WrappedLayout.Size.X, 10.0f);
	TestEqual(TEXT("Wrapping does not change trailing-space width"), WrappedLayout.Size.X, UnwrappedLayout.Size.X);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontBoundedStringViewLayoutTest,
	"UnrealBMFont.Layout.BoundedStringView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontBoundedStringViewLayoutTest::RunTest(const FString& Parameters)
{
	if constexpr (sizeof(TCHAR) != 2)
	{
		AddInfo(TEXT("The bounded UTF-16 surrogate test only applies to 16-bit TCHAR platforms."));
		return true;
	}

	FBMFontData Data = MakeLayoutFont();
	Data.Glyphs.Add(0xD800, MakeGlyph(0xD800, 6, 10, 0, 0, 7));
	Data.Glyphs.Add(0x10000, MakeGlyph(0x10000, 12, 10, 0, 0, 17));

	const TCHAR Buffer[] = {
		static_cast<TCHAR>(0xD800),
		static_cast<TCHAR>(0xDC00)
	};
	const FStringView BoundedView(Buffer, 1);
	FBMFontLayoutSettings Settings;
	FBMFontLayoutResult Layout;
	FBMFontLayout::Build(Data, BoundedView, Settings, Layout);

	TestEqual(TEXT("The view contains one glyph"), Layout.Glyphs.Num(), 1);
	if (!Layout.Glyphs.IsEmpty())
	{
		TestEqual(TEXT("A surrogate outside the view is not consumed"), Layout.Glyphs[0].GlyphCodepoint, 0xD800);
	}
	TestEqual(TEXT("The in-view code unit controls the advance"), Layout.Size.X, 7.0f);
	return true;
}

#endif
