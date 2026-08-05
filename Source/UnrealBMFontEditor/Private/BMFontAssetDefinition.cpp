// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#include "BMFontAssetDefinition.h"

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

#undef LOCTEXT_NAMESPACE
