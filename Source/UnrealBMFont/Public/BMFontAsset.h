// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#pragma once

#include "BMFontTypes.h"
#include "UObject/Object.h"
#include "BMFontAsset.generated.h"

class UAssetImportData;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class FBMFontLayout;

/** Imported BMFont descriptor data and hard references to every atlas page. */
UCLASS(BlueprintType)
class UNREALBMFONT_API UBMFontAsset : public UObject
{
	GENERATED_BODY()

public:
	UBMFontAsset();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BMFont")
	FBMFontData FontData;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Instanced, Category = "Import Settings")
	TObjectPtr<UAssetImportData> AssetImportData;
#endif

	/**
	 * Material that extracts glyph coverage from packed-channel atlas pages.
	 * Defaults to the plugin's M_BMFontPacked; ignored for non-packed descriptors.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BMFont", AdvancedDisplay)
	TSoftObjectPtr<UMaterialInterface> PackedRenderMaterial;

	UFUNCTION(BlueprintPure, Category = "BMFont")
	bool HasGlyph(int32 Codepoint) const;

	UFUNCTION(BlueprintPure, Category = "BMFont")
	int32 GetLineHeight() const;

	UFUNCTION(BlueprintPure, Category = "BMFont")
	int32 GetPageCount() const;

	/** Copies glyph metrics for Blueprint callers without exposing an internal map pointer. */
	UFUNCTION(BlueprintPure, Category = "BMFont")
	bool GetGlyph(int32 Codepoint, FBMFontGlyph& OutGlyph) const;

	UFUNCTION(BlueprintPure, Category = "BMFont")
	UTexture2D* GetPageTexture(int32 PageId) const;

	/**
	 * Returns the Slate brush resource for a page and glyph channel: the page texture
	 * itself, or a cached channel-extraction material instance when the descriptor uses
	 * packed channels.
	 */
	UObject* GetPageRenderResource(int32 PageId, int32 GlyphChannel = 15);

	const FBMFontGlyph* FindGlyph(int32 Codepoint) const;
	const FBMFontPage* FindPage(int32 PageId) const;
	UFUNCTION(BlueprintPure, Category = "BMFont")
	int32 GetKerning(int32 FirstCodepoint, int32 SecondCodepoint) const;
	uint32 GetDataRevision() const;
	void SetFontData(FBMFontData InFontData);

	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	friend class FBMFontLayout;

	void RebuildLookup();
	void ClearRenderResourceCache();

	TMap<uint64, int32> KerningLookup;
	uint32 DataRevision = 1;

	UPROPERTY(Transient)
	TMap<uint64, TObjectPtr<UMaterialInstanceDynamic>> PageMaterialCache;

	TMap<uint64, TWeakObjectPtr<UTexture2D>> PageMaterialSources;
	bool bWarnedMissingPackedMaterial = false;
};
