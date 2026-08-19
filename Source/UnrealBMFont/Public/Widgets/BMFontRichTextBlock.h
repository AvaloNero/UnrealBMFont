// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#pragma once

#include "Components/RichTextBlock.h"
#include "Styling/SlateColor.h"
#include "BMFontRichTextBlock.generated.h"

class FBMFontTextDecorator;
class UBMFontAsset;
struct FBMFontRunStyle;

/**
 * Rich text block whose runs render through an imported BMFont asset.
 * Runs tagged with the decorator tag (default <bmfont>) always use BMFont; when
 * bDecoratePlainTextRuns is set, unstyled runs render through BMFont as well.
 * Kerning resolves within each run only, and the adapter performs no shaping,
 * matching the plain-text widget.
 */
UCLASS(meta = (DisplayName = "BMFont Rich Text"), HideCategories = (Localization))
class UNREALBMFONT_API UBMFontRichTextBlock : public URichTextBlock
{
	GENERATED_BODY()

public:
	UBMFontRichTextBlock(const FObjectInitializer& ObjectInitializer);

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

	/** Markup tag whose runs render through BMFont. Other tags keep the rich text style set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter = GetDecoratorTag, Setter = SetDecoratorTag, Category = "BMFont", AdvancedDisplay)
	FName DecoratorTag = TEXT("bmfont");

	/** When true, plain (untagged) runs also render through BMFont instead of the style set font. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter = GetDecoratePlainTextRuns, Setter = SetDecoratePlainTextRuns, Category = "BMFont")
	bool bDecoratePlainTextRuns = true;

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

	FName GetDecoratorTag() const;

	UFUNCTION(BlueprintCallable, Category = "BMFont")
	void SetDecoratorTag(FName InDecoratorTag);

	bool GetDecoratePlainTextRuns() const;

	UFUNCTION(BlueprintCallable, Category = "BMFont")
	void SetDecoratePlainTextRuns(bool bInDecoratePlainTextRuns);

	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	virtual void CreateDecorators(TArray<TSharedRef<ITextDecorator>>& OutDecorators) override;

private:
	FBMFontRunStyle MakeRunStyle() const;
	void UpdateDecoratorConfig();
	void UpdateDecoratorMatching();
	void RefreshRuns();

	TSharedPtr<FBMFontTextDecorator> ActiveDecorator;
};
