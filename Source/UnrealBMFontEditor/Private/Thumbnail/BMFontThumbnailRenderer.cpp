// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#include "Thumbnail/BMFontThumbnailRenderer.h"

#include "BMFontAsset.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Texture2D.h"

UTexture2D* UBMFontThumbnailRenderer::ResolvePreviewTexture(const UBMFontAsset* Asset)
{
	return Asset != nullptr && !Asset->FontData.Pages.IsEmpty()
		? Asset->GetPageTexture(Asset->FontData.Pages[0].Id)
		: nullptr;
}

void UBMFontThumbnailRenderer::Draw(
	UObject* Object,
	const int32 X,
	const int32 Y,
	const uint32 Width,
	const uint32 Height,
	FRenderTarget* Viewport,
	FCanvas* Canvas,
	const bool bAdditionalViewFamily)
{
	const UBMFontAsset* Asset = Cast<UBMFontAsset>(Object);
	UTexture2D* Texture = ResolvePreviewTexture(Asset);
	if (Texture == nullptr || Texture->GetResource() == nullptr)
	{
		return;
	}

	const float TextureWidth = static_cast<float>(Texture->GetSizeX());
	const float TextureHeight = static_cast<float>(Texture->GetSizeY());
	if (TextureWidth <= 0.0f || TextureHeight <= 0.0f)
	{
		return;
	}

	// Fit the page into the thumbnail without upscaling small atlases.
	const float FitScale = FMath::Min3(
		static_cast<float>(Width) / TextureWidth,
		static_cast<float>(Height) / TextureHeight,
		1.0f
	);
	const float DrawWidth = TextureWidth * FitScale;
	const float DrawHeight = TextureHeight * FitScale;
	const float DrawX = static_cast<float>(X) + (static_cast<float>(Width) - DrawWidth) * 0.5f;
	const float DrawY = static_cast<float>(Y) + (static_cast<float>(Height) - DrawHeight) * 0.5f;

	FCanvasTileItem TileItem(
		FVector2D(DrawX, DrawY),
		Texture->GetResource(),
		FVector2D(DrawWidth, DrawHeight),
		FVector2D(0.0f, 0.0f),
		FVector2D(1.0f, 1.0f),
		FLinearColor::White
	);
	TileItem.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(TileItem);
}
