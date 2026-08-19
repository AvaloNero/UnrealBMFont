# Packed sample

A minimal packed-channel (`packed=1`) fixture: glyph coverage lives in the
green channel, and every other channel is declared zero. It exists to
exercise the channel-extraction material path during manual verification.

- Import `Packed.fnt` as a BMFont asset.
- The imported page texture has sRGB disabled.
- Each glyph's `chnl=2` mask selects the green-channel coverage, tinted by the widget color.

Generated fixture, original to this repository, MIT licensed.
