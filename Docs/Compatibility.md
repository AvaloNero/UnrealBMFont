# Compatibility

| Area | Verified | Not yet claimed |
| --- | --- | --- |
| Unreal Engine | 5.8 source build | 5.7 and earlier; future minors |
| Host platform | Windows 11, Win64 | Linux, macOS |
| Targets | UnrealEditor Development, UnrealGame Development, UnrealGame Shipping | Consoles and mobile |
| Runtime smoke | Headless Editor automation plus automated packaged Development and Shipping launches with NullRHI, widget construction, and packed-material load markers | Interactive packaged screenshot comparison |
| Widget rendering | GPU screenshot tests against committed ground truth, reference GPU/driver | Cross-vendor GPU variance beyond the comparison tolerance |
| Descriptor formats | Text, compact XML, binary v3 | Binary v1/v2 |
| Page images | PNG | Every format accepted by TextureFactory |
| Packed atlases | Glyph coverage extraction via the bundled UI material | Separate outline-channel compositing |

The Runtime module uses platform-neutral Engine, Slate, SlateCore, UMG, and XmlParser APIs, but portability is a hypothesis until the target is compiled and exercised in CI.

Source distributions intentionally omit `EngineVersion` from the plugin descriptor so developers can attempt newer compatible engines. That omission is not a promise of forward compatibility. Releases should state the exact tested engine CL/version and regenerate packaged binaries per engine version.
