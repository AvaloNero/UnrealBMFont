# Format support

## Descriptor matrix

| Capability | Status | Notes |
| --- | --- | --- |
| Text descriptor | Supported | Quoted values, arbitrary field order, BOM, and multi-page files are covered. |
| XML descriptor | Supported | Includes compact one-line XML and optional XML declarations. |
| Binary descriptor v3 | Supported | Blocks 1–5, signed metrics, Unicode IDs, and unknown-block warnings. |
| Binary descriptor v1/v2 | Rejected | The importer reports the unsupported version. |
| Multiple pages | Supported | Page IDs are resolved to their declared files and textures. |
| Unicode scalar IDs | Supported | Supplementary-plane code points are decoded as one glyph. |
| Kerning pairs | Supported | Applied before tracking. |
| Packed channels | Parsed only | Import is rejected until a channel-aware rendering material is available. |

## Import validation

- The descriptor must have positive line height and atlas dimensions.
- The declared page count must match parsed page records.
- Every glyph must reference a known page and remain inside the declared atlas.
- Every page file must exist beneath the descriptor directory. Absolute paths and `..` traversal cannot escape that directory.
- Imported source texture dimensions must match `scaleW` and `scaleH`.
- Duplicate glyphs and kerning pairs use the last value and emit a warning.

## Image formats

Page decoding is delegated to Unreal's `UTextureFactory`. PNG is covered by automated import tests. Other formats supported by a given Unreal installation may work but are not part of the current compatibility claim.

## Text behavior

- Explicit CR, LF, and CRLF line endings are supported.
- Tabs advance by four space glyph advances and do not count as missing glyphs.
- Word wrapping uses Unreal's line-break iterator. `AllowPerCharacterWrapping` provides the same long-word fallback meaning as Unreal text widgets.
- Missing source characters use `FallbackCodepoint` when that glyph exists. Otherwise they advance by half the line height and increment the missing-glyph count.
- `info.spacing` is retained as atlas-generator metadata. It describes spacing used while generating the bitmap atlas; runtime layout uses each glyph's `xadvance` and `common.lineHeight` and does not add it again.
- Layout and rendering are left-to-right and do not perform OpenType shaping, ligatures, combining-mark positioning, or bidirectional reordering.

BMFont atlases intended for Arabic, Indic scripts, or other shaping-dependent writing systems should contain and address the final presentation glyphs explicitly; even then, automatic language-correct shaping is outside this plugin's current scope.
