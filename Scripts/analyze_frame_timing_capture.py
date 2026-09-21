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
FRAME_VALUE_RE = re.compile(r"^(?:0|[1-9][0-9]*)\.[0-9]{6}$")

FIXED_METADATA = {
    "astral_frame_timing_schema": "1",
    "metric": "cpu_frame_interval_ms",
    "units": "milliseconds",
    "measurement_semantics": "wall_clock_interval_between_consecutive_Clock_Tick_calls",
    "timing_source": "Engine/Core/Clock.cpp:std::chrono::steady_clock",
    "gpu_timing": "unavailable",
    "acceptance_claim": "none",
}
REQUIRED_METADATA = set(FIXED_METADATA) | {"warmup_frames", "max_samples", "samples_saturated"}
ACCEPTANCE = {
    "performance_budget_verified": False,
    "comparative_parity_verified": False,
    "instrumentation_overhead_verified": False,
    "gpu_timing_verified": False,
    "independent_acceptance": False,
}
LIMITATIONS = [
    "The source metric is the wall-clock interval between consecutive Clock::Tick calls; it includes waits and overhead and is not isolated CPU work time.",
    "This analysis does not provide GPU timing, render-thread/main-thread attribution, Present-wait attribution, or profiler trace events.",
    "Descriptive percentiles do not establish a performance budget, matched Unreal/Unity parity, instrumentation overhead, clean-machine compatibility, or independent acceptance.",
    "Benchmark environment fields are operator-supplied labels whose truth still depends on retained machine receipts and independent review.",
]


class FrameTimingAnalysisError(ValueError):
    pass


def _sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def _safe_rel(value: Any) -> str:
    if not isinstance(value, str) or not value or "\x00" in value or "\\" in value:
        raise FrameTimingAnalysisError("timing evidence path must be a non-empty forward-slash relative path")
    if ":" in value.split("/", 1)[0]:
        raise FrameTimingAnalysisError("timing evidence path must not be drive-qualified")
    path = PurePosixPath(value)
    if path.is_absolute() or any(part in ("", ".", "..") for part in path.parts) or path.as_posix() != value:
        raise FrameTimingAnalysisError(f"unsafe timing evidence path: {value!r}")
    return value


def _resolve_evidence(root: Path, rel: str) -> Path:
    if root.is_symlink():
        raise FrameTimingAnalysisError("evidence root must not be a symlink")
    try:
        resolved_root = root.resolve(strict=True)
    except OSError as exc:
        raise FrameTimingAnalysisError(f"cannot resolve evidence root: {exc}") from exc
    if not resolved_root.is_dir():
        raise FrameTimingAnalysisError("evidence root must be a directory")
    current = resolved_root
    for part in PurePosixPath(rel).parts:
        current = current / part
        if current.is_symlink():
            raise FrameTimingAnalysisError(f"symlinked timing evidence path component: {rel}")
    try:
        resolved = current.resolve(strict=True)
        if os.path.commonpath((str(resolved_root), str(resolved))) != str(resolved_root):
            raise FrameTimingAnalysisError("timing evidence escapes evidence root")
    except (OSError, ValueError) as exc:
        if isinstance(exc, FrameTimingAnalysisError):
            raise
        raise FrameTimingAnalysisError(f"cannot resolve timing evidence: {exc}") from exc
    if not resolved.is_file():
        raise FrameTimingAnalysisError("timing evidence must be a regular file")
    return resolved


def _parse_uint(text: str, label: str, maximum: int | None = None) -> int:
    if not text or not text.isascii() or not text.isdecimal():
        raise FrameTimingAnalysisError(f"{label} must be an unsigned decimal integer")
    value = int(text, 10)
    if maximum is not None and value > maximum:
        raise FrameTimingAnalysisError(f"{label} exceeds {maximum}")
    return value


def _percentile(sorted_values: list[float], probability: float) -> float:
    if not sorted_values:
        raise FrameTimingAnalysisError("cannot calculate a percentile of an empty sample")
    if not 0.0 <= probability <= 1.0:
        raise FrameTimingAnalysisError("percentile probability must be in [0, 1]")
    position = (len(sorted_values) - 1) * probability
    lower = int(math.floor(position))
    upper = int(math.ceil(position))
    if lower == upper:
        return sorted_values[lower]
    weight = position - lower
    return sorted_values[lower] * (1.0 - weight) + sorted_values[upper] * weight


def parse_frame_timing_csv(path: Path) -> dict[str, Any]:
    try:
        size = path.stat().st_size
    except OSError as exc:
        raise FrameTimingAnalysisError(f"cannot stat frame timing CSV: {exc}") from exc
    if size <= 0:
        raise FrameTimingAnalysisError("frame timing CSV is empty")
    if size > MAX_CSV_BYTES:
        raise FrameTimingAnalysisError("frame timing CSV byte limit exceeded")

    metadata: dict[str, str] = {}
    rows: list[tuple[int, float]] = []
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
                            raise FrameTimingAnalysisError(f"line {line_number}: malformed metadata")
                        key, value = body.split("=", 1)
                        if key not in REQUIRED_METADATA:
                            raise FrameTimingAnalysisError(f"line {line_number}: unexpected metadata key {key!r}")
                        if key in metadata:
                            raise FrameTimingAnalysisError(f"line {line_number}: duplicate metadata key {key!r}")
                        metadata[key] = value
                        continue
                    if raw != "frame_index,cpu_frame_interval_ms":
                        raise FrameTimingAnalysisError(f"line {line_number}: expected exact CSV header")
                    header_seen = True
                    continue

                if not raw:
                    raise FrameTimingAnalysisError(f"line {line_number}: blank data row")
                fields = next(csv.reader([raw], strict=True))
                if len(fields) != 2:
                    raise FrameTimingAnalysisError(f"line {line_number}: expected two CSV columns")
                frame_index = _parse_uint(fields[0], f"line {line_number} frame_index")
                if not FRAME_VALUE_RE.fullmatch(fields[1]):
                    raise FrameTimingAnalysisError(f"line {line_number}: frame interval must use canonical fixed six-decimal format")
                try:
                    interval = float(fields[1])
                except ValueError as exc:
                    raise FrameTimingAnalysisError(f"line {line_number}: invalid frame interval") from exc
                if not math.isfinite(interval) or interval <= 0.0:
                    raise FrameTimingAnalysisError(f"line {line_number}: frame interval must be finite and positive")
                rows.append((frame_index, interval))
                if len(rows) > MAX_SAMPLES:
                    raise FrameTimingAnalysisError("frame timing CSV exceeds hard sample limit")
    except FrameTimingAnalysisError:
        raise
    except (OSError, UnicodeError, csv.Error) as exc:
        raise FrameTimingAnalysisError(f"cannot parse frame timing CSV: {exc}") from exc

    if not header_seen:
        raise FrameTimingAnalysisError("frame timing CSV is missing its header")
    missing = REQUIRED_METADATA - set(metadata)
    if missing:
        raise FrameTimingAnalysisError(f"missing frame timing metadata: {sorted(missing)}")
    for key, expected in FIXED_METADATA.items():
        if metadata[key] != expected:
            raise FrameTimingAnalysisError(f"frame timing metadata {key} must equal {expected!r}")

    warmup = _parse_uint(metadata["warmup_frames"], "warmup_frames")
    max_samples = _parse_uint(metadata["max_samples"], "max_samples", MAX_SAMPLES)
    if max_samples == 0:
        raise FrameTimingAnalysisError("max_samples must be at least 1")
    if metadata["samples_saturated"] not in ("0", "1"):
        raise FrameTimingAnalysisError("samples_saturated must be 0 or 1")
    saturated = metadata["samples_saturated"] == "1"
    if not rows:
        raise FrameTimingAnalysisError("frame timing CSV contains no measured samples")
    if len(rows) > max_samples:
        raise FrameTimingAnalysisError("frame timing CSV contains more rows than max_samples")
    if saturated and len(rows) != max_samples:
        raise FrameTimingAnalysisError("saturated capture must contain exactly max_samples rows")

    previous: int | None = None
    for frame_index, _ in rows:
        if frame_index < warmup:
            raise FrameTimingAnalysisError("measured frame index is inside declared warmup")
        if previous is not None and frame_index != previous + 1:
            raise FrameTimingAnalysisError("measured frame indices must be strictly contiguous")
        previous = frame_index

    values = [value for _, value in rows]
    ordered = sorted(values)
    total = math.fsum(values)
    return {
        "schema_version": SCHEMA_VERSION,
        "source_metadata": {
            **{key: metadata[key] for key in sorted(FIXED_METADATA)},
            "warmup_frames": warmup,
            "max_samples": max_samples,
            "samples_saturated": saturated,
        },
        "sample_count": len(values),
        "sample_count_reached_limit": len(values) == max_samples,
        "start_frame_index": rows[0][0],
        "end_frame_index": rows[-1][0],
        "captured_interval_sum_ms": total,
        "statistics_ms": {
            "min": min(values),
            "mean": total / len(values),
            "p50": _percentile(ordered, 0.50),
            "p95": _percentile(ordered, 0.95),
            "p99": _percentile(ordered, 0.99),
            "max": max(values),
            "percentile_method": "linear_interpolation_position=(n-1)*p",
        },
    }


def _load_json(path: Path, label: str, max_bytes: int = MAX_ANALYSIS_BYTES) -> dict[str, Any]:
    try:
        if path.stat().st_size > max_bytes:
            raise FrameTimingAnalysisError(f"{label} byte limit exceeded")
        value = json.loads(path.read_text(encoding="utf-8"))
    except FrameTimingAnalysisError:
        raise
    except Exception as exc:
        raise FrameTimingAnalysisError(f"cannot read {label}: {exc}") from exc
    if not isinstance(value, dict):
        raise FrameTimingAnalysisError(f"{label} root must be an object")
    return value


def analyze_bound_capture(
    benchmark: dict[str, Any],
    package_root: Path,
    release_manifest_path: Path,
    evidence_root: Path,
    *,
    expected_commit: str,
    expected_executable_sha256: str,
    timing_role: str = "cpu_frame_timing_csv",
) -> dict[str, Any]:
    if benchmark_manifest is None:
        raise FrameTimingAnalysisError("benchmark_manifest module is unavailable")
    if not COMMIT_RE.fullmatch(expected_commit):
        raise FrameTimingAnalysisError("expected_commit must be 40 hex characters")
    if not SHA256_RE.fullmatch(expected_executable_sha256):
        raise FrameTimingAnalysisError("expected_executable_sha256 must be 64 hex characters")
    if not ROLE_RE.fullmatch(timing_role):
        raise FrameTimingAnalysisError("timing_role is invalid")

    try:
        verified = benchmark_manifest.verify_benchmark_manifest(
            benchmark, package_root, release_manifest_path, evidence_root
        )
    except Exception as exc:
        error_type = getattr(benchmark_manifest, "BenchmarkManifestError", ValueError)
        if isinstance(exc, (error_type, OSError, ValueError)):
            raise FrameTimingAnalysisError(f"benchmark manifest verification failed: {exc}") from exc
        raise

    candidate = benchmark.get("candidate")
    if not isinstance(candidate, dict):
        raise FrameTimingAnalysisError("benchmark candidate is malformed")
    commit = candidate.get("commit")
    executable_sha = candidate.get("executable_sha256")
    if not isinstance(commit, str) or commit.lower() != expected_commit.lower():
        raise FrameTimingAnalysisError("benchmark candidate commit does not match expected_commit")
    if not isinstance(executable_sha, str) or executable_sha.lower() != expected_executable_sha256.lower():
        raise FrameTimingAnalysisError("benchmark executable hash does not match expected_executable_sha256")

    evidence = benchmark.get("evidence")
    if not isinstance(evidence, list):
        raise FrameTimingAnalysisError("benchmark evidence must be an array")
    matches = [item for item in evidence if isinstance(item, dict) and item.get("role") == timing_role]
    if len(matches) != 1:
        raise FrameTimingAnalysisError(f"benchmark must contain exactly one {timing_role!r} evidence entry")
    item = matches[0]
    rel = _safe_rel(item.get("path"))
    timing_path = _resolve_evidence(evidence_root, rel)
    actual_size, actual_sha = timing_path.stat().st_size, _sha256(timing_path)
    if item.get("bytes") != actual_size or not isinstance(item.get("sha256"), str) or item["sha256"].lower() != actual_sha:
        raise FrameTimingAnalysisError("timing evidence bytes/hash do not match benchmark manifest")

    parsed = parse_frame_timing_csv(timing_path)
    descriptor = verified.get("benchmark_descriptor_sha256")
    if not isinstance(descriptor, str) or not SHA256_RE.fullmatch(descriptor):
        raise FrameTimingAnalysisError("verified benchmark descriptor hash is malformed")
    workload = benchmark.get("workload")
    protocol = benchmark.get("run_protocol")
    provenance = benchmark.get("provenance")
    if not isinstance(workload, dict) or not isinstance(protocol, dict) or not isinstance(provenance, dict):
        raise FrameTimingAnalysisError("benchmark workload/protocol/provenance is malformed")

    return {
        "schema_version": SCHEMA_VERSION,
        "analysis_kind": "astral_cpu_frame_timing_descriptive",
        "generated_at_utc": datetime.now(timezone.utc).isoformat(timespec="seconds").replace("+00:00", "Z"),
        "benchmark_descriptor_sha256": descriptor.lower(),
        "candidate_commit": expected_commit.lower(),
        "executable_sha256": expected_executable_sha256.lower(),
        "workload": json.loads(json.dumps(workload)),
        "run_protocol": json.loads(json.dumps(protocol)),
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
        raise FrameTimingAnalysisError("analysis output parent must already exist and be a directory")
    try:
        with path.open("x", encoding="utf-8", newline="\n") as stream:
            json.dump(value, stream, indent=2, sort_keys=True)
            stream.write("\n")
    except FileExistsError as exc:
        raise FrameTimingAnalysisError("analysis output already exists; refusing to overwrite") from exc
    except OSError as exc:
        raise FrameTimingAnalysisError(f"cannot write analysis output: {exc}") from exc


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Verify and descriptively analyze one package-bound Astral CPU frame timing CSV")
    parser.add_argument("benchmark_manifest", type=Path)
    parser.add_argument("package_root", type=Path)
    parser.add_argument("release_manifest", type=Path)
    parser.add_argument("evidence_root", type=Path)
    parser.add_argument("--expected-commit", required=True)
    parser.add_argument("--expected-executable-sha256", required=True)
    parser.add_argument("--timing-role", default="cpu_frame_timing_csv")
    parser.add_argument("--json", type=Path)
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
        if args.json is not None:
            _write_new_json(args.json, result)
    except FrameTimingAnalysisError as exc:
        print(f"Frame timing analysis failed: {exc}", file=sys.stderr)
        return 1
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
