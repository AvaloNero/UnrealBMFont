# Contributing

Thanks for improving Unreal BMFont. Small, reviewable changes with reproducible evidence are preferred.

## Before opening a change

- Search existing issues before creating a duplicate.
- For behavior changes, describe the affected `.fnt` format, Unreal version, and target platform.
- Do not add proprietary fonts, generated atlases with unclear licenses, or copied engine/plugin source.
- Keep Runtime free of Editor-only dependencies.

## Build

Run the dependency-free metadata and repository checks first:

```powershell
python ./Scripts/ValidateRepository.py
```

Use an empty output directory and a UE 5.8 installation or source tree:

```powershell
pwsh ./Scripts/BuildPlugin.ps1 `
  -EngineRoot "C:\Program Files\Epic Games\UE_5.8" `
  -OutputDirectory "$env:TEMP\UnrealBMFont-Package"
```

`BuildPlugin` must pass for UnrealEditor, UnrealGame Development, and UnrealGame Shipping on the platform being claimed.

Unreal Automation Tool copies the complete plugin directory into a temporary host before applying `FilterPlugin.ini`. Keep engine trees, local build caches, packages, and automation reports outside the plugin root. The wrapper creates a clean temporary staging copy and rejects output directories inside the source tree so `.build`, `Artifacts`, and earlier packages are never restaged.

## Test

Install the packaged plugin in a lightweight host project, then run:

```powershell
pwsh ./Scripts/TestPlugin.ps1 `
  -EngineRoot "C:\Program Files\Epic Games\UE_5.8" `
  -Project "D:\Projects\BMFontHost\BMFontHost.uproject" `
  -ReportDirectory "$env:TEMP\UnrealBMFont-Reports"
```

Add or update automation coverage for parser, layout, importer, or rendering behavior. Tests must guard lookups before dereferencing so a regression reports a failure instead of crashing the editor.

## Pull request checklist

- The change has a focused rationale and no unrelated generated files.
- New public behavior is documented in both READMEs or the relevant document.
- Format/compatibility claims match actual tests.
- Runtime code builds without Editor modules.
- Automation tests pass and the report is attached or summarized.
- New files have the repository copyright header where appropriate.
- `CHANGELOG.md` is updated for user-visible changes.

By contributing, you agree that your contribution is licensed under this repository's MIT License.
