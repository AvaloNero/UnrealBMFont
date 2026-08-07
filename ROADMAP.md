# Roadmap

The roadmap records direction, not a compatibility promise.

## 0.3

- Validate Linux and macOS builds and runtime rendering.
- Profile large glyph sets and long localized strings.
- Add optional import policies for texture filtering and texture asset placement.

## 1.0 criteria

- Public API and serialized asset versioning policy.
- Verified compatibility across at least two Unreal minor versions.
- CI coverage for Editor, Development, Shipping, automation, cook, and a packaged runtime smoke test.
- Documented migration path for every breaking beta change.

Bidirectional text and OpenType shaping are not planned for the core renderer: those jobs require a shaping engine and source font data, which BMFont does not contain.
