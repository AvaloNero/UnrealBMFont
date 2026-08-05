# Compatibility

| Area | Verified | Not yet claimed |
| --- | --- | --- |
| Unreal Engine | 5.8 source build | 5.7 and earlier; future minors |
| Host platform | Windows 11, Win64 | Linux, macOS |
| Targets | UnrealEditor Development, UnrealGame Development, UnrealGame Shipping | Consoles and mobile |
| Runtime smoke | Headless Editor automation and packaged Development launch with NullRHI | Packaged Shipping launch; interactive screenshot comparison |
| Descriptor formats | Text, compact XML, binary v3 | Binary v1/v2 |
| Page images | PNG | Every format accepted by TextureFactory |

The Runtime module uses platform-neutral Engine, Slate, SlateCore, UMG, and XmlParser APIs, but portability is a hypothesis until the target is compiled and exercised in CI.

Source distributions intentionally omit `EngineVersion` from the plugin descriptor so developers can attempt newer compatible engines. That omission is not a promise of forward compatibility. Releases should state the exact tested engine CL/version and regenerate packaged binaries per engine version.
