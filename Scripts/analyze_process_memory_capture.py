#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
import os
import re
import sys
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath
from typing import Any

try:
    import benchmark_manifest
except ImportError:  # Portable parser/unit fixtures may intentionally omit repository modules.
    benchmark_manifest = None  # type: ignore[assignment]

SCHEMA_VERSION = 1
MAX_CSV_BYTES = 128 * 1024 * 1024
MAX_ANALYSIS_BYTES = 4 * 1024 * 1024
MAX_SAMPLES = 1_000_000
COMMIT_RE = re.compile(r"^[0-9a-fA-F]{40}$")
SHA256_RE = re.compile(r"^[0-9a-fA-F]{64}$")
ROLE_RE = re.compile(r"^[a-z0-9_.-]{1,64}$")
UINT_RE = re.compile(r"^(?:0|[1-9][0-9]*)$")

FIXED_METADATA = {
    "astral_process_memory_schema": "1",
    "metric": "process_os_memory_counters",
    "units": "bytes_except_page_fault_count",
    "memory_scope": "current_process",
    "working_set_semantics": "resident_working_set_bytes",
    "private_usage_semantics": "process_commit_charge_bytes",
    "allocator_attribution": "unavailable",
    "vram": "unavailable",
    "leak_detection": "not_established",
    "performance_budget_claim": "none",
    "acceptance_claim": "none",
}
ALLOWED_SAMPLE_SOURCES = {
    "caller_supplied_contract_sample",
    "Windows_GetProcessMemoryInfo_PROCESS_MEMORY_COUNTERS_EX",
}
REQUIRED_METADATA = set(FIXED_METADATA) | {
    "sample_source",
    "warmup_frames",
    "sample_every_frames",
    "max_samples",
    "samples_saturated",
}
ACCEPTANCE = {
    "ram_budget_verified": False,
    "memory_leak_free_verified": False,
    "vram_budget_verified": False,
    "allocator_attribution_verified": False,
    "comparative_parity_verified": False,
    "instrumentation_overhead_verified": False,
    "independent_acceptance": False,
}
LIMITATIONS = [
    "The source is current-process OS memory counters, not allocator ownership or callstack attribution.",
    "Working-set and private-commit trends are descriptive process-level observations and do not prove leak freedom.",
    "The capture contains frame indices but no per-sample timestamps, so this analysis reports frame-index trends rather than elapsed-time memory rates.",
    "VRAM is unavailable and no RAM/VRAM budget, matched Unreal/Unity parity, instrumentation overhead, clean-machine compatibility, or independent acceptance follows from this analysis.",
]


class ProcessMemoryAnalysisError(ValueError):
    pass


def _sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def _safe_rel(value: Any) -> str:
    if not isinstance(value, str) or not value or "\x00" in value or "\\" in value:
        raise ProcessMemoryAnalysisError("memory evidence path must be a non-empty forward-slash relative path")
    if ":" in value.split("/", 1)[0]:
        raise ProcessMemoryAnalysisError("memory evidence path must not be drive-qualified")
    path = PurePosixPath(value)
    if path.is_absolute() or any(part in ("", ".", "..") for part in path.parts) or path.as_posix() != value:
        raise ProcessMemoryAnalysisError(f"unsafe memory evidence path: {value!r}")
    return value


def _resolve_evidence(root: Path, rel: str) -> Path:
    if root.is_symlink():
        raise ProcessMemoryAnalysisError("evidence root must not be a symlink")
    try:
        resolved_root = root.resolve(strict=True)
    except OSError as exc:
        raise ProcessMemoryAnalysisError(f"cannot resolve evidence root: {exc}") from exc
    if not resolved_root.is_dir():
        raise ProcessMemoryAnalysisError("evidence root must be a directory")
    current = resolved_root
    for part in PurePosixPath(rel).parts:
        current = current / part
        if current.is_symlink():
            raise ProcessMemoryAnalysisError(f"symlinked memory evidence path component: {rel}")
    try:
        resolved = current.resolve(strict=True)
        if os.path.commonpath((str(resolved_root), str(resolved))) != str(resolved_root):
            raise ProcessMemoryAnalysisError("memory evidence escapes evidence root")
    except (OSError, ValueError) as exc:
        if isinstance(exc, ProcessMemoryAnalysisError):
            raise
        raise ProcessMemoryAnalysisError(f"cannot resolve memory evidence: {exc}") from exc
    if not resolved.is_file():
        raise ProcessMemoryAnalysisError("memory evidence must be a regular file")
    return resolved


def _parse_uint(text: str, label: str, maximum: int | None = None) -> int:
    if not isinstance(text, str) or not UINT_RE.fullmatch(text):
        raise ProcessMemoryAnalysisError(f"{label} must be a canonical unsigned decimal integer")
    value = int(text, 10)
    if maximum is not None and value > maximum:
        raise ProcessMemoryAnalysisError(f"{label} exceeds {maximum}")
    return value


def _percentile(sorted_values: list[float], probability: float) -> float:
    if not sorted_values:
        raise ProcessMemoryAnalysisError("cannot calculate a percentile of an empty sample")
    position = (len(sorted_values) - 1) * probability
    lower = int(math.floor(position))
    upper = int(math.ceil(position))
    if lower == upper:
        return sorted_values[lower]
    weight = position - lower
    return sorted_values[lower] * (1.0 - weight) + sorted_values[upper] * weight


def _median(values: list[float]) -> float:
    return _percentile(sorted(values), 0.5)


def _slope_per_1000_frames(frames: list[int], values: list[int]) -> float | None:
    if len(frames) < 2:
        return None
    x_mean = math.fsum(frames) / len(frames)
    y_mean = math.fsum(values) / len(values)
    denominator = math.fsum((frame - x_mean) ** 2 for frame in frames)
    if denominator == 0.0:
        return None
    numerator = math.fsum((frame - x_mean) * (value - y_mean) for frame, value in zip(frames, values))
    return (numerator / denominator) * 1000.0


def _metric_stats(frames: list[int], values: list[int], *, cumulative: bool = False) -> dict[str, Any]:
    ordered = sorted(float(value) for value in values)
    total = math.fsum(values)
    window = max(1, math.ceil(len(values) * 0.10))
    first_median = _median([float(value) for value in values[:window]])
    last_median = _median([float(value) for value in values[-window:]])
    result: dict[str, Any] = {
        "min": min(values),
        "mean": total / len(values),
        "p50": _percentile(ordered, 0.50),
        "p95": _percentile(ordered, 0.95),
        "p99": _percentile(ordered, 0.99),
        "max": max(values),
        "first_decile_median": first_median,
        "last_decile_median": last_median,
        "last_minus_first_decile_median": last_median - first_median,
        "ols_slope_per_1000_frames": _slope_per_1000_frames(frames, values),
        "percentile_method": "linear_interpolation_position=(n-1)*p",
        "trend_axis": "frame_index",
    }
    if cumulative:
        result["first"] = values[0]
        result["last"] = values[-1]
        result["delta"] = values[-1] - values[0]
    return result


def parse_process_memory_csv(path: Path) -> dict[str, Any]:
    try:
        size = path.stat().st_size
    except OSError as exc:
        raise ProcessMemoryAnalysisError(f"cannot stat process memory CSV: {exc}") from exc
    if size <= 0:
        raise ProcessMemoryAnalysisError("process memory CSV is empty")
    if size > MAX_CSV_BYTES:
        raise ProcessMemoryAnalysisError("process memory CSV byte limit exceeded")

    metadata: dict[str, str] = {}
    rows: list[tuple[int, int, int, int, int]] = []
    header_seen = False
    expected_header = "frame_index,working_set_bytes,peak_working_set_bytes,private_usage_bytes,page_fault_count"
    try:
        with path.open("r", encoding="utf-8", newline="") as stream:
            for line_number, raw in enumerate(stream, 1):
                if raw.endswith("\n"):
                    raw = raw[:-1]
                    if raw.endswith("\r"):
                        raw = raw[:-1]
                if not header_seen:
                    if raw.startswith("# "):
                        body = raw[2:]
                        if "=" not in body:
                            raise ProcessMemoryAnalysisError(f"line {line_number}: malformed metadata")
                        key, value = body.split("=", 1)
                        if key not in REQUIRED_METADATA:
                            raise ProcessMemoryAnalysisError(f"line {line_number}: unexpected metadata key {key!r}")
                        if key in metadata:
                            raise ProcessMemoryAnalysisError(f"line {line_number}: duplicate metadata key {key!r}")
                        metadata[key] = value
                        continue
                    if raw != expected_header:
                        raise ProcessMemoryAnalysisError(f"line {line_number}: expected exact CSV header")
                    header_seen = True
                    continue

                if not raw:
                    raise ProcessMemoryAnalysisError(f"line {line_number}: blank data row")
                fields = next(csv.reader([raw], strict=True))
                if len(fields) != 5:
                    raise ProcessMemoryAnalysisError(f"line {line_number}: expected five CSV columns")
                row = tuple(_parse_uint(value, f"line {line_number} column {index + 1}") for index, value in enumerate(fields))
                rows.append(row)  # type: ignore[arg-type]
                if len(rows) > MAX_SAMPLES:
                    raise ProcessMemoryAnalysisError("process memory CSV exceeds hard sample limit")
    except ProcessMemoryAnalysisError:
        raise
    except (OSError, UnicodeError, csv.Error) as exc:
        raise ProcessMemoryAnalysisError(f"cannot parse process memory CSV: {exc}") from exc

    if not header_seen:
        raise ProcessMemoryAnalysisError("process memory CSV is missing its header")
    missing = REQUIRED_METADATA - set(metadata)
    if missing:
        raise ProcessMemoryAnalysisError(f"missing process memory metadata: {sorted(missing)}")
    for key, expected in FIXED_METADATA.items():
        if metadata[key] != expected:
            raise ProcessMemoryAnalysisError(f"process memory metadata {key} must equal {expected!r}")
    if metadata["sample_source"] not in ALLOWED_SAMPLE_SOURCES:
        raise ProcessMemoryAnalysisError("sample_source is not a recognized Astral process-memory source")

    warmup = _parse_uint(metadata["warmup_frames"], "warmup_frames")
    stride = _parse_uint(metadata["sample_every_frames"], "sample_every_frames", MAX_SAMPLES)
    max_samples = _parse_uint(metadata["max_samples"], "max_samples", MAX_SAMPLES)
    if stride == 0:
        raise ProcessMemoryAnalysisError("sample_every_frames must be at least 1")
    if max_samples == 0:
        raise ProcessMemoryAnalysisError("max_samples must be at least 1")
    if metadata["samples_saturated"] not in ("0", "1"):
        raise ProcessMemoryAnalysisError("samples_saturated must be 0 or 1")
    saturated = metadata["samples_saturated"] == "1"
    if not rows:
        raise ProcessMemoryAnalysisError("process memory CSV contains no measured samples")
    if len(rows) > max_samples:
        raise ProcessMemoryAnalysisError("process memory CSV contains more rows than max_samples")
    if saturated and len(rows) != max_samples:
        raise ProcessMemoryAnalysisError("saturated capture must contain exactly max_samples rows")

    expected_frame = warmup
    previous_peak: int | None = None
    previous_faults: int | None = None
    for frame, working, peak, _private, faults in rows:
        if frame != expected_frame:
            raise ProcessMemoryAnalysisError("sampled frame indices must begin at warmup and advance by sample_every_frames")
        expected_frame += stride
        if peak < working:
            raise ProcessMemoryAnalysisError("peak working set must be at least current working set")
        if previous_peak is not None and peak < previous_peak:
            raise ProcessMemoryAnalysisError("peak working set must not decrease within one process capture")
        if previous_faults is not None and faults < previous_faults:
            raise ProcessMemoryAnalysisError("page fault count must not decrease within one process capture")
        previous_peak = peak
        previous_faults = faults

    frames = [row[0] for row in rows]
    working = [row[1] for row in rows]
    peak = [row[2] for row in rows]
    private = [row[3] for row in rows]
    faults = [row[4] for row in rows]
    return {
        "schema_version": SCHEMA_VERSION,
        "source_metadata": {
            **{key: metadata[key] for key in sorted(FIXED_METADATA)},
            "sample_source": metadata["sample_source"],
            "warmup_frames": warmup,
            "sample_every_frames": stride,
            "max_samples": max_samples,
            "samples_saturated": saturated,
        },
        "sample_count": len(rows),
        "sample_count_reached_limit": len(rows) == max_samples,
        "start_frame_index": frames[0],
        "end_frame_index": frames[-1],
        "first_last_window_fraction": 0.10,
        "statistics": {
            "working_set_bytes": _metric_stats(frames, working),
            "peak_working_set_bytes": _metric_stats(frames, peak),
            "private_usage_bytes": _metric_stats(frames, private),
            "page_fault_count": _metric_stats(frames, faults, cumulative=True),
        },
    }


def _load_json(path: Path, label: str, max_bytes: int = MAX_ANALYSIS_BYTES) -> dict[str, Any]:
    try:
        if path.stat().st_size > max_bytes:
            raise ProcessMemoryAnalysisError(f"{label} byte limit exceeded")
        value = json.loads(path.read_text(encoding="utf-8"))
    except ProcessMemoryAnalysisError:
        raise
    except Exception as exc:
        raise ProcessMemoryAnalysisError(f"cannot read {label}: {exc}") from exc
    if not isinstance(value, dict):
        raise ProcessMemoryAnalysisError(f"{label} root must be an object")
    return value


def analyze_bound_capture(
    benchmark: dict[str, Any],
    package_root: Path,
    release_manifest_path: Path,
    evidence_root: Path,
    *,
    expected_commit: str,
    expected_executable_sha256: str,
    memory_role: str = "process_memory_csv",
) -> dict[str, Any]:
    if benchmark_manifest is None:
        raise ProcessMemoryAnalysisError("benchmark_manifest module is unavailable")
    if not COMMIT_RE.fullmatch(expected_commit):
        raise ProcessMemoryAnalysisError("expected_commit must be 40 hex characters")
    if not SHA256_RE.fullmatch(expected_executable_sha256):
        raise ProcessMemoryAnalysisError("expected_executable_sha256 must be 64 hex characters")
    if not ROLE_RE.fullmatch(memory_role):
        raise ProcessMemoryAnalysisError("memory_role is invalid")

    try:
        verified = benchmark_manifest.verify_benchmark_manifest(
            benchmark, package_root, release_manifest_path, evidence_root
        )
    except Exception as exc:
        error_type = getattr(benchmark_manifest, "BenchmarkManifestError", ValueError)
        if isinstance(exc, (error_type, OSError, ValueError)):
            raise ProcessMemoryAnalysisError(f"benchmark manifest verification failed: {exc}") from exc
        raise

    candidate = benchmark.get("candidate")
    if not isinstance(candidate, dict):
        raise ProcessMemoryAnalysisError("benchmark candidate is malformed")
    commit = candidate.get("commit")
    executable_sha = candidate.get("executable_sha256")
    if not isinstance(commit, str) or commit.lower() != expected_commit.lower():
        raise ProcessMemoryAnalysisError("benchmark candidate commit does not match expected_commit")
    if not isinstance(executable_sha, str) or executable_sha.lower() != expected_executable_sha256.lower():
        raise ProcessMemoryAnalysisError("benchmark executable hash does not match expected_executable_sha256")

    evidence = benchmark.get("evidence")
    if not isinstance(evidence, list):
        raise ProcessMemoryAnalysisError("benchmark evidence must be an array")
    matches = [item for item in evidence if isinstance(item, dict) and item.get("role") == memory_role]
    if len(matches) != 1:
        raise ProcessMemoryAnalysisError(f"benchmark must contain exactly one {memory_role!r} evidence entry")
    item = matches[0]
    rel = _safe_rel(item.get("path"))
    memory_path = _resolve_evidence(evidence_root, rel)
    actual_size, actual_sha = memory_path.stat().st_size, _sha256(memory_path)
    if item.get("bytes") != actual_size or not isinstance(item.get("sha256"), str) or item["sha256"].lower() != actual_sha:
        raise ProcessMemoryAnalysisError("memory evidence bytes/hash do not match benchmark manifest")

    parsed = parse_process_memory_csv(memory_path)
    descriptor = verified.get("benchmark_descriptor_sha256")
    if not isinstance(descriptor, str) or not SHA256_RE.fullmatch(descriptor):
        raise ProcessMemoryAnalysisError("verified benchmark descriptor hash is malformed")
    workload = benchmark.get("workload")
    protocol = benchmark.get("run_protocol")
    provenance = benchmark.get("provenance")
    if not isinstance(workload, dict) or not isinstance(protocol, dict) or not isinstance(provenance, dict):
        raise ProcessMemoryAnalysisError("benchmark workload/protocol/provenance is malformed")

    return {
        "schema_version": SCHEMA_VERSION,
        "analysis_kind": "astral_process_memory_descriptive",
        "generated_at_utc": datetime.now(timezone.utc).isoformat(timespec="seconds").replace("+00:00", "Z"),
        "benchmark_descriptor_sha256": descriptor.lower(),
        "candidate_commit": expected_commit.lower(),
        "executable_sha256": expected_executable_sha256.lower(),
        "workload": json.loads(json.dumps(workload)),
        "run_protocol": json.loads(json.dumps(protocol)),
        "provenance": json.loads(json.dumps(provenance)),
        "source_csv": {
            "path": rel,
            "role": memory_role,
            "bytes": actual_size,
            "sha256": actual_sha,
        },
        "capture": parsed,
        "acceptance": dict(ACCEPTANCE),
        "limitations": list(LIMITATIONS),
    }


def _write_new_json(path: Path, value: dict[str, Any]) -> None:
    parent = path.parent
    if not parent.exists() or not parent.is_dir():
        raise ProcessMemoryAnalysisError("analysis output parent must already exist and be a directory")
    if path.exists() or path.is_symlink():
        raise ProcessMemoryAnalysisError("analysis output already exists; refusing to overwrite")
    payload = json.dumps(value, sort_keys=True, indent=2) + "\n"
    encoded = payload.encode("utf-8")
    if len(encoded) > MAX_ANALYSIS_BYTES:
        raise ProcessMemoryAnalysisError("analysis output byte limit exceeded")
    try:
        with path.open("x", encoding="utf-8", newline="\n") as stream:
            stream.write(payload)
    except FileExistsError as exc:
        raise ProcessMemoryAnalysisError("analysis output appeared concurrently; refusing to overwrite") from exc
    except OSError as exc:
        raise ProcessMemoryAnalysisError(f"cannot write analysis output: {exc}") from exc


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Analyze provenance-bound Astral process-memory capture evidence")
    parser.add_argument("--benchmark-manifest", required=True, type=Path)
    parser.add_argument("--package-root", required=True, type=Path)
    parser.add_argument("--release-manifest", required=True, type=Path)
    parser.add_argument("--evidence-root", required=True, type=Path)
    parser.add_argument("--expected-commit", required=True)
    parser.add_argument("--expected-executable-sha256", required=True)
    parser.add_argument("--memory-role", default="process_memory_csv")
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args(argv)
    try:
        benchmark = _load_json(args.benchmark_manifest, "benchmark manifest")
        result = analyze_bound_capture(
            benchmark,
            args.package_root,
            args.release_manifest,
            args.evidence_root,
            expected_commit=args.expected_commit,
            expected_executable_sha256=args.expected_executable_sha256,
            memory_role=args.memory_role,
        )
        _write_new_json(args.output, result)
    except (ProcessMemoryAnalysisError, OSError, ValueError) as exc:
        print(f"process-memory analysis failed: {exc}", file=sys.stderr)
        return 1
    print(f"process-memory analysis written: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
