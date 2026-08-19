// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#include "BMFontRendering.h"

namespace
{
	struct FChannelDescription
	{
		int32 Mask;
		EBMFontChannelContent Content;
		FLinearColor Weight;
	};

	bool CarriesCoverage(const EBMFontChannelContent Content)
	{
		return Content == EBMFontChannelContent::Glyph
			|| Content == EBMFontChannelContent::GlyphAndOutline;
	}

	TArray<FChannelDescription, TInlineAllocator<4>> GetChannels(const FBMFontCommon& Common)
	{
		// Prefer alpha when a glyph is duplicated across all channels, then RGB.
		return {
			{8, Common.AlphaChannel, FLinearColor(0.0f, 0.0f, 0.0f, 1.0f)},
			{4, Common.RedChannel, FLinearColor(1.0f, 0.0f, 0.0f, 0.0f)},
			{2, Common.GreenChannel, FLinearColor(0.0f, 1.0f, 0.0f, 0.0f)},
			{1, Common.BlueChannel, FLinearColor(0.0f, 0.0f, 1.0f, 0.0f)}
		};
	}

	const FChannelDescription* FindChannel(
		const TArray<FChannelDescription, TInlineAllocator<4>>& Channels,
		const int32 ChannelMask,
		const TFunctionRef<bool(EBMFontChannelContent)> Predicate)
	{
		for (const FChannelDescription& Channel : Channels)
		{
			if ((ChannelMask & Channel.Mask) != 0 && Predicate(Channel.Content))
			{
				return &Channel;
			}
		}
		return nullptr;
	}
}

FBMFontPackedChannelMapping BMFontRendering::ComputePackedChannelMapping(
	const FBMFontCommon& Common,
	const int32 GlyphChannel)
{
	FBMFontPackedChannelMapping Mapping;
	if (!Common.bPacked)
	{
		return Mapping;
	}

	Mapping.ChannelWeights = FLinearColor::Transparent;
	const TArray<FChannelDescription, TInlineAllocator<4>> Channels = GetChannels(Common);
	const int32 NormalizedGlyphChannel = GlyphChannel & 0x0F;
	const bool bHasGlyphChannel = NormalizedGlyphChannel != 0;
	const int32 CandidateMask = bHasGlyphChannel ? NormalizedGlyphChannel : 0x0F;

	if (const FChannelDescription* CoverageChannel = FindChannel(Channels, CandidateMask, CarriesCoverage))
	{
		Mapping.ChannelWeights = CoverageChannel->Weight;
		return Mapping;
	}

	if (FindChannel(
		Channels,
		CandidateMask,
		[](const EBMFontChannelContent Content) { return Content == EBMFontChannelContent::One; }) != nullptr)
	{
		Mapping.ConstantBias = 1.0f;
		return Mapping;
	}

	if (bHasGlyphChannel)
	{
		// char.chnl is authoritative when common metadata is missing or contradictory.
		if (const FChannelDescription* SelectedChannel = FindChannel(
			Channels,
			NormalizedGlyphChannel,
			[](const EBMFontChannelContent) { return true; }))
		{
			Mapping.ChannelWeights = SelectedChannel->Weight;
			return Mapping;
		}
	}

	// Missing glyph-channel and common metadata: interpret alpha as conventional coverage.
	Mapping.ChannelWeights = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
	return Mapping;
}

const TCHAR* BMFontRendering::GetDefaultPackedMaterialPath()
{
	return TEXT("/UnrealBMFont/M_BMFontPacked.M_BMFontPacked");
}
