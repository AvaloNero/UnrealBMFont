# Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

[CmdletBinding()]
param(
	[string]$ShowcaseFontFile
)

$ErrorActionPreference = 'Stop'
if ([System.Environment]::OSVersion.Platform -ne [System.PlatformID]::Win32NT) {
	throw 'Brand asset generation currently requires Windows System.Drawing.'
}

Add-Type -AssemblyName System.Drawing

function New-RoundedRectanglePath {
	param(
		[float]$X,
		[float]$Y,
		[float]$Width,
		[float]$Height,
		[float]$Radius
	)

	$path = [System.Drawing.Drawing2D.GraphicsPath]::new()
	$diameter = $Radius * 2
	$path.AddArc($X, $Y, $diameter, $diameter, 180, 90)
	$path.AddArc($X + $Width - $diameter, $Y, $diameter, $diameter, 270, 90)
	$path.AddArc($X + $Width - $diameter, $Y + $Height - $diameter, $diameter, $diameter, 0, 90)
	$path.AddArc($X, $Y + $Height - $diameter, $diameter, $diameter, 90, 90)
	$path.CloseFigure()
	return $path
}

function Draw-PixelGlyph {
	param(
		[System.Drawing.Graphics]$Graphics,
		[string[]]$Pattern,
		[int]$X,
		[int]$Y,
		[int]$CellSize,
		[System.Drawing.Brush]$Brush
	)

	for ($row = 0; $row -lt $Pattern.Count; $row++) {
		for ($column = 0; $column -lt $Pattern[$row].Length; $column++) {
			if ($Pattern[$row][$column] -eq '1') {
				$Graphics.FillRectangle(
					$Brush,
					$X + ($column * $CellSize),
					$Y + ($row * $CellSize),
					$CellSize,
					$CellSize
				)
			}
		}
	}
}

function Get-AlphaBounds {
	param(
		[System.Drawing.Bitmap]$Bitmap,
		[System.Drawing.Rectangle]$SearchArea
	)

	$minimumX = $SearchArea.Right
	$minimumY = $SearchArea.Bottom
	$maximumX = -1
	$maximumY = -1
	for ($y = $SearchArea.Top; $y -lt $SearchArea.Bottom; $y++) {
		for ($x = $SearchArea.Left; $x -lt $SearchArea.Right; $x++) {
			if ($Bitmap.GetPixel($x, $y).A -eq 0) {
				continue
			}
			$minimumX = [Math]::Min($minimumX, $x)
			$minimumY = [Math]::Min($minimumY, $y)
			$maximumX = [Math]::Max($maximumX, $x)
			$maximumY = [Math]::Max($maximumY, $y)
		}
	}

	if ($maximumX -lt $minimumX -or $maximumY -lt $minimumY) {
		return [System.Drawing.Rectangle]::Empty
	}
	return [System.Drawing.Rectangle]::FromLTRB($minimumX, $minimumY, $maximumX + 1, $maximumY + 1)
}

function Resolve-ShowcaseFontFile {
	param([string]$RequestedFontFile)

	if (-not [string]::IsNullOrWhiteSpace($RequestedFontFile)) {
		return (Resolve-Path -LiteralPath $RequestedFontFile -ErrorAction Stop).Path
	}

	$windowsDirectory = [Environment]::GetFolderPath([Environment+SpecialFolder]::Windows)
	$candidates = @(
		(Join-Path $windowsDirectory 'Fonts\NotoSansSC-VF.ttf'),
		(Join-Path $windowsDirectory 'Fonts\NotoSansSC[wght].ttf')
	)
	foreach ($candidate in $candidates) {
		if (Test-Path -LiteralPath $candidate -PathType Leaf) {
			return (Resolve-Path -LiteralPath $candidate).Path
		}
	}

	return $null
}

function New-ShowcaseFontAssets {
	param(
		[string]$FontFile,
		[string]$OutputDirectory
	)

	$fontSize = 32
	$lineHeight = 48
	$atlasWidth = 1024
	$atlasHeight = 512
	$cellWidth = 48
	$cellHeight = 64
	$columnCount = [Math]::Floor($atlasWidth / $cellWidth)
	$drawPaddingX = 8

	$codepoints = [Collections.Generic.List[int]]::new()
	$codepoints.Add(32)
	foreach ($codepoint in 48..57) { $codepoints.Add($codepoint) }
	foreach ($codepoint in 65..90) { $codepoints.Add($codepoint) }
	foreach ($codepoint in 0x3041..0x3096) { $codepoints.Add($codepoint) }
	$codepoints.Add(0x30FC)
	foreach ($codepoint in @(0x96F6, 0x4E00, 0x4E8C, 0x4E09, 0x56DB, 0x4E94, 0x516D, 0x4E03, 0x516B, 0x4E5D)) {
		$codepoints.Add($codepoint)
	}
	$codepoints.Add(0xFFFD)

	$requiredRows = [Math]::Ceiling($codepoints.Count / $columnCount)
	if ($requiredRows * $cellHeight -gt $atlasHeight) {
		throw "Showcase glyphs do not fit in the configured $atlasWidth x $atlasHeight atlas."
	}

	New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
	$privateFonts = [System.Drawing.Text.PrivateFontCollection]::new()
	$privateFonts.AddFontFile($FontFile)
	$fontFamily = $privateFonts.Families | Where-Object { $_.Name -eq 'Noto Sans SC' } | Select-Object -First 1
	if ($null -eq $fontFamily) {
		$fontFamily = $privateFonts.Families | Select-Object -First 1
	}
	if ($null -eq $fontFamily) {
		$privateFonts.Dispose()
		throw "No font family could be loaded from: $FontFile"
	}

	$font = [System.Drawing.Font]::new(
		$fontFamily,
		$fontSize,
		[System.Drawing.FontStyle]::Regular,
		[System.Drawing.GraphicsUnit]::Pixel
	)
	$stringFormat = [System.Drawing.StringFormat]::GenericTypographic.Clone()
	$stringFormat.FormatFlags = $stringFormat.FormatFlags `
		-bor [System.Drawing.StringFormatFlags]::NoWrap `
		-bor [System.Drawing.StringFormatFlags]::MeasureTrailingSpaces
	$atlas = [System.Drawing.Bitmap]::new(
		$atlasWidth,
		$atlasHeight,
		[System.Drawing.Imaging.PixelFormat]::Format32bppArgb
	)
	$graphics = [System.Drawing.Graphics]::FromImage($atlas)
	$whiteBrush = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::White)
	$glyphRecords = [Collections.Generic.List[object]]::new()

	try {
		$graphics.Clear([System.Drawing.Color]::Transparent)
		$graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
		$graphics.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
		$graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality

		$emHeight = $fontFamily.GetEmHeight([System.Drawing.FontStyle]::Regular)
		$cellAscent = $fontFamily.GetCellAscent([System.Drawing.FontStyle]::Regular)
		$base = [Math]::Ceiling($fontSize * $cellAscent / $emHeight)

		for ($index = 0; $index -lt $codepoints.Count; $index++) {
			$codepoint = $codepoints[$index]
			if ($codepoint -eq 32) {
				$glyphRecords.Add([pscustomobject]@{
					Id = $codepoint; X = 0; Y = 0; Width = 0; Height = 0
					XOffset = 0; YOffset = 0; XAdvance = 16
				})
				continue
			}

			$column = $index % $columnCount
			$row = [Math]::Floor($index / $columnCount)
			$cellX = $column * $cellWidth
			$cellY = $row * $cellHeight
			$text = [char]::ConvertFromUtf32($codepoint)
			$drawOriginX = $cellX + $drawPaddingX
			$graphics.DrawString(
				$text,
				$font,
				$whiteBrush,
				[System.Drawing.PointF]::new($drawOriginX, $cellY),
				$stringFormat
			)

			$cellBounds = [System.Drawing.Rectangle]::new($cellX, $cellY, $cellWidth, $cellHeight)
			$glyphBounds = Get-AlphaBounds -Bitmap $atlas -SearchArea $cellBounds
			if ($glyphBounds.IsEmpty) {
				throw ('Font produced no pixels for U+{0:X4}.' -f $codepoint)
			}

			$measuredSize = $graphics.MeasureString(
				$text,
				$font,
				[System.Drawing.PointF]::Empty,
				$stringFormat
			)
			$glyphRecords.Add([pscustomobject]@{
				Id = $codepoint
				X = $glyphBounds.X
				Y = $glyphBounds.Y
				Width = $glyphBounds.Width
				Height = $glyphBounds.Height
				XOffset = $glyphBounds.X - $drawOriginX
				YOffset = $glyphBounds.Y - $cellY
				XAdvance = [Math]::Max(1, [Math]::Ceiling($measuredSize.Width))
			})
		}

		$atlas.Save(
			(Join-Path $OutputDirectory 'Showcase.png'),
			[System.Drawing.Imaging.ImageFormat]::Png
		)

		$descriptor = [Collections.Generic.List[string]]::new()
		$descriptor.Add("info face=`"$($fontFamily.Name)`" size=$fontSize bold=0 italic=0 charset=`"`" unicode=1 stretchH=100 smooth=1 aa=1 padding=0,0,0,0 spacing=2,2 outline=0")
		$descriptor.Add("common lineHeight=$lineHeight base=$base scaleW=$atlasWidth scaleH=$atlasHeight pages=1 packed=0 alphaChnl=0 redChnl=4 greenChnl=4 blueChnl=4")
		$descriptor.Add('page id=0 file="Showcase.png"')
		$descriptor.Add("chars count=$($glyphRecords.Count)")
		foreach ($glyph in $glyphRecords) {
			$descriptor.Add(
				"char id=$($glyph.Id) x=$($glyph.X) y=$($glyph.Y) width=$($glyph.Width) height=$($glyph.Height) xoffset=$($glyph.XOffset) yoffset=$($glyph.YOffset) xadvance=$($glyph.XAdvance) page=0 chnl=15"
			)
		}
		$descriptor.Add('kernings count=1')
		$descriptor.Add('kerning first=65 second=66 amount=-1')
		$utf8WithoutBom = [System.Text.UTF8Encoding]::new($false)
		[System.IO.File]::WriteAllLines(
			(Join-Path $OutputDirectory 'Showcase.fnt'),
			$descriptor,
			$utf8WithoutBom
		)
	} finally {
		$whiteBrush.Dispose()
		$graphics.Dispose()
		$atlas.Dispose()
		$stringFormat.Dispose()
		$font.Dispose()
		$privateFonts.Dispose()
	}

	Write-Host "Generated Showcase.fnt and Showcase.png with $($glyphRecords.Count) glyphs from $FontFile."
}

$pluginRoot = Split-Path -Parent $PSScriptRoot
$resourceDirectory = Join-Path $pluginRoot 'Resources'
$sampleDirectory = Join-Path $pluginRoot 'Samples\Minimal'
$showcaseDirectory = Join-Path $pluginRoot 'Samples\Showcase'
New-Item -ItemType Directory -Path $resourceDirectory -Force | Out-Null
New-Item -ItemType Directory -Path $sampleDirectory -Force | Out-Null

$patternB = @('11110', '10001', '10001', '11110', '10001', '10001', '11110')
$patternM = @('10001', '11011', '10101', '10101', '10001', '10001', '10001')
$patternA = @('01110', '10001', '10001', '11111', '10001', '10001', '10001')
$patternReplacement = @('01110', '10001', '00010', '00100', '00000', '00100', '00000')

$largeSize = 512
$largeIcon = [System.Drawing.Bitmap]::new(
	$largeSize,
	$largeSize,
	[System.Drawing.Imaging.PixelFormat]::Format32bppArgb
)
$iconGraphics = [System.Drawing.Graphics]::FromImage($largeIcon)
try {
	$iconGraphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
	$iconGraphics.Clear([System.Drawing.Color]::Transparent)
	$backgroundPath = New-RoundedRectanglePath -X 14 -Y 14 -Width 484 -Height 484 -Radius 92
	$backgroundBrush = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(255, 11, 18, 32))
	$borderPen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(255, 69, 149, 255), 18)
	$gridPen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(48, 123, 177, 255), 5)
	$glyphBrush = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(255, 238, 246, 255))
	try {
		$iconGraphics.FillPath($backgroundBrush, $backgroundPath)
		$iconGraphics.DrawPath($borderPen, $backgroundPath)
		foreach ($position in @(128, 256, 384)) {
			$iconGraphics.DrawLine($gridPen, $position, 42, $position, 470)
			$iconGraphics.DrawLine($gridPen, 42, $position, 470, $position)
		}
		Draw-PixelGlyph -Graphics $iconGraphics -Pattern $patternB -X 82 -Y 144 -CellSize 31 -Brush $glyphBrush
		Draw-PixelGlyph -Graphics $iconGraphics -Pattern $patternM -X 280 -Y 144 -CellSize 31 -Brush $glyphBrush
	} finally {
		$backgroundPath.Dispose()
		$backgroundBrush.Dispose()
		$borderPen.Dispose()
		$gridPen.Dispose()
		$glyphBrush.Dispose()
	}
} finally {
	$iconGraphics.Dispose()
}

$smallIcon = [System.Drawing.Bitmap]::new(128, 128, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$smallGraphics = [System.Drawing.Graphics]::FromImage($smallIcon)
try {
	$smallGraphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
	$smallGraphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
	$smallGraphics.DrawImage($largeIcon, 0, 0, 128, 128)
	$smallIcon.Save(
		(Join-Path $resourceDirectory 'Icon128.png'),
		[System.Drawing.Imaging.ImageFormat]::Png
	)
} finally {
	$smallGraphics.Dispose()
	$smallIcon.Dispose()
	$largeIcon.Dispose()
}

$atlas = [System.Drawing.Bitmap]::new(64, 16, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$atlasGraphics = [System.Drawing.Graphics]::FromImage($atlas)
try {
	$atlasGraphics.Clear([System.Drawing.Color]::Transparent)
	$whiteBrush = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::White)
	try {
		Draw-PixelGlyph -Graphics $atlasGraphics -Pattern $patternA -X 0 -Y 1 -CellSize 2 -Brush $whiteBrush
		Draw-PixelGlyph -Graphics $atlasGraphics -Pattern $patternB -X 12 -Y 1 -CellSize 2 -Brush $whiteBrush
		Draw-PixelGlyph -Graphics $atlasGraphics -Pattern $patternReplacement -X 24 -Y 1 -CellSize 2 -Brush $whiteBrush
	} finally {
		$whiteBrush.Dispose()
	}
	$atlas.Save(
		(Join-Path $sampleDirectory 'Minimal.png'),
		[System.Drawing.Imaging.ImageFormat]::Png
	)
} finally {
	$atlasGraphics.Dispose()
	$atlas.Dispose()
}

Write-Host 'Generated Resources\Icon128.png and Samples\Minimal\Minimal.png.'

$resolvedShowcaseFontFile = Resolve-ShowcaseFontFile -RequestedFontFile $ShowcaseFontFile
if ($null -eq $resolvedShowcaseFontFile) {
	Write-Warning 'Showcase generation was skipped. Pass -ShowcaseFontFile with NotoSansSC[wght].ttf from the official Noto CJK project.'
} else {
	New-ShowcaseFontAssets -FontFile $resolvedShowcaseFontFile -OutputDirectory $showcaseDirectory
}
