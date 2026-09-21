#!/usr/bin/env python3
from __future__ import annotations

import argparse
from decimal import Decimal, InvalidOperation
import hashlib
import json
import sys
from pathlib import Path
from typing import Any

import benchmark_manifest

MAX_RECEIPT_BYTES = 64 * 1024
ROLE = "benchmark_run_control_json"
FALSE_CLAIMS = {
    "performance_budget_verified": False,
    "comparative_parity_verified": False,
    "independent_acceptance": False,
}
RECEIPT_KEYS = {
    "schema_version",
    "mode",
    "simulation_fixed_hz",
    "warmup_frames",
    "measured_frames",
    "total_frames",
    "completed_frames",
    "client_width_px",
    "client_height_px",
    "client_area_observations",
    "client_area_stable",
    "client_area_control",
    "window_mode",
    "live_input",
    "termination",
    *FALSE_CLAIMS.keys(),
}


class BenchmarkRunControlError(ValueError):
    pass


def _sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def _load_json(path: Path, limit: int, label: str) -> dict[str, Any]:
    try:
        if path.stat().st_size > limit:
            raise BenchmarkRunControlError(f"{label} byte limit exceeded")
        value = json.loads(path.read_text(encoding="utf-8"))
    except BenchmarkRunControlError:
        raise
    except Exception as exc:
        raise BenchmarkRunControlError(f"cannot read {label}: {exc}") from exc
    if not isinstance(value, dict):
        raise BenchmarkRunControlError(f"{label} root must be an object")
    return value


def _duration_frames(value: Any, fixed_hz: int, label: str) -> int:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise BenchmarkRunControlError(f"{label} must be numeric")
    try:
        seconds = Decimal(str(value))
    except (InvalidOperation, ValueError) as exc:
        raise BenchmarkRunControlError(f"{label} must be finite numeric seconds") from exc
    if not seconds.is_finite() or seconds < 0:
        raise BenchmarkRunControlError(f"{label} must be finite non-negative seconds")
    frames = seconds * Decimal(fixed_hz)
    integral = frames.to_integral_value()
    if frames != integral:
        raise BenchmarkRunControlError(
            f"{label} does not map to an exact integer frame count at {fixed_hz} Hz"
        )
    return int(integral)


def validate_receipt(receipt: dict[str, Any]) -> dict[str, Any]:
    if set(receipt) != RECEIPT_KEYS:
        raise BenchmarkRunControlError("run-control receipt shape/schema mismatch")
    if receipt["schema_version"] != 2 or isinstance(receipt["schema_version"], bool):
        raise BenchmarkRunControlError("run-control schema_version must be 2")
    if receipt["mode"] != "fixed_frame_count":
        raise BenchmarkRunControlError("run-control mode must be fixed_frame_count")
    if receipt["live_input"] != "suppressed":
        raise BenchmarkRunControlError("benchmark live input was not suppressed")
    if receipt["termination"] != "exact_frame_limit":
        raise BenchmarkRunControlError("benchmark did not use exact frame-limit termination")
    if receipt["window_mode"] != "windowed":
        raise BenchmarkRunControlError("run-control window_mode must be windowed")
    if receipt["client_area_stable"] is not True:
        raise BenchmarkRunControlError("benchmark client area was not stable")
    if receipt["client_area_control"] != "environment_requested_and_verified":
        raise BenchmarkRunControlError(
            "benchmark client area was not explicitly requested and verified"
        )
    for key, expected in FALSE_CLAIMS.items():
        if receipt[key] is not expected:
            raise BenchmarkRunControlError(f"run-control claim boundary changed: {key}")

    fixed_hz = receipt["simulation_fixed_hz"]
    warmup = receipt["warmup_frames"]
    measured = receipt["measured_frames"]
    total = receipt["total_frames"]
    completed = receipt["completed_frames"]
    width = receipt["client_width_px"]
    height = receipt["client_height_px"]
    observations = receipt["client_area_observations"]
    for key, value in (
        ("simulation_fixed_hz", fixed_hz),
        ("warmup_frames", warmup),
        ("measured_frames", measured),
        ("total_frames", total),
        ("completed_frames", completed),
        ("client_width_px", width),
        ("client_height_px", height),
        ("client_area_observations", observations),
    ):
        if isinstance(value, bool) or not isinstance(value, int):
            raise BenchmarkRunControlError(f"{key} must be an integer")
    if not 1 <= fixed_hz <= 1000:
        raise BenchmarkRunControlError("simulation_fixed_hz must be in [1, 1000]")
    if not 0 <= warmup <= 1_000_000:
        raise BenchmarkRunControlError("warmup_frames is out of bounds")
    if not 1 <= measured <= 1_000_000:
        raise BenchmarkRunControlError("measured_frames is out of bounds")
    if warmup + measured > 1_000_000:
        raise BenchmarkRunControlError("warmup plus measured frames exceeds hard limit")
    if total != warmup + measured or completed != total:
        raise BenchmarkRunControlError("benchmark did not complete the exact admitted frame count")
    if not 1 <= width <= 16384 or not 1 <= height <= 16384:
        raise BenchmarkRunControlError("client dimensions must be in [1, 16384]")
    if observations != total:
        raise BenchmarkRunControlError("client area was not observed exactly once per completed frame")
    return dict(receipt)


def _validate_protocol_coherence(protocol: dict[str, Any], receipt: dict[str, Any]) -> dict[str, Any]:
    fixed_hz = receipt["simulation_fixed_hz"]
    warmup_seconds = protocol.get("warmup_seconds")
    sample_seconds = protocol.get("sample_seconds")
    expected_warmup_frames = _duration_frames(
        warmup_seconds, fixed_hz, "benchmark protocol warmup_seconds"
    )
    expected_measured_frames = _duration_frames(
        sample_seconds, fixed_hz, "benchmark protocol sample_seconds"
    )
    if expected_warmup_frames != receipt["warmup_frames"]:
        raise BenchmarkRunControlError(
            "run-control warmup frame count does not match benchmark protocol duration"
        )
    if expected_measured_frames != receipt["measured_frames"]:
        raise BenchmarkRunControlError(
            "run-control measured frame count does not match benchmark protocol duration"
        )
    return {
        "warmup_seconds": warmup_seconds,
        "sample_seconds": sample_seconds,
        "warmup_frames": expected_warmup_frames,
        "measured_frames": expected_measured_frames,
        "simulation_fixed_hz": fixed_hz,
    }


def verify(
    manifest_path: Path,
    package_root: Path,
    release_path: Path,
    evidence_root: Path,
) -> dict[str, Any]:
    manifest = benchmark_manifest._load_json(
        manifest_path, benchmark_manifest.MAX_MANIFEST_BYTES, "benchmark manifest"
    )
    benchmark_report = benchmark_manifest.verify_benchmark_manifest(
        manifest, package_root, release_path, evidence_root
    )

    entries = [
        item for item in manifest.get("evidence", [])
        if isinstance(item, dict) and item.get("role") == ROLE
    ]
    if len(entries) != 1:
        raise BenchmarkRunControlError(
            f"benchmark manifest must contain exactly one {ROLE} evidence entry"
        )
    entry = entries[0]
    path = benchmark_manifest._evidence_file(
        evidence_root, benchmark_manifest._safe_rel(entry.get("path"))
    )
    receipt = validate_receipt(_load_json(path, MAX_RECEIPT_BYTES, "benchmark run-control receipt"))
    digest = _sha256(path)
    if entry.get("sha256") != digest:
        raise BenchmarkRunControlError("run-control receipt hash does not match benchmark manifest")

    protocol = manifest.get("run_protocol")
    if not isinstance(protocol, dict):
        raise BenchmarkRunControlError("benchmark manifest run_protocol is invalid")
    if receipt["client_width_px"] != protocol.get("width"):
        raise BenchmarkRunControlError("run-control client width does not match benchmark protocol")
    if receipt["client_height_px"] != protocol.get("height"):
        raise BenchmarkRunControlError("run-control client height does not match benchmark protocol")
    if receipt["window_mode"] != protocol.get("window_mode"):
        raise BenchmarkRunControlError("run-control window mode does not match benchmark protocol")
    duration = _validate_protocol_coherence(protocol, receipt)

    return {
        "schema_version": 1,
        "benchmark_run_control_verified": True,
        "benchmark_duration_protocol_coherent": True,
        "candidate_commit": benchmark_report["candidate_commit"],
        "executable_sha256": benchmark_report["executable_sha256"],
        "benchmark_descriptor_sha256": benchmark_report["benchmark_descriptor_sha256"],
        "run_control_receipt_sha256": digest,
        "simulation_fixed_hz": receipt["simulation_fixed_hz"],
        "warmup_frames": receipt["warmup_frames"],
        "measured_frames": receipt["measured_frames"],
        "warmup_seconds": duration["warmup_seconds"],
        "sample_seconds": duration["sample_seconds"],
        "total_frames": receipt["total_frames"],
        "completed_frames": receipt["completed_frames"],
        "client_width_px": receipt["client_width_px"],
        "client_height_px": receipt["client_height_px"],
        "client_area_observations": receipt["client_area_observations"],
        "client_area_stable": receipt["client_area_stable"],
        "client_area_control": receipt["client_area_control"],
        "window_mode": receipt["window_mode"],
        "live_input": receipt["live_input"],
        "termination": receipt["termination"],
        "acceptance": {
            "performance_budget_verified": False,
            "comparative_parity_verified": False,
            "independent_acceptance": False,
        },
        "limitations": [
            "The fixed simulation rate, exact frame counts, descriptor-declared warmup/sample durations, and explicitly requested stable render-client area are bound and checked for mutual consistency through the SHA-256-bound run-control receipt evidence.",
            "VSync remains a benchmark-descriptor setting rather than a runtime-proven run-control field in the current GDI path.",
            "This verifier proves run-control/package/protocol/evidence consistency only; it does not prove GPU timing, performance budgets, matched Unreal/Unity workloads, clean-machine compatibility, or independent acceptance.",
        ],
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("manifest", type=Path)
    parser.add_argument("package_root", type=Path)
    parser.add_argument("release_manifest", type=Path)
    parser.add_argument("evidence_root", type=Path)
    parser.add_argument("--json", type=Path)
    args = parser.parse_args(argv)
    try:
        report = verify(
            args.manifest, args.package_root, args.release_manifest, args.evidence_root
        )
        if args.json:
            if args.json.exists():
                raise BenchmarkRunControlError("output JSON already exists")
            args.json.parent.mkdir(parents=True, exist_ok=True)
            args.json.write_text(
                json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8"
            )
    except (BenchmarkRunControlError, benchmark_manifest.BenchmarkManifestError, OSError) as exc:
        print(f"Benchmark run-control verification failed: {exc}", file=sys.stderr)
        return 1
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())