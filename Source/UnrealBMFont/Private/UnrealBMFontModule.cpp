// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#include "UnrealBMFontModule.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogUnrealBMFont);

void FUnrealBMFontModule::StartupModule()
{
	UE_LOG(LogUnrealBMFont, Log, TEXT("Unreal BMFont runtime module initialized."));
}

void FUnrealBMFontModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FUnrealBMFontModule, UnrealBMFont)
