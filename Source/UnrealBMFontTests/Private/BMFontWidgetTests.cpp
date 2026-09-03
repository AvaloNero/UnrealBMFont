// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#if WITH_DEV_AUTOMATION_TESTS

#include "BMFontAsset.h"
#include "Engine/Texture2D.h"
#include "HAL/PlatformTime.h"
#include "Misc/AutomationTest.h"
#include "Widgets/SBMFontText.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontSlateWidgetDesiredSizeTest,
	"UnrealBMFont.Widget.DesiredSizeAndInvalidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontSlateWidgetDesiredSizeTest::RunTest(const FString& Parameters)
{
	UBMFontAsset* Asset = NewObject<UBMFontAsset>();
	FBMFontData Data;
	Data.DescriptorFormat = EBMFontDescriptorFormat::Text;
	Data.Common.LineHeight = 20;
	Data.Common.Base = 15;
	Data.Common.ScaleWidth = 32;
	Data.Common.ScaleHeight = 32;
	Data.Common.PageCount = 1;
	FBMFontPage& Page = Data.Pages.AddDefaulted_GetRef();
	Page.Id = 0;
	Page.File = TEXT("atlas.png");
	FBMFontGlyph& Glyph = Data.Glyphs.Add(65);
	Glyph.Codepoint = 65;
	Glyph.Width = 8;
	Glyph.Height = 10;
	Glyph.XAdvance = 10;
	Glyph.Page = 0;
	Asset->SetFontData(MoveTemp(Data));

	TSharedRef<SBMFontText> Widget = SNew(SBMFontText)
		.Text(FText::FromString(TEXT("AA")))
		.FontAsset(Asset)
		.LetterSpacing(1.0f)
		.Margin(FMargin(1.0f, 2.0f, 3.0f, 4.0f));
	Widget->SlatePrepass(1.0f);
	TestEqual(TEXT("Desired width includes tracking and horizontal margins"), Widget->GetDesiredSize().X, 25.0f);
	TestEqual(TEXT("Desired height includes line height and vertical margins"), Widget->GetDesiredSize().Y, 26.0f);

	Widget->SetText(FText::FromString(TEXT("A")));
	Widget->SlatePrepass(1.0f);
	TestEqual(TEXT("Changing text invalidates desired width"), Widget->GetDesiredSize().X, 14.0f);
	TestEqual(TEXT("Display text follows the Slate attribute"), Widget->GetText().ToString(), FString(TEXT("A")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontSlateWidgetBrushCacheTest,
	"UnrealBMFont.Widget.LayoutScopedBrushCache",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontSlateWidgetBrushCacheTest::RunTest(const FString& Parameters)
{
	UBMFontAsset* Asset = NewObject<UBMFontAsset>();
	FBMFontData Data;
	Data.DescriptorFormat = EBMFontDescriptorFormat::Text;
	Data.Common.LineHeight = 20;
	Data.Common.Base = 15;
	Data.Common.ScaleWidth = 32;
	Data.Common.ScaleHeight = 32;
	Data.Common.PageCount = 1;
	FBMFontPage& Page = Data.Pages.AddDefaulted_GetRef();
	Page.Id = 0;
	Page.File = TEXT("atlas.png");
	Page.Texture = NewObject<UTexture2D>();

	constexpr int32 LargeGlyphCount = 20000;
	for (int32 GlyphIndex = 0; GlyphIndex < LargeGlyphCount; ++GlyphIndex)
	{
		const int32 Codepoint = GlyphIndex + 32;
		FBMFontGlyph& Glyph = Data.Glyphs.Add(Codepoint);
		Glyph.Codepoint = Codepoint;
		Glyph.X = GlyphIndex % 32;
		Glyph.Y = (GlyphIndex / 32) % 32;
		Glyph.Width = 1;
		Glyph.Height = 1;
		Glyph.XAdvance = 1;
		Glyph.Page = 0;
	}
	Asset->SetFontData(MoveTemp(Data));

	TSharedRef<SBMFontText> Widget = SNew(SBMFontText)
		.Text(FText::FromString(TEXT("ABBA")))
		.FontAsset(Asset);
	const double CacheStart = FPlatformTime::Seconds();
	TestEqual(
		TEXT("Only glyphs used by the current layout receive brushes"),
		Widget->GetCachedGlyphBrushCountForTesting(100.0f),
		2
	);
	const double CacheSeconds = FPlatformTime::Seconds() - CacheStart;
	AddInfo(FString::Printf(TEXT("Cached two brushes from a %d-glyph font in %.3f ms."), LargeGlyphCount, CacheSeconds * 1000.0));
	TestTrue(TEXT("Layout-scoped cache creation stays below the regression ceiling"), CacheSeconds < 2.0);

	Widget->SetText(FText::FromString(TEXT("C")));
	TestEqual(
		TEXT("Changing text drops brushes no longer used by the layout"),
		Widget->GetCachedGlyphBrushCountForTesting(100.0f),
		1
	);
	return true;
}

#endif
