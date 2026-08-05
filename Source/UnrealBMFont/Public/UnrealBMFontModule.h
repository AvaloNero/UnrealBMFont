// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#pragma once

#include "Logging/LogMacros.h"
#include "Modules/ModuleInterface.h"

UNREALBMFONT_API DECLARE_LOG_CATEGORY_EXTERN(LogUnrealBMFont, Log, All);

class UNREALBMFONT_API FUnrealBMFontModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
