// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#pragma once

#include "BMFontTypes.h"
#include "CoreMinimal.h"

/** Channel extraction parameters used to render a packed-channel BMFont atlas. */
struct UNREALBMFONT_API FBMFontPackedChannelMapping
{
	/** Per-channel weights dotted with the atlas sample to recover glyph coverage. */
	FLinearColor ChannelWeights = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);

	/** Constant coverage applied when the descriptor marks the coverage channel as always-one. */
	float ConstantBias = 0.0f;
};

namespace BMFontRendering
{
	/**
	 * Computes channel extraction parameters from a descriptor's common record and one
	 * glyph's char.chnl mask. Non-packed descriptors always sample alpha as coverage.
	 * Packed descriptors select one channel from the glyph mask, using the common record
	 * to prefer channels that carry Glyph or GlyphAndOutline data. Unknown or conflicting
	 * metadata falls back to sampling the channel selected by the glyph.
	 */
	UNREALBMFONT_API FBMFontPackedChannelMapping ComputePackedChannelMapping(
		const FBMFontCommon& Common,
		int32 GlyphChannel = 15
	);

	/** Object path of the plugin's default packed-channel UI material. */
	UNREALBMFONT_API const TCHAR* GetDefaultPackedMaterialPath();
}
