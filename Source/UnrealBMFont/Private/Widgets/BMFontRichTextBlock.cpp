// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#include "Widgets/BMFontRichTextBlock.h"

#include "BMFontAsset.h"
#include "Framework/Application/SlateApplication.h"
#include "RichText/BMFontTextDecorator.h"
#include "Widgets/Text/SRichTextBlock.h"

#define LOCTEXT_NAMESPACE "UnrealBMFontRichTextBlock"

UBMFontRichTextBlock::UBMFontRichTextBlock(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ColorAndOpacity(FLinearColor::White)
{
}

UBMFontAsset* UBMFontRichTextBlock::GetFontAsset() const
{
	return FontAsset;
}

void UBMFontRichTextBlock::SetFontAsset(UBMFontAsset* InFontAsset)
{
	FontAsset = InFontAsset;
	UpdateDecoratorConfig();
}

FSlateColor UBMFontRichTextBlock::GetColorAndOpacity() const
{
	return ColorAndOpacity;
}

void UBMFontRichTextBlock::SetColorAndOpacity(const FSlateColor InColorAndOpacity)
{
	ColorAndOpacity = InColorAndOpacity;
	UpdateDecoratorConfig();
}

float UBMFontRichTextBlock::GetFontScale() const
{
	return FontScale;
}

void UBMFontRichTextBlock::SetFontScale(const float InFontScale)
{
	FontScale = FMath::Max(InFontScale, 0.001f);
	UpdateDecoratorConfig();
}

float UBMFontRichTextBlock::GetLetterSpacing() const
{
	return LetterSpacing;
}

void UBMFontRichTextBlock::SetLetterSpacing(const float InLetterSpacing)
{
	LetterSpacing = InLetterSpacing;
	UpdateDecoratorConfig();
}

int32 UBMFontRichTextBlock::GetFallbackCodepoint() const
{
	return FallbackCodepoint;
}

void UBMFontRichTextBlock::SetFallbackCodepoint(const int32 InFallbackCodepoint)
{
	FallbackCodepoint = FMath::Clamp(InFallbackCodepoint, 0, 0x10FFFF);
	UpdateDecoratorConfig();
}

FVector2D UBMFontRichTextBlock::GetShadowOffset() const
{
	return ShadowOffset;
}

void UBMFontRichTextBlock::SetShadowOffset(const FVector2D InShadowOffset)
{
	ShadowOffset = InShadowOffset;
	UpdateDecoratorConfig();
}

FLinearColor UBMFontRichTextBlock::GetShadowColorAndOpacity() const
{
	return ShadowColorAndOpacity;
}

void UBMFontRichTextBlock::SetShadowColorAndOpacity(const FLinearColor InShadowColorAndOpacity)
{
	ShadowColorAndOpacity = InShadowColorAndOpacity;
	UpdateDecoratorConfig();
}

FName UBMFontRichTextBlock::GetDecoratorTag() const
{
	return DecoratorTag;
}

void UBMFontRichTextBlock::SetDecoratorTag(const FName InDecoratorTag)
{
	DecoratorTag = InDecoratorTag;
	UpdateDecoratorMatching();
}

bool UBMFontRichTextBlock::GetDecoratePlainTextRuns() const
{
	return bDecoratePlainTextRuns;
}

void UBMFontRichTextBlock::SetDecoratePlainTextRuns(const bool bInDecoratePlainTextRuns)
{
	bDecoratePlainTextRuns = bInDecoratePlainTextRuns;
	UpdateDecoratorMatching();
}

void UBMFontRichTextBlock::SynchronizeProperties()
{
	// Configure the decorator before the base class re-pushes the text, so any
	// resulting re-marshal already sees the new configuration.
	UpdateDecoratorConfig();
	UpdateDecoratorMatching();
	Super::SynchronizeProperties();
}

void UBMFontRichTextBlock::ReleaseSlateResources(const bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	ActiveDecorator.Reset();
}

#if WITH_EDITOR
const FText UBMFontRichTextBlock::GetPaletteCategory()
{
	return LOCTEXT("PaletteCategory", "Text");
}
#endif

void UBMFontRichTextBlock::CreateDecorators(TArray<TSharedRef<ITextDecorator>>& OutDecorators)
{
	Super::CreateDecorators(OutDecorators);

	ActiveDecorator = MakeShared<FBMFontTextDecorator>(
		FontAsset,
		MakeRunStyle(),
		DecoratorTag.ToString(),
		bDecoratePlainTextRuns
	);
	OutDecorators.Add(ActiveDecorator.ToSharedRef());
}

FBMFontRunStyle UBMFontRichTextBlock::MakeRunStyle() const
{
	FBMFontRunStyle RunStyle;
	RunStyle.Color = ColorAndOpacity.GetSpecifiedColor();
	RunStyle.ShadowColor = ShadowColorAndOpacity;
	RunStyle.ShadowOffset = ShadowOffset;
	RunStyle.FontScale = FontScale;
	RunStyle.LetterSpacing = LetterSpacing;
	RunStyle.FallbackCodepoint = FallbackCodepoint;
	return RunStyle;
}

void UBMFontRichTextBlock::UpdateDecoratorConfig()
{
	if (!ActiveDecorator.IsValid())
	{
		return;
	}

	// The layout caches measured runs and does not observe decorator changes on its
	// own, so configuration updates force a re-marshal; new runs read the shared block.
	ActiveDecorator->ApplyStyle(FontAsset, MakeRunStyle());
	RefreshRuns();
}

void UBMFontRichTextBlock::UpdateDecoratorMatching()
{
	if (!ActiveDecorator.IsValid())
	{
		return;
	}

	ActiveDecorator->SetMatching(DecoratorTag.ToString(), bDecoratePlainTextRuns);
	RefreshRuns();
}

void UBMFontRichTextBlock::RefreshRuns()
{
	if (!MyRichTextBlock.IsValid())
	{
		return;
	}

	// Matching-rule changes alter which runs get claimed, which only happens during a
	// re-marshal. SRichTextBlock and the layout both early-out on identical text, so
	// force the change through an intermediate clear observed by a prepass.
	const FText CurrentText = GetText();
	MyRichTextBlock->SetText(FText::GetEmpty());
	if (FSlateApplication::IsInitialized())
	{
		MyRichTextBlock->SlatePrepass(FSlateApplication::Get().GetApplicationScale());
	}
	MyRichTextBlock->SetText(CurrentText);
}

#undef LOCTEXT_NAMESPACE
