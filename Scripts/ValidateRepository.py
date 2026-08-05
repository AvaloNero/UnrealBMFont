# Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

from __future__ import annotations

import json
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
IGNORED_PARTS = {".build", ".git", "Artifacts", "Binaries", "Intermediate", "Saved"}
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
    "Docs/Testing.md",
    "Resources/Icon128.png",
    "Samples/Minimal/Minimal.fnt",
    "Samples/Minimal/Minimal.png",
    "UnrealBMFont.uplugin",
}


def fail(errors: list[str], message: str) -> None:
    errors.append(message)


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
    if descriptor.get("EnabledByDefault") is not False:
        fail(
            errors,
            "EnabledByDefault must be false so content-only projects generate a plugin-aware target",
        )
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

    runtime_rules = (ROOT / "Source/UnrealBMFont/UnrealBMFont.Build.cs").read_text(encoding="utf-8")
    for editor_dependency in ("UnrealEd", "AssetDefinition", "AssetTools"):
        if f'"{editor_dependency}"' in runtime_rules:
            fail(errors, f"Runtime module depends on Editor module {editor_dependency}")

    source_suffixes = {".cpp", ".cs", ".h", ".ps1", ".py"}
    for path in iter_repository_files():
        relative_path = path.relative_to(ROOT).as_posix()
        if path.suffix.lower() in TEXT_SUFFIXES or path.name == "LICENSE":
            try:
                content = path.read_text(encoding="utf-8")
            except UnicodeError as error:
                fail(errors, f"{relative_path}: not valid UTF-8 ({error})")
                continue
            if content and not content.endswith("\n"):
                fail(errors, f"{relative_path}: missing final newline")
            for line_number, line in enumerate(content.splitlines(), start=1):
                if line.endswith((" ", "\t")):
                    fail(errors, f"{relative_path}:{line_number}: trailing whitespace")
        if path.suffix.lower() in source_suffixes:
            first_line = path.read_text(encoding="utf-8").splitlines()[0]
            if "Copyright (c) 2026 AvaloNero" not in first_line:
                fail(errors, f"{relative_path}: missing project copyright header")
        if path.name.startswith("AssetTypeActions_"):
            fail(errors, f"AssetTypeActions_ file naming is not allowed: {relative_path}")

    for relative_path, expected in (
        ("Resources/Icon128.png", (128, 128)),
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
