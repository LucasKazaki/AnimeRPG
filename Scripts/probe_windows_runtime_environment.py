#!/usr/bin/env python3
"""Probe the central Windows VC runtime environment for one Astral package image.

This tool is evidence-only. It does not download, install, repair, copy, or launch
anything. A successful probe means the runtime files named by the package's PE
import report exist in the expected central Windows directory and their file
metadata could be read. It does not establish clean-machine launch compatibility.
"""
from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import os
import platform
import re
import sys
from pathlib import Path
from typing import Any, Callable

SCHEMA_VERSION = 1
_MICROSOFT_LIFECYCLE = "https://learn.microsoft.com/en-us/lifecycle/faq/visual-c-faq"
_MICROSOFT_AUDIT = "https://learn.microsoft.com/en-us/cpp/windows/redist-version-auditing?view=msvc-170"
_MACHINE_ARCH = {"AMD64": "x64", "I386": "x86", "ARM64": "arm64"}
_SAFE_DLL = re.compile(r"^[A-Za-z0-9_.-]+\.dll$", re.IGNORECASE)


class RuntimeProbeError(ValueError):
    pass


def _load_json(path: Path, label: str) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise RuntimeProbeError(f"cannot read {label}: {exc}") from exc
    if not isinstance(value, dict):
        raise RuntimeProbeError(f"{label} root must be an object")
    return value


def _string_list(value: Any, field: str) -> list[str]:
    if not isinstance(value, list) or not all(isinstance(item, str) and item for item in value):
        raise RuntimeProbeError(f"{field} must be a list of non-empty strings")
    return value


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    try:
        with path.open("rb") as handle:
            while chunk := handle.read(1024 * 1024):
                digest.update(chunk)
    except OSError as exc:
        raise RuntimeProbeError(f"cannot hash package image {path}: {exc}") from exc
    return digest.hexdigest()


def validate_evidence(
    dependency_report: dict[str, Any], prerequisite_plan: dict[str, Any], image_path: Path
) -> tuple[str, list[str], str]:
    if dependency_report.get("schema_version") != 1:
        raise RuntimeProbeError("unsupported dependency-report schema_version")
    if prerequisite_plan.get("schema_version") != 1:
        raise RuntimeProbeError("unsupported prerequisite-plan schema_version")

    machine = dependency_report.get("machine")
    if machine not in _MACHINE_ARCH:
        raise RuntimeProbeError(f"unsupported PE machine: {machine!r}")
    architecture = _MACHINE_ARCH[machine]

    report_sha = dependency_report.get("sha256")
    if not isinstance(report_sha, str) or len(report_sha) != 64:
        raise RuntimeProbeError("dependency report has no valid sha256")
    actual_sha = _sha256(image_path)
    if actual_sha.lower() != report_sha.lower():
        raise RuntimeProbeError("package image sha256 does not match dependency report")

    source = prerequisite_plan.get("source_dependency_report")
    if not isinstance(source, dict):
        raise RuntimeProbeError("prerequisite plan has no source_dependency_report")
    if source.get("image_sha256", "").lower() != report_sha.lower():
        raise RuntimeProbeError("prerequisite plan references a different package image")
    if source.get("machine") != machine or source.get("architecture") != architecture:
        raise RuntimeProbeError("prerequisite plan machine/architecture disagrees with dependency report")

    analysis = dependency_report.get("analysis")
    if not isinstance(analysis, dict):
        raise RuntimeProbeError("dependency report has no analysis object")
    report_imports = _string_list(analysis.get("vc_runtime_imports"), "analysis.vc_runtime_imports")
    vc = prerequisite_plan.get("vc_runtime")
    if not isinstance(vc, dict):
        raise RuntimeProbeError("prerequisite plan has no vc_runtime object")
    plan_imports = _string_list(vc.get("imports"), "vc_runtime.imports")
    if sorted(name.lower() for name in report_imports) != sorted(name.lower() for name in plan_imports):
        raise RuntimeProbeError("prerequisite-plan VC imports disagree with dependency report")

    if report_imports:
        if vc.get("strategy") != "central_vc_redist":
            raise RuntimeProbeError("runtime probe only accepts the reviewed central_vc_redist strategy")
        if vc.get("architecture") != architecture:
            raise RuntimeProbeError("VC runtime architecture disagrees with package architecture")
    for name in report_imports:
        if not _SAFE_DLL.fullmatch(name) or Path(name).name != name or "/" in name or "\\" in name:
            raise RuntimeProbeError(f"unsafe VC runtime import name: {name!r}")

    release = prerequisite_plan.get("release_policy")
    if not isinstance(release, dict):
        raise RuntimeProbeError("prerequisite plan has no release_policy object")
    if release.get("clean_machine_compatibility_verified") is not False:
        raise RuntimeProbeError("input plan must not claim clean-machine compatibility")
    if release.get("reject_release") is True:
        raise RuntimeProbeError("prerequisite plan rejects this Release image")
    return architecture, report_imports, actual_sha


class VS_FIXEDFILEINFO(ctypes.Structure):
    _fields_ = [
        ("dwSignature", ctypes.c_uint32),
        ("dwStrucVersion", ctypes.c_uint32),
        ("dwFileVersionMS", ctypes.c_uint32),
        ("dwFileVersionLS", ctypes.c_uint32),
        ("dwProductVersionMS", ctypes.c_uint32),
        ("dwProductVersionLS", ctypes.c_uint32),
        ("dwFileFlagsMask", ctypes.c_uint32),
        ("dwFileFlags", ctypes.c_uint32),
        ("dwFileOS", ctypes.c_uint32),
        ("dwFileType", ctypes.c_uint32),
        ("dwFileSubtype", ctypes.c_uint32),
        ("dwFileDateMS", ctypes.c_uint32),
        ("dwFileDateLS", ctypes.c_uint32),
    ]


def _words(value: int) -> tuple[int, int]:
    return ((value >> 16) & 0xFFFF, value & 0xFFFF)


def read_windows_file_version(path: Path) -> str:
    if os.name != "nt":
        raise RuntimeProbeError("Windows file-version metadata is only available on Windows")
    version = ctypes.WinDLL("version", use_last_error=True)
    version.GetFileVersionInfoSizeW.argtypes = [ctypes.c_wchar_p, ctypes.POINTER(ctypes.c_uint32)]
    version.GetFileVersionInfoSizeW.restype = ctypes.c_uint32
    version.GetFileVersionInfoW.argtypes = [ctypes.c_wchar_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_void_p]
    version.GetFileVersionInfoW.restype = ctypes.c_int
    version.VerQueryValueW.argtypes = [ctypes.c_void_p, ctypes.c_wchar_p, ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(ctypes.c_uint32)]
    version.VerQueryValueW.restype = ctypes.c_int

    ignored = ctypes.c_uint32(0)
    size = version.GetFileVersionInfoSizeW(str(path), ctypes.byref(ignored))
    if size == 0:
        raise RuntimeProbeError(f"cannot read file-version size for {path} (winerror={ctypes.get_last_error()})")
    buffer = ctypes.create_string_buffer(size)
    if not version.GetFileVersionInfoW(str(path), 0, size, buffer):
        raise RuntimeProbeError(f"cannot read file-version data for {path} (winerror={ctypes.get_last_error()})")
    pointer = ctypes.c_void_p()
    length = ctypes.c_uint32(0)
    if not version.VerQueryValueW(buffer, "\\", ctypes.byref(pointer), ctypes.byref(length)):
        raise RuntimeProbeError(f"cannot query fixed file version for {path} (winerror={ctypes.get_last_error()})")
    if length.value < ctypes.sizeof(VS_FIXEDFILEINFO):
        raise RuntimeProbeError(f"truncated fixed file version for {path}")
    fixed = ctypes.cast(pointer, ctypes.POINTER(VS_FIXEDFILEINFO)).contents
    if fixed.dwSignature != 0xFEEF04BD:
        raise RuntimeProbeError(f"invalid fixed file-version signature for {path}")
    major, minor = _words(fixed.dwFileVersionMS)
    build, revision = _words(fixed.dwFileVersionLS)
    return f"{major}.{minor}.{build}.{revision}"


def central_runtime_directory(system_root: Path, architecture: str) -> Path:
    if architecture in {"x64", "arm64"}:
        return system_root / "System32"
    if architecture == "x86":
        return system_root / "SysWOW64"
    raise RuntimeProbeError(f"unsupported runtime architecture: {architecture}")


def probe_runtime(
    dependency_report: dict[str, Any],
    prerequisite_plan: dict[str, Any],
    image_path: Path,
    *,
    system_root: Path,
    version_reader: Callable[[Path], str] = read_windows_file_version,
) -> dict[str, Any]:
    architecture, imports, image_sha = validate_evidence(dependency_report, prerequisite_plan, image_path)
    runtime_dir = central_runtime_directory(system_root, architecture)
    files: list[dict[str, Any]] = []
    missing: list[str] = []
    metadata_failures: list[str] = []

    for name in sorted(imports, key=str.lower):
        path = runtime_dir / name
        entry: dict[str, Any] = {"name": name, "path": str(path), "present": path.is_file()}
        if not entry["present"]:
            missing.append(name)
        else:
            try:
                entry["sha256"] = _sha256(path)
                entry["size_bytes"] = path.stat().st_size
                entry["file_version"] = version_reader(path)
            except (OSError, RuntimeProbeError) as exc:
                entry["metadata_error"] = str(exc)
                metadata_failures.append(name)
        files.append(entry)

    all_present = not missing
    all_metadata = not metadata_failures
    return {
        "schema_version": SCHEMA_VERSION,
        "package": {
            "path": str(image_path),
            "sha256": image_sha,
            "architecture": architecture,
        },
        "host": {
            "os_name": os.name,
            "platform": platform.platform(),
            "machine": platform.machine(),
            "system_root": str(system_root),
            "central_runtime_directory": str(runtime_dir),
        },
        "vc_runtime": {
            "strategy": prerequisite_plan["vc_runtime"]["strategy"],
            "required_imports": imports,
            "files": files,
            "missing_imports": missing,
            "metadata_failures": metadata_failures,
            "all_required_files_present": all_present,
            "all_present_files_versioned": all_metadata,
            "runtime_version_compatibility_verified": False,
        },
        "acceptance": {
            "preflight_passed": all_present and all_metadata,
            "package_launch_verified": False,
            "clean_machine_compatibility_verified": False,
            "independent_acceptance": False,
        },
        "sources": {
            "microsoft_vc_runtime_lifecycle": _MICROSOFT_LIFECYCLE,
            "microsoft_vc_runtime_auditing": _MICROSOFT_AUDIT,
        },
        "limitations": [
            "This probe does not install, repair, copy, or launch any prerequisite or package.",
            "File presence and metadata do not prove that the installed Redistributable package version is supported for the build toolchain.",
            "The prerequisite plan does not yet bind an exact minimum Redistributable package version from the build environment.",
            "A finished package still requires a launch test on a supported clean Windows machine and independent acceptance.",
        ],
    }


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Probe central VC runtime files required by an Astral Windows package.")
    parser.add_argument("dependency_report", type=Path)
    parser.add_argument("prerequisite_plan", type=Path)
    parser.add_argument("image", type=Path)
    parser.add_argument("--json", type=Path, help="Write probe evidence to this path.")
    parser.add_argument("--fail-on-missing", action="store_true", help="Return exit 2 if required central runtime files/metadata are missing.")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    if os.name != "nt":
        print("Windows runtime environment probing requires Windows.", file=sys.stderr)
        return 3
    system_root_value = os.environ.get("SystemRoot")
    if not system_root_value:
        print("Windows runtime environment probing failed: SystemRoot is unset.", file=sys.stderr)
        return 1
    try:
        result = probe_runtime(
            _load_json(args.dependency_report, "dependency report"),
            _load_json(args.prerequisite_plan, "prerequisite plan"),
            args.image,
            system_root=Path(system_root_value),
        )
    except RuntimeProbeError as exc:
        print(f"Windows runtime environment probing failed: {exc}", file=sys.stderr)
        return 1

    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    print(encoded, end="")
    if args.json is not None:
        try:
            args.json.parent.mkdir(parents=True, exist_ok=True)
            args.json.write_text(encoded, encoding="utf-8")
        except OSError as exc:
            print(f"Windows runtime probe write failed: {exc}", file=sys.stderr)
            return 1
    if args.fail_on_missing and not result["acceptance"]["preflight_passed"]:
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
