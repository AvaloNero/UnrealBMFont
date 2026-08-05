// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

#include "BMFontTypes.h"

FBox2D FBMFontGlyph::GetUvRegion(const FBMFontCommon& Common) const
{
	if (Common.ScaleWidth <= 0 || Common.ScaleHeight <= 0)
	{
		return FBox2D(EForceInit::ForceInit);
	}

	const FVector2D AtlasSize(Common.ScaleWidth, Common.ScaleHeight);
	return FBox2D(
		FVector2D(X, Y) / AtlasSize,
		FVector2D(X + Width, Y + Height) / AtlasSize
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
