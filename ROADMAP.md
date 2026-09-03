# Roadmap

The roadmap records direction, not a compatibility promise.

## 0.3

- Bound parser work and imported atlas declarations with explicit, configurable limits.
- Keep plain-widget brush creation proportional to the glyphs in the current layout and retain a large-glyph-set regression baseline.
- Automate Development and Shipping cook, package, launch, and deterministic runtime verification.
- Make GPU screenshot execution an explicit self-hosted CI capability.

## Later beta releases

- Verify another Unreal minor version and publish upgrade notes for any API differences.
- Validate Linux and macOS builds and runtime rendering before claiming those platforms.
- Add optional import policies for texture filtering and texture asset placement.
- Add a continuously running coverage-guided parser fuzz target when the build infrastructure can retain and minimize corpora.

## 1.0 criteria

- Public API and serialized asset versioning policy.
- Verified compatibility across at least two Unreal minor versions.
- Required CI gates for Editor, Development, Shipping, automation, cook, and packaged runtime smoke tests on every release commit.
- Documented upgrade guidance for every breaking beta change.

Bidirectional text and OpenType shaping are not planned for the core renderer: those jobs require a shaping engine and source font data, which BMFont does not contain.
