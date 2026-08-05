# Showcase sample

`Showcase.fnt` and `Showcase.png` form a larger manual-validation fixture. Import only `Showcase.fnt`; the importer resolves the PNG beside it automatically.

The atlas contains 135 unique code points:

- Space, ASCII digits `0`–`9`, and uppercase Latin `A`–`Z`.
- Every code point from `U+3041` through `U+3096`, covering the standard Hiragana letters, small forms, voiced forms, and historic forms.
- The Japanese prolonged sound mark `ー` (`U+30FC`).
- Chinese numerals `零一二三四五六七八九`.
- The replacement character `U+FFFD` for fallback testing.

Suggested smoke text:

```text
0123456789
ABCDEFGHIJKLMNOPQRSTUVWXYZ
あいうえお かきくけこ さしすせそ
がぎぐげご ざじずぜぞ ぱぴぷぺぽ
零一二三四五六七八九
Missing: ?
```

The `?` character is intentionally absent and should resolve to `U+FFFD` with the default fallback setting.

The glyph image was generated with Noto Sans SC from the official [Noto CJK project](https://github.com/notofonts/noto-cjk), licensed under the [SIL Open Font License 1.1](https://github.com/notofonts/noto-cjk/blob/main/Sans/LICENSE). No font file is bundled in this repository. To regenerate the fixture, download `NotoSansSC[wght].ttf` from that project and run:

```powershell
pwsh ./Scripts/GenerateBrandAssets.ps1 -ShowcaseFontFile "C:\Fonts\NotoSansSC[wght].ttf"
```
