// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#include "BMFontRendering.h"

namespace
{
	float GetCoverageWeight(const EBMFontChannelContent Content)
	{
		switch (Content)
		{
		case EBMFontChannelContent::Glyph:
		case EBMFontChannelContent::GlyphAndOutline:
			return 1.0f;
		default:
			return 0.0f;
		}
	}

	bool IsAlwaysOne(const EBMFontChannelContent Content)
	{
		return Content == EBMFontChannelContent::One;
	}
}

FBMFontPackedChannelMapping BMFontRendering::ComputePackedChannelMapping(const FBMFontCommon& Common)
{
	FBMFontPackedChannelMapping Mapping;
	if (!Common.bPacked)
	{
		return Mapping;
	}

	Mapping.ChannelWeights = FLinearColor(
		GetCoverageWeight(Common.RedChannel),
		GetCoverageWeight(Common.GreenChannel),
		GetCoverageWeight(Common.BlueChannel),
		GetCoverageWeight(Common.AlphaChannel)
	);

	const bool bHasSampledCoverage = !Mapping.ChannelWeights.Equals(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
	if (bHasSampledCoverage)
	{
		return Mapping;
	}

	const bool bHasConstantCoverage = IsAlwaysOne(Common.RedChannel)
		|| IsAlwaysOne(Common.GreenChannel)
		|| IsAlwaysOne(Common.BlueChannel)
		|| IsAlwaysOne(Common.AlphaChannel);
	if (bHasConstantCoverage)
	{
		Mapping.ChannelWeights = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
		Mapping.ConstantBias = 1.0f;
		return Mapping;
	}

	// Unknown channel metadata: interpret alpha as conventional coverage.
	Mapping.ChannelWeights = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
	return Mapping;
}
