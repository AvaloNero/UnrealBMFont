// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#pragma once

#include "Components/TextWidgetTypes.h"
#include "Styling/SlateColor.h"
#include "BMFontText.generated.h"

class SBMFontText;
class UBMFontAsset;

/** Plain-text UMG widget that renders quads from an imported BMFont asset. */
UCLASS(meta = (DisplayName = "BMFont Text"), HideCategories = (Localization, Font))
class UNREALBMFONT_API UBMFontText : public UTextLayoutWidget
{
	GENERATED_BODY()

public:
	UBMFontText(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter = GetText, Setter = SetText, Category = "Content", meta = (MultiLine = "true"))
	FText Text;

	UPROPERTY()
	FGetText TextDelegate;

	UPROPERTY()
	FGetSlateColor ColorAndOpacityDelegate;

	UPROPERTY()
	FGetLinearColor ShadowColorAndOpacityDelegate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter = GetFontAsset, Setter = SetFontAsset, Category = "BMFont")
	TObjectPtr<UBMFontAsset> FontAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter = GetColorAndOpacity, Setter = SetColorAndOpacity, Category = "Appearance")
	FSlateColor ColorAndOpacity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter = GetFontScale, Setter = SetFontScale, Category = "Appearance", meta = (ClampMin = "0.001"))
	float FontScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter = GetLetterSpacing, Setter = SetLetterSpacing, Category = "Appearance")
	float LetterSpacing = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter = GetFallbackCodepoint, Setter = SetFallbackCodepoint, Category = "BMFont", AdvancedDisplay, meta = (ClampMin = "0", ClampMax = "1114111"))
	int32 FallbackCodepoint = 0xFFFD;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter = GetShadowOffset, Setter = SetShadowOffset, Category = "Appearance")
	FVector2D ShadowOffset = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter = GetShadowColorAndOpacity, Setter = SetShadowColorAndOpacity, Category = "Appearance", meta = (DisplayName = "Shadow Color"))
	FLinearColor ShadowColorAndOpacity = FLinearColor::Transparent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter = GetPixelSnapping, Setter = SetPixelSnapping, Category = "Performance", AdvancedDisplay)
	bool bPixelSnapping = true;

	UFUNCTION(BlueprintPure, Category = "Widget", meta = (DisplayName = "GetText (BMFont Text)"))
	FText GetText() const;

	UFUNCTION(BlueprintCallable, Category = "Widget", meta = (DisplayName = "SetText (BMFont Text)"))
	void SetText(FText InText);

	UFUNCTION(BlueprintPure, Category = "BMFont")
	UBMFontAsset* GetFontAsset() const;

	UFUNCTION(BlueprintCallable, Category = "BMFont")
	void SetFontAsset(UBMFontAsset* InFontAsset);

	FSlateColor GetColorAndOpacity() const;

	UFUNCTION(BlueprintCallable, Category = "Appearance")
	void SetColorAndOpacity(FSlateColor InColorAndOpacity);

	float GetFontScale() const;

	UFUNCTION(BlueprintCallable, Category = "Appearance")
	void SetFontScale(float InFontScale);

	float GetLetterSpacing() const;

	UFUNCTION(BlueprintCallable, Category = "Appearance")
	void SetLetterSpacing(float InLetterSpacing);

	int32 GetFallbackCodepoint() const;

	UFUNCTION(BlueprintCallable, Category = "BMFont")
	void SetFallbackCodepoint(int32 InFallbackCodepoint);

	FVector2D GetShadowOffset() const;

	UFUNCTION(BlueprintCallable, Category = "Appearance")
	void SetShadowOffset(FVector2D InShadowOffset);

	FLinearColor GetShadowColorAndOpacity() const;

	UFUNCTION(BlueprintCallable, Category = "Appearance")
	void SetShadowColorAndOpacity(FLinearColor InShadowColorAndOpacity);

	bool GetPixelSnapping() const;

	UFUNCTION(BlueprintCallable, Category = "Performance")
	void SetPixelSnapping(bool bInPixelSnapping);

	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void OnBindingChanged(const FName& Property) override;
#if WITH_ACCESSIBILITY
	virtual TSharedPtr<SWidget> GetAccessibleWidget() const override;
#endif
	virtual void OnJustificationChanged(ETextJustify::Type InJustification) override;
	virtual void OnWrappingPolicyChanged(ETextWrappingPolicy InWrappingPolicy) override;
	virtual void OnAutoWrapTextChanged(bool InAutoWrapText) override;
	virtual void OnWrapTextAtChanged(float InWrapTextAt) override;
	virtual void OnLineHeightPercentageChanged(float InLineHeightPercentage) override;
	virtual void OnApplyLineHeightToBottomLineChanged(bool bInApplyLineHeightToBottomLine) override;
	virtual void OnMarginChanged(const FMargin& InMargin) override;

	TSharedPtr<SBMFontText> MyBMFontText;
	PROPERTY_BINDING_IMPLEMENTATION(FText, Text);
	PROPERTY_BINDING_IMPLEMENTATION(FSlateColor, ColorAndOpacity);
	PROPERTY_BINDING_IMPLEMENTATION(FLinearColor, ShadowColorAndOpacity);
};
