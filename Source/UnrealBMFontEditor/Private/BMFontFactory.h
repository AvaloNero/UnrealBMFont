// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#pragma once

#include "EditorReimportHandler.h"
#include "Factories/Factory.h"
#include "BMFontFactory.generated.h"

class UBMFontAsset;

/** Imports a descriptor and its atlas pages, and owns the descriptor reimport path. */
UCLASS()
class UNREALBMFONTEDITOR_API UBMFontFactory final : public UFactory, public FReimportHandler
{
	GENERATED_BODY()

public:
	UBMFontFactory();

	virtual FText GetDisplayName() const override;
	virtual bool FactoryCanImport(const FString& Filename) override;
	virtual UObject* FactoryCreateFile(
		UClass* InClass,
		UObject* InParent,
		FName InName,
		EObjectFlags Flags,
		const FString& Filename,
		const TCHAR* Parms,
		FFeedbackContext* Warn,
		bool& bOutOperationCanceled) override;

	virtual bool CanReimport(UObject* Object, TArray<FString>& OutFilenames) override;
	virtual void SetReimportPaths(UObject* Object, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Object) override;
	virtual int32 GetPriority() const override;

private:
	UBMFontAsset* ImportFromFile(
		UBMFontAsset* ExistingAsset,
		UObject* InParent,
		FName InName,
		EObjectFlags Flags,
		const FString& Filename,
		FFeedbackContext* Warn,
		bool& bOutOperationCanceled) const;
};
