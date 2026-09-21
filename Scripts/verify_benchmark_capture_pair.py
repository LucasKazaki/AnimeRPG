#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

import benchmark_manifest
import verify_benchmark_run_control as run_control

SCHEMA_VERSION = 1
MAX_JSON_BYTES = benchmark_manifest.MAX_MANIFEST_BYTES
PROFILE_ROLES = (
    "cpu_frame_timing_csv",
    "cpu_phase_timing_csv",
    "process_memory_csv",
)
MATCHED_DESCRIPTOR_FIELDS = (
    "candidate",
    "workload",
    "run_protocol",
    "environment",
    "reference_versions",
    "provenance",
)
MATCHED_RUN_CONTROL_FIELDS = (
    "candidate_commit",
    "executable_sha256",
    "simulation_fixed_hz",
    "warmup_frames",
    "measured_frames",
    "warmup_seconds",
    "sample_seconds",
    "total_frames",
    "completed_frames",
    "client_width_px",
    "client_height_px",
    "client_area_observations",
    "client_area_stable",
    "client_area_control",
    "window_mode",
    "vsync_requested",
    "presentation_backend",
    "vsync_control",
    "frame_pacing",
    "live_input",
    "termination",
)
ACCEPTANCE = {
    "capture_pairing_verified": True,
    "instrumentation_overhead_verified": False,
    "performance_budget_verified": False,
    "ram_budget_verified": False,
    "vram_budget_verified": False,
    "gpu_timing_verified": False,
    "comparative_parity_verified": False,
    "clean_machine_compatibility_verified": False,
    "independent_acceptance": False,
}
LIMITATIONS = [
    "This receipt proves a profiled/control benchmark pair is package-, workload-, protocol-, environment-, and run-control-matched while the control manifest omits Astral profiling-stream evidence roles.",
    "The profiled manifest role checks do not replace the existing stream-coherence verifier, and the absence of profiling roles from the control manifest does not itself prove that every profiling environment variable was absent at process launch.",
    "This packet does not calculate instrumentation overhead. A later native analysis must use retained launch/environment receipts and measured control/profiled timings before instrumentation_overhead_verified can become true.",
    "No GPU timing, VRAM budget, performance budget, Unreal/Unity parity, clean-machine compatibility, soak completion, or independent acceptance follows from capture-pair verification.",
]


class CapturePairError(ValueError):
    pass


def _now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds").replace("+00:00", "Z")


def _load_json(path: Path, label: str) -> dict[str, Any]:
    try:
        if path.is_symlink() or not path.is_file():
            raise CapturePairError(f"{label} must be a regular non-symlink file")
        if path.stat().st_size > MAX_JSON_BYTES:
            raise CapturePairError(f"{label} byte limit exceeded")
        value = json.loads(path.read_text(encoding="utf-8"))
    except CapturePairError:
        raise
    except Exception as exc:
        raise CapturePairError(f"cannot read {label}: {exc}") from exc
    if not isinstance(value, dict):
        raise CapturePairError(f"{label} root must be an object")
    return value


def _profile_role_counts(manifest: dict[str, Any]) -> dict[str, int]:
    evidence = manifest.get("evidence")
    if not isinstance(evidence, list):
        raise CapturePairError("benchmark evidence must be an array")
    counts = {role: 0 for role in PROFILE_ROLES}
    for item in evidence:
        if not isinstance(item, dict):
            continue
        role = item.get("role")
        if role in counts:
            counts[role] += 1
    return counts


def _matched_run_control(report: dict[str, Any]) -> dict[str, Any]:
    missing = [key for key in MATCHED_RUN_CONTROL_FIELDS if key not in report]
    if missing:
        raise CapturePairError(f"run-control verification report missing fields: {missing}")
    return {key: report[key] for key in MATCHED_RUN_CONTROL_FIELDS}


def _verify_one(
    manifest_path: Path,
    package_root: Path,
    release_manifest: Path,
    evidence_root: Path,
    label: str,
) -> tuple[dict[str, Any], dict[str, Any], dict[str, Any]]:
    manifest = _load_json(manifest_path, f"{label} benchmark manifest")
    try:
        benchmark_report = benchmark_manifest.verify_benchmark_manifest(
            manifest, package_root, release_manifest, evidence_root
        )
    except Exception as exc:
        raise CapturePairError(f"{label} benchmark manifest verification failed: {exc}") from exc
    try:
        run_report = run_control.verify(
            manifest_path, package_root, release_manifest, evidence_root
        )
    except Exception as exc:
        raise CapturePairError(f"{label} run-control verification failed: {exc}") from exc
    return manifest, benchmark_report, run_report


def verify_capture_pair(
    profiled_manifest_path: Path,
    control_manifest_path: Path,
    package_root: Path,
    release_manifest: Path,
    profiled_evidence_root: Path,
    control_evidence_root: Path,
) -> dict[str, Any]:
    profiled, profiled_benchmark, profiled_run = _verify_one(
        profiled_manifest_path,
        package_root,
        release_manifest,
        profiled_evidence_root,
        "profiled",
    )
    control, control_benchmark, control_run = _verify_one(
        control_manifest_path,
        package_root,
        release_manifest,
        control_evidence_root,
        "control",
    )

    for field in MATCHED_DESCRIPTOR_FIELDS:
        if profiled.get(field) != control.get(field):
            raise CapturePairError(f"profiled/control descriptor mismatch: {field}")

    profiled_identity = (
        profiled_benchmark.get("candidate_commit"),
        profiled_benchmark.get("executable_sha256"),
    )
    control_identity = (
        control_benchmark.get("candidate_commit"),
        control_benchmark.get("executable_sha256"),
    )
    if profiled_identity != control_identity:
        raise CapturePairError("profiled/control candidate or executable identity mismatch")

    profiled_control = _matched_run_control(profiled_run)
    control_control = _matched_run_control(control_run)
    if profiled_control != control_control:
        changed = [
            key for key in MATCHED_RUN_CONTROL_FIELDS
            if profiled_control.get(key) != control_control.get(key)
        ]
        raise CapturePairError(
            "profiled/control run-control mismatch: " + ", ".join(changed)
        )

    profiled_roles = _profile_role_counts(profiled)
    control_roles = _profile_role_counts(control)
    wrong_profiled = {
        role: count for role, count in profiled_roles.items() if count != 1
    }
    if wrong_profiled:
        raise CapturePairError(
            f"profiled manifest must contain each profiling role exactly once: {wrong_profiled}"
        )
    present_control = {
        role: count for role, count in control_roles.items() if count != 0
    }
    if present_control:
        raise CapturePairError(
            f"control manifest must omit profiling-stream evidence roles: {present_control}"
        )

    profiled_descriptor = profiled_benchmark.get("benchmark_descriptor_sha256")
    control_descriptor = control_benchmark.get("benchmark_descriptor_sha256")
    if not isinstance(profiled_descriptor, str) or not isinstance(control_descriptor, str):
        raise CapturePairError("verified benchmark descriptor hash missing")

    return {
        "schema_version": SCHEMA_VERSION,
        "verification_kind": "astral_benchmark_capture_pair",
        "generated_at_utc": _now(),
        "capture_pairing_verified": True,
        "candidate_commit": profiled_run["candidate_commit"],
        "executable_sha256": profiled_run["executable_sha256"],
        "profiled_benchmark_descriptor_sha256": profiled_descriptor,
        "control_benchmark_descriptor_sha256": control_descriptor,
        "workload": json.loads(json.dumps(profiled["workload"])),
        "run_protocol": json.loads(json.dumps(profiled["run_protocol"])),
        "environment": json.loads(json.dumps(profiled["environment"])),
        "reference_versions": json.loads(json.dumps(profiled["reference_versions"])),
        "provenance": json.loads(json.dumps(profiled["provenance"])),
        "run_control": profiled_control,
        "profiled_capture_roles": sorted(PROFILE_ROLES),
        "control_capture_roles": [],
        "acceptance": dict(ACCEPTANCE),
        "limitations": list(LIMITATIONS),
    }


def _write_new_json(path: Path, value: dict[str, Any]) -> None:
    if path.exists() or path.is_symlink():
        raise CapturePairError("output already exists or is a symlink")
    if not path.parent.is_dir():
        raise CapturePairError("output parent must be an existing directory")
    text = json.dumps(value, indent=2, sort_keys=True) + "\n"
    if len(text.encode("utf-8")) > MAX_JSON_BYTES:
        raise CapturePairError("output byte limit exceeded")
    with path.open("x", encoding="utf-8", newline="\n") as stream:
        stream.write(text)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--profiled-manifest", required=True, type=Path)
    parser.add_argument("--control-manifest", required=True, type=Path)
    parser.add_argument("--package-root", required=True, type=Path)
    parser.add_argument("--release-manifest", required=True, type=Path)
    parser.add_argument("--profiled-evidence-root", required=True, type=Path)
    parser.add_argument("--control-evidence-root", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args(argv)

    try:
        report = verify_capture_pair(
            args.profiled_manifest,
            args.control_manifest,
            args.package_root,
            args.release_manifest,
            args.profiled_evidence_root,
            args.control_evidence_root,
        )
        _write_new_json(args.output, report)
    except (CapturePairError, OSError, ValueError) as exc:
        print(f"benchmark capture-pair verification failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
