// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#include "BMFontAssetDefinition.h"

#include "AssetDefinition.h"
#include "AssetEditor/BMFontAssetEditor.h"
#include "BMFontAsset.h"

#define LOCTEXT_NAMESPACE "UnrealBMFontAssetDefinition"

FText UBMFontAssetDefinition::GetAssetDisplayName() const
{
	return LOCTEXT("DisplayName", "BMFont");
}

FLinearColor UBMFontAssetDefinition::GetAssetColor() const
{
	return FLinearColor(FColor(68, 128, 230));
}

TSoftClassPtr<UObject> UBMFontAssetDefinition::GetAssetClass() const
{
	return UBMFontAsset::StaticClass();
}

TConstArrayView<FAssetCategoryPath> UBMFontAssetDefinition::GetAssetCategories() const
{
	static const auto Categories = {
		FAssetCategoryPath(
			EAssetCategoryPaths::UI,
			LOCTEXT("FontSubMenu", "Font"),
			ECategoryMenuType::Section
		)
	};
	return Categories;
}

EAssetCommandResult UBMFontAssetDefinition::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	TArray<UBMFontAsset*> Assets = OpenArgs.LoadObjects<UBMFontAsset>();
	for (UBMFontAsset* Asset : Assets)
	{
		FBMFontAssetEditor::Open(Asset, OpenArgs);
	}
	return Assets.IsEmpty() ? EAssetCommandResult::Unhandled : EAssetCommandResult::Handled;
}

#undef LOCTEXT_NAMESPACE
