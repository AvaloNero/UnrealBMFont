// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#if WITH_DEV_AUTOMATION_TESTS

#include "BMFontAsset.h"
#include "Misc/AutomationTest.h"
#include "Widgets/BMFontRichTextBlock.h"
#include "Widgets/SWidget.h"

namespace
{
	UBMFontAsset* MakeRichTextFontAsset()
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

		const auto AddGlyph = [&Data](const int32 Codepoint, const int32 Advance)
		{
			FBMFontGlyph& Glyph = Data.Glyphs.Add(Codepoint);
			Glyph.Codepoint = Codepoint;
			Glyph.Width = 8;
			Glyph.Height = 10;
			Glyph.XAdvance = Advance;
			Glyph.Page = 0;
		};
		AddGlyph(TEXT('A'), 10);
		AddGlyph(TEXT('V'), 10);
		AddGlyph(0xFFFD, 6);

		FBMFontKerningPair& Pair = Data.KerningPairs.AddDefaulted_GetRef();
		Pair.First = TEXT('A');
		Pair.Second = TEXT('V');
		Pair.Amount = -2;

		Asset->SetFontData(MoveTemp(Data));
		return Asset;
	}

	FVector2D GetRichTextDesiredSize(UBMFontRichTextBlock* Widget)
	{
		TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();
		SlateWidget->SlatePrepass(1.0f);
		return SlateWidget->GetDesiredSize();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontRichTextPlainRunsTest,
	"UnrealBMFont.RichText.PlainRuns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontRichTextPlainRunsTest::RunTest(const FString& Parameters)
{
	UBMFontAsset* Asset = MakeRichTextFontAsset();

	UBMFontRichTextBlock* Widget = NewObject<UBMFontRichTextBlock>();
	Widget->SetFontAsset(Asset);
	Widget->SetDecoratePlainTextRuns(true);
	Widget->SetText(FText::FromString(TEXT("AV")));

	FVector2D DesiredSize = GetRichTextDesiredSize(Widget);
	TestEqual(TEXT("Plain run width applies advances and kerning"), DesiredSize.X, 18.0, 0.5);
	TestEqual(TEXT("Plain run height uses the line height"), DesiredSize.Y, 20.0, 0.5);

	Widget->SetLetterSpacing(1.0f);
	DesiredSize = GetRichTextDesiredSize(Widget);
	TestEqual(TEXT("Letter spacing widens the plain run"), DesiredSize.X, 19.0, 0.5);

	Widget->SetLetterSpacing(0.0f);
	Widget->SetText(FText::FromString(TEXT("A")));
	Widget->SetShadowOffset(FVector2D(-2.0f, -3.0f));
	Widget->SetShadowColorAndOpacity(FLinearColor::Black);
	DesiredSize = GetRichTextDesiredSize(Widget);
	TestEqual(TEXT("Negative shadow expands the run width"), DesiredSize.X, 12.0, 0.5);
	TestEqual(TEXT("Negative shadow expands the run height"), DesiredSize.Y, 23.0, 0.5);
	Widget->SetShadowOffset(FVector2D::ZeroVector);
	Widget->SetShadowColorAndOpacity(FLinearColor::Transparent);

	Widget->SetText(FText::FromString(FString::Chr(TEXT('A')) + FString::Chr(0x4E2D)));
	DesiredSize = GetRichTextDesiredSize(Widget);
	TestEqual(TEXT("Missing codepoints fall back to the fallback glyph advance"), DesiredSize.X, 16.0, 0.5);

	Widget->SetText(FText::FromString(TEXT("A\nV")));
	DesiredSize = GetRichTextDesiredSize(Widget);
	TestEqual(TEXT("Explicit line breaks stack line heights"), DesiredSize.Y, 40.0, 0.5);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontRichTextTaggedRunsTest,
	"UnrealBMFont.RichText.TaggedRuns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontRichTextTaggedRunsTest::RunTest(const FString& Parameters)
{
	UBMFontAsset* Asset = MakeRichTextFontAsset();

	UBMFontRichTextBlock* Widget = NewObject<UBMFontRichTextBlock>();
	Widget->SetFontAsset(Asset);
	Widget->SetDecoratePlainTextRuns(false);
	Widget->SetText(FText::FromString(TEXT("<bmfont>AV</>")));

	FVector2D DesiredSize = GetRichTextDesiredSize(Widget);
	TestEqual(TEXT("Tagged run renders through BMFont"), DesiredSize.X, 18.0, 0.5);
	TestEqual(TEXT("Tagged run height uses the line height"), DesiredSize.Y, 20.0, 0.5);

	Widget->SetDecoratorTag(TEXT("fancy"));
	Widget->SetText(FText::FromString(TEXT("<fancy>AV</>")));
	DesiredSize = GetRichTextDesiredSize(Widget);
	TestEqual(TEXT("A renamed tag still matches"), DesiredSize.X, 18.0, 0.5);

	Widget->SetText(FText::FromString(TEXT("<fancy></>")));
	DesiredSize = GetRichTextDesiredSize(Widget);
	TestEqual(TEXT("An empty tagged run does not render its markup"), DesiredSize.X, 0.0, 0.5);

	Widget->SetText(FText::FromString(TEXT("<fancy/>")));
	DesiredSize = GetRichTextDesiredSize(Widget);
	TestEqual(TEXT("A self-closing tagged run does not render its markup"), DesiredSize.X, 0.0, 0.5);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontRichTextNoFontTest,
	"UnrealBMFont.RichText.NoFontAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontRichTextNoFontTest::RunTest(const FString& Parameters)
{
	UBMFontRichTextBlock* Widget = NewObject<UBMFontRichTextBlock>();
	Widget->SetText(FText::FromString(TEXT("<bmfont>AV</>plain")));

	const FVector2D DesiredSize = GetRichTextDesiredSize(Widget);
	TestTrue(TEXT("Desired size stays finite without a font asset"), FMath::IsFinite(DesiredSize.X) && FMath::IsFinite(DesiredSize.Y));

	UBMFontAsset* Asset = MakeRichTextFontAsset();
	Widget->SetFontAsset(Asset);
	Widget->SetDecoratePlainTextRuns(false);
	const FVector2D DecoratedSize = GetRichTextDesiredSize(Widget);
	TestTrue(TEXT("Assigning a font asset at runtime rebuilds runs"), DecoratedSize.X >= 18.0);
	return true;
}

#endif
