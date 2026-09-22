#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import re
import signal
import subprocess
import sys
from datetime import datetime
from pathlib import Path
from typing import Any, Callable

MAX_OUTPUT_BYTES = 2 * 1024 * 1024
MIN_TIMEOUT_SECONDS = 0.1
MAX_TIMEOUT_SECONDS = 300.0
SHA256_RE = re.compile(r"^[0-9a-fA-F]{64}$")
NATIVE_PASS_MARKER = "M10 AUTOMATED NATIVE RUNTIME SMOKE: PASS"


class RuntimeSmokeError(RuntimeError):
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
        raise RuntimeSmokeError(f"{first_label} and {second_label} must be disjoint")


def _prepare_runtime_root(package_root: Path, runtime_root: Path) -> Path:
    package = package_root.resolve(strict=True)
    if runtime_root.is_symlink():
        raise RuntimeSmokeError("runtime root must not be a symlink")
    root = runtime_root.resolve(strict=False)
    _require_disjoint(package, root, "package root", "runtime root")
    if root.exists():
        if not root.is_dir() or root.is_symlink():
            raise RuntimeSmokeError("runtime root must be a real directory")
        try:
            if any(root.iterdir()):
                raise RuntimeSmokeError("runtime root must be empty before launch")
        except OSError as exc:
            raise RuntimeSmokeError(f"cannot inspect runtime root: {exc}") from exc
    else:
        parent = root.parent.resolve(strict=True)
        if parent.is_symlink() or not parent.is_dir():
            raise RuntimeSmokeError("runtime root parent must be a real directory")
        root.mkdir()
    return root.resolve(strict=True)


def _validate_smoke_executable(path: Path) -> Path:
    if path.is_symlink():
        raise RuntimeSmokeError("smoke executable must not be a symlink")
    resolved = path.resolve(strict=True)
    if not resolved.is_file():
        raise RuntimeSmokeError("smoke executable must be a regular file")
    return resolved


def _terminate_tree(process: subprocess.Popen[Any]) -> None:
    if process.poll() is not None:
        return
    if os.name == "nt":
        try:
            subprocess.run(
                ["taskkill", "/PID", str(process.pid), "/T", "/F"],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                check=False,
                timeout=10,
            )
        except (OSError, subprocess.SubprocessError):
            pass
    else:
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except (ProcessLookupError, PermissionError, OSError):
            pass
    if process.poll() is None:
        try:
            process.kill()
        except OSError:
            pass
    try:
        process.wait(timeout=10)
    except (subprocess.TimeoutExpired, OSError) as exc:
        raise RuntimeSmokeError("runtime smoke process tree did not terminate") from exc


def _read_output(path: Path) -> tuple[str, int, bool]:
    size = path.stat().st_size if path.exists() else 0
    exceeded = size > MAX_OUTPUT_BYTES
    if size == 0:
        return "", 0, exceeded
    with path.open("rb") as stream:
        if exceeded:
            stream.seek(-MAX_OUTPUT_BYTES, os.SEEK_END)
        data = stream.read(MAX_OUTPUT_BYTES)
    return data.decode("utf-8", errors="replace"), size, exceeded


def _execute_smoke(command: list[str], cwd: Path, timeout_seconds: float, output_path: Path) -> dict[str, Any]:
    creationflags = getattr(subprocess, "CREATE_NEW_PROCESS_GROUP", 0) if os.name == "nt" else 0
    kwargs: dict[str, Any] = {"cwd": str(cwd), "stdout": None, "stderr": subprocess.STDOUT}
    if os.name == "nt":
        kwargs["creationflags"] = creationflags
    else:
        kwargs["start_new_session"] = True
    timed_out = False
    with output_path.open("wb") as output:
        kwargs["stdout"] = output
        process = subprocess.Popen(command, **kwargs)
        try:
            return_code = process.wait(timeout=timeout_seconds)
        except subprocess.TimeoutExpired:
            timed_out = True
            _terminate_tree(process)
            return_code = process.returncode
        except BaseException:
            _terminate_tree(process)
            raise
    text, byte_count, exceeded = _read_output(output_path)
    return {
        "return_code": return_code,
        "timed_out": timed_out,
        "output": text,
        "output_bytes": byte_count,
        "output_limit_exceeded": exceeded,
    }


def _load_and_verify_package(
    manifest_path: Path,
    package_root: Path,
    expected_commit: str,
    expected_executable_sha256: str,
) -> dict[str, Any]:
    try:
        import release_manifest
    except ImportError as exc:
        raise RuntimeSmokeError("release_manifest.py must be importable beside this script") from exc
    try:
        return release_manifest.verify_manifest(
            release_manifest.load_manifest(manifest_path),
            package_root,
            manifest_path=manifest_path,
            expected_commit=expected_commit,
            expected_executable_sha256=expected_executable_sha256,
            executable_path="AstralGame.exe",
        )
    except (release_manifest.ManifestError, OSError) as exc:
        raise RuntimeSmokeError(f"package manifest verification failed: {exc}") from exc


def verify_and_run(
    manifest_path: Path,
    package_root: Path,
    expected_commit: str,
    expected_executable_sha256: str,
    smoke_executable: Path,
    runtime_root: Path,
    *,
    timeout_seconds: float = 45.0,
    smoke_prefix_args: list[str] | None = None,
    synthetic_contract_test: bool = False,
    expected_smoke_sha256: str | None = None,
    platform_name: str | None = None,
    verifier: Callable[[Path, Path, str, str], dict[str, Any]] = _load_and_verify_package,
    executor: Callable[[list[str], Path, float, Path], dict[str, Any]] = _execute_smoke,
) -> dict[str, Any]:
    if not MIN_TIMEOUT_SECONDS <= timeout_seconds <= MAX_TIMEOUT_SECONDS:
        raise RuntimeSmokeError(
            f"timeout must be between {MIN_TIMEOUT_SECONDS} and {MAX_TIMEOUT_SECONDS} seconds"
        )
    package = package_root.resolve(strict=True)
    if not package.is_dir():
        raise RuntimeSmokeError("package root must be a directory")
    if manifest_path.is_symlink():
        raise RuntimeSmokeError("manifest must not be a symlink")
    manifest = manifest_path.resolve(strict=True)
    if not manifest.is_file():
        raise RuntimeSmokeError("manifest must be a real file")
    if not _is_under(package, manifest):
        raise RuntimeSmokeError("manifest must be inside the package root")

    smoke = _validate_smoke_executable(smoke_executable)
    smoke_hash = _sha256(smoke)
    effective_platform = os.name if platform_name is None else platform_name
    if not synthetic_contract_test:
        if effective_platform != "nt":
            raise RuntimeSmokeError("native package runtime acceptance requires Windows")
        if smoke_prefix_args:
            raise RuntimeSmokeError("native package runtime acceptance does not allow smoke prefix arguments")
        if smoke.name.casefold() != "m10runtimesmoke.exe":
            raise RuntimeSmokeError("native package runtime acceptance requires M10RuntimeSmoke.exe")
        if expected_smoke_sha256 is None or not SHA256_RE.fullmatch(expected_smoke_sha256):
            raise RuntimeSmokeError("native package runtime acceptance requires a 64-hex expected smoke SHA-256")
        if smoke_hash != expected_smoke_sha256.lower():
            raise RuntimeSmokeError("runtime smoke executable SHA-256 mismatch")
    runtime = _prepare_runtime_root(package, runtime_root)
    if _is_under(package, smoke):
        raise RuntimeSmokeError("smoke executable must be outside the package root")
    executable_path = package / "AstralGame.exe"
    if executable_path.is_symlink():
        raise RuntimeSmokeError("AstralGame.exe must not be a symlink")
    executable = executable_path.resolve(strict=True)
    if not _is_under(package, executable) or not executable.is_file():
        raise RuntimeSmokeError("AstralGame.exe must be a real file inside the package root")

    before = verifier(manifest, package, expected_commit, expected_executable_sha256)
    command = [str(smoke), *(smoke_prefix_args or []), str(executable), str(runtime)]
    output_path = runtime / "runtime-smoke-output.log"
    started_at = _iso_now()
    execution = executor(command, runtime, timeout_seconds, output_path)
    finished_at = _iso_now()

    post_verify_error = None
    after = None
    try:
        after = verifier(manifest, package, expected_commit, expected_executable_sha256)
    except RuntimeSmokeError as exc:
        post_verify_error = str(exc)

    marker_verified = synthetic_contract_test or NATIVE_PASS_MARKER in str(execution.get("output", ""))
    smoke_passed = (
        execution.get("return_code") == 0
        and execution.get("timed_out") is False
        and execution.get("output_limit_exceeded") is False
        and marker_verified
    )
    package_unchanged = after is not None
    passed = smoke_passed and package_unchanged
    real_native_execution = (
        passed
        and not synthetic_contract_test
        and platform_name is None
        and os.name == "nt"
        and verifier is _load_and_verify_package
        and executor is _execute_smoke
    )

    return {
        "schema_version": 1,
        "started_at": started_at,
        "finished_at": finished_at,
        "platform": platform.platform(),
        "evidence_kind": (
            "synthetic_contract"
            if synthetic_contract_test
            else "native_package_runtime_smoke" if real_native_execution else "native_contract_test"
        ),
        "package_root": str(package),
        "manifest": str(manifest),
        "commit": expected_commit.lower(),
        "AstralGame_sha256": expected_executable_sha256.lower(),
        "smoke_executable": str(smoke),
        "smoke_executable_sha256": smoke_hash,
        "expected_smoke_executable_sha256": expected_smoke_sha256.lower() if expected_smoke_sha256 else None,
        "native_pass_marker_verified": marker_verified and not synthetic_contract_test,
        "smoke_prefix_args": smoke_prefix_args or [],
        "runtime_root": str(runtime),
        "command": command,
        "timeout_seconds": timeout_seconds,
        "execution": execution,
        "package_manifest_before": before,
        "package_manifest_after": after,
        "post_launch_manifest_error": post_verify_error,
        "acceptance": {
            "smoke_process_passed": smoke_passed,
            "package_unchanged_after_smoke": package_unchanged,
            "package_launch_verified": real_native_execution,
            "clean_machine_compatibility_verified": False,
            "owned_interactive_desktop_verified": False,
            "independent_acceptance": False,
        },
        "passed": passed,
        "limitations": [
            "A passing native receipt means the supplied smoke executable returned zero for the exact manifest-bound AstralGame.exe and the package bytes were unchanged afterward.",
            "Synthetic contract mode never proves package launch.",
            "This tool does not prove that the machine is clean or supported, that Visual C++ prerequisite installation is supported, or that an interactive desktop is exclusively owned.",
            "This tool does not replace independent review, performance/stress evidence, or the required soak.",
        ],
    }


def _write_json(path: Path, data: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("manifest", type=Path)
    parser.add_argument("package_root", type=Path)
    parser.add_argument("--expected-commit", required=True)
    parser.add_argument("--expected-executable-sha256", required=True)
    parser.add_argument("--smoke-executable", required=True, type=Path)
    parser.add_argument("--smoke-prefix-arg", action="append", default=[])
    parser.add_argument("--runtime-root", required=True, type=Path)
    parser.add_argument("--expected-smoke-sha256")
    parser.add_argument("--timeout-seconds", type=float, default=45.0)
    parser.add_argument("--json", type=Path)
    parser.add_argument("--synthetic-contract-test", action="store_true")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    result: dict[str, Any]
    try:
        package_resolved = args.package_root.resolve(strict=True)
        if args.json is not None:
            _require_disjoint(package_resolved, args.json.resolve(strict=False), "package root", "receipt path")
        result = verify_and_run(
            args.manifest,
            args.package_root,
            args.expected_commit,
            args.expected_executable_sha256,
            args.smoke_executable,
            args.runtime_root,
            timeout_seconds=args.timeout_seconds,
            smoke_prefix_args=args.smoke_prefix_arg,
            synthetic_contract_test=args.synthetic_contract_test,
            expected_smoke_sha256=args.expected_smoke_sha256,
        )
    except (RuntimeSmokeError, OSError, ValueError) as exc:
        result = {
            "schema_version": 1,
            "passed": False,
            "error": str(exc),
            "acceptance": {
                "package_launch_verified": False,
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
            print(f"cannot write receipt: {exc}", file=sys.stderr)
            return 1
    return 0 if result.get("passed") else 1


if __name__ == "__main__":
    raise SystemExit(main())
