# Testing

## Automation coverage

The `UnrealBMFont.*` suite covers:

- Text descriptors with BOM, quoted names, arbitrary field order, multiple pages, and supplementary Unicode code points.
- Compact XML descriptors with the declaration and document on one line.
- Generated binary v3 blocks, signed offsets, and kerning.
- Invalid metrics and atlas validation.
- Kerning, tracking, glyph offsets, wrapping, explicit lines, tabs, fallback, missing glyphs, and line-height semantics.
- A real generated PNG plus `.fnt` import through `UBMFontFactory`.
- The packaged multilingual Showcase fixture, including representative Latin, Hiragana, Chinese, and fallback code points.
- UI texture defaults, source path tracking, descriptor reimport, and preservation of user-edited texture filtering.
- Slate desired-size calculation and invalidation after text changes.

## Run against a host project

The host project must have the current plugin installed and enabled.

```powershell
pwsh ./Scripts/TestPlugin.ps1 `
  -EngineRoot "C:\Program Files\Epic Games\UE_5.8" `
  -Project "D:\Projects\BMFontHost\BMFontHost.uproject" `
  -ReportDirectory "$env:TEMP\UnrealBMFont-Reports"
```

The script runs `UnrealEditor-Cmd` with `NullRHI`, waits for the automation queue to empty, and fails if the process or JSON report reports a failure.

## Package verification

```powershell
pwsh ./Scripts/BuildPlugin.ps1 `
  -EngineRoot "C:\Program Files\Epic Games\UE_5.8" `
  -OutputDirectory "$env:TEMP\UnrealBMFont-Package" `
  -TargetPlatforms Win64
```

The wrapper first creates a clean source staging copy outside the repository, excluding local engine trees, build products, reports, and previous packages. This matters because UE 5.8's `BuildPlugin` command copies the complete input plugin to its temporary host before applying `FilterPlugin.ini`. The wrapper also verifies that the packaged descriptor is both installed and opt-in. Raw `BuildPlugin` removes `EnabledByDefault`; publishing that uncorrected descriptor from a project-local package can prevent a content-only project from generating a plugin-aware game target.

Do not describe a static source scan as a successful runtime test. A release checklist should retain the BuildPlugin log, automation `index.json`, cook log, and packaged runtime evidence.

For a packaged runtime smoke test, launch a Development build with logging enabled and verify `LogUnrealBMFont: Unreal BMFont runtime module initialized.` appears before exit.

## GitHub Actions

The dependency-free repository validation job runs on every push and pull request. The UE 5.8 build and automation job is opt-in because GitHub-hosted runners do not include Unreal Engine.

To enable the Unreal job, register a Windows x64 self-hosted runner with the `ue-5.8` label, then set these repository variables:

- `UE_CI_ENABLED=true`
- `UE_ROOT` to the Unreal Engine 5.8 installation or source-tree root on that runner
