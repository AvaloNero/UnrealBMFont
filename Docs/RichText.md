# Rich text

`UBMFontRichTextBlock` is a `URichTextBlock` subclass that renders text runs with an imported BMFont asset instead of font faces. It is an adapter layered over the plain-text layout primitives; the plain-text core is unchanged.

## Usage

1. Create a **BMFont Rich Text** widget (Text palette category).
2. Assign **Font Asset** to an imported BMFont asset.
3. Set the text. Markup works as usual; runs tagged with the decorator tag render through BMFont:

```html
<bmfont>Score: 1250</>
```

When **Decorate Plain Text Runs** is enabled (the default), untagged runs also render through BMFont, so a plain string works without any markup. Named runs that match neither the decorator tag nor the plain-text claim keep the rich text style set and render with regular fonts, so BMFont and font runs can be mixed in one block.

The tag name defaults to `bmfont` and can be changed with **Decorator Tag**.

## Behavior and limitations

- Each run is laid out independently. Kerning resolves within a run only; pairs that span a tag boundary do not kern.
- Run metrics come from the asset: line height from `common.lineHeight`, scaled by **Font Scale**. Runs anchor to the line top; on lines shared with taller font runs, the BMFont run bottom-aligns within the line.
- Tint and shadow come from the widget's own appearance properties, not from the rich text style set. A named style from the style set table is not consulted for BMFont runs.
- Packed-channel assets render through the same channel-extraction material as the plain-text widget.
- Widget property setters (font asset, scale, spacing, colors) apply immediately by rebuilding the affected runs; the markup is not re-tokenized by callers.
- Reimporting the assigned asset bumps its data revision, which runs observe on the next layout invalidate; measured positions then refresh without any text change.
- The adapter inherits the core renderer's scope: left-to-right layout without bidirectional reordering, OpenType shaping, ligatures, or combining-mark positioning.
