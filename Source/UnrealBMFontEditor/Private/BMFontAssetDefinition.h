// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#pragma once

#include "AssetDefinitionDefault.h"
#include "BMFontAssetDefinition.generated.h"

UCLASS()
class UBMFontAssetDefinition final : public UAssetDefinitionDefault
{
	GENERATED_BODY()

public:
	virtual FText GetAssetDisplayName() const override;
	virtual FLinearColor GetAssetColor() const override;
	virtual TSoftClassPtr<UObject> GetAssetClass() const override;
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override;
	virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
};
