// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#include "BMFontText.h"

#include "BMFontAsset.h"
#include "Widgets/SBMFontText.h"

#define LOCTEXT_NAMESPACE "UnrealBMFontText"

UBMFontText::UBMFontText(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ColorAndOpacity(FLinearColor::White)
{
#if WITH_EDITORONLY_DATA
	AccessibleBehavior = ESlateAccessibleBehavior::Auto;
	bCanChildrenBeAccessible = false;
#endif
}

FText UBMFontText::GetText() const
{
	if (MyBMFontText.IsValid())
	{
		return MyBMFontText->GetText();
	}
	return Text;
}

void UBMFontText::SetText(FText InText)
{
	TextDelegate.Unbind();
	Text = MoveTemp(InText);
	if (MyBMFontText.IsValid())
	{
		MyBMFontText->SetText(Text);
	}
}

UBMFontAsset* UBMFontText::GetFontAsset() const
{
	return FontAsset;
}

void UBMFontText::SetFontAsset(UBMFontAsset* InFontAsset)
{
	FontAsset = InFontAsset;
	if (MyBMFontText.IsValid())
	{
		MyBMFontText->SetFontAsset(InFontAsset);
	}
}

FSlateColor UBMFontText::GetColorAndOpacity() const
{
	if (ColorAndOpacityDelegate.IsBound() && !IsDesignTime())
	{
		return ColorAndOpacityDelegate.Execute();
	}
	return ColorAndOpacity;
}

void UBMFontText::SetColorAndOpacity(const FSlateColor InColorAndOpacity)
{
	ColorAndOpacity = InColorAndOpacity;
	if (MyBMFontText.IsValid())
	{
		MyBMFontText->SetColorAndOpacity(ColorAndOpacity);
	}
}

float UBMFontText::GetFontScale() const
{
	return FontScale;
}

void UBMFontText::SetFontScale(const float InFontScale)
{
	FontScale = FMath::Max(InFontScale, 0.001f);
	if (MyBMFontText.IsValid())
	{
		MyBMFontText->SetFontScale(FontScale);
	}
}

float UBMFontText::GetLetterSpacing() const
{
	return LetterSpacing;
}

void UBMFontText::SetLetterSpacing(const float InLetterSpacing)
{
	LetterSpacing = InLetterSpacing;
	if (MyBMFontText.IsValid())
	{
		MyBMFontText->SetLetterSpacing(LetterSpacing);
	}
}

int32 UBMFontText::GetFallbackCodepoint() const
{
	return FallbackCodepoint;
}

void UBMFontText::SetFallbackCodepoint(const int32 InFallbackCodepoint)
{
	FallbackCodepoint = FMath::Clamp(InFallbackCodepoint, 0, 0x10FFFF);
	if (MyBMFontText.IsValid())
	{
		MyBMFontText->SetFallbackCodepoint(FallbackCodepoint);
	}
}

FVector2D UBMFontText::GetShadowOffset() const
{
	return ShadowOffset;
}

void UBMFontText::SetShadowOffset(const FVector2D InShadowOffset)
{
	ShadowOffset = InShadowOffset;
	if (MyBMFontText.IsValid())
	{
		MyBMFontText->SetShadowOffset(ShadowOffset);
	}
}

FLinearColor UBMFontText::GetShadowColorAndOpacity() const
{
	if (ShadowColorAndOpacityDelegate.IsBound() && !IsDesignTime())
	{
		return ShadowColorAndOpacityDelegate.Execute();
	}
	return ShadowColorAndOpacity;
}

void UBMFontText::SetShadowColorAndOpacity(const FLinearColor InShadowColorAndOpacity)
{
	ShadowColorAndOpacity = InShadowColorAndOpacity;
	if (MyBMFontText.IsValid())
	{
		MyBMFontText->SetShadowColorAndOpacity(ShadowColorAndOpacity);
	}
}

bool UBMFontText::GetPixelSnapping() const
{
	return bPixelSnapping;
}

void UBMFontText::SetPixelSnapping(const bool bInPixelSnapping)
{
	bPixelSnapping = bInPixelSnapping;
	if (MyBMFontText.IsValid())
	{
		MyBMFontText->SetPixelSnapping(bPixelSnapping);
	}
}

void UBMFontText::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (!MyBMFontText.IsValid())
	{
		return;
	}

	MyBMFontText->SetText(PROPERTY_BINDING(FText, Text));
	MyBMFontText->SetFontAsset(FontAsset);
	MyBMFontText->SetColorAndOpacity(PROPERTY_BINDING(FSlateColor, ColorAndOpacity));
	MyBMFontText->SetFontScale(FontScale);
	MyBMFontText->SetLetterSpacing(LetterSpacing);
	MyBMFontText->SetFallbackCodepoint(FallbackCodepoint);
	MyBMFontText->SetJustification(Justification);
	MyBMFontText->SetWrappingPolicy(WrappingPolicy);
	MyBMFontText->SetAutoWrapText(AutoWrapText != 0);
	MyBMFontText->SetWrapTextAt(WrapTextAt);
	MyBMFontText->SetLineHeightPercentage(LineHeightPercentage);
	MyBMFontText->SetApplyLineHeightToBottomLine(ApplyLineHeightToBottomLine);
	MyBMFontText->SetMargin(Margin);
	MyBMFontText->SetShadowOffset(ShadowOffset);
	MyBMFontText->SetShadowColorAndOpacity(PROPERTY_BINDING(FLinearColor, ShadowColorAndOpacity));
	MyBMFontText->SetPixelSnapping(bPixelSnapping);
}

void UBMFontText::ReleaseSlateResources(const bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyBMFontText.Reset();
}

#if WITH_EDITOR
const FText UBMFontText::GetPaletteCategory()
{
	return LOCTEXT("PaletteCategory", "Text");
}
#endif

TSharedRef<SWidget> UBMFontText::RebuildWidget()
{
	MyBMFontText = SNew(SBMFontText)
		.Text(PROPERTY_BINDING(FText, Text))
		.FontAsset(FontAsset)
		.ColorAndOpacity(PROPERTY_BINDING(FSlateColor, ColorAndOpacity))
		.FontScale(FontScale)
		.LetterSpacing(LetterSpacing)
		.FallbackCodepoint(FallbackCodepoint)
		.Justification(Justification)
		.WrappingPolicy(WrappingPolicy)
		.AutoWrapText(AutoWrapText != 0)
		.WrapTextAt(WrapTextAt)
		.LineHeightPercentage(LineHeightPercentage)
		.ApplyLineHeightToBottomLine(ApplyLineHeightToBottomLine)
		.Margin(Margin)
		.ShadowOffset(ShadowOffset)
		.ShadowColorAndOpacity(PROPERTY_BINDING(FLinearColor, ShadowColorAndOpacity))
		.PixelSnapping(bPixelSnapping);
	return MyBMFontText.ToSharedRef();
}

void UBMFontText::OnBindingChanged(const FName& Property)
{
	Super::OnBindingChanged(Property);
	if (!MyBMFontText.IsValid())
	{
		return;
	}

	static const FName TextProperty(TEXT("TextDelegate"));
	static const FName ColorProperty(TEXT("ColorAndOpacityDelegate"));
	static const FName ShadowColorProperty(TEXT("ShadowColorAndOpacityDelegate"));
	if (Property == TextProperty)
	{
		MyBMFontText->SetText(PROPERTY_BINDING(FText, Text));
	}
	else if (Property == ColorProperty)
	{
		MyBMFontText->SetColorAndOpacity(PROPERTY_BINDING(FSlateColor, ColorAndOpacity));
	}
	else if (Property == ShadowColorProperty)
	{
		MyBMFontText->SetShadowColorAndOpacity(PROPERTY_BINDING(FLinearColor, ShadowColorAndOpacity));
	}
}

#if WITH_ACCESSIBILITY
TSharedPtr<SWidget> UBMFontText::GetAccessibleWidget() const
{
	return MyBMFontText;
}
#endif

void UBMFontText::OnJustificationChanged(const ETextJustify::Type InJustification)
{
	if (MyBMFontText.IsValid())
	{
		MyBMFontText->SetJustification(InJustification);
	}
}

void UBMFontText::OnWrappingPolicyChanged(const ETextWrappingPolicy InWrappingPolicy)
{
	if (MyBMFontText.IsValid())
	{
		MyBMFontText->SetWrappingPolicy(InWrappingPolicy);
	}
}

void UBMFontText::OnAutoWrapTextChanged(const bool InAutoWrapText)
{
	if (MyBMFontText.IsValid())
	{
		MyBMFontText->SetAutoWrapText(InAutoWrapText);
	}
}

void UBMFontText::OnWrapTextAtChanged(const float InWrapTextAt)
{
	if (MyBMFontText.IsValid())
	{
		MyBMFontText->SetWrapTextAt(InWrapTextAt);
	}
}

void UBMFontText::OnLineHeightPercentageChanged(const float InLineHeightPercentage)
{
	if (MyBMFontText.IsValid())
	{
		MyBMFontText->SetLineHeightPercentage(InLineHeightPercentage);
	}
}

void UBMFontText::OnApplyLineHeightToBottomLineChanged(const bool bInApplyLineHeightToBottomLine)
{
	if (MyBMFontText.IsValid())
	{
		MyBMFontText->SetApplyLineHeightToBottomLine(bInApplyLineHeightToBottomLine);
	}
}

void UBMFontText::OnMarginChanged(const FMargin& InMargin)
{
	if (MyBMFontText.IsValid())
	{
		MyBMFontText->SetMargin(InMargin);
	}
}

#undef LOCTEXT_NAMESPACE
