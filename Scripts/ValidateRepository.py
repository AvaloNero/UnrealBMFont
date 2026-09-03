# Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

from __future__ import annotations

import json
import re
import struct
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
TEXT_SUFFIXES = {
    ".cpp",
    ".cs",
    ".h",
    ".ini",
    ".json",
    ".md",
    ".ps1",
    ".py",
    ".yml",
}
IGNORED_PARTS = {
    ".build",
    ".codegraph",
    ".git",
    ".idea",
    ".vs",
    ".vscode",
    "Artifacts",
    "Binaries",
    "DerivedDataCache",
    "Intermediate",
    "Saved",
    "__pycache__",
}
REQUIRED_FILES = {
    ".editorconfig",
    ".gitattributes",
    ".github/workflows/ci.yml",
    ".gitignore",
    "LICENSE",
    "README.md",
    "README.zh-CN.md",
    "CHANGELOG.md",
    "CODE_OF_CONDUCT.md",
    "CONTRIBUTING.md",
    "ROADMAP.md",
    "SECURITY.md",
    "SUPPORT.md",
    "THIRD_PARTY_NOTICES.md",
    "Config/FilterPlugin.ini",
    "Docs/Architecture.md",
    "Docs/Compatibility.md",
    "Docs/FormatSupport.md",
    "Docs/ReleaseChecklist.md",
    "Docs/RichText.md",
    "Docs/Testing.md",
    "Docs/Images/showcase-runtime.png",
    "Resources/Icon128.png",
    "Samples/GroundTruth/PackedAtlas.png",
    "Samples/GroundTruth/PlainGlyphs.png",
    "Samples/GroundTruth/RichTextEllipsis.png",
    "Samples/GroundTruth/TintShadowWrap.png",
    "Samples/Minimal/Minimal.fnt",
    "Samples/Minimal/Minimal.png",
    "Samples/Packed/Packed.fnt",
    "Samples/Packed/Packed.png",
    "Samples/Showcase/Showcase.fnt",
    "Samples/Showcase/Showcase.png",
    "Scripts/BuildPlugin.ps1",
    "Scripts/TestPackagedRuntime.ps1",
    "Scripts/TestPlugin.ps1",
    "UnrealBMFont.uplugin",
}


def fail(errors: list[str], message: str) -> None:
    errors.append(message)


def read_text_if_file(errors: list[str], relative_path: str) -> str | None:
    path = ROOT / relative_path
    if not path.is_file():
        return None
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as error:
        fail(errors, f"{relative_path}: cannot read as UTF-8 ({error})")
        return None


def png_dimensions(path: Path) -> tuple[int, int]:
    data = path.read_bytes()
    if len(data) < 24 or data[:8] != b"\x89PNG\r\n\x1a\n" or data[12:16] != b"IHDR":
        raise ValueError("not a PNG with an IHDR header")
    return struct.unpack(">II", data[16:24])


def iter_repository_files() -> list[Path]:
    return [
        path
        for path in ROOT.rglob("*")
        if path.is_file() and not any(part in IGNORED_PARTS for part in path.relative_to(ROOT).parts)
    ]


def main() -> int:
    errors: list[str] = []
    for relative_path in sorted(REQUIRED_FILES):
        if not (ROOT / relative_path).is_file():
            fail(errors, f"missing required file: {relative_path}")

    descriptor_path = ROOT / "UnrealBMFont.uplugin"
    try:
        descriptor = json.loads(descriptor_path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        fail(errors, f"invalid plugin descriptor: {error}")
        descriptor = {}

    if descriptor.get("FriendlyName") != "Unreal BMFont":
        fail(errors, "FriendlyName must be 'Unreal BMFont'")
    version = descriptor.get("Version")
    if not isinstance(version, int) or isinstance(version, bool) or version <= 0:
        fail(errors, "Version must be a positive integer")
    version_name = descriptor.get("VersionName")
    if not isinstance(version_name, str) or re.fullmatch(
        r"(?:0|[1-9]\d*)\.(?:0|[1-9]\d*)\.(?:0|[1-9]\d*)", version_name
    ) is None:
        fail(errors, "VersionName must be a three-component semantic version")
    if descriptor.get("EnabledByDefault") is not False:
        fail(
            errors,
            "EnabledByDefault must be false so content-only projects generate a plugin-aware target",
        )
    if isinstance(version_name, str):
        for readme_name in ("README.md", "README.zh-CN.md"):
            readme_text = read_text_if_file(errors, readme_name)
            if readme_text is not None and f"`{version_name}`" not in readme_text:
                fail(errors, f"{readme_name}: does not identify the descriptor version {version_name}")
        changelog_text = read_text_if_file(errors, "CHANGELOG.md")
        release_heading = re.compile(
            rf"^## \[{re.escape(version_name)}\] - \d{{4}}-\d{{2}}-\d{{2}}$",
            re.MULTILINE,
        )
        if changelog_text is not None and release_heading.search(changelog_text) is None:
            fail(errors, f"CHANGELOG.md: missing a dated {version_name} release section")
    expected_modules = [
        ("UnrealBMFont", "Runtime"),
        ("UnrealBMFontEditor", "Editor"),
        ("UnrealBMFontTests", "Editor"),
    ]
    actual_modules = [
        (module.get("Name"), module.get("Type")) for module in descriptor.get("Modules", [])
    ]
    if actual_modules != expected_modules:
        fail(errors, f"unexpected module topology: {actual_modules!r}")

    package_filter = read_text_if_file(errors, "Config/FilterPlugin.ini")
    if package_filter is not None:
        package_filter_lines = package_filter.splitlines()
        for required_filter in ("/*.md", "/LICENSE", "/Docs/...", "/Samples/..."):
            if required_filter not in package_filter_lines:
                fail(errors, f"FilterPlugin.ini must retain release file {required_filter}")
        if "/Scripts/..." in package_filter_lines:
            fail(errors, "FilterPlugin.ini must not ship repository-only scripts")

    ci_text = read_text_if_file(errors, ".github/workflows/ci.yml")
    if ci_text is not None and "Scripts/TestPackagedRuntime.ps1" not in ci_text:
        fail(errors, "CI must invoke the packaged runtime smoke script")

    runtime_rules = read_text_if_file(errors, "Source/UnrealBMFont/UnrealBMFont.Build.cs")
    if runtime_rules is not None:
        for editor_dependency in ("UnrealEd", "AssetDefinition", "AssetTools"):
            if f'"{editor_dependency}"' in runtime_rules:
                fail(errors, f"Runtime module depends on Editor module {editor_dependency}")

    source_suffixes = {".cpp", ".cs", ".h", ".ps1", ".py"}
    for path in iter_repository_files():
        relative_path = path.relative_to(ROOT).as_posix()
        content: str | None = None
        if path.suffix.lower() in TEXT_SUFFIXES or path.name == "LICENSE":
            try:
                content = path.read_text(encoding="utf-8")
            except (OSError, UnicodeError) as error:
                fail(errors, f"{relative_path}: cannot read as UTF-8 ({error})")
                continue
            if content and not content.endswith("\n"):
                fail(errors, f"{relative_path}: missing final newline")
            for line_number, line in enumerate(content.splitlines(), start=1):
                if line.endswith((" ", "\t")):
                    fail(errors, f"{relative_path}:{line_number}: trailing whitespace")
        if path.suffix.lower() in source_suffixes and content is not None:
            first_line = content.splitlines()[0] if content else ""
            if "Copyright (c) 2026 AvaloNero" not in first_line:
                fail(errors, f"{relative_path}: missing project copyright header")
        if path.name.startswith("AssetTypeActions_"):
            fail(errors, f"AssetTypeActions_ file naming is not allowed: {relative_path}")

    for relative_path, expected in (
        ("Resources/Icon128.png", (128, 128)),
        ("Samples/GroundTruth/PackedAtlas.png", (64, 32)),
        ("Samples/GroundTruth/PlainGlyphs.png", (64, 32)),
        ("Samples/GroundTruth/RichTextEllipsis.png", (24, 32)),
        ("Samples/GroundTruth/TintShadowWrap.png", (64, 48)),
        ("Samples/Minimal/Minimal.png", (64, 16)),
    ):
        try:
            actual = png_dimensions(ROOT / relative_path)
            if actual != expected:
                fail(errors, f"{relative_path}: expected {expected}, got {actual}")
        except (OSError, ValueError) as error:
            fail(errors, f"{relative_path}: {error}")

    if errors:
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 1

    print("Repository validation passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
