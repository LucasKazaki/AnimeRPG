#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path, PurePosixPath
from typing import Any

import benchmark_manifest
import verify_benchmark_run_control as run_control

SCHEMA_VERSION = 1
MAX_RECEIPT_BYTES = 64 * 1024
ROLE = "profiling_capture_state_json"
STREAM_ROLES = {
    "cpu_frame_timing": "cpu_frame_timing_csv",
    "cpu_phase_timing": "cpu_phase_timing_csv",
    "process_memory": "process_memory_csv",
}
FALSE_CLAIMS = {
    "instrumentation_overhead_verified": False,
    "performance_budget_verified": False,
    "gpu_timing_verified": False,
    "comparative_parity_verified": False,
    "independent_acceptance": False,
}
RUN_CONTROL_KEYS = {
    "simulation_fixed_hz",
    "warmup_frames",
    "measured_frames",
    "total_frames",
    "client_width_px",
    "client_height_px",
    "window_mode",
    "vsync_requested",
    "presentation_backend",
    "vsync_control",
    "frame_pacing",
    "live_input",
    "termination",
}
FRAME_STREAM_KEYS = {"state", "output_path", "warmup_frames", "max_samples"}
MEMORY_STREAM_KEYS = {
    "state",
    "output_path",
    "warmup_frames",
    "sample_every_frames",
    "max_samples",
    "sample_source",
}


class ProfilingCaptureStateError(ValueError):
    pass


def _sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def _load_json(path: Path, label: str) -> dict[str, Any]:
    try:
        if path.is_symlink() or not path.is_file():
            raise ProfilingCaptureStateError(f"{label} must be a regular non-symlink file")
        if path.stat().st_size > MAX_RECEIPT_BYTES:
            raise ProfilingCaptureStateError(f"{label} byte limit exceeded")
        value = json.loads(path.read_text(encoding="utf-8"))
    except ProfilingCaptureStateError:
        raise
    except Exception as exc:
        raise ProfilingCaptureStateError(f"cannot read {label}: {exc}") from exc
    if not isinstance(value, dict):
        raise ProfilingCaptureStateError(f"{label} root must be an object")
    return value


def _keys(value: Any, expected: set[str], label: str) -> dict[str, Any]:
    if not isinstance(value, dict) or set(value) != expected:
        raise ProfilingCaptureStateError(f"{label} shape/schema mismatch")
    return value


def _int(value: Any, label: str, low: int, high: int) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or not low <= value <= high:
        raise ProfilingCaptureStateError(f"{label} must be an integer in [{low}, {high}]")
    return value


def _safe_stream_rel(value: Any, label: str) -> str:
    try:
        rel = benchmark_manifest._safe_rel(value)
    except Exception as exc:
        raise ProfilingCaptureStateError(f"{label} is unsafe: {exc}") from exc
    return rel


def _validate_disabled_stream(stream: dict[str, Any], keys: set[str], label: str) -> None:
    if stream.get("state") != "not_requested":
        raise ProfilingCaptureStateError(f"{label} must be not_requested")
    for key in keys - {"state"}:
        if stream.get(key) is not None:
            raise ProfilingCaptureStateError(f"{label}.{key} must be null when capture is disabled")


def validate_receipt(receipt: dict[str, Any]) -> dict[str, Any]:
    top = {
        "schema_version",
        "capture_state_semantics",
        "environment_scope",
        "raw_environment_dumped",
        "run_control",
        "streams",
        "claim_boundaries",
    }
    _keys(receipt, top, "profiling capture-state receipt")
    if receipt["schema_version"] != SCHEMA_VERSION or isinstance(receipt["schema_version"], bool):
        raise ProfilingCaptureStateError("profiling capture-state schema_version must be 1")
    if receipt["capture_state_semantics"] != "post_configuration_pre_frame_loop":
        raise ProfilingCaptureStateError("profiling capture-state semantics changed")
    if receipt["environment_scope"] != "known_astral_profiling_controls_only":
        raise ProfilingCaptureStateError("profiling capture-state environment scope changed")
    if receipt["raw_environment_dumped"] is not False:
        raise ProfilingCaptureStateError("profiling capture-state receipt must not dump raw environment data")

    rc = _keys(receipt["run_control"], RUN_CONTROL_KEYS, "capture-state run_control")
    _int(rc["simulation_fixed_hz"], "run_control.simulation_fixed_hz", 1, 1000)
    _int(rc["warmup_frames"], "run_control.warmup_frames", 0, 1_000_000)
    _int(rc["measured_frames"], "run_control.measured_frames", 1, 1_000_000)
    _int(rc["total_frames"], "run_control.total_frames", 1, 1_000_000)
    _int(rc["client_width_px"], "run_control.client_width_px", 1, 16384)
    _int(rc["client_height_px"], "run_control.client_height_px", 1, 16384)
    if rc["total_frames"] != rc["warmup_frames"] + rc["measured_frames"]:
        raise ProfilingCaptureStateError("capture-state total_frames is inconsistent")
    constants = {
        "window_mode": "windowed",
        "vsync_requested": False,
        "presentation_backend": "win32_gdi_window_dc",
        "vsync_control": "unavailable_in_gdi_path",
        "frame_pacing": "sleep_1ms_not_refresh_locked",
        "live_input": "suppressed",
        "termination": "exact_frame_limit",
    }
    for key, expected in constants.items():
        if rc[key] != expected:
            raise ProfilingCaptureStateError(f"capture-state run-control policy changed: {key}")

    streams = _keys(receipt["streams"], set(STREAM_ROLES), "capture-state streams")
    for name in ("cpu_frame_timing", "cpu_phase_timing"):
        stream = _keys(streams[name], FRAME_STREAM_KEYS, name)
        if stream["state"] == "enabled":
            _safe_stream_rel(stream["output_path"], f"{name}.output_path")
            _int(stream["warmup_frames"], f"{name}.warmup_frames", 0, 1_000_000)
            _int(stream["max_samples"], f"{name}.max_samples", 1, 1_000_000)
        elif stream["state"] == "not_requested":
            _validate_disabled_stream(stream, FRAME_STREAM_KEYS, name)
        else:
            raise ProfilingCaptureStateError(f"{name}.state is invalid")

    memory = _keys(streams["process_memory"], MEMORY_STREAM_KEYS, "process_memory")
    if memory["state"] == "enabled":
        _safe_stream_rel(memory["output_path"], "process_memory.output_path")
        _int(memory["warmup_frames"], "process_memory.warmup_frames", 0, 1_000_000)
        _int(memory["sample_every_frames"], "process_memory.sample_every_frames", 1, 1_000_000)
        _int(memory["max_samples"], "process_memory.max_samples", 1, 1_000_000)
        if memory["sample_source"] not in (
            "windows_process_counters",
            "caller_supplied_contract_sample",
        ):
            raise ProfilingCaptureStateError("process_memory.sample_source is invalid")
    elif memory["state"] == "not_requested":
        _validate_disabled_stream(memory, MEMORY_STREAM_KEYS, "process_memory")
    else:
        raise ProfilingCaptureStateError("process_memory.state is invalid")

    claims = _keys(receipt["claim_boundaries"], set(FALSE_CLAIMS), "claim_boundaries")
    for key, expected in FALSE_CLAIMS.items():
        if claims[key] is not expected:
            raise ProfilingCaptureStateError(f"profiling capture-state claim boundary changed: {key}")
    return json.loads(json.dumps(receipt))


def _role_entries(manifest: dict[str, Any], role: str) -> list[dict[str, Any]]:
    evidence = manifest.get("evidence")
    if not isinstance(evidence, list):
        raise ProfilingCaptureStateError("benchmark evidence must be an array")
    return [item for item in evidence if isinstance(item, dict) and item.get("role") == role]


def _joined_evidence_path(receipt_rel: str, stream_rel: str) -> str:
    parent = PurePosixPath(receipt_rel).parent
    joined = (parent / PurePosixPath(stream_rel)).as_posix()
    try:
        return benchmark_manifest._safe_rel(joined)
    except Exception as exc:
        raise ProfilingCaptureStateError(f"capture output escapes benchmark evidence root: {exc}") from exc


def _check_run_control_binding(receipt: dict[str, Any], run_report: dict[str, Any]) -> None:
    for key in RUN_CONTROL_KEYS:
        if key not in run_report:
            raise ProfilingCaptureStateError(f"run-control verification report missing {key}")
        if receipt["run_control"][key] != run_report[key]:
            raise ProfilingCaptureStateError(f"capture-state/run-control mismatch: {key}")


def _check_streams(
    receipt: dict[str, Any],
    manifest: dict[str, Any],
    receipt_rel: str,
    run_report: dict[str, Any],
    expected_mode: str,
) -> None:
    streams = receipt["streams"]
    measured = run_report["measured_frames"]
    warmup = run_report["warmup_frames"]

    if expected_mode == "control":
        for name, raw_role in STREAM_ROLES.items():
            _validate_disabled_stream(
                streams[name], MEMORY_STREAM_KEYS if name == "process_memory" else FRAME_STREAM_KEYS, name
            )
            if _role_entries(manifest, raw_role):
                raise ProfilingCaptureStateError(
                    f"control manifest must omit profiling-stream evidence role: {raw_role}"
                )
        return

    if expected_mode != "profiled":
        raise ProfilingCaptureStateError("expected_mode must be profiled or control")

    for name, raw_role in STREAM_ROLES.items():
        stream = streams[name]
        if stream["state"] != "enabled":
            raise ProfilingCaptureStateError(f"profiled run did not enable {name}")
        if stream["warmup_frames"] != warmup:
            raise ProfilingCaptureStateError(f"{name} warmup does not match run control")
        entries = _role_entries(manifest, raw_role)
        if len(entries) != 1:
            raise ProfilingCaptureStateError(
                f"profiled manifest must contain exactly one {raw_role} evidence entry"
            )
        expected_path = _joined_evidence_path(receipt_rel, stream["output_path"])
        if entries[0].get("path") != expected_path:
            raise ProfilingCaptureStateError(f"{name} startup output does not match manifest evidence")

    for name in ("cpu_frame_timing", "cpu_phase_timing"):
        if streams[name]["max_samples"] < measured:
            raise ProfilingCaptureStateError(f"{name} max_samples cannot cover measured frames")
    memory = streams["process_memory"]
    required_memory = (measured + memory["sample_every_frames"] - 1) // memory["sample_every_frames"]
    if memory["max_samples"] < required_memory:
        raise ProfilingCaptureStateError("process_memory max_samples cannot cover measured frames")
    if memory["sample_source"] != "windows_process_counters":
        raise ProfilingCaptureStateError(
            "profiled native benchmark must use Windows process memory counters"
        )


def verify(
    manifest_path: Path,
    package_root: Path,
    release_manifest: Path,
    evidence_root: Path,
    expected_mode: str,
) -> dict[str, Any]:
    try:
        manifest = benchmark_manifest._load_json(
            manifest_path, benchmark_manifest.MAX_MANIFEST_BYTES, "benchmark manifest"
        )
        benchmark_report = benchmark_manifest.verify_benchmark_manifest(
            manifest, package_root, release_manifest, evidence_root
        )
        run_report = run_control.verify(
            manifest_path, package_root, release_manifest, evidence_root
        )
    except Exception as exc:
        raise ProfilingCaptureStateError(f"benchmark/run-control verification failed: {exc}") from exc

    entries = _role_entries(manifest, ROLE)
    if len(entries) != 1:
        raise ProfilingCaptureStateError(
            f"benchmark manifest must contain exactly one {ROLE} evidence entry"
        )
    entry = entries[0]
    receipt_rel = benchmark_manifest._safe_rel(entry.get("path"))
    receipt_path = benchmark_manifest._evidence_file(evidence_root, receipt_rel)
    receipt = validate_receipt(_load_json(receipt_path, "profiling capture-state receipt"))
    digest = _sha256(receipt_path)
    if entry.get("sha256") != digest:
        raise ProfilingCaptureStateError(
            "profiling capture-state receipt hash does not match benchmark manifest"
        )

    _check_run_control_binding(receipt, run_report)
    _check_streams(receipt, manifest, receipt_rel, run_report, expected_mode)

    return {
        "schema_version": 1,
        "profiling_capture_state_verified": True,
        "expected_mode": expected_mode,
        "candidate_commit": benchmark_report["candidate_commit"],
        "executable_sha256": benchmark_report["executable_sha256"],
        "benchmark_descriptor_sha256": benchmark_report["benchmark_descriptor_sha256"],
        "capture_state_receipt_sha256": digest,
        "run_control": json.loads(json.dumps(receipt["run_control"])),
        "streams": json.loads(json.dumps(receipt["streams"])),
        "acceptance": {
            "capture_state_binding_verified": True,
            **FALSE_CLAIMS,
        },
        "limitations": [
            "This verifies the three known Astral profiling capture subsystems as interpreted by the process after configuration and before the benchmark frame loop; it does not dump or certify unrelated environment variables.",
            "The startup receipt binds configured output identities and settings. Final profiling-file bytes are separately SHA-256-bound by the benchmark manifest after the run because those files do not exist yet at startup.",
            "Capture-state proof is necessary for a matched profiled/control experiment but does not calculate instrumentation overhead or establish performance, GPU timing, parity, clean-machine compatibility, soak completion, or independent acceptance.",
        ],
    }


def _write_new(path: Path, value: dict[str, Any]) -> None:
    if path.exists() or path.is_symlink():
        raise ProfilingCaptureStateError("output JSON already exists or is a symlink")
    if not path.parent.is_dir():
        raise ProfilingCaptureStateError("output JSON parent must be an existing directory")
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8", newline="\n")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("manifest", type=Path)
    parser.add_argument("package_root", type=Path)
    parser.add_argument("release_manifest", type=Path)
    parser.add_argument("evidence_root", type=Path)
    parser.add_argument("--expected-mode", required=True, choices=("profiled", "control"))
    parser.add_argument("--json", type=Path)
    args = parser.parse_args(argv)
    try:
        report = verify(
            args.manifest,
            args.package_root,
            args.release_manifest,
            args.evidence_root,
            args.expected_mode,
        )
        if args.json:
            _write_new(args.json, report)
    except (ProfilingCaptureStateError, OSError, ValueError) as exc:
        print(f"Profiling capture-state verification failed: {exc}", file=sys.stderr)
        return 1
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
