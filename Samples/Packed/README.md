# Packed sample

A minimal packed-channel (`packed=1`) fixture: glyph coverage lives in the
green channel, and every other channel is declared zero. It exists to
exercise the channel-extraction material path during manual verification.

- Import `Packed.fnt`; the import succeeds (packed descriptors were rejected before 0.2.0).
- The imported page texture has sRGB disabled.
- Rendered glyphs show the channel-extracted coverage tinted by the widget color.

Generated fixture, original to this repository, MIT licensed.
