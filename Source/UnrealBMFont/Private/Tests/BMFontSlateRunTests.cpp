// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#if WITH_DEV_AUTOMATION_TESTS && WITH_FANCY_TEXT

#include "RichText/BMFontSlateRun.h"

#include "BMFontAsset.h"
#include "Fonts/FontCache.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Text/ShapedTextCache.h"
#include "Framework/Text/TextHitPoint.h"
#include "Misc/AutomationTest.h"
#include "Rendering/SlateRenderer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBMFontSlateRunHitTest,
	"UnrealBMFont.RichText.SlateRunHitTest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBMFontSlateRunHitTest::RunTest(const FString& Parameters)
{
	if (!TestTrue(TEXT("Slate is initialized"), FSlateApplication::IsInitialized()))
	{
		return false;
	}
	FSlateRenderer* SlateRenderer = FSlateApplication::Get().GetRenderer();
	if (!TestNotNull(TEXT("Slate renderer is available"), SlateRenderer))
	{
		return false;
	}

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
	const auto AddGlyph = [&Data](const int32 Codepoint)
	{
		FBMFontGlyph& Glyph = Data.Glyphs.Add(Codepoint);
		Glyph.Codepoint = Codepoint;
		Glyph.Width = 8;
		Glyph.Height = 10;
		Glyph.XAdvance = 10;
		Glyph.Page = 0;
	};
	AddGlyph(TEXT('A'));
	AddGlyph(TEXT('V'));
	FBMFontKerningPair& Pair = Data.KerningPairs.AddDefaulted_GetRef();
	Pair.First = TEXT('A');
	Pair.Second = TEXT('V');
	Pair.Amount = -2;
	Asset->SetFontData(MoveTemp(Data));

	const TSharedRef<const FString> Text = MakeShared<FString>(TEXT("AV"));
	const TSharedPtr<FBMFontRunStyleBlock> StyleBlock = MakeShared<FBMFontRunStyleBlock>();
	StyleBlock->FontAsset = Asset;
	const TSharedRef<FBMFontSlateRun> Run = FBMFontSlateRun::Create(
		FRunInfo(TEXT("bmfont")),
		Text,
		FTextRange(0, Text->Len()),
		StyleBlock
	);

	const FShapedTextCacheRef ShapedTextCache = FShapedTextCache::Create(SlateRenderer->GetFontCache());
	const FRunTextContext RunTextContext(
		ETextShapingMethod::KerningOnly,
		TextBiDi::ETextDirection::LeftToRight,
		ShapedTextCache
	);
	const FLayoutBlockTextContext BlockTextContext(
		RunTextContext,
		TextBiDi::ETextDirection::LeftToRight
	);
	const FVector2D BlockSize = Run->Measure(0, Text->Len(), 1.0f, RunTextContext);
	const TSharedRef<ILayoutBlock> Block = Run->CreateBlock(
		0,
		Text->Len(),
		BlockSize,
		BlockTextContext,
		TSharedPtr<IRunRenderer>()
	);
	const FVector2D BlockOffset(10.0f, 20.0f);
	Block->SetLocationOffset(BlockOffset);

	TestEqual(
		TEXT("A point left of the block does not hit the run"),
		Run->GetTextIndexAt(Block, FVector2D(BlockOffset.X - 0.1f, BlockOffset.Y + 1.0f), 1.0f),
		INDEX_NONE
	);
	TestEqual(
		TEXT("A point at the right edge does not hit the run"),
		Run->GetTextIndexAt(Block, FVector2D(BlockOffset.X + BlockSize.X, BlockOffset.Y + 1.0f), 1.0f),
		INDEX_NONE
	);
	TestEqual(
		TEXT("A point above the block does not hit the run"),
		Run->GetTextIndexAt(Block, FVector2D(BlockOffset.X + 1.0f, BlockOffset.Y - 0.1f), 1.0f),
		INDEX_NONE
	);
	TestEqual(
		TEXT("A point at the bottom edge does not hit the run"),
		Run->GetTextIndexAt(Block, FVector2D(BlockOffset.X + 1.0f, BlockOffset.Y + BlockSize.Y), 1.0f),
		INDEX_NONE
	);

	ETextHitPoint HitPoint = ETextHitPoint::LeftGutter;
	TestEqual(
		TEXT("The left half of the first glyph maps to its leading index"),
		Run->GetTextIndexAt(Block, BlockOffset + FVector2D(1.0f, 1.0f), 1.0f, &HitPoint),
		0
	);
	TestTrue(TEXT("An in-run hit reports WithinText"), HitPoint == ETextHitPoint::WithinText);

	HitPoint = ETextHitPoint::LeftGutter;
	TestEqual(
		TEXT("The trailing edge maps to the block end index"),
		Run->GetTextIndexAt(
			Block,
			BlockOffset + FVector2D(BlockSize.X - 0.1f, 1.0f),
			1.0f,
			&HitPoint
		),
		Text->Len()
	);
	TestTrue(TEXT("A block-end hit reports RightGutter"), HitPoint == ETextHitPoint::RightGutter);
	return true;
}

#endif
