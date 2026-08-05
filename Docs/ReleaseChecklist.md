# Release checklist

## Scope and metadata

- Choose the version and update `UnrealBMFont.uplugin` plus `CHANGELOG.md`.
- Confirm `IsBetaVersion` matches the release maturity.
- Set repository-specific documentation and support URLs only after their final public URLs exist.
- Verify every new source, fixture, image, and document has known provenance and compatible licensing.

## Verification

- Run `Scripts/GenerateBrandAssets.ps1` and confirm it produces no unintended visual diff.
- Run `Scripts/BuildPlugin.ps1` for every claimed target platform.
- Confirm UnrealEditor Development, UnrealGame Development, and UnrealGame Shipping all succeed without warnings introduced by the plugin.
- Run `Scripts/TestPlugin.ps1`; archive `index.json` and the editor log.
- Import `Samples/Minimal/Minimal.fnt` in a clean project and visually inspect `AB`, fallback, wrap, tint, shadow, and reimport.
- Cook and launch a packaged Development build containing `UBMFontText`.
- Launch a packaged Shipping build and retain a screenshot or deterministic runtime marker.

## Package audit

- Extract the package into a clean directory.
- Confirm Runtime has no dependency on UnrealEd or AssetDefinition.
- Confirm the test module is absent from UnrealGame targets.
- Confirm the packaged descriptor has `Installed=true` and `EnabledByDefault=false`.
- Confirm the package contains `LICENSE`, `THIRD_PARTY_NOTICES.md`, README files, and format/compatibility documentation.
- Scan for local absolute paths, credentials, private URLs, stale vendor names, and generated host-project files.

## Publish

- Tag the exact tested commit.
- Attach immutable checksums for release archives.
- Publish the compatibility matrix and known limitations with the release notes.
- Do not claim a platform, engine version, cook, or runtime result that was not exercised for this release.
