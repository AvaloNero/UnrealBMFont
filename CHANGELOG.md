# Changelog

All notable changes to Unreal BMFont are documented here. The project follows [Semantic Versioning](https://semver.org/) and the structure of [Keep a Changelog](https://keepachangelog.com/).

## [0.2.0] - 2026-08-07

### Added

- Channel-aware rendering for packed-channel (`packed=1`) atlases: glyphs sharing a page can select independent `char.chnl` masks through cached instances of the bundled `M_BMFontPacked` UI material. The asset's **Packed Render Material** property overrides the material per asset.
- Packed descriptor import with sRGB-disabled page textures and a warning when reimport preserves an sRGB-enabled texture.
- `UBMFontRichTextBlock`, a rich text adapter that renders tagged (and optionally plain) runs with a BMFont asset. Kerning resolves within each run, empty tags stay empty, and ellipsis overflow policies are rendered with asset glyphs.
- Editor thumbnails for BMFont assets, drawn from the first atlas page.
- A read-only font/atlas inspector (double-click an asset) with a descriptor summary, an atlas preview with glyph rectangles and click-to-select, and a glyph table.
- Screenshot-based render tests (`UnrealBMFont.Render.*`) that draw widgets off-screen and compare against committed ground truth, including per-glyph packed channels and rich-text ellipsis. They skip cleanly under NullRHI; ground truth regenerates with `-UpdateBMFontGroundTruth`.
- `TestPlugin.ps1 -DisableNullRHI` for GPU runs.

### Changed

- The plugin descriptor now allows content (`CanContainContent`) to ship `M_BMFontPacked`.
- Rich-text brush caches are scoped to glyphs used by each run and refreshed by asset revision.
- The asset inspector refreshes its summary, pages, atlas, and glyph rows after successful reimport.

## [0.1.0] - 2026-08-05

### Added

- Initial Unreal BMFont plugin release for importing and rendering AngelCode BMFont assets.
- Runtime parser for AngelCode BMFont text, XML, and binary v3 descriptors, with consistent duplicate-kerning handling (the last record wins and emits a warning) in all three formats.
- Serializable font asset model with multiple pages, Unicode glyphs, and kerning.
- Plain-text Slate and UMG bitmap-font widgets.
- Wrapping, justification, margins, line-height behavior, spacing, tint, shadow, fallback glyphs, and pixel snapping.
- Deterministic layout that reuses the asset's kerning lookup, keeps trailing-whitespace metrics consistent with and without wrapping, and never reads past a bounded string view.
- Per-line paint culling for off-screen lines and correct volatility tracking for bound tint attributes.
- Editor importer, page texture import, source tracking, and reimport that validates every atlas image before touching existing textures.
- Modern Asset Definition integration.
- Parser, layout, import, and reimport automation tests.
- Opt-in code-plugin packaging support for C++ and content-only projects.
- Build/test scripts that stage a clean plugin source copy and keep default outputs outside the repository, preventing local engine caches and old artifacts from being copied into UAT hosts.
- Original Minimal and multilingual Showcase sample fixtures; Showcase covers digits, uppercase Latin, the `U+3041`–`U+3096` Hiragana range, Japanese `ー`, Chinese numerals, and fallback rendering.
- Bilingual documentation, CI template, and project governance files.
