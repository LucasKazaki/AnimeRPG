#!/usr/bin/env python3
"""Turn a PE dependency inventory into an explicit Windows prerequisite plan.

This tool does not download, install, or copy redistributables. It converts the
bounded evidence emitted by inspect_pe_dependencies.py into a package-side policy
record so a release cannot silently forget the runtime prerequisite decision.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

SCHEMA_VERSION = 1
MICROSOFT_REDIST_PAGE = (
    "https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170"
)
MICROSOFT_DEPLOYMENT_PAGE = (
    "https://learn.microsoft.com/en-us/cpp/windows/choosing-a-deployment-method?view=msvc-170"
)

_MACHINE_ARCH = {
    "AMD64": "x64",
    "I386": "x86",
    "ARM64": "arm64",
}


class PrerequisitePlanError(ValueError):
    pass


def _string_list(value: Any, field: str) -> list[str]:
    if not isinstance(value, list) or not all(isinstance(item, str) and item for item in value):
        raise PrerequisitePlanError(f"{field} must be a list of non-empty strings")
    return value


def _require_dict(value: Any, field: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise PrerequisitePlanError(f"{field} must be an object")
    return value


def build_plan(report: dict[str, Any]) -> dict[str, Any]:
    if report.get("schema_version") != 1:
        raise PrerequisitePlanError("unsupported dependency-report schema_version")

    machine = report.get("machine")
    if machine not in _MACHINE_ARCH:
        raise PrerequisitePlanError(f"unsupported or unknown PE machine: {machine!r}")
    architecture = _MACHINE_ARCH[machine]

    sha256 = report.get("sha256")
    if not isinstance(sha256, str) or len(sha256) != 64:
        raise PrerequisitePlanError("dependency report has no 64-character sha256")

    all_imports = set(_string_list(report.get("all_imports"), "all_imports"))
    analysis = _require_dict(report.get("analysis"), "analysis")
    debug_imports = _string_list(analysis.get("debug_crt_imports"), "analysis.debug_crt_imports")
    vc_imports = _string_list(analysis.get("vc_runtime_imports"), "analysis.vc_runtime_imports")
    ucrt_imports = _string_list(analysis.get("ucrt_imports"), "analysis.ucrt_imports")
    api_imports = _string_list(analysis.get("api_set_imports"), "analysis.api_set_imports")

    classified = set(debug_imports) | set(vc_imports) | set(ucrt_imports) | set(api_imports)
    if not classified.issubset(all_imports):
        missing = sorted(classified - all_imports, key=str.lower)
        raise PrerequisitePlanError(
            "dependency report analysis references imports missing from all_imports: " + ", ".join(missing)
        )
    if analysis.get("clean_machine_compatibility_verified") is not False:
        raise PrerequisitePlanError(
            "dependency inventory must not claim clean-machine compatibility; launch evidence is separate"
        )

    requires_vc = bool(vc_imports)
    rejects_release = bool(debug_imports)
    plan: dict[str, Any] = {
        "schema_version": SCHEMA_VERSION,
        "source_dependency_report": {
            "image_sha256": sha256,
            "machine": machine,
            "architecture": architecture,
        },
        "release_policy": {
            "debug_runtime_imports": debug_imports,
            "reject_release": rejects_release,
            "requires_vc_runtime": requires_vc,
            "clean_machine_compatibility_verified": False,
            "clean_machine_launch_required": True,
        },
        "vc_runtime": {
            "strategy": "central_vc_redist" if requires_vc else "not_required_by_import_table",
            "architecture": architecture if requires_vc else None,
            "runtime_family": "Microsoft Visual C++ v14 Redistributable" if requires_vc else None,
            "imports": vc_imports,
            "installer_bundled": False,
            "installer_executed": False,
            "app_local_fallback_selected": False,
            "static_runtime_switch_selected": False,
            "official_download_reference": MICROSOFT_REDIST_PAGE if requires_vc else None,
            "deployment_guidance_reference": MICROSOFT_DEPLOYMENT_PAGE if requires_vc else None,
        },
        "limitations": [
            "This plan does not download, install, copy, or license-check Microsoft redistributable files.",
            "Import-table inspection does not prove that the target machine has required runtime DLLs.",
            "The package still requires a launch test on a supported clean Windows machine.",
            "Libraries loaded dynamically at runtime can require additional prerequisite review.",
        ],
    }
    return plan


def load_report(path: Path) -> dict[str, Any]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise PrerequisitePlanError(f"cannot read dependency report: {exc}") from exc
    if not isinstance(data, dict):
        raise PrerequisitePlanError("dependency report root must be an object")
    return data


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Create a Windows prerequisite deployment plan from a PE dependency report."
    )
    parser.add_argument("dependency_report", type=Path)
    parser.add_argument("--json", type=Path, help="Write the prerequisite plan to this path.")
    parser.add_argument(
        "--fail-on-debug-runtime",
        action="store_true",
        help="Return exit 2 when the dependency report includes a Debug Microsoft C/C++ runtime.",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    try:
        plan = build_plan(load_report(args.dependency_report))
    except PrerequisitePlanError as exc:
        print(f"Windows prerequisite planning failed: {exc}", file=sys.stderr)
        return 1

    encoded = json.dumps(plan, indent=2, sort_keys=True) + "\n"
    print(encoded, end="")
    if args.json is not None:
        try:
            args.json.parent.mkdir(parents=True, exist_ok=True)
            args.json.write_text(encoded, encoding="utf-8")
        except OSError as exc:
            print(f"Windows prerequisite plan write failed: {exc}", file=sys.stderr)
            return 1

    if args.fail_on_debug_runtime and plan["release_policy"]["reject_release"]:
        print("Release image imports a Debug Microsoft C/C++ runtime DLL.", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
