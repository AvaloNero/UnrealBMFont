# Changelog

All notable changes to Unreal BMFont are documented here. The project follows [Semantic Versioning](https://semver.org/) and the structure of [Keep a Changelog](https://keepachangelog.com/).

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
