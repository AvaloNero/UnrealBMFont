# Architecture

Unreal BMFont uses a narrow pipeline so each responsibility can be tested without a UMG scene.

```mermaid
flowchart LR
    A[".fnt bytes"] --> B["FBMFontParser"]
    B --> C["FBMFontData"]
    D["Atlas page files"] --> E["UBMFontFactory"]
    C --> E
    E --> F["UBMFontAsset"]
    F --> G["FBMFontLayout"]
    G --> H["SBMFontText"]
    H --> I["UBMFontText"]
```

## Modules

### UnrealBMFont (Runtime)

- Owns serializable data structures and `UBMFontAsset`.
- Parses descriptors without any Editor dependency.
- Converts a Unicode string into positioned bitmap glyphs.
- Paints glyph UV rectangles through Slate brushes.
- Exposes the UMG `UBMFontText` wrapper.

### UnrealBMFontEditor (Editor)

- Registers the modern Asset Definition.
- Imports `.fnt` descriptors and their page textures.
- Tracks source data and implements reimport.
- Draws Content Browser thumbnails and the read-only font/atlas inspector opened on double-click.
- Contains importer-specific automation coverage.

### UnrealBMFontTests (Editor)

- Tests parser and layout behavior through public Runtime APIs.
- Is an Editor module, so it cannot enter game targets or packaged builds.

## Data ownership

`UBMFontAsset` owns descriptor metadata and hard references to imported page textures. It also maintains a non-serialized kerning lookup and a data revision used by Slate caches. Reimport replaces the serializable model, rebuilds the lookup, and invalidates layout/brush caches through that revision.

## Layout model

Layout decodes Unicode scalar values, resolves fallback glyphs, applies pair kerning and tracking, uses Unreal's line-break iterator, and produces positioned quads. The wrapping scan is linear per paragraph. It deliberately does not invoke Unreal's font shaping pipeline because the source data contains bitmap rectangles rather than a font face.

## Rendering model

`SBMFontText` is an `SLeafWidget`. It keeps a revision-aware brush cache and emits Slate boxes using the correct page texture and UV region. All glyphs on the same texture/layer remain eligible for Slate batching. Layout is cached by text, font data revision, settings, and resolved wrapping width.

Packed-channel glyphs render through `UMaterialInstanceDynamic` resources created from the asset's packed render material (default: the plugin's `M_BMFontPacked`). The asset caches them by page ID and glyph channel mask. Each instance receives the page texture and channel mapping computed from `char.chnl` plus the common channel metadata, while Slate's vertex color continues to apply tint and shadow. Non-packed pages keep the plain texture path.

## Text model

The core accepts plain text and maps each code point through the BMFont layout and paint primitives. Rich Text markup lives in an optional adapter: `UBMFontRichTextBlock` contributes a Slate decorator that converts matching runs into BMFont runs measured and painted with the same glyph metrics. Each rich-text run is laid out independently, so kerning does not cross run boundaries. Run-local brushes are created only for glyphs the run uses and are retained until the asset revision changes.
