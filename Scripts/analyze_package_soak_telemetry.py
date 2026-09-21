#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import re
import sys
from pathlib import Path
from statistics import median
from typing import Any

MAX_RECEIPT_BYTES = 8 * 1024 * 1024
MAX_TELEMETRY_BYTES = 128 * 1024 * 1024
MAX_SAMPLES = 100_000
REQUIRED_SOAK_SECONDS = 86_400.0
SHA256_RE = re.compile(r"^[0-9a-fA-F]{64}$")
COMMIT_RE = re.compile(r"^[0-9a-fA-F]{40}$")
METRICS = (
    "working_set_bytes",
    "peak_working_set_bytes",
    "private_usage_bytes",
    "pagefile_usage_bytes",
    "peak_pagefile_usage_bytes",
    "handle_count",
    "gdi_objects",
    "user_objects",
)


class SoakAnalysisError(RuntimeError):
    pass


def _sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def _require_regular_file(path: Path, label: str, max_bytes: int) -> Path:
    if path.is_symlink():
        raise SoakAnalysisError(f"{label} must not be a symlink")
    resolved = path.resolve(strict=True)
    if not resolved.is_file():
        raise SoakAnalysisError(f"{label} must be a regular file")
    try:
        size = resolved.stat().st_size
    except OSError as exc:
        raise SoakAnalysisError(f"cannot stat {label}: {exc}") from exc
    if size > max_bytes:
        raise SoakAnalysisError(f"{label} exceeds {max_bytes} byte analysis limit")
    return resolved


def _load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise SoakAnalysisError(f"cannot read soak receipt JSON: {exc}") from exc
    if not isinstance(value, dict):
        raise SoakAnalysisError("soak receipt must be a JSON object")
    return value


def _nonnegative_number(value: Any, label: str) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise SoakAnalysisError(f"{label} must be a finite nonnegative number")
    number = float(value)
    if not math.isfinite(number) or number < 0:
        raise SoakAnalysisError(f"{label} must be a finite nonnegative number")
    return number


def _nonnegative_int(value: Any, label: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value < 0:
        raise SoakAnalysisError(f"{label} must be a nonnegative integer")
    return int(value)


def _validate_receipt(receipt: dict[str, Any]) -> tuple[str, str, float, float, int, str]:
    if receipt.get("schema_version") != 1:
        raise SoakAnalysisError("unsupported soak receipt schema")
    commit = receipt.get("commit")
    exe_hash = receipt.get("AstralGame_sha256")
    if not isinstance(commit, str) or not COMMIT_RE.fullmatch(commit):
        raise SoakAnalysisError("soak receipt commit must be 40 hex characters")
    if not isinstance(exe_hash, str) or not SHA256_RE.fullmatch(exe_hash):
        raise SoakAnalysisError("soak receipt AstralGame SHA-256 must be 64 hex characters")
    requested = _nonnegative_number(receipt.get("requested_duration_seconds"), "requested_duration_seconds")
    elapsed = _nonnegative_number(receipt.get("elapsed_seconds"), "elapsed_seconds")
    telemetry_hash = receipt.get("telemetry_sha256")
    if not isinstance(telemetry_hash, str) or not SHA256_RE.fullmatch(telemetry_hash):
        raise SoakAnalysisError("soak receipt telemetry SHA-256 must be 64 hex characters")
    summary = receipt.get("telemetry_summary")
    if not isinstance(summary, dict):
        raise SoakAnalysisError("soak receipt telemetry_summary must be an object")
    sample_count = _nonnegative_int(summary.get("sample_count"), "telemetry_summary.sample_count")
    acceptance = receipt.get("acceptance")
    if not isinstance(acceptance, dict):
        raise SoakAnalysisError("soak receipt acceptance must be an object")
    forbidden_true = (
        "required_24h_soak_verified",
        "ram_budget_verified",
        "vram_budget_verified",
        "frame_time_budget_verified",
        "memory_leak_free_verified",
        "clean_machine_compatibility_verified",
        "owned_interactive_desktop_verified",
        "independent_acceptance",
    )
    for key in forbidden_true:
        if acceptance.get(key) is not False:
            raise SoakAnalysisError(f"soak receipt must keep {key}=false for this evidence tier")
    observed_24h = acceptance.get("required_24h_duration_observed")
    if not isinstance(observed_24h, bool):
        raise SoakAnalysisError("required_24h_duration_observed must be boolean")
    if observed_24h and (requested < REQUIRED_SOAK_SECONDS or elapsed < REQUIRED_SOAK_SECONDS):
        raise SoakAnalysisError("24-hour duration claim is inconsistent with requested/elapsed duration")
    return commit.lower(), exe_hash.lower(), requested, elapsed, sample_count, telemetry_hash.lower()


def _load_telemetry(path: Path) -> list[dict[str, Any]]:
    samples: list[dict[str, Any]] = []
    previous_elapsed: float | None = None
    try:
        with path.open("r", encoding="utf-8") as f:
            for line_number, raw in enumerate(f, 1):
                if line_number > MAX_SAMPLES:
                    raise SoakAnalysisError(f"telemetry exceeds {MAX_SAMPLES} samples")
                if not raw.strip():
                    raise SoakAnalysisError(f"blank telemetry line at {line_number}")
                try:
                    sample = json.loads(raw)
                except json.JSONDecodeError as exc:
                    raise SoakAnalysisError(f"invalid telemetry JSON at line {line_number}: {exc}") from exc
                if not isinstance(sample, dict):
                    raise SoakAnalysisError(f"telemetry line {line_number} must be a JSON object")
                timestamp = sample.get("timestamp")
                if not isinstance(timestamp, str) or not timestamp.strip():
                    raise SoakAnalysisError(f"telemetry line {line_number} has invalid timestamp")
                elapsed = _nonnegative_number(sample.get("elapsed_seconds"), f"line {line_number} elapsed_seconds")
                if previous_elapsed is not None and elapsed <= previous_elapsed:
                    raise SoakAnalysisError(f"telemetry elapsed_seconds must strictly increase at line {line_number}")
                previous_elapsed = elapsed
                normalized: dict[str, Any] = {"timestamp": timestamp, "elapsed_seconds": elapsed}
                for metric in METRICS:
                    normalized[metric] = _nonnegative_int(sample.get(metric), f"line {line_number} {metric}")
                samples.append(normalized)
    except (OSError, UnicodeError) as exc:
        raise SoakAnalysisError(f"cannot read telemetry JSONL: {exc}") from exc
    if not samples:
        raise SoakAnalysisError("telemetry JSONL contains no samples")
    return samples


def _percentile(values: list[int], q: float) -> float:
    if not values:
        raise SoakAnalysisError("cannot calculate percentile of empty sample set")
    ordered = sorted(values)
    if len(ordered) == 1:
        return float(ordered[0])
    position = (len(ordered) - 1) * q
    lower = math.floor(position)
    upper = math.ceil(position)
    if lower == upper:
        return float(ordered[lower])
    weight = position - lower
    return float(ordered[lower] * (1.0 - weight) + ordered[upper] * weight)


def _ols_slope_per_hour(samples: list[dict[str, Any]], metric: str) -> float:
    xs = [float(s["elapsed_seconds"]) for s in samples]
    ys = [float(s[metric]) for s in samples]
    if len(xs) < 2:
        return 0.0
    x_mean = sum(xs) / len(xs)
    y_mean = sum(ys) / len(ys)
    denominator = sum((x - x_mean) ** 2 for x in xs)
    if denominator == 0.0:
        return 0.0
    slope_per_second = sum((x - x_mean) * (y - y_mean) for x, y in zip(xs, ys)) / denominator
    return slope_per_second * 3600.0


def _window_median(values: list[int], *, first: bool) -> float:
    width = max(1, math.ceil(len(values) * 0.1))
    window = values[:width] if first else values[-width:]
    return float(median(window))


def _metric_analysis(samples: list[dict[str, Any]], metric: str) -> dict[str, Any]:
    values = [int(s[metric]) for s in samples]
    first_window = _window_median(values, first=True)
    last_window = _window_median(values, first=False)
    return {
        "first": values[0],
        "last": values[-1],
        "min": min(values),
        "max": max(values),
        "delta_first_to_last": values[-1] - values[0],
        "first_decile_median": first_window,
        "last_decile_median": last_window,
        "delta_decile_medians": last_window - first_window,
        "p50": _percentile(values, 0.50),
        "p95": _percentile(values, 0.95),
        "p99": _percentile(values, 0.99),
        "ols_slope_per_hour": _ols_slope_per_hour(samples, metric),
    }


def analyze(receipt_path: Path, telemetry_path: Path, *,
            expected_commit: str | None = None,
            expected_executable_sha256: str | None = None) -> dict[str, Any]:
    receipt_file = _require_regular_file(receipt_path, "soak receipt", MAX_RECEIPT_BYTES)
    telemetry_file = _require_regular_file(telemetry_path, "telemetry file", MAX_TELEMETRY_BYTES)
    receipt = _load_json(receipt_file)
    commit, exe_hash, requested, elapsed, expected_samples, receipt_telemetry_hash = _validate_receipt(receipt)
    if expected_commit is not None:
        if not COMMIT_RE.fullmatch(expected_commit):
            raise SoakAnalysisError("expected commit must be 40 hex characters")
        if commit != expected_commit.lower():
            raise SoakAnalysisError("soak receipt commit does not match expected commit")
    if expected_executable_sha256 is not None:
        if not SHA256_RE.fullmatch(expected_executable_sha256):
            raise SoakAnalysisError("expected executable SHA-256 must be 64 hex characters")
        if exe_hash != expected_executable_sha256.lower():
            raise SoakAnalysisError("soak receipt AstralGame SHA-256 does not match expected hash")
    telemetry_hash = _sha256(telemetry_file)
    if telemetry_hash != receipt_telemetry_hash:
        raise SoakAnalysisError("telemetry SHA-256 does not match soak receipt")
    samples = _load_telemetry(telemetry_file)
    if len(samples) != expected_samples:
        raise SoakAnalysisError("telemetry sample count does not match soak receipt summary")
    first_elapsed = float(samples[0]["elapsed_seconds"])
    last_elapsed = float(samples[-1]["elapsed_seconds"])
    if last_elapsed > elapsed + 1e-6:
        raise SoakAnalysisError("telemetry extends beyond receipt elapsed_seconds")
    receipt_summary = receipt["telemetry_summary"]
    for metric in METRICS:
        summary_metric = receipt_summary.get(metric)
        if not isinstance(summary_metric, dict):
            raise SoakAnalysisError(f"receipt telemetry_summary missing {metric}")
        expected_first = _nonnegative_int(summary_metric.get("first"), f"summary {metric}.first")
        expected_last = _nonnegative_int(summary_metric.get("last"), f"summary {metric}.last")
        expected_min = _nonnegative_int(summary_metric.get("min"), f"summary {metric}.min")
        expected_max = _nonnegative_int(summary_metric.get("max"), f"summary {metric}.max")
        expected_delta = summary_metric.get("delta_first_to_last")
        if isinstance(expected_delta, bool) or not isinstance(expected_delta, int):
            raise SoakAnalysisError(f"summary {metric}.delta_first_to_last must be an integer")
        values = [int(s[metric]) for s in samples]
        actual = (values[0], values[-1], min(values), max(values), values[-1] - values[0])
        expected = (expected_first, expected_last, expected_min, expected_max, expected_delta)
        if actual != expected:
            raise SoakAnalysisError(f"receipt telemetry_summary does not match telemetry for {metric}")
    intervals = [
        float(samples[i]["elapsed_seconds"]) - float(samples[i - 1]["elapsed_seconds"])
        for i in range(1, len(samples))
    ]
    interval_analysis = {
        "count": len(intervals),
        "min_seconds": min(intervals) if intervals else None,
        "max_seconds": max(intervals) if intervals else None,
        "median_seconds": float(median(intervals)) if intervals else None,
        "p95_seconds": _percentile([int(round(v * 1000.0)) for v in intervals], 0.95) / 1000.0 if intervals else None,
    }
    metrics = {metric: _metric_analysis(samples, metric) for metric in METRICS}
    source_acceptance = receipt["acceptance"]
    return {
        "schema_version": 1,
        "source_soak_receipt": str(receipt_file),
        "source_soak_receipt_sha256": _sha256(receipt_file),
        "telemetry_file": str(telemetry_file),
        "telemetry_sha256": telemetry_hash,
        "commit": commit,
        "AstralGame_sha256": exe_hash,
        "source_soak_passed": bool(receipt.get("passed")),
        "requested_duration_seconds": requested,
        "receipt_elapsed_seconds": elapsed,
        "first_sample_elapsed_seconds": first_elapsed,
        "last_sample_elapsed_seconds": last_elapsed,
        "sample_count": len(samples),
        "sample_intervals": interval_analysis,
        "metrics": metrics,
        "acceptance": {
            "telemetry_integrity_verified": True,
            "soak_receipt_binding_verified": True,
            "required_24h_duration_observed": bool(source_acceptance.get("required_24h_duration_observed")),
            "required_24h_soak_verified": False,
            "ram_budget_verified": False,
            "vram_budget_verified": False,
            "frame_time_budget_verified": False,
            "memory_leak_free_verified": False,
            "clean_machine_compatibility_verified": False,
            "owned_interactive_desktop_verified": False,
            "independent_acceptance": False,
        },
        "limitations": [
            "Trend statistics summarize OS process telemetry only; they do not identify allocator callsites or prove a memory leak.",
            "OLS slope and percentile values are descriptive evidence, not approved performance or leak thresholds.",
            "Working set can vary because of operating-system paging and includes shared as well as private pages.",
            "No VRAM, frame-time, allocator-lifetime, clean-machine, desktop-ownership, or independent acceptance claim is made.",
            "Independent QA must inspect the bound soak receipt, telemetry, machine state, logs, and any allocator-level evidence before accepting the 24-hour soak.",
        ],
    }


def _write_json(path: Path, value: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    p = argparse.ArgumentParser()
    p.add_argument("soak_receipt", type=Path)
    p.add_argument("telemetry", type=Path)
    p.add_argument("--expected-commit")
    p.add_argument("--expected-executable-sha256")
    p.add_argument("--json", type=Path)
    return p.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    try:
        result = analyze(
            args.soak_receipt,
            args.telemetry,
            expected_commit=args.expected_commit,
            expected_executable_sha256=args.expected_executable_sha256,
        )
    except (SoakAnalysisError, OSError, ValueError) as exc:
        result = {
            "schema_version": 1,
            "passed": False,
            "error": str(exc),
            "acceptance": {
                "telemetry_integrity_verified": False,
                "soak_receipt_binding_verified": False,
                "required_24h_duration_observed": False,
                "required_24h_soak_verified": False,
                "ram_budget_verified": False,
                "vram_budget_verified": False,
                "frame_time_budget_verified": False,
                "memory_leak_free_verified": False,
                "clean_machine_compatibility_verified": False,
                "owned_interactive_desktop_verified": False,
                "independent_acceptance": False,
            },
        }
        text = json.dumps(result, indent=2, sort_keys=True) + "\n"
        print(text, end="")
        if args.json is not None:
            try:
                _write_json(args.json, result)
            except OSError as write_exc:
                print(f"cannot write analysis receipt: {write_exc}", file=sys.stderr)
        return 1
    result["passed"] = True
    text = json.dumps(result, indent=2, sort_keys=True) + "\n"
    print(text, end="")
    if args.json is not None:
        try:
            _write_json(args.json, result)
        except OSError as exc:
            print(f"cannot write analysis receipt: {exc}", file=sys.stderr)
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
