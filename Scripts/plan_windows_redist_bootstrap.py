#!/usr/bin/env python3
"""Plan Astral's central Visual C++ Redistributable prerequisite bootstrap.

The planner binds an already-reviewed prerequisite plan and runtime compatibility
report to the Visual C++ v14 registration visible on the current Windows host. It
never downloads, installs, repairs, copies, or executes the Redistributable.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import sys
from pathlib import Path
from typing import Any

SCHEMA_VERSION = 1
MICROSOFT_REDISTRIBUTE = "https://learn.microsoft.com/en-us/cpp/windows/redistributing-visual-cpp-files?view=msvc-170"
MICROSOFT_LATEST = "https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170"
UE_PREREQUISITES = "https://dev.epicgames.com/documentation/en-us/unreal-engine/project-section-of-the-unreal-engine-project-settings"

_DOWNLOADS = {
    "x64": ("vc_redist.x64.exe", "https://aka.ms/vc14/vc_redist.x64.exe"),
    "x86": ("vc_redist.x86.exe", "https://aka.ms/vc14/vc_redist.x86.exe"),
    "arm64": ("vc_redist.arm64.exe", "https://aka.ms/vc14/vc_redist.arm64.exe"),
}
_VERSION = re.compile(r"^v?(\d+)\.(\d+)\.(\d+)(?:\.(\d+))?$", re.IGNORECASE)


class BootstrapPlanError(ValueError):
    pass


def _load_json(path: Path, label: str) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise BootstrapPlanError(f"cannot read {label}: {exc}") from exc
    if not isinstance(value, dict):
        raise BootstrapPlanError(f"{label} root must be an object")
    return value


def _parse_version(value: Any, label: str) -> tuple[int, int, int, int]:
    if not isinstance(value, str):
        raise BootstrapPlanError(f"{label} must be a version string")
    match = _VERSION.fullmatch(value.strip())
    if not match:
        raise BootstrapPlanError(f"{label} must contain 3 or 4 numeric components")
    return tuple(int(part or "0") for part in match.groups())  # type: ignore[return-value]


def _version_text(value: tuple[int, int, int, int]) -> str:
    return ".".join(str(part) for part in value)


def _validate_inputs(prereq: dict[str, Any], compatibility: dict[str, Any]) -> tuple[str, str, tuple[int, int, int, int]]:
    if prereq.get("schema_version") != 1 or compatibility.get("schema_version") != 1:
        raise BootstrapPlanError("unsupported prerequisite or compatibility schema_version")
    source = prereq.get("source_dependency_report")
    policy = prereq.get("release_policy")
    runtime = prereq.get("vc_runtime")
    comp = compatibility.get("compatibility")
    toolchain = compatibility.get("build_toolchain")
    acceptance = compatibility.get("acceptance")
    if not all(isinstance(value, dict) for value in (source, policy, runtime, comp, toolchain, acceptance)):
        raise BootstrapPlanError("required prerequisite/compatibility objects are missing")
    assert isinstance(source, dict) and isinstance(policy, dict) and isinstance(runtime, dict)
    assert isinstance(comp, dict) and isinstance(toolchain, dict) and isinstance(acceptance, dict)

    sha = source.get("image_sha256")
    architecture = runtime.get("architecture")
    if not isinstance(sha, str) or len(sha) != 64:
        raise BootstrapPlanError("prerequisite plan has no valid package sha256")
    if compatibility.get("package_sha256") != sha:
        raise BootstrapPlanError("compatibility evidence is bound to a different package")
    if architecture not in _DOWNLOADS or source.get("architecture") != architecture:
        raise BootstrapPlanError("prerequisite architecture is missing, unsupported, or inconsistent")
    if runtime.get("strategy") != "central_vc_redist" or policy.get("requires_vc_runtime") is not True:
        raise BootstrapPlanError("bootstrap planner only accepts the reviewed central_vc_redist policy")
    if policy.get("reject_release") is not False or policy.get("clean_machine_compatibility_verified") is not False:
        raise BootstrapPlanError("prerequisite plan contains a rejected release or false clean-machine claim")
    if runtime.get("installer_bundled") is not False or runtime.get("installer_executed") is not False:
        raise BootstrapPlanError("input prerequisite plan must not claim installer bundling or execution")
    if comp.get("runtime_file_version_floor_satisfied") is not True or comp.get("runtime_version_compatibility_verified") is not True:
        raise BootstrapPlanError("runtime compatibility must be verified before bootstrap planning")
    if comp.get("supported_redist_installation_verified") is not False:
        raise BootstrapPlanError("input compatibility evidence must not pre-claim Redistributable installation verification")
    for field in ("package_launch_verified", "clean_machine_compatibility_verified", "independent_acceptance"):
        if acceptance.get(field) is not False:
            raise BootstrapPlanError(f"input compatibility evidence must not pre-claim {field}")

    minimum = _parse_version(toolchain.get("vc_tools_version"), "build_toolchain.vc_tools_version")
    return sha.lower(), architecture, minimum


def _normalize_registry_record(record: dict[str, Any], architecture: str) -> dict[str, Any]:
    if record.get("architecture") != architecture:
        raise BootstrapPlanError("registry snapshot architecture does not match package architecture")
    version = _parse_version(record.get("version"), "registered Redistributable version")
    components = record.get("components")
    if components is not None:
        if not isinstance(components, dict):
            raise BootstrapPlanError("registry components must be an object when present")
        expected = {"Major": version[0], "Minor": version[1], "Bld": version[2], "Rbld": version[3]}
        for key, expected_value in expected.items():
            if key in components and components[key] != expected_value:
                raise BootstrapPlanError(f"registry {key} does not match Version")
    installed = record.get("installed")
    if installed is not None and installed not in (1, True):
        raise BootstrapPlanError("registry record explicitly reports the Redistributable as not installed")
    source = record.get("source")
    if not isinstance(source, str) or not source:
        source = "injected registry snapshot"
    return {
        "architecture": architecture,
        "version": _version_text(version),
        "version_tuple": version,
        "source": source,
        "installed_flag": installed,
    }


def _read_registry_record(architecture: str) -> dict[str, Any] | None:
    if os.name != "nt":
        raise BootstrapPlanError("live registry probing requires Windows; use --registry-snapshot for portable tests")
    try:
        import winreg
    except ImportError as exc:  # pragma: no cover - only meaningful on Windows
        raise BootstrapPlanError("winreg is unavailable on this Windows Python") from exc

    subkey = rf"SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\{architecture}"
    candidates: list[tuple[str, tuple[int, int, int, int]]] = []
    for view_name, view_flag in (("32-bit registry view", winreg.KEY_WOW64_32KEY), ("64-bit registry view", winreg.KEY_WOW64_64KEY)):
        try:
            with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, subkey, 0, winreg.KEY_READ | view_flag) as key:
                def query(name: str) -> Any:
                    try:
                        return winreg.QueryValueEx(key, name)[0]
                    except FileNotFoundError:
                        return None
                version = query("Version")
                if version is None:
                    continue
                components: dict[str, Any] = {}
                for name in ("Major", "Minor", "Bld", "Rbld"):
                    value = query(name)
                    if value is not None:
                        components[name] = value
                raw = {
                    "architecture": architecture,
                    "version": str(version),
                    "installed": query("Installed"),
                    "components": components,
                    "source": f"HKLM\\{subkey} ({view_name})",
                }
                normalized = _normalize_registry_record(raw, architecture)
                candidates.append((normalized["source"], normalized["version_tuple"]))
        except FileNotFoundError:
            continue
        except OSError as exc:
            raise BootstrapPlanError(f"cannot read Visual C++ Redistributable registry key: {exc}") from exc

    if not candidates:
        return None
    source, version = max(candidates, key=lambda item: item[1])
    return {"architecture": architecture, "version": _version_text(version), "source": source, "installed": 1}


def build_bootstrap_plan(
    prereq: dict[str, Any],
    compatibility: dict[str, Any],
    registry_record: dict[str, Any] | None,
) -> dict[str, Any]:
    package_sha, architecture, minimum = _validate_inputs(prereq, compatibility)
    normalized = _normalize_registry_record(registry_record, architecture) if registry_record is not None else None
    registered_version = normalized["version_tuple"] if normalized is not None else None
    floor_satisfied = registered_version is not None and registered_version >= minimum
    action = "skip_install" if floor_satisfied else "install_latest_supported"
    filename, permalink = _DOWNLOADS[architecture]

    return {
        "schema_version": SCHEMA_VERSION,
        "package_sha256": package_sha,
        "architecture": architecture,
        "minimum_vc_tools_version": _version_text(minimum),
        "registered_v14_redist": None if normalized is None else {
            "version": normalized["version"],
            "source": normalized["source"],
            "version_floor_satisfied": floor_satisfied,
        },
        "bootstrap": {
            "strategy": "central_vc_redist",
            "action": action,
            "official_latest_supported_permalink": permalink,
            "expected_installer_filename": filename,
            "installer_command_template": [filename, "/install", "/passive", "/norestart", "/log", "%TEMP%\\Astral-vc-redist.log"],
            "requires_elevation": True,
            "skip_when_registered_version_is_same_or_newer_than_minimum": True,
            "installer_downloaded": False,
            "installer_executed": False,
            "installation_receipt_verified": False,
            "redistribution_license_review_required": True,
        },
        "acceptance": {
            "bootstrap_decision_verified": True,
            "supported_redist_installation_verified": False,
            "package_launch_verified": False,
            "clean_machine_compatibility_verified": False,
            "independent_acceptance": False,
        },
        "sources": {
            "microsoft_redistribution_and_registry": MICROSOFT_REDISTRIBUTE,
            "microsoft_latest_supported_downloads": MICROSOFT_LATEST,
            "unreal_engine_prerequisite_reference": UE_PREREQUISITES,
        },
        "limitations": [
            "The registry probe is a prerequisite decision input, not proof of clean-machine package compatibility.",
            "This tool never downloads, installs, repairs, copies, signs, or executes the Microsoft Redistributable.",
            "The latest-supported permalink can change target bytes over time; a release workflow must retain installer provenance/hash/version if it distributes a copy.",
            "Redistribution is subject to Microsoft license terms and must be reviewed before bundling an installer.",
            "A finished package still needs launch testing on a supported clean Windows machine plus independent acceptance.",
        ],
    }


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Plan the central VC Redistributable bootstrap for an Astral Windows package.")
    parser.add_argument("prerequisite_plan", type=Path)
    parser.add_argument("runtime_compatibility", type=Path)
    parser.add_argument("--registry-snapshot", type=Path, help="Use a JSON registry fixture instead of probing the live Windows registry.")
    parser.add_argument("--json", type=Path, help="Write the bootstrap plan to this path.")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    try:
        prereq = _load_json(args.prerequisite_plan, "prerequisite plan")
        compatibility = _load_json(args.runtime_compatibility, "runtime compatibility report")
        architecture = prereq.get("vc_runtime", {}).get("architecture") if isinstance(prereq.get("vc_runtime"), dict) else None
        if architecture not in _DOWNLOADS:
            raise BootstrapPlanError("cannot determine a supported architecture from the prerequisite plan")
        registry = _load_json(args.registry_snapshot, "registry snapshot") if args.registry_snapshot else _read_registry_record(architecture)
        result = build_bootstrap_plan(prereq, compatibility, registry)
    except BootstrapPlanError as exc:
        print(f"Windows Redistributable bootstrap planning failed: {exc}", file=sys.stderr)
        return 1

    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    print(encoded, end="")
    if args.json is not None:
        try:
            args.json.parent.mkdir(parents=True, exist_ok=True)
            args.json.write_text(encoded, encoding="utf-8")
        except OSError as exc:
            print(f"Windows Redistributable bootstrap plan write failed: {exc}", file=sys.stderr)
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
