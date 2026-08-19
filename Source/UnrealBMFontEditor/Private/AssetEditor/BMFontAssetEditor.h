// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#pragma once

#include "Toolkits/AssetEditorToolkit.h"
#include "Types/SlateEnums.h"

class SBMFontAtlasPreview;
class STextBlock;
class SVerticalBox;
class UBMFontAsset;
struct FAssetOpenArgs;
template <typename OptionType>
class SComboBox;

/**
 * Read-only font/atlas inspector opened when double-clicking a BMFont asset.
 * Shows descriptor metadata, the selected atlas page with glyph rectangles,
 * and a glyph table. Editing happens through reimport, not this editor.
 */
class FBMFontAssetEditor final : public FAssetEditorToolkit
{
public:
	virtual ~FBMFontAssetEditor() override;

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
	void PopulateSummaryPanel();
	void AddSummaryRow(const TSharedRef<SVerticalBox>& Panel, const FText& Label, const FText& Value) const;
	void HandlePageSelected(TSharedPtr<int32> Item, ESelectInfo::Type SelectInfo);
	void HandleGlyphSelected(int32 Codepoint);
	void HandlePostReimport(UObject* ReimportedObject, bool bSuccess);
	void RefreshEditorData();
	void RefreshPageOptions();
	void RefreshGlyphRows();

	TObjectPtr<UBMFontAsset> EditingAsset;
	TSharedPtr<SVerticalBox> SummaryPanel;
	TSharedPtr<SBMFontAtlasPreview> AtlasPreview;
	TSharedPtr<SComboBox<TSharedPtr<int32>>> PageSelector;
	TSharedPtr<STextBlock> PageLabel;
	TSharedPtr<SListView<TSharedPtr<int32>>> GlyphListView;
	TArray<TSharedPtr<int32>> PageOptions;
	TArray<TSharedPtr<int32>> GlyphRows;
	FDelegateHandle PostReimportHandle;
	int32 CurrentPageId = INDEX_NONE;
};
