// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#pragma once

#include "ThumbnailRendering/ThumbnailRenderer.h"
#include "BMFontThumbnailRenderer.generated.h"

class UBMFontAsset;
class UTexture2D;

/** Draws the first atlas page fitted into the Content Browser thumbnail. */
UCLASS()
class UBMFontThumbnailRenderer final : public UThumbnailRenderer
{
	GENERATED_BODY()

public:
	/** Resolves the first descriptor page even when page IDs do not start at zero. */
	static UTexture2D* ResolvePreviewTexture(const UBMFontAsset* Asset);

	//~ UThumbnailRenderer
	virtual void Draw(
		UObject* Object,
		int32 X,
		int32 Y,
		uint32 Width,
		uint32 Height,
		FRenderTarget* Viewport,
		FCanvas* Canvas,
		bool bAdditionalViewFamily
	) override;
};
