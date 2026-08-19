# Release checklist

## Scope and metadata

- Choose the version, update `UnrealBMFont.uplugin`, and replace the changelog's `[Unreleased]` heading with `[version] - YYYY-MM-DD`, using the actual publication date only when cutting the release.
- Confirm `IsBetaVersion` matches the release maturity.
- Set repository-specific documentation and support URLs only after their final public URLs exist.
- Verify every new source, fixture, image, and document has known provenance and compatible licensing.

## Verification

- Run `Scripts/GenerateBrandAssets.ps1` and confirm it produces no unintended visual diff.
- Regenerate `Content/M_BMFontPacked.uasset` with `Scripts/GeneratePackedMaterial.py` and confirm the parameter contract is unchanged.
- Regenerate the render-test ground truth with `-UpdateBMFontGroundTruth` on the reference GPU and review every PNG diff.
- Run `Scripts/BuildPlugin.ps1` for every claimed target platform.
- Confirm UnrealEditor Development, UnrealGame Development, and UnrealGame Shipping all succeed without warnings introduced by the plugin.
- Run `Scripts/TestPlugin.ps1`; archive `index.json` and the editor log.
- Run the `UnrealBMFont.Render.*` suite on a GPU (no `-NullRHI`) and archive its report.
- Import `Samples/Minimal/Minimal.fnt` in a clean project and visually inspect `AB`, fallback, wrap, tint, shadow, and reimport.
- Open the asset inspector on an imported font and verify the summary, atlas overlay, and glyph table.
- Cook and launch a packaged Development build containing `UBMFontText`.
- Launch a packaged Shipping build and retain a screenshot or deterministic runtime marker.

## Package audit

- Extract the package into a clean directory.
- Confirm Runtime has no dependency on UnrealEd or AssetDefinition.
- Confirm the test module is absent from UnrealGame targets.
- Confirm the packaged descriptor has `Installed=true` and `EnabledByDefault=false`.
- Confirm the package contains `LICENSE`, `THIRD_PARTY_NOTICES.md`, README files, format/compatibility documentation, and `Content/M_BMFontPacked.uasset`.
- Confirm `BuildPlugin.ps1` reports that its automated release-file and personal/source-path audit passed.
- Scan for credentials, private URLs, stale vendor names, and generated host-project files.

## Publish

- Tag the exact tested commit.
- Attach immutable checksums for release archives.
- Publish the compatibility matrix and known limitations with the release notes.
- Do not claim a platform, engine version, cook, or runtime result that was not exercised for this release.
