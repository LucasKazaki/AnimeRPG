#!/usr/bin/env python3
from __future__ import annotations

import argparse, ctypes, hashlib, json, os, platform, re, signal, subprocess, sys, time
from ctypes import wintypes
from datetime import datetime
from pathlib import Path
from typing import Any, Callable

MIN_NATIVE_DURATION_SECONDS = 60.0
MAX_DURATION_SECONDS = 90000.0
MIN_NATIVE_SAMPLE_INTERVAL_SECONDS = 1.0
MAX_SAMPLE_INTERVAL_SECONDS = 300.0
MIN_SYNTHETIC_DURATION_SECONDS = 0.01
MIN_SYNTHETIC_SAMPLE_INTERVAL_SECONDS = 0.001
MAX_WINDOW_WAIT_SECONDS = 60.0
REQUIRED_SOAK_SECONDS = 86400
SHA256_RE = re.compile(r"^[0-9a-fA-F]{64}$")
COMMIT_RE = re.compile(r"^[0-9a-fA-F]{40}$")
WM_CLOSE = 0x0010
PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
GR_GDIOBJECTS, GR_USEROBJECTS = 0, 1


class ContinuousSoakError(RuntimeError):
    pass


def _iso_now() -> str:
    return datetime.now().astimezone().isoformat(timespec="seconds")


def _sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def _is_under(parent: Path, child: Path) -> bool:
    try:
        return os.path.commonpath((str(parent), str(child))) == str(parent)
    except ValueError:
        return False


def _require_disjoint(a: Path, b: Path, a_label: str, b_label: str) -> None:
    a, b = a.resolve(strict=False), b.resolve(strict=False)
    if a == b or _is_under(a, b) or _is_under(b, a):
        raise ContinuousSoakError(f"{a_label} and {b_label} must be disjoint")


def _prepare_runtime_root(package_root: Path, runtime_root: Path) -> Path:
    package = package_root.resolve(strict=True)
    if not package.is_dir():
        raise ContinuousSoakError("package root must be a directory")
    if runtime_root.is_symlink():
        raise ContinuousSoakError("runtime root must not be a symlink")
    root = runtime_root.resolve(strict=False)
    _require_disjoint(package, root, "package root", "runtime root")
    if root.exists():
        if not root.is_dir() or root.is_symlink():
            raise ContinuousSoakError("runtime root must be a real directory")
        try:
            if any(root.iterdir()):
                raise ContinuousSoakError("runtime root must be empty before launch")
        except OSError as exc:
            raise ContinuousSoakError(f"cannot inspect runtime root: {exc}") from exc
    else:
        parent = root.parent.resolve(strict=True)
        if parent.is_symlink() or not parent.is_dir():
            raise ContinuousSoakError("runtime root parent must be a real directory")
        root.mkdir()
    return root.resolve(strict=True)


def _load_and_verify_package(manifest: Path, package: Path, commit: str, exe_hash: str) -> dict[str, Any]:
    try:
        import release_manifest
    except ImportError as exc:
        raise ContinuousSoakError("release_manifest.py must be importable beside this script") from exc
    try:
        return release_manifest.verify_manifest(
            release_manifest.load_manifest(manifest), package, manifest_path=manifest,
            expected_commit=commit, expected_executable_sha256=exe_hash,
            executable_path="AstralGame.exe")
    except (release_manifest.ManifestError, OSError) as exc:
        raise ContinuousSoakError(f"package manifest verification failed: {exc}") from exc


def _launch_game(executable: Path, runtime_root: Path) -> subprocess.Popen[Any]:
    kw: dict[str, Any] = dict(cwd=str(runtime_root), stdin=subprocess.DEVNULL,
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if os.name == "nt":
        kw["creationflags"] = getattr(subprocess, "CREATE_NEW_PROCESS_GROUP", 0)
    else:
        kw["start_new_session"] = True
    return subprocess.Popen([str(executable)], **kw)


def _terminate_tree(process: subprocess.Popen[Any]) -> None:
    if process.poll() is not None:
        return
    if os.name == "nt":
        try:
            subprocess.run(["taskkill", "/PID", str(process.pid), "/T", "/F"],
                           stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL, check=False, timeout=10)
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
        raise ContinuousSoakError("soak process tree did not terminate") from exc


def _enum_visible_windows_for_pid(pid: int) -> list[int]:
    if os.name != "nt":
        raise ContinuousSoakError("visible-window inspection requires Windows")
    user32 = ctypes.WinDLL("user32", use_last_error=True)
    callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    enum_windows = user32.EnumWindows
    enum_windows.argtypes, enum_windows.restype = [callback_type, wintypes.LPARAM], wintypes.BOOL
    get_pid = user32.GetWindowThreadProcessId
    get_pid.argtypes, get_pid.restype = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)], wintypes.DWORD
    visible = user32.IsWindowVisible
    visible.argtypes, visible.restype = [wintypes.HWND], wintypes.BOOL
    windows: list[int] = []

    @callback_type
    def callback(hwnd: int, _param: int) -> bool:
        found = wintypes.DWORD()
        get_pid(hwnd, ctypes.byref(found))
        if found.value == pid and visible(hwnd):
            windows.append(int(hwnd))
        return True

    ctypes.set_last_error(0)
    if not enum_windows(callback, 0):
        error = ctypes.get_last_error()
        if error:
            raise ContinuousSoakError(f"EnumWindows failed with Win32 error {error}")
    return windows


def _wait_for_visible_window(pid: int, timeout_seconds: float, *,
                             sleeper: Callable[[float], None] = time.sleep,
                             monotonic: Callable[[], float] = time.monotonic) -> bool:
    deadline = monotonic() + timeout_seconds
    while monotonic() < deadline:
        if _enum_visible_windows_for_pid(pid):
            return True
        sleeper(min(0.25, max(0.0, deadline - monotonic())))
    return bool(_enum_visible_windows_for_pid(pid))


def _request_graceful_close(pid: int) -> int:
    if os.name != "nt":
        raise ContinuousSoakError("graceful window close requires Windows")
    windows = _enum_visible_windows_for_pid(pid)
    if not windows:
        return 0
    user32 = ctypes.WinDLL("user32", use_last_error=True)
    post = user32.PostMessageW
    post.argtypes, post.restype = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM], wintypes.BOOL
    for hwnd in windows:
        ctypes.set_last_error(0)
        if not post(hwnd, WM_CLOSE, 0, 0):
            raise ContinuousSoakError(f"PostMessageW(WM_CLOSE) failed with Win32 error {ctypes.get_last_error()}")
    return len(windows)


class _PROCESS_MEMORY_COUNTERS_EX(ctypes.Structure):
    _fields_ = [("cb", wintypes.DWORD), ("PageFaultCount", wintypes.DWORD),
                ("PeakWorkingSetSize", ctypes.c_size_t), ("WorkingSetSize", ctypes.c_size_t),
                ("QuotaPeakPagedPoolUsage", ctypes.c_size_t), ("QuotaPagedPoolUsage", ctypes.c_size_t),
                ("QuotaPeakNonPagedPoolUsage", ctypes.c_size_t), ("QuotaNonPagedPoolUsage", ctypes.c_size_t),
                ("PagefileUsage", ctypes.c_size_t), ("PeakPagefileUsage", ctypes.c_size_t),
                ("PrivateUsage", ctypes.c_size_t)]


def _sample_windows_process(pid: int) -> dict[str, int]:
    if os.name != "nt":
        raise ContinuousSoakError("process telemetry requires Windows")
    kernel32, user32 = ctypes.WinDLL("kernel32", use_last_error=True), ctypes.WinDLL("user32", use_last_error=True)
    open_process = kernel32.OpenProcess
    open_process.argtypes, open_process.restype = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD], wintypes.HANDLE
    close_handle = kernel32.CloseHandle
    close_handle.argtypes, close_handle.restype = [wintypes.HANDLE], wintypes.BOOL
    get_memory = kernel32.K32GetProcessMemoryInfo
    get_memory.argtypes = [wintypes.HANDLE, ctypes.POINTER(_PROCESS_MEMORY_COUNTERS_EX), wintypes.DWORD]
    get_memory.restype = wintypes.BOOL
    get_handles = kernel32.GetProcessHandleCount
    get_handles.argtypes, get_handles.restype = [wintypes.HANDLE, ctypes.POINTER(wintypes.DWORD)], wintypes.BOOL
    get_gui = user32.GetGuiResources
    get_gui.argtypes, get_gui.restype = [wintypes.HANDLE, wintypes.DWORD], wintypes.DWORD

    ctypes.set_last_error(0)
    handle = open_process(PROCESS_QUERY_LIMITED_INFORMATION, False, pid)
    if not handle:
        raise ContinuousSoakError(f"OpenProcess failed with Win32 error {ctypes.get_last_error()}")
    try:
        counters = _PROCESS_MEMORY_COUNTERS_EX()
        counters.cb = ctypes.sizeof(counters)
        if not get_memory(handle, ctypes.byref(counters), counters.cb):
            raise ContinuousSoakError(f"K32GetProcessMemoryInfo failed with Win32 error {ctypes.get_last_error()}")
        handle_count = wintypes.DWORD()
        if not get_handles(handle, ctypes.byref(handle_count)):
            raise ContinuousSoakError(f"GetProcessHandleCount failed with Win32 error {ctypes.get_last_error()}")
        # Zero is documented as a valid successful result when a process has no GUI
        # objects, and is therefore not distinguishable from failure by return value alone.
        # Production soak admission separately requires a visible AstralGame window.
        return {
            "working_set_bytes": int(counters.WorkingSetSize),
            "peak_working_set_bytes": int(counters.PeakWorkingSetSize),
            "private_usage_bytes": int(counters.PrivateUsage),
            "pagefile_usage_bytes": int(counters.PagefileUsage),
            "peak_pagefile_usage_bytes": int(counters.PeakPagefileUsage),
            "handle_count": int(handle_count.value),
            "gdi_objects": int(get_gui(handle, GR_GDIOBJECTS)),
            "user_objects": int(get_gui(handle, GR_USEROBJECTS)),
        }
    finally:
        close_handle(handle)


def _summarize_samples(samples: list[dict[str, Any]]) -> dict[str, Any]:
    metrics = ("working_set_bytes", "peak_working_set_bytes", "private_usage_bytes",
               "pagefile_usage_bytes", "peak_pagefile_usage_bytes", "handle_count",
               "gdi_objects", "user_objects")
    out: dict[str, Any] = {"sample_count": len(samples)}
    if samples:
        for name in metrics:
            values = [int(s[name]) for s in samples]
            out[name] = {"first": values[0], "last": values[-1], "min": min(values),
                         "max": max(values), "delta_first_to_last": values[-1] - values[0]}
    return out


def _append_jsonl(path: Path, record: dict[str, Any]) -> None:
    with path.open("a", encoding="utf-8", newline="\n") as f:
        f.write(json.dumps(record, sort_keys=True) + "\n")
        f.flush()
        try:
            os.fsync(f.fileno())
        except OSError:
            pass


def run_continuous_soak(manifest_path: Path, package_root: Path, expected_commit: str,
                        expected_executable_sha256: str, runtime_root: Path, *,
                        duration_seconds: float, sample_interval_seconds: float = 10.0,
                        window_wait_seconds: float = 30.0, close_timeout_seconds: float = 10.0,
                        synthetic_contract_test: bool = False,
                        verifier: Callable[[Path, Path, str, str], dict[str, Any]] = _load_and_verify_package,
                        launcher: Callable[[Path, Path], Any] = _launch_game,
                        sampler: Callable[[int], dict[str, int]] = _sample_windows_process,
                        window_probe: Callable[[int, float], bool] = _wait_for_visible_window,
                        graceful_closer: Callable[[int], int] = _request_graceful_close,
                        terminator: Callable[[Any], None] = _terminate_tree,
                        sleeper: Callable[[float], None] = time.sleep,
                        monotonic: Callable[[], float] = time.monotonic,
                        now: Callable[[], str] = _iso_now) -> dict[str, Any]:
    min_duration = MIN_SYNTHETIC_DURATION_SECONDS if synthetic_contract_test else MIN_NATIVE_DURATION_SECONDS
    min_interval = MIN_SYNTHETIC_SAMPLE_INTERVAL_SECONDS if synthetic_contract_test else MIN_NATIVE_SAMPLE_INTERVAL_SECONDS
    if not min_duration <= duration_seconds <= MAX_DURATION_SECONDS:
        raise ContinuousSoakError(f"duration must be between {min_duration} and {MAX_DURATION_SECONDS} seconds")
    if not min_interval <= sample_interval_seconds <= MAX_SAMPLE_INTERVAL_SECONDS:
        raise ContinuousSoakError(f"sample interval must be between {min_interval} and {MAX_SAMPLE_INTERVAL_SECONDS} seconds")
    if not 0.0 <= window_wait_seconds <= MAX_WINDOW_WAIT_SECONDS:
        raise ContinuousSoakError(f"window wait must be between 0 and {MAX_WINDOW_WAIT_SECONDS} seconds")
    if not 0.1 <= close_timeout_seconds <= 60.0:
        raise ContinuousSoakError("close timeout must be between 0.1 and 60 seconds")
    if not COMMIT_RE.fullmatch(expected_commit):
        raise ContinuousSoakError("expected commit must be 40 hex characters")
    if not SHA256_RE.fullmatch(expected_executable_sha256):
        raise ContinuousSoakError("expected AstralGame SHA-256 must be 64 hex characters")

    package = package_root.resolve(strict=True)
    if manifest_path.is_symlink():
        raise ContinuousSoakError("manifest must not be a symlink")
    manifest = manifest_path.resolve(strict=True)
    if not manifest.is_file() or not _is_under(package, manifest):
        raise ContinuousSoakError("manifest must be a real file inside the package root")
    candidate = package / "AstralGame.exe"
    if candidate.is_symlink():
        raise ContinuousSoakError("AstralGame.exe must not be a symlink")
    executable = candidate.resolve(strict=True)
    if not executable.is_file() or not _is_under(package, executable):
        raise ContinuousSoakError("AstralGame.exe must be a real file inside the package root")
    if _sha256(executable) != expected_executable_sha256.lower():
        raise ContinuousSoakError("AstralGame.exe SHA-256 mismatch before soak")

    runtime = _prepare_runtime_root(package, runtime_root)
    telemetry_path = runtime / "continuous-soak-telemetry.jsonl"
    before = verifier(manifest, package, expected_commit, expected_executable_sha256)
    started_at = now()
    started = monotonic()
    process = launcher(executable, runtime)
    pid = int(process.pid)
    samples: list[dict[str, Any]] = []
    failure: str | None = None
    interrupted = False
    visible = False
    close_requested = False
    clean_exit = False
    close_windows = 0
    return_code: int | None = None
    try:
        if not synthetic_contract_test and os.name != "nt":
            raise ContinuousSoakError("native continuous package soak requires Windows")
        visible = bool(window_probe(pid, window_wait_seconds))
        if not visible:
            failure = "no visible AstralGame window observed within the startup bound"
        while failure is None:
            elapsed = monotonic() - started
            if process.poll() is not None:
                return_code = int(process.returncode)
                if elapsed < duration_seconds:
                    failure = f"AstralGame exited before requested duration with code {return_code}"
                break
            try:
                metrics = sampler(pid)
            except (ContinuousSoakError, OSError, ValueError) as exc:
                failure = f"telemetry sample failed: {exc}"
                break
            sample = {"timestamp": now(), "elapsed_seconds": max(0.0, elapsed), **metrics}
            samples.append(sample)
            _append_jsonl(telemetry_path, sample)
            if elapsed >= duration_seconds:
                break
            sleeper(min(sample_interval_seconds, max(0.0, duration_seconds - elapsed)))
        if failure is None and process.poll() is None:
            close_windows = int(graceful_closer(pid))
            close_requested = close_windows > 0
            if not close_requested:
                failure = "no visible window accepted WM_CLOSE at the end of the soak"
            else:
                try:
                    return_code = int(process.wait(timeout=close_timeout_seconds))
                    clean_exit = return_code == 0
                    if not clean_exit:
                        failure = f"AstralGame returned nonzero exit code {return_code} after WM_CLOSE"
                except subprocess.TimeoutExpired:
                    failure = "AstralGame did not exit within the graceful-close timeout"
    except KeyboardInterrupt:
        interrupted, failure = True, "interrupted"
    except (ContinuousSoakError, OSError, ValueError) as exc:
        failure = str(exc)
    finally:
        if process.poll() is None:
            terminator(process)
        if return_code is None and process.poll() is not None:
            return_code = int(process.returncode)

    finished_at = now()
    elapsed = max(0.0, monotonic() - started)
    after = None
    post_error = None
    try:
        after = verifier(manifest, package, expected_commit, expected_executable_sha256)
    except (ContinuousSoakError, OSError, ValueError) as exc:
        post_error = str(exc)
        if failure is None:
            failure = f"post-soak package verification failed: {exc}"
    summary = _summarize_samples(samples)
    package_unchanged = after is not None
    passed = (failure is None and not interrupted and visible and elapsed >= duration_seconds
              and clean_exit and return_code == 0 and package_unchanged and bool(samples))
    production = (verifier is _load_and_verify_package and launcher is _launch_game
                  and sampler is _sample_windows_process and window_probe is _wait_for_visible_window
                  and graceful_closer is _request_graceful_close and terminator is _terminate_tree
                  and sleeper is time.sleep and monotonic is time.monotonic)
    native = passed and not synthetic_contract_test and os.name == "nt" and production
    full_duration = native and duration_seconds >= REQUIRED_SOAK_SECONDS and elapsed >= REQUIRED_SOAK_SECONDS
    return {
        "schema_version": 1, "started_at": started_at, "finished_at": finished_at,
        "platform": platform.platform(),
        "evidence_kind": "synthetic_contract" if synthetic_contract_test else
                         "native_continuous_package_soak" if native else "native_contract_test",
        "package_root": str(package), "manifest": str(manifest), "commit": expected_commit.lower(),
        "AstralGame_sha256": expected_executable_sha256.lower(), "runtime_root": str(runtime),
        "process_id": pid, "requested_duration_seconds": duration_seconds, "elapsed_seconds": elapsed,
        "sample_interval_seconds": sample_interval_seconds, "window_wait_seconds": window_wait_seconds,
        "close_timeout_seconds": close_timeout_seconds, "visible_window_observed": visible,
        "graceful_close_requested": close_requested, "graceful_close_window_count": close_windows,
        "return_code": return_code, "interrupted": interrupted, "failure": failure,
        "telemetry_path": str(telemetry_path),
        "telemetry_sha256": _sha256(telemetry_path) if telemetry_path.exists() else None,
        "telemetry_summary": summary, "package_manifest_before": before,
        "package_manifest_after": after, "post_soak_manifest_error": post_error, "passed": passed,
        "acceptance": {
            "continuous_package_uptime_observed": native,
            "required_24h_duration_observed": full_duration,
            "required_24h_soak_verified": False,
            "ram_telemetry_observed": native and bool(samples),
            "ram_budget_verified": False, "vram_budget_verified": False,
            "frame_time_budget_verified": False, "memory_leak_free_verified": False,
            "package_unchanged_after_soak": package_unchanged,
            "clean_machine_compatibility_verified": False,
            "owned_interactive_desktop_verified": False, "independent_acceptance": False,
        },
        "limitations": [
            "This harness observes one continuously running AstralGame process; it is distinct from the restart-stress sequence and does not exercise repeated process creation.",
            "Working-set/private-memory and handle/GDI/USER trends are operating-system process telemetry, not allocator-level leak proof.",
            "This harness does not measure VRAM or frame time and applies no unapproved RAM, handle, or growth threshold.",
            "Even a full 24-hour production observation leaves required_24h_soak_verified false until the retained evidence, machine state, desktop ownership, and independent QA are reviewed.",
            "Synthetic or dependency-injected runs never establish native uptime, RAM, or 24-hour claims.",
            "This harness does not establish clean-machine compatibility or independent acceptance.",
        ],
    }


def _write_json(path: Path, data: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    p = argparse.ArgumentParser()
    p.add_argument("manifest", type=Path)
    p.add_argument("package_root", type=Path)
    p.add_argument("--expected-commit", required=True)
    p.add_argument("--expected-executable-sha256", required=True)
    p.add_argument("--runtime-root", required=True, type=Path)
    p.add_argument("--duration-seconds", type=float, required=True)
    p.add_argument("--sample-interval-seconds", type=float, default=10.0)
    p.add_argument("--window-wait-seconds", type=float, default=30.0)
    p.add_argument("--close-timeout-seconds", type=float, default=10.0)
    p.add_argument("--json", type=Path)
    p.add_argument("--synthetic-contract-test", action="store_true")
    return p.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    try:
        package = args.package_root.resolve(strict=True)
        if args.json is not None:
            _require_disjoint(package, args.json.resolve(strict=False), "package root", "summary path")
        result = run_continuous_soak(args.manifest, args.package_root, args.expected_commit,
                                     args.expected_executable_sha256, args.runtime_root,
                                     duration_seconds=args.duration_seconds,
                                     sample_interval_seconds=args.sample_interval_seconds,
                                     window_wait_seconds=args.window_wait_seconds,
                                     close_timeout_seconds=args.close_timeout_seconds,
                                     synthetic_contract_test=args.synthetic_contract_test)
    except (ContinuousSoakError, OSError, ValueError) as exc:
        result = {"schema_version": 1, "passed": False, "error": str(exc), "acceptance": {
            "continuous_package_uptime_observed": False, "required_24h_duration_observed": False,
            "required_24h_soak_verified": False, "ram_telemetry_observed": False,
            "ram_budget_verified": False, "vram_budget_verified": False, "frame_time_budget_verified": False,
            "memory_leak_free_verified": False, "clean_machine_compatibility_verified": False,
            "owned_interactive_desktop_verified": False, "independent_acceptance": False}}
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
