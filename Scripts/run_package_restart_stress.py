#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import hashlib
import os
import re
import sys
import time
from datetime import datetime
from pathlib import Path
from typing import Any, Callable

import run_package_runtime_smoke

MIN_ITERATIONS = 2
MAX_ITERATIONS = 1000
MIN_INTERVAL_SECONDS = 0.0
MAX_INTERVAL_SECONDS = 3600.0
SHA256_RE = re.compile(r"^[0-9a-fA-F]{64}$")
COMMIT_RE = re.compile(r"^[0-9a-fA-F]{40}$")


class RestartStressError(RuntimeError):
    pass


def _iso_now() -> str:
    return datetime.now().astimezone().isoformat(timespec="seconds")


def _sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def _is_under(parent: Path, child: Path) -> bool:
    try:
        return os.path.commonpath((str(parent), str(child))) == str(parent)
    except ValueError:
        return False


def _require_disjoint(first: Path, second: Path, first_label: str, second_label: str) -> None:
    a = first.resolve(strict=False)
    b = second.resolve(strict=False)
    if a == b or _is_under(a, b) or _is_under(b, a):
        raise RestartStressError(f"{first_label} and {second_label} must be disjoint")


def _prepare_stress_root(package_root: Path, stress_root: Path) -> Path:
    package = package_root.resolve(strict=True)
    if not package.is_dir():
        raise RestartStressError("package root must be a directory")
    if stress_root.is_symlink():
        raise RestartStressError("stress root must not be a symlink")
    root = stress_root.resolve(strict=False)
    _require_disjoint(package, root, "package root", "stress root")
    if root.exists():
        if not root.is_dir() or root.is_symlink():
            raise RestartStressError("stress root must be a real directory")
        try:
            if any(root.iterdir()):
                raise RestartStressError("stress root must be empty before the sequence")
        except OSError as exc:
            raise RestartStressError(f"cannot inspect stress root: {exc}") from exc
    else:
        parent = root.parent.resolve(strict=True)
        if parent.is_symlink() or not parent.is_dir():
            raise RestartStressError("stress root parent must be a real directory")
        root.mkdir()
    return root.resolve(strict=True)


def _write_json(path: Path, data: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def run_restart_sequence(
    manifest_path: Path,
    package_root: Path,
    expected_commit: str,
    expected_executable_sha256: str,
    smoke_executable: Path,
    expected_smoke_sha256: str,
    stress_root: Path,
    *,
    iterations: int,
    interval_seconds: float = 1.0,
    timeout_seconds: float = 60.0,
    synthetic_contract_test: bool = False,
    iteration_runner: Callable[..., dict[str, Any]] = run_package_runtime_smoke.verify_and_run,
    sleeper: Callable[[float], None] = time.sleep,
) -> dict[str, Any]:
    if not isinstance(iterations, int) or isinstance(iterations, bool):
        raise RestartStressError("iterations must be an integer")
    if not MIN_ITERATIONS <= iterations <= MAX_ITERATIONS:
        raise RestartStressError(f"iterations must be between {MIN_ITERATIONS} and {MAX_ITERATIONS}")
    if not MIN_INTERVAL_SECONDS <= interval_seconds <= MAX_INTERVAL_SECONDS:
        raise RestartStressError(
            f"interval must be between {MIN_INTERVAL_SECONDS} and {MAX_INTERVAL_SECONDS} seconds"
        )
    if not COMMIT_RE.fullmatch(expected_commit):
        raise RestartStressError("expected commit must be 40 hex characters")
    if not SHA256_RE.fullmatch(expected_executable_sha256):
        raise RestartStressError("expected AstralGame SHA-256 must be 64 hex characters")
    if not SHA256_RE.fullmatch(expected_smoke_sha256):
        raise RestartStressError("expected smoke SHA-256 must be 64 hex characters")
    min_timeout = getattr(run_package_runtime_smoke, "MIN_TIMEOUT_SECONDS", 0.1)
    max_timeout = getattr(run_package_runtime_smoke, "MAX_TIMEOUT_SECONDS", 300.0)
    if not min_timeout <= timeout_seconds <= max_timeout:
        raise RestartStressError(f"timeout must be between {min_timeout} and {max_timeout} seconds")

    package = package_root.resolve(strict=True)
    if manifest_path.is_symlink():
        raise RestartStressError("manifest must not be a symlink")
    manifest = manifest_path.resolve(strict=True)
    if not manifest.is_file() or not _is_under(package, manifest):
        raise RestartStressError("manifest must be a real file inside the package root")
    executable = package / "AstralGame.exe"
    if executable.is_symlink():
        raise RestartStressError("AstralGame.exe must not be a symlink")
    executable = executable.resolve(strict=True)
    if not executable.is_file() or not _is_under(package, executable):
        raise RestartStressError("AstralGame.exe must be a real file inside the package root")
    if _sha256(executable) != expected_executable_sha256.lower():
        raise RestartStressError("AstralGame.exe SHA-256 mismatch before restart sequence")
    if smoke_executable.is_symlink():
        raise RestartStressError("smoke executable must not be a symlink")
    smoke = smoke_executable.resolve(strict=True)
    if not smoke.is_file() or _is_under(package, smoke):
        raise RestartStressError("smoke executable must be a real file outside the package root")
    if _sha256(smoke) != expected_smoke_sha256.lower():
        raise RestartStressError("smoke executable SHA-256 mismatch before restart sequence")

    root = _prepare_stress_root(package, stress_root)
    started_at = _iso_now()
    started_monotonic = time.monotonic()
    results: list[dict[str, Any]] = []
    failed_iteration: int | None = None
    interrupted = False

    for index in range(1, iterations + 1):
        runtime_root = root / f"runtime-{index:04d}"
        receipt_path = root / f"iteration-{index:04d}.json"
        try:
            result = iteration_runner(
                manifest,
                package,
                expected_commit,
                expected_executable_sha256,
                smoke,
                runtime_root,
                timeout_seconds=timeout_seconds,
                synthetic_contract_test=synthetic_contract_test,
                expected_smoke_sha256=expected_smoke_sha256,
            )
        except KeyboardInterrupt:
            interrupted = True
            failed_iteration = index
            break
        except (run_package_runtime_smoke.RuntimeSmokeError, OSError, ValueError) as exc:
            result = {
                "schema_version": 1,
                "passed": False,
                "error": str(exc),
                "acceptance": {"package_launch_verified": False},
            }

        _write_json(receipt_path, result)
        results.append(
            {
                "iteration": index,
                "receipt": str(receipt_path),
                "passed": result.get("passed") is True,
                "package_launch_verified": (
                    isinstance(result.get("acceptance"), dict)
                    and result["acceptance"].get("package_launch_verified") is True
                ),
                "evidence_kind": result.get("evidence_kind"),
            }
        )
        if result.get("passed") is not True:
            failed_iteration = index
            break
        if index != iterations and interval_seconds:
            try:
                sleeper(interval_seconds)
            except KeyboardInterrupt:
                interrupted = True
                failed_iteration = index + 1
                break

    finished_at = _iso_now()
    elapsed_seconds = max(0.0, time.monotonic() - started_monotonic)
    completed_iterations = len(results)
    passed = (
        not interrupted
        and failed_iteration is None
        and completed_iterations == iterations
        and all(item["passed"] for item in results)
    )
    production_runner_used = iteration_runner is run_package_runtime_smoke.verify_and_run
    real_repeated_native_sequence = (
        passed
        and not synthetic_contract_test
        and os.name == "nt"
        and production_runner_used
        and all(item["package_launch_verified"] for item in results)
    )

    return {
        "schema_version": 1,
        "started_at": started_at,
        "finished_at": finished_at,
        "elapsed_seconds": elapsed_seconds,
        "package_root": str(package),
        "manifest": str(manifest),
        "commit": expected_commit.lower(),
        "AstralGame_sha256": expected_executable_sha256.lower(),
        "smoke_executable": str(smoke),
        "expected_smoke_executable_sha256": expected_smoke_sha256.lower(),
        "stress_root": str(root),
        "requested_iterations": iterations,
        "completed_iterations": completed_iterations,
        "interval_seconds": interval_seconds,
        "per_iteration_timeout_seconds": timeout_seconds,
        "failed_iteration": failed_iteration,
        "interrupted": interrupted,
        "iterations": results,
        "passed": passed,
        "acceptance": {
            "repeated_native_package_launch_sequence_verified": real_repeated_native_sequence,
            "restart_stress_acceptance_approved": False,
            "required_24h_soak_verified": False,
            "clean_machine_compatibility_verified": False,
            "owned_interactive_desktop_verified": False,
            "independent_acceptance": False,
        },
        "limitations": [
            "This harness repeats the existing package-bound M10 smoke in fresh working directories; it does not keep one game process alive continuously.",
            "A passing production Windows sequence proves only the requested repeated launch/input/clean-exit cycles for the exact bound package and smoke hashes.",
            "Synthetic or dependency-injected iterations never establish a native repeated-launch claim.",
            "Restart-stress acceptance thresholds remain a separate QA decision; this harness never self-approves them.",
            "The required 24-hour soak remains separate and false even after a long repeated-launch sequence.",
            "This harness does not prove clean-machine compatibility, exclusive interactive-desktop ownership, or independent acceptance.",
        ],
    }


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("manifest", type=Path)
    parser.add_argument("package_root", type=Path)
    parser.add_argument("--expected-commit", required=True)
    parser.add_argument("--expected-executable-sha256", required=True)
    parser.add_argument("--smoke-executable", required=True, type=Path)
    parser.add_argument("--expected-smoke-sha256", required=True)
    parser.add_argument("--stress-root", required=True, type=Path)
    parser.add_argument("--iterations", required=True, type=int)
    parser.add_argument("--interval-seconds", type=float, default=1.0)
    parser.add_argument("--timeout-seconds", type=float, default=60.0)
    parser.add_argument("--synthetic-contract-test", action="store_true")
    parser.add_argument("--json", type=Path)
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    try:
        if args.json is not None:
            package = args.package_root.resolve(strict=True)
            _require_disjoint(package, args.json.resolve(strict=False), "package root", "summary path")
        result = run_restart_sequence(
            args.manifest,
            args.package_root,
            args.expected_commit,
            args.expected_executable_sha256,
            args.smoke_executable,
            args.expected_smoke_sha256,
            args.stress_root,
            iterations=args.iterations,
            interval_seconds=args.interval_seconds,
            timeout_seconds=args.timeout_seconds,
            synthetic_contract_test=args.synthetic_contract_test,
        )
    except (RestartStressError, run_package_runtime_smoke.RuntimeSmokeError, OSError, ValueError) as exc:
        result = {
            "schema_version": 1,
            "passed": False,
            "error": str(exc),
            "acceptance": {
                "repeated_native_package_launch_sequence_verified": False,
                "restart_stress_acceptance_approved": False,
                "required_24h_soak_verified": False,
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
        except OSError as exc:
            print(f"cannot write summary receipt: {exc}", file=sys.stderr)
            return 1
    return 0 if result.get("passed") else 1


if __name__ == "__main__":
    raise SystemExit(main())
