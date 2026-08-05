# Minimal sample

`Minimal.fnt` and `Minimal.png` are original fixtures containing space, `A`, `B`, and the replacement character (`U+FFFD`). Import `Minimal.fnt`; the page file is resolved automatically.

Suggested smoke text:

```text
AB BA
A missing glyph: C
```

The `C` resolves to the replacement glyph. Recreate the atlas and plugin logo on Windows with `Scripts/GenerateBrandAssets.ps1`.
