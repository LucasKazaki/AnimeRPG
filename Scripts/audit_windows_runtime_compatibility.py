#!/usr/bin/env python3
"""Audit Astral's observed central VC runtime files against the build toolset version.

This is an evidence check only. It does not prove that the Microsoft
Redistributable installer is present, supported, bundled, or launch-tested.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any

SCHEMA_VERSION = 1
_MICROSOFT_LIFECYCLE = "https://learn.microsoft.com/en-us/lifecycle/faq/visual-c-faq"
_MICROSOFT_COMPILER_VERSIONS = "https://learn.microsoft.com/en-us/cpp/overview/compiler-versions?view=msvc-170"
_VERSION = re.compile(r"^v?(\d+)\.(\d+)\.(\d+)(?:\.(\d+))?$", re.IGNORECASE)


class RuntimeCompatibilityError(ValueError):
    pass


def _load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise RuntimeCompatibilityError(f"cannot read runtime preflight: {exc}") from exc
    if not isinstance(value, dict):
        raise RuntimeCompatibilityError("runtime preflight root must be an object")
    return value


def parse_version(value: str, label: str) -> tuple[int, int, int, int]:
    if not isinstance(value, str):
        raise RuntimeCompatibilityError(f"{label} must be a version string")
    match = _VERSION.fullmatch(value.strip())
    if not match:
        raise RuntimeCompatibilityError(f"{label} must contain 3 or 4 numeric components")
    return tuple(int(part or "0") for part in match.groups())  # type: ignore[return-value]


def normalize_version(value: tuple[int, int, int, int]) -> str:
    return ".".join(str(part) for part in value)


def _validate_preflight(preflight: dict[str, Any]) -> tuple[str, list[dict[str, Any]]]:
    if preflight.get("schema_version") != 1:
        raise RuntimeCompatibilityError("unsupported runtime-preflight schema_version")
    package = preflight.get("package")
    runtime = preflight.get("vc_runtime")
    acceptance = preflight.get("acceptance")
    if not isinstance(package, dict) or not isinstance(runtime, dict) or not isinstance(acceptance, dict):
        raise RuntimeCompatibilityError("runtime preflight is missing package/vc_runtime/acceptance objects")
    if runtime.get("strategy") != "central_vc_redist":
        raise RuntimeCompatibilityError("only the reviewed central_vc_redist strategy is accepted")
    if runtime.get("all_required_files_present") is not True or runtime.get("all_present_files_versioned") is not True:
        raise RuntimeCompatibilityError("runtime preflight must have all required files present and versioned")
    if acceptance.get("preflight_passed") is not True:
        raise RuntimeCompatibilityError("runtime preflight did not pass")
    for field in ("package_launch_verified", "clean_machine_compatibility_verified", "independent_acceptance"):
        if acceptance.get(field) is not False:
            raise RuntimeCompatibilityError(f"runtime preflight must not pre-claim {field}")
    if runtime.get("runtime_version_compatibility_verified") is not False:
        raise RuntimeCompatibilityError("input preflight must leave runtime-version compatibility unverified")

    required = runtime.get("required_imports")
    files = runtime.get("files")
    if not isinstance(required, list) or not all(isinstance(name, str) and name for name in required):
        raise RuntimeCompatibilityError("vc_runtime.required_imports must be a list of non-empty strings")
    if not isinstance(files, list) or not all(isinstance(item, dict) for item in files):
        raise RuntimeCompatibilityError("vc_runtime.files must be a list of objects")
    by_name: dict[str, dict[str, Any]] = {}
    for item in files:
        name = item.get("name")
        if not isinstance(name, str) or not name:
            raise RuntimeCompatibilityError("runtime file entry has no valid name")
        key = name.lower()
        if key in by_name:
            raise RuntimeCompatibilityError(f"duplicate runtime file entry: {name}")
        by_name[key] = item
    if set(name.lower() for name in required) != set(by_name):
        raise RuntimeCompatibilityError("runtime file entries do not exactly match required_imports")
    sha = package.get("sha256")
    if not isinstance(sha, str) or len(sha) != 64:
        raise RuntimeCompatibilityError("package has no valid sha256")
    return sha.lower(), [by_name[name.lower()] for name in required]


def audit_runtime_compatibility(preflight: dict[str, Any], vc_tools_version: str) -> dict[str, Any]:
    image_sha, files = _validate_preflight(preflight)
    toolset = parse_version(vc_tools_version, "VC tools version")
    comparisons: list[dict[str, Any]] = []
    compatible = True
    for item in files:
        file_version = parse_version(item.get("file_version"), f"{item.get('name')} file_version")
        satisfies = file_version >= toolset
        compatible = compatible and satisfies
        comparisons.append({
            "name": item["name"],
            "runtime_file_version": normalize_version(file_version),
            "minimum_build_tools_version": normalize_version(toolset),
            "satisfies_version_floor": satisfies,
        })
    return {
        "schema_version": SCHEMA_VERSION,
        "package_sha256": image_sha,
        "build_toolchain": {
            "vc_tools_version": normalize_version(toolset),
            "version_source": "explicit build-environment evidence",
        },
        "runtime_files": comparisons,
        "compatibility": {
            "rule": "Each required observed VC runtime file version must be greater than or equal to the VC Build Tools version used for the build.",
            "runtime_file_version_floor_satisfied": compatible,
            "runtime_version_compatibility_verified": compatible,
            "supported_redist_installation_verified": False,
        },
        "acceptance": {
            "package_launch_verified": False,
            "clean_machine_compatibility_verified": False,
            "independent_acceptance": False,
        },
        "sources": {
            "microsoft_vc_lifecycle": _MICROSOFT_LIFECYCLE,
            "microsoft_compiler_versions": _MICROSOFT_COMPILER_VERSIONS,
        },
        "limitations": [
            "This compares the observed runtime DLL file versions to the recorded VC Build Tools version; it does not prove installer package provenance or support state.",
            "This does not bundle, install, repair, copy, or execute the Microsoft Visual C++ Redistributable.",
            "A finished package still requires launch testing on a supported clean Windows machine and independent acceptance.",
        ],
    }


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Compare observed VC runtime file versions with Astral's build toolset.")
    parser.add_argument("runtime_preflight", type=Path)
    parser.add_argument("--vc-tools-version", required=True)
    parser.add_argument("--json", type=Path)
    parser.add_argument("--fail-on-incompatible", action="store_true")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    try:
        result = audit_runtime_compatibility(_load_json(args.runtime_preflight), args.vc_tools_version)
    except RuntimeCompatibilityError as exc:
        print(f"Windows runtime compatibility audit failed: {exc}", file=sys.stderr)
        return 1
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    print(encoded, end="")
    if args.json is not None:
        try:
            args.json.parent.mkdir(parents=True, exist_ok=True)
            args.json.write_text(encoded, encoding="utf-8")
        except OSError as exc:
            print(f"Windows runtime compatibility audit write failed: {exc}", file=sys.stderr)
            return 1
    if args.fail_on_incompatible and not result["compatibility"]["runtime_file_version_floor_satisfied"]:
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
