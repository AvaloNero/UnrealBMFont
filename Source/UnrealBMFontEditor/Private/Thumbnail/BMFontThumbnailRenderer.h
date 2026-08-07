// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#pragma once

#include "ThumbnailRendering/ThumbnailRenderer.h"
#include "BMFontThumbnailRenderer.generated.h"

/** Draws the first atlas page fitted into the Content Browser thumbnail. */
UCLASS()
class UBMFontThumbnailRenderer final : public UThumbnailRenderer
{
	GENERATED_BODY()

public:
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
