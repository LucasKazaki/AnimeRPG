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
except ImportError:  # Portable parser/unit fixtures may intentionally omit the repository module.
    benchmark_manifest = None  # type: ignore[assignment]

SCHEMA_VERSION = 1
MAX_CSV_BYTES = 128 * 1024 * 1024
MAX_ANALYSIS_BYTES = 4 * 1024 * 1024
MAX_SAMPLES = 1_000_000
COMMIT_RE = re.compile(r"^[0-9a-fA-F]{40}$")
SHA256_RE = re.compile(r"^[0-9a-fA-F]{64}$")
ROLE_RE = re.compile(r"^[a-z0-9_.-]{1,64}$")
FIXED_MS_RE = re.compile(r"^(?:0|[1-9][0-9]*)\.[0-9]{6}$")
ROW_TOTAL_ROUNDING_TOLERANCE_US = 3

PHASE_COLUMNS = (
    "message_pump_ms",
    "update_control_ms",
    "render_submit_ms",
    "frame_wait_ms",
)
ALL_TIMING_COLUMNS = PHASE_COLUMNS + ("loop_total_ms",)
CSV_HEADER = "frame_index," + ",".join(ALL_TIMING_COLUMNS)

FIXED_METADATA = {
    "astral_frame_phase_timing_schema": "1",
    "metric": "main_thread_phase_wall_ms",
    "units": "milliseconds",
    "measurement_semantics": "wall_clock_intervals_for_contiguous_Win32_main_loop_phases",
    "timing_source": "std::chrono::steady_clock",
    "phase_order": "message_pump,update_control,render_submit,frame_wait",
    "cpu_scope": "Win32_main_thread_only",
    "gpu_timing": "unavailable",
    "acceptance_claim": "none",
}
REQUIRED_METADATA = set(FIXED_METADATA) | {"warmup_frames", "max_samples", "samples_saturated"}
ACCEPTANCE = {
    "performance_budget_verified": False,
    "comparative_parity_verified": False,
    "instrumentation_overhead_verified": False,
    "gpu_timing_verified": False,
    "worker_or_render_thread_attribution_verified": False,
    "independent_acceptance": False,
}
LIMITATIONS = [
    "The source contains flat wall-clock intervals from Astral's Win32 main thread only; it is not a hierarchical CPU trace.",
    "render_submit_ms covers GDI submission work on the main thread and is not GPU execution time.",
    "The four named phases do not attribute worker threads, render threads, driver work, Present wait, context switches, or asynchronous GPU execution.",
    "Descriptive phase percentiles and aggregate shares do not establish a performance budget, instrumentation overhead, matched Unreal/Unity parity, clean-machine compatibility, or independent acceptance.",
    "Benchmark environment fields are operator-supplied labels whose truth still depends on retained machine receipts and independent review.",
]


class FramePhaseTimingAnalysisError(ValueError):
    pass


def _sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def _safe_rel(value: Any) -> str:
    if not isinstance(value, str) or not value or "\x00" in value or "\\" in value:
        raise FramePhaseTimingAnalysisError(
            "phase timing evidence path must be a non-empty forward-slash relative path"
        )
    if ":" in value.split("/", 1)[0]:
        raise FramePhaseTimingAnalysisError("phase timing evidence path must not be drive-qualified")
    path = PurePosixPath(value)
    if path.is_absolute() or any(part in ("", ".", "..") for part in path.parts) or path.as_posix() != value:
        raise FramePhaseTimingAnalysisError(f"unsafe phase timing evidence path: {value!r}")
    return value


def _resolve_evidence(root: Path, rel: str) -> Path:
    if root.is_symlink():
        raise FramePhaseTimingAnalysisError("evidence root must not be a symlink")
    try:
        resolved_root = root.resolve(strict=True)
    except OSError as exc:
        raise FramePhaseTimingAnalysisError(f"cannot resolve evidence root: {exc}") from exc
    if not resolved_root.is_dir():
        raise FramePhaseTimingAnalysisError("evidence root must be a directory")
    current = resolved_root
    for part in PurePosixPath(rel).parts:
        current = current / part
        if current.is_symlink():
            raise FramePhaseTimingAnalysisError(f"symlinked phase timing evidence path component: {rel}")
    try:
        resolved = current.resolve(strict=True)
        if os.path.commonpath((str(resolved_root), str(resolved))) != str(resolved_root):
            raise FramePhaseTimingAnalysisError("phase timing evidence escapes evidence root")
    except (OSError, ValueError) as exc:
        if isinstance(exc, FramePhaseTimingAnalysisError):
            raise
        raise FramePhaseTimingAnalysisError(f"cannot resolve phase timing evidence: {exc}") from exc
    if not resolved.is_file():
        raise FramePhaseTimingAnalysisError("phase timing evidence must be a regular file")
    return resolved


def _parse_uint(text: str, label: str, maximum: int | None = None) -> int:
    if not text or not text.isascii() or not text.isdecimal():
        raise FramePhaseTimingAnalysisError(f"{label} must be an unsigned decimal integer")
    value = int(text, 10)
    if maximum is not None and value > maximum:
        raise FramePhaseTimingAnalysisError(f"{label} exceeds {maximum}")
    return value


def _parse_fixed_ms(text: str, label: str) -> int:
    if not FIXED_MS_RE.fullmatch(text):
        raise FramePhaseTimingAnalysisError(
            f"{label} must use canonical non-negative fixed six-decimal millisecond format"
        )
    whole, fractional = text.split(".", 1)
    return int(whole, 10) * 1_000_000 + int(fractional, 10)


def _percentile(sorted_values: list[float], probability: float) -> float:
    if not sorted_values:
        raise FramePhaseTimingAnalysisError("cannot calculate a percentile of an empty sample")
    if not 0.0 <= probability <= 1.0:
        raise FramePhaseTimingAnalysisError("percentile probability must be in [0, 1]")
    position = (len(sorted_values) - 1) * probability
    lower = int(math.floor(position))
    upper = int(math.ceil(position))
    if lower == upper:
        return sorted_values[lower]
    weight = position - lower
    return sorted_values[lower] * (1.0 - weight) + sorted_values[upper] * weight


def _statistics_ms(values_us: list[int]) -> dict[str, Any]:
    if not values_us:
        raise FramePhaseTimingAnalysisError("cannot summarize an empty phase")
    values_ms = [value / 1_000_000.0 for value in values_us]
    ordered = sorted(values_ms)
    total_us = sum(values_us)
    return {
        "min": min(values_ms),
        "mean": (total_us / len(values_us)) / 1_000_000.0,
        "p50": _percentile(ordered, 0.50),
        "p95": _percentile(ordered, 0.95),
        "p99": _percentile(ordered, 0.99),
        "max": max(values_ms),
        "sum": total_us / 1_000_000.0,
        "percentile_method": "linear_interpolation_position=(n-1)*p",
    }


def parse_frame_phase_timing_csv(path: Path) -> dict[str, Any]:
    try:
        size = path.stat().st_size
    except OSError as exc:
        raise FramePhaseTimingAnalysisError(f"cannot stat frame phase timing CSV: {exc}") from exc
    if size <= 0:
        raise FramePhaseTimingAnalysisError("frame phase timing CSV is empty")
    if size > MAX_CSV_BYTES:
        raise FramePhaseTimingAnalysisError("frame phase timing CSV byte limit exceeded")

    metadata: dict[str, str] = {}
    rows: list[tuple[int, tuple[int, int, int, int, int]]] = []
    header_seen = False
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
                            raise FramePhaseTimingAnalysisError(f"line {line_number}: malformed metadata")
                        key, value = body.split("=", 1)
                        if key not in REQUIRED_METADATA:
                            raise FramePhaseTimingAnalysisError(
                                f"line {line_number}: unexpected metadata key {key!r}"
                            )
                        if key in metadata:
                            raise FramePhaseTimingAnalysisError(
                                f"line {line_number}: duplicate metadata key {key!r}"
                            )
                        metadata[key] = value
                        continue
                    if raw != CSV_HEADER:
                        raise FramePhaseTimingAnalysisError(f"line {line_number}: expected exact CSV header")
                    header_seen = True
                    continue

                if not raw:
                    raise FramePhaseTimingAnalysisError(f"line {line_number}: blank data row")
                fields = next(csv.reader([raw], strict=True))
                if len(fields) != 6:
                    raise FramePhaseTimingAnalysisError(f"line {line_number}: expected six CSV columns")
                frame_index = _parse_uint(fields[0], f"line {line_number} frame_index")
                timings_us = tuple(
                    _parse_fixed_ms(fields[index], f"line {line_number} {ALL_TIMING_COLUMNS[index - 1]}")
                    for index in range(1, 6)
                )
                phase_sum_us = sum(timings_us[:4])
                total_us = timings_us[4]
                if total_us <= 0:
                    raise FramePhaseTimingAnalysisError(
                        f"line {line_number}: loop_total_ms must be positive"
                    )
                if abs(phase_sum_us - total_us) > ROW_TOTAL_ROUNDING_TOLERANCE_US:
                    raise FramePhaseTimingAnalysisError(
                        f"line {line_number}: loop_total_ms differs from serialized phase sum by more than "
                        f"{ROW_TOTAL_ROUNDING_TOLERANCE_US} microseconds"
                    )
                rows.append((frame_index, timings_us))
                if len(rows) > MAX_SAMPLES:
                    raise FramePhaseTimingAnalysisError("frame phase timing CSV exceeds hard sample limit")
    except FramePhaseTimingAnalysisError:
        raise
    except (OSError, UnicodeError, csv.Error) as exc:
        raise FramePhaseTimingAnalysisError(f"cannot parse frame phase timing CSV: {exc}") from exc

    if not header_seen:
        raise FramePhaseTimingAnalysisError("frame phase timing CSV is missing its header")
    missing = REQUIRED_METADATA - set(metadata)
    if missing:
        raise FramePhaseTimingAnalysisError(f"missing frame phase timing metadata: {sorted(missing)}")
    for key, expected in FIXED_METADATA.items():
        if metadata[key] != expected:
            raise FramePhaseTimingAnalysisError(
                f"frame phase timing metadata {key} must equal {expected!r}"
            )

    warmup = _parse_uint(metadata["warmup_frames"], "warmup_frames")
    max_samples = _parse_uint(metadata["max_samples"], "max_samples", MAX_SAMPLES)
    if max_samples == 0:
        raise FramePhaseTimingAnalysisError("max_samples must be at least 1")
    if metadata["samples_saturated"] not in ("0", "1"):
        raise FramePhaseTimingAnalysisError("samples_saturated must be 0 or 1")
    saturated = metadata["samples_saturated"] == "1"
    if not rows:
        raise FramePhaseTimingAnalysisError("frame phase timing CSV contains no measured samples")
    if len(rows) > max_samples:
        raise FramePhaseTimingAnalysisError("frame phase timing CSV contains more rows than max_samples")
    if saturated and len(rows) != max_samples:
        raise FramePhaseTimingAnalysisError("saturated capture must contain exactly max_samples rows")

    previous: int | None = None
    for frame_index, _ in rows:
        if frame_index < warmup:
            raise FramePhaseTimingAnalysisError("measured frame index is inside declared warmup")
        if previous is not None and frame_index != previous + 1:
            raise FramePhaseTimingAnalysisError("measured frame indices must be strictly contiguous")
        previous = frame_index

    column_values: dict[str, list[int]] = {name: [] for name in ALL_TIMING_COLUMNS}
    errors_us: list[int] = []
    for _, timings_us in rows:
        for name, value in zip(ALL_TIMING_COLUMNS, timings_us):
            column_values[name].append(value)
        errors_us.append(sum(timings_us[:4]) - timings_us[4])

    stats = {name: _statistics_ms(values) for name, values in column_values.items()}
    aggregate_loop_us = sum(column_values["loop_total_ms"])
    shares = {
        name: sum(column_values[name]) / aggregate_loop_us
        for name in PHASE_COLUMNS
    }
    return {
        "schema_version": SCHEMA_VERSION,
        "source_metadata": {
            **{key: metadata[key] for key in sorted(FIXED_METADATA)},
            "warmup_frames": warmup,
            "max_samples": max_samples,
            "samples_saturated": saturated,
        },
        "sample_count": len(rows),
        "sample_count_reached_limit": len(rows) == max_samples,
        "start_frame_index": rows[0][0],
        "end_frame_index": rows[-1][0],
        "statistics_ms": stats,
        "aggregate_phase_share_of_loop_total": shares,
        "row_total_consistency": {
            "rounding_tolerance_microseconds": ROW_TOTAL_ROUNDING_TOLERANCE_US,
            "max_absolute_serialized_error_microseconds": max(abs(value) for value in errors_us),
            "rows_with_nonzero_serialized_error": sum(value != 0 for value in errors_us),
        },
    }


def _load_json(path: Path, label: str, max_bytes: int = MAX_ANALYSIS_BYTES) -> dict[str, Any]:
    try:
        if path.stat().st_size > max_bytes:
            raise FramePhaseTimingAnalysisError(f"{label} byte limit exceeded")
        value = json.loads(path.read_text(encoding="utf-8"))
    except FramePhaseTimingAnalysisError:
        raise
    except Exception as exc:
        raise FramePhaseTimingAnalysisError(f"cannot read {label}: {exc}") from exc
    if not isinstance(value, dict):
        raise FramePhaseTimingAnalysisError(f"{label} root must be an object")
    return value


def analyze_bound_capture(
    benchmark: dict[str, Any],
    package_root: Path,
    release_manifest_path: Path,
    evidence_root: Path,
    *,
    expected_commit: str,
    expected_executable_sha256: str,
    timing_role: str = "cpu_phase_timing_csv",
) -> dict[str, Any]:
    if benchmark_manifest is None:
        raise FramePhaseTimingAnalysisError("benchmark_manifest module is unavailable")
    if not COMMIT_RE.fullmatch(expected_commit):
        raise FramePhaseTimingAnalysisError("expected_commit must be 40 hex characters")
    if not SHA256_RE.fullmatch(expected_executable_sha256):
        raise FramePhaseTimingAnalysisError("expected_executable_sha256 must be 64 hex characters")
    if not ROLE_RE.fullmatch(timing_role):
        raise FramePhaseTimingAnalysisError("timing_role is invalid")

    try:
        verify = getattr(benchmark_manifest, "verify_benchmark_manifest", None)
        if verify is None:
            verify = getattr(benchmark_manifest, "verify_manifest", None)
        if verify is None:
            raise FramePhaseTimingAnalysisError("benchmark_manifest verifier is unavailable")
        verified = verify(benchmark, package_root, release_manifest_path, evidence_root)
    except FramePhaseTimingAnalysisError:
        raise
    except Exception as exc:
        error_type = getattr(benchmark_manifest, "BenchmarkManifestError", ValueError)
        if isinstance(exc, (error_type, OSError, ValueError)):
            raise FramePhaseTimingAnalysisError(f"benchmark manifest verification failed: {exc}") from exc
        raise

    candidate = benchmark.get("candidate")
    if not isinstance(candidate, dict):
        raise FramePhaseTimingAnalysisError("benchmark candidate is malformed")
    commit = candidate.get("commit")
    executable_sha = candidate.get("executable_sha256")
    if not isinstance(commit, str) or commit.lower() != expected_commit.lower():
        raise FramePhaseTimingAnalysisError("benchmark candidate commit does not match expected_commit")
    if not isinstance(executable_sha, str) or executable_sha.lower() != expected_executable_sha256.lower():
        raise FramePhaseTimingAnalysisError(
            "benchmark executable hash does not match expected_executable_sha256"
        )

    evidence = benchmark.get("evidence")
    if not isinstance(evidence, list):
        raise FramePhaseTimingAnalysisError("benchmark evidence must be an array")
    matches = [item for item in evidence if isinstance(item, dict) and item.get("role") == timing_role]
    if len(matches) != 1:
        raise FramePhaseTimingAnalysisError(
            f"benchmark must contain exactly one {timing_role!r} evidence entry"
        )
    item = matches[0]
    rel = _safe_rel(item.get("path"))
    timing_path = _resolve_evidence(evidence_root, rel)
    actual_size, actual_sha = timing_path.stat().st_size, _sha256(timing_path)
    if (
        isinstance(item.get("bytes"), bool)
        or not isinstance(item.get("bytes"), int)
        or item.get("bytes") != actual_size
        or not isinstance(item.get("sha256"), str)
        or item["sha256"].lower() != actual_sha
    ):
        raise FramePhaseTimingAnalysisError(
            "phase timing evidence bytes/hash do not match benchmark manifest"
        )

    parsed = parse_frame_phase_timing_csv(timing_path)
    descriptor = verified.get("benchmark_descriptor_sha256") if isinstance(verified, dict) else None
    if not isinstance(descriptor, str) or not SHA256_RE.fullmatch(descriptor):
        descriptor = benchmark.get("benchmark_descriptor_sha256")
    if not isinstance(descriptor, str) or not SHA256_RE.fullmatch(descriptor):
        raise FramePhaseTimingAnalysisError("verified benchmark descriptor hash is malformed")

    workload = benchmark.get("workload")
    protocol = benchmark.get("run_protocol")
    environment = benchmark.get("environment")
    references = benchmark.get("reference_versions")
    provenance = benchmark.get("provenance")
    if not all(isinstance(value, dict) for value in (workload, protocol, environment, references, provenance)):
        raise FramePhaseTimingAnalysisError(
            "benchmark workload/protocol/environment/reference_versions/provenance is malformed"
        )

    return {
        "schema_version": SCHEMA_VERSION,
        "analysis_kind": "astral_main_thread_phase_timing_descriptive",
        "generated_at_utc": datetime.now(timezone.utc)
        .isoformat(timespec="seconds")
        .replace("+00:00", "Z"),
        "benchmark_descriptor_sha256": descriptor.lower(),
        "candidate_commit": expected_commit.lower(),
        "executable_sha256": expected_executable_sha256.lower(),
        "workload": json.loads(json.dumps(workload)),
        "run_protocol": json.loads(json.dumps(protocol)),
        "environment": json.loads(json.dumps(environment)),
        "reference_versions": json.loads(json.dumps(references)),
        "provenance": json.loads(json.dumps(provenance)),
        "source_csv": {
            "path": rel,
            "role": timing_role,
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
        raise FramePhaseTimingAnalysisError(
            "analysis output parent must already exist and be a directory"
        )
    rendered = (json.dumps(value, indent=2, sort_keys=True) + "\n").encode("utf-8")
    if len(rendered) > MAX_ANALYSIS_BYTES:
        raise FramePhaseTimingAnalysisError("analysis output byte limit exceeded")
    try:
        with path.open("xb") as stream:
            stream.write(rendered)
            stream.flush()
            os.fsync(stream.fileno())
    except FileExistsError as exc:
        raise FramePhaseTimingAnalysisError("analysis output already exists") from exc
    except OSError as exc:
        raise FramePhaseTimingAnalysisError(f"cannot write analysis output: {exc}") from exc


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Verify and analyze a benchmark-bound Astral main-thread phase timing CSV."
    )
    parser.add_argument("--benchmark-manifest", required=True, type=Path)
    parser.add_argument("--package-root", required=True, type=Path)
    parser.add_argument("--release-manifest", required=True, type=Path)
    parser.add_argument("--evidence-root", required=True, type=Path)
    parser.add_argument("--expected-commit", required=True)
    parser.add_argument("--expected-executable-sha256", required=True)
    parser.add_argument("--timing-role", default="cpu_phase_timing_csv")
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
            timing_role=args.timing_role,
        )
        _write_new_json(args.output, result)
    except FramePhaseTimingAnalysisError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
