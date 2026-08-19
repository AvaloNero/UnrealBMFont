// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#include "BMFontAsset.h"
#include "Modules/ModuleManager.h"
#include "Thumbnail/BMFontThumbnailRenderer.h"
#include "ThumbnailRendering/ThumbnailManager.h"

class FUnrealBMFontEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UThumbnailManager::Get().RegisterCustomRenderer(
			UBMFontAsset::StaticClass(),
			UBMFontThumbnailRenderer::StaticClass()
		);
	}

	virtual void ShutdownModule() override
	{
		if (UObjectInitialized())
		{
			UThumbnailManager::Get().UnregisterCustomRenderer(UBMFontAsset::StaticClass());
		}
	}
};

IMPLEMENT_MODULE(FUnrealBMFontEditorModule, UnrealBMFontEditor)
