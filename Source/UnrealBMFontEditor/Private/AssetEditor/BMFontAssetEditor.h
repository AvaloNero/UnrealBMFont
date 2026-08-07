// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#pragma once

#include "Toolkits/AssetEditorToolkit.h"

class SBMFontAtlasPreview;
class SVerticalBox;
class UBMFontAsset;
struct FAssetOpenArgs;

/**
 * Read-only font/atlas inspector opened when double-clicking a BMFont asset.
 * Shows descriptor metadata, the selected atlas page with glyph rectangles,
 * and a sortable glyph table. Editing happens through reimport, not this editor.
 */
class FBMFontAssetEditor final : public FAssetEditorToolkit
{
public:
	static void Open(UBMFontAsset* InAsset, const FAssetOpenArgs& OpenArgs);

	void Init(UBMFontAsset* InAsset, const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost);

	//~ FAssetEditorToolkit
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

private:
	TSharedRef<SDockTab> SpawnSummaryTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnAtlasTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnGlyphTableTab(const FSpawnTabArgs& Args);

	TSharedRef<SWidget> BuildSummaryPanel();
	TSharedRef<SWidget> BuildGlyphTable();
	void AddSummaryRow(const TSharedRef<SVerticalBox>& Panel, const FText& Label, const FText& Value) const;
	void HandleGlyphSelected(int32 Codepoint);
	void RefreshGlyphRows();

	TObjectPtr<UBMFontAsset> EditingAsset;
	TSharedPtr<SBMFontAtlasPreview> AtlasPreview;
	TSharedPtr<SListView<TSharedPtr<int32>>> GlyphListView;
	TArray<TSharedPtr<int32>> GlyphRows;
	int32 CurrentPageId = 0;
};
