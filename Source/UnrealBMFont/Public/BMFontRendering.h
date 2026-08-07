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
	 * Computes channel extraction parameters from a descriptor's common record.
	 * Non-packed descriptors always sample alpha as coverage. Packed descriptors sample
	 * channels marked Glyph or GlyphAndOutline; a constant-one coverage channel becomes
	 * ConstantBias, and unknown metadata falls back to alpha sampling.
	 */
	UNREALBMFONT_API FBMFontPackedChannelMapping ComputePackedChannelMapping(const FBMFontCommon& Common);
}
