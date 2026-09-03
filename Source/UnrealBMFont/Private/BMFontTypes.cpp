// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#include "BMFontTypes.h"

FBox2D FBMFontGlyph::GetUvRegion(const FBMFontCommon& Common) const
{
	const int64 Right = static_cast<int64>(X) + static_cast<int64>(Width);
	const int64 Bottom = static_cast<int64>(Y) + static_cast<int64>(Height);
	if (Common.ScaleWidth <= 0
		|| Common.ScaleHeight <= 0
		|| X < 0
		|| Y < 0
		|| Width < 0
		|| Height < 0
		|| Right > Common.ScaleWidth
		|| Bottom > Common.ScaleHeight)
	{
		return FBox2D(FVector2D::ZeroVector, FVector2D::ZeroVector);
	}

	const FVector2D AtlasSize(static_cast<double>(Common.ScaleWidth), static_cast<double>(Common.ScaleHeight));
	return FBox2D(
		FVector2D(static_cast<double>(X), static_cast<double>(Y)) / AtlasSize,
		FVector2D(static_cast<double>(Right), static_cast<double>(Bottom)) / AtlasSize
	);
}

uint64 FBMFontKerningPair::MakeKey(const int32 FirstCodepoint, const int32 SecondCodepoint)
{
	return (static_cast<uint64>(static_cast<uint32>(FirstCodepoint)) << 32)
		| static_cast<uint32>(SecondCodepoint);
}

bool FBMFontData::IsValid() const
{
	return Common.LineHeight > 0
		&& Common.ScaleWidth > 0
		&& Common.ScaleHeight > 0
		&& Pages.Num() > 0
		&& Glyphs.Num() > 0;
}
