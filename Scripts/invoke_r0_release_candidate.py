#!/usr/bin/env python3
"""Run the bounded R0 recovery/package gate on the registered Windows host.

The runner is intentionally one-shot for a specific admitted revision. It validates
all filesystem roots before mutation, bounds every child command, preserves partial
logs on failure, and leaves independent review as a separate acceptance gate.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import shutil
import signal
import subprocess
import sys
import threading
from dataclasses import asdict, dataclass
from datetime import datetime
from pathlib import Path
from typing import Iterable, Sequence

HISTORICAL_DEADLINE = "2026-08-14T23:59:59-04:00"
REQUIRED_BASE = "181298f69b42899db83040d2d5b6c3e67804e242"
DEFAULT_BRANCH = "agent/r0-loop-recovery-2026-08-11"
DEFAULT_COMMAND_TIMEOUT_SECONDS = 1800.0
DEFAULT_CAPTURE_TIMEOUT_SECONDS = 120.0
TERMINATION_GRACE_SECONDS = 10.0
CAPTURE_SUPERVISOR_CODE = r"""import os, signal, subprocess, sys
if os.name == "nt":
    import ctypes
    from ctypes import wintypes

    class IO_COUNTERS(ctypes.Structure):
        _fields_ = [
            ("ReadOperationCount", ctypes.c_ulonglong),
            ("WriteOperationCount", ctypes.c_ulonglong),
            ("OtherOperationCount", ctypes.c_ulonglong),
            ("ReadTransferCount", ctypes.c_ulonglong),
            ("WriteTransferCount", ctypes.c_ulonglong),
            ("OtherTransferCount", ctypes.c_ulonglong),
        ]

    class JOBOBJECT_BASIC_LIMIT_INFORMATION(ctypes.Structure):
        _fields_ = [
            ("PerProcessUserTimeLimit", ctypes.c_longlong),
            ("PerJobUserTimeLimit", ctypes.c_longlong),
            ("LimitFlags", wintypes.DWORD),
            ("MinimumWorkingSetSize", ctypes.c_size_t),
            ("MaximumWorkingSetSize", ctypes.c_size_t),
            ("ActiveProcessLimit", wintypes.DWORD),
            ("Affinity", ctypes.c_size_t),
            ("PriorityClass", wintypes.DWORD),
            ("SchedulingClass", wintypes.DWORD),
        ]

    class JOBOBJECT_EXTENDED_LIMIT_INFORMATION(ctypes.Structure):
        _fields_ = [
            ("BasicLimitInformation", JOBOBJECT_BASIC_LIMIT_INFORMATION),
            ("IoInfo", IO_COUNTERS),
            ("ProcessMemoryLimit", ctypes.c_size_t),
            ("JobMemoryLimit", ctypes.c_size_t),
            ("PeakProcessMemoryUsed", ctypes.c_size_t),
            ("PeakJobMemoryUsed", ctypes.c_size_t),
        ]

    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel32.CreateJobObjectW.argtypes = [ctypes.c_void_p, wintypes.LPCWSTR]
    kernel32.CreateJobObjectW.restype = wintypes.HANDLE
    kernel32.SetInformationJobObject.argtypes = [
        wintypes.HANDLE, ctypes.c_int, ctypes.c_void_p, wintypes.DWORD
    ]
    kernel32.SetInformationJobObject.restype = wintypes.BOOL
    kernel32.AssignProcessToJobObject.argtypes = [wintypes.HANDLE, wintypes.HANDLE]
    kernel32.AssignProcessToJobObject.restype = wintypes.BOOL
    kernel32.GetCurrentProcess.argtypes = []
    kernel32.GetCurrentProcess.restype = wintypes.HANDLE

    job = kernel32.CreateJobObjectW(None, None)
    if not job:
        raise ctypes.WinError(ctypes.get_last_error())
    limits = JOBOBJECT_EXTENDED_LIMIT_INFORMATION()
    limits.BasicLimitInformation.LimitFlags = 0x00002000  # JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
    if not kernel32.SetInformationJobObject(
        job, 9, ctypes.byref(limits), ctypes.sizeof(limits)
    ):
        raise ctypes.WinError(ctypes.get_last_error())
    if not kernel32.AssignProcessToJobObject(job, kernel32.GetCurrentProcess()):
        raise ctypes.WinError(ctypes.get_last_error())

process = subprocess.Popen(sys.argv[1:], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
assert process.stdout is not None
while True:
    chunk = os.read(process.stdout.fileno(), 65536)
    if not chunk:
        break
    sys.stdout.buffer.write(chunk)
    sys.stdout.buffer.flush()
returncode = process.wait()
if os.name != "nt" and returncode < 0:
    signal_number = -returncode
    try:
        signal.signal(signal_number, signal.SIG_DFL)
    except (OSError, ValueError):
        pass
    signal.pthread_sigmask(signal.SIG_UNBLOCK, {signal_number})
    os.kill(os.getpid(), signal_number)
    os._exit(128 + signal_number)
raise SystemExit(returncode)
"""
ALLOWED_CHANGES = {
    "Docs/Agents/LOOP_HEARTBEAT.json",
    "Docs/Agents/LOOP_STATUS_2026-08-11.md",
    "Docs/QA/MILESTONE-8.md",
    "Docs/QA/MILESTONE-9.md",
    "Docs/QA/MILESTONE-10.md",
    "Docs/QA/RECOVERY-2026-08-11.md",
    "Docs/Decision-Log.md",
    "Docs/Planning/MILESTONES.md",
    "Docs/Reviews/R0-independent-review.md",
    "Release/README-M10-RC.md",
    "Release/MANIFEST-M10-RC.json",
}


@dataclass
class CommandRecord:
    index: int
    label: str
    command: list[str]
    cwd: str
    started_at: str
    finished_at: str
    exit_code: int | None
    timed_out: bool
    interrupted: bool
    spawn_error: str | None
    timeout_seconds: float
    log: str


class RecoveryFailure(RuntimeError):
    pass


def iso_now() -> str:
    return datetime.now().astimezone().isoformat(timespec="seconds")


def windows_path(path: str | Path) -> Path:
    return Path(path).expanduser().resolve(strict=False)


def canonical_path(path: Path) -> str:
    """Return a symlink/junction-resolved, platform-normalized absolute path."""
    return os.path.normcase(str(path.resolve(strict=False)))


def paths_overlap(first: Path, second: Path) -> bool:
    """Return True when two paths are equal or one is an ancestor of the other."""
    a = canonical_path(first)
    b = canonical_path(second)
    try:
        common = os.path.commonpath((a, b))
    except ValueError:  # different Windows drives
        return False
    return common == a or common == b


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build, test, package, and document the AnimeRPG M10 R0 release candidate."
    )
    parser.add_argument("--source-repository", type=Path)
    parser.add_argument(
        "--worktree",
        type=Path,
        default=Path(r"C:\AI\worktrees\AnimeRPG\r0-loop-recovery-2026-08-11"),
    )
    parser.add_argument(
        "--build-root",
        type=Path,
        default=Path(r"C:\AI\builds\AnimeRPG\r0-loop-recovery-2026-08-11"),
    )
    parser.add_argument(
        "--release-root",
        type=Path,
        default=Path(r"C:\AI\releases\AnimeRPG\2026-08-14-m10-release-candidate"),
    )
    parser.add_argument(
        "--evidence-root",
        type=Path,
        default=Path(r"C:\AI\evidence\AnimeRPG\r0-loop-recovery-2026-08-11"),
    )
    parser.add_argument("--recovery-branch", default=DEFAULT_BRANCH)
    parser.add_argument("--base-ref", default="origin/main")
    parser.add_argument("--skip-fetch", action="store_true")
    parser.add_argument(
        "--command-timeout-seconds",
        type=float,
        default=DEFAULT_COMMAND_TIMEOUT_SECONDS,
        help="Per long-running build/test/package command deadline.",
    )
    parser.add_argument(
        "--capture-timeout-seconds",
        type=float,
        default=DEFAULT_CAPTURE_TIMEOUT_SECONDS,
        help="Per short git/probe command deadline.",
    )
    return parser.parse_args()


class R0Runner:
    def __init__(self, args: argparse.Namespace) -> None:
        script_repo = Path(__file__).resolve().parents[1]
        self.source = windows_path(args.source_repository or script_repo)
        self.worktree = windows_path(args.worktree)
        self.build_root = windows_path(args.build_root)
        self.release_root = windows_path(args.release_root)
        self.evidence_root = windows_path(args.evidence_root)
        self.branch = args.recovery_branch
        self.base_ref = args.base_ref
        self.skip_fetch = args.skip_fetch
        self.command_timeout = float(
            getattr(args, "command_timeout_seconds", DEFAULT_COMMAND_TIMEOUT_SECONDS)
        )
        self.capture_timeout = float(
            getattr(args, "capture_timeout_seconds", DEFAULT_CAPTURE_TIMEOUT_SECONDS)
        )
        if (
            not math.isfinite(self.command_timeout)
            or not math.isfinite(self.capture_timeout)
            or self.command_timeout <= 0
            or self.capture_timeout <= 0
        ):
            raise RecoveryFailure("Command deadlines must be positive finite numbers of seconds.")
        self.heartbeat: Path | None = None
        self.records: list[CommandRecord] = []
        self.last_command: list[str] | None = None
        self.last_log: Path | None = None
        self.run_started_at = iso_now()
        self.run_stamp = datetime.now().strftime("%Y%m%d-%H%M%S")
        self.admitted_revision: str | None = None

    def validate_path_layout(self) -> None:
        named = {
            "source repository": self.source,
            "R0 worktree": self.worktree,
            "build root": self.build_root,
            "release root": self.release_root,
            "evidence root": self.evidence_root,
        }
        items = list(named.items())
        for index, (first_name, first_path) in enumerate(items):
            for second_name, second_path in items[index + 1 :]:
                if paths_overlap(first_path, second_path):
                    raise RecoveryFailure(
                        "Unsafe path layout: "
                        f"{first_name} ({first_path}) overlaps {second_name} ({second_path}). "
                        "Use disjoint roots before retrying."
                    )

    def _popen_isolation(self) -> dict[str, object]:
        if os.name == "nt":
            return {"creationflags": subprocess.CREATE_NEW_PROCESS_GROUP}
        return {"start_new_session": True}

    def _terminate_process_tree(
        self, process: subprocess.Popen[str], *, wait_for_parent: bool = True
    ) -> None:
        if process.poll() is not None:
            return
        if os.name == "nt":
            try:
                subprocess.run(
                    ["taskkill", "/PID", str(process.pid), "/T", "/F"],
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL,
                    check=False,
                    timeout=TERMINATION_GRACE_SECONDS,
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
        if wait_for_parent:
            try:
                process.wait(timeout=TERMINATION_GRACE_SECONDS)
            except (subprocess.TimeoutExpired, OSError):
                pass

    def _wait_process(self, process: subprocess.Popen[str], timeout: float) -> int:
        return process.wait(timeout=timeout)

    @staticmethod
    def _timeout_output(error: subprocess.TimeoutExpired) -> str:
        output = error.output
        if output is None:
            return ""
        if isinstance(output, bytes):
            return output.decode(errors="replace")
        return output

    def _drain_after_timeout(self, process: subprocess.Popen[str]) -> str:
        """Finish pipe handling without ever falling back to an unbounded communicate()."""
        partial = ""
        for attempt in range(2):
            try:
                output, _ = process.communicate(timeout=TERMINATION_GRACE_SECONDS)
                return output or partial
            except subprocess.TimeoutExpired as exc:
                observed = self._timeout_output(exc)
                if observed:
                    partial = observed
                if attempt == 0:
                    try:
                        process.kill()
                    except OSError:
                        pass
                    continue
                raise RecoveryFailure(
                    "Timed-out command cleanup exceeded the bounded pipe-drain budget "
                    f"({2 * TERMINATION_GRACE_SECONDS:g}s)."
                    + (f" Partial output:\n{partial}" if partial else "")
                ) from exc
            except OSError as exc:
                raise RecoveryFailure(
                    f"Timed-out command cleanup failed while draining pipes: {exc}"
                ) from exc
        raise RecoveryFailure("Timed-out command cleanup reached an unreachable state.")

    def capture(
        self,
        command: Sequence[str | Path],
        *,
        cwd: Path | None = None,
        check: bool = True,
        timeout_seconds: float | None = None,
    ) -> subprocess.CompletedProcess[str]:
        normalized = [str(part) for part in command]
        timeout = self.capture_timeout if timeout_seconds is None else float(timeout_seconds)
        if not math.isfinite(timeout) or timeout <= 0:
            raise RecoveryFailure("Capture command has a non-positive timeout or a non-finite timeout.")
        supervised = [sys.executable, "-c", CAPTURE_SUPERVISOR_CODE, *normalized]
        process = subprocess.Popen(
            supervised,
            cwd=str(cwd or self.source),
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            errors="replace",
            **self._popen_isolation(),
        )
        try:
            output, _ = process.communicate(timeout=timeout)
        except subprocess.TimeoutExpired as exc:
            self._terminate_process_tree(process, wait_for_parent=False)
            try:
                output = self._drain_after_timeout(process)
            except RecoveryFailure as cleanup_error:
                raise RecoveryFailure(
                    f"Command timed out after {timeout:g}s and cleanup remained incomplete: "
                    f"{' '.join(normalized)}\n{cleanup_error}"
                ) from exc
            raise RecoveryFailure(
                f"Command timed out after {timeout:g}s: {' '.join(normalized)}\n{output or ''}"
            ) from exc
        except BaseException:
            self._terminate_process_tree(process)
            try:
                process.communicate(timeout=TERMINATION_GRACE_SECONDS)
            except (OSError, subprocess.SubprocessError):
                pass
            raise
        result = subprocess.CompletedProcess(normalized, process.returncode, output, None)
        if check and result.returncode != 0:
            raise RecoveryFailure(
                f"Command failed ({result.returncode}): {' '.join(normalized)}\n{result.stdout}"
            )
        return result

    def current_head(self) -> str | None:
        if not self.worktree.is_dir():
            return None
        result = self.capture(
            ["git", "-C", self.worktree, "rev-parse", "HEAD"],
            cwd=self.worktree,
            check=False,
        )
        if result.returncode != 0:
            return None
        return result.stdout.strip() or None

    def set_heartbeat(
        self,
        status: str,
        *,
        current_command: Sequence[str] | None = None,
        last_result: str | None = None,
        blocker: str | None = None,
        next_action: str | None = None,
    ) -> None:
        if self.heartbeat is None:
            return
        self.heartbeat.parent.mkdir(parents=True, exist_ok=True)
        payload = {
            "loop": "game-dev",
            "task": "R0-loop-recovery-release-candidate",
            "deadline": HISTORICAL_DEADLINE,
            "deadline_is_historical": True,
            "run_started_at": self.run_started_at,
            "status": status,
            "updated_at": iso_now(),
            "worktree": str(self.worktree),
            "branch": self.branch,
            "head": self.current_head(),
            "admitted_revision": self.admitted_revision,
            "pid": os.getpid(),
            "current_command": list(current_command) if current_command else None,
            "last_artifact": str(self.release_root) if self.release_root.exists() else None,
            "last_result": last_result,
            "blocker": blocker,
            "next_action": next_action,
        }
        temporary = self.heartbeat.with_suffix(self.heartbeat.suffix + ".tmp")
        temporary.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
        temporary.replace(self.heartbeat)

    def run_command(
        self,
        label: str,
        command: Sequence[str | Path],
        *,
        cwd: Path | None = None,
        timeout_seconds: float | None = None,
    ) -> None:
        normalized = [str(part) for part in command]
        working_directory = cwd or self.worktree
        timeout = self.command_timeout if timeout_seconds is None else float(timeout_seconds)
        if not math.isfinite(timeout) or timeout <= 0:
            raise RecoveryFailure(f"{label} has a non-positive timeout or a non-finite timeout.")
        index = len(self.records) + 1
        log_path = self.evidence_root / f"{index:02d}-{label}.log"
        self.last_command = normalized
        self.last_log = log_path
        self.set_heartbeat(
            "running",
            current_command=normalized,
            last_result=f"Starting {label}",
            next_action=f"Complete {label}, or stop at its first deterministic failure.",
        )

        started = iso_now()
        finished = started
        exit_code: int | None = None
        timed_out = False
        interrupted = False
        spawn_error: str | None = None
        log_path.parent.mkdir(parents=True, exist_ok=True)
        with log_path.open("w", encoding="utf-8", newline="\n") as log:
            log.write(f"label: {label}\n")
            log.write(f"started_at: {started}\n")
            log.write(f"timeout_seconds: {timeout:g}\n")
            log.write(f"working_directory: {working_directory}\n")
            log.write("command: " + subprocess.list2cmdline(normalized) + "\n\n")
            log.flush()
            try:
                process = subprocess.Popen(
                    normalized,
                    cwd=str(working_directory),
                    text=True,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    errors="replace",
                    bufsize=1,
                    **self._popen_isolation(),
                )
            except (OSError, ValueError) as exc:
                spawn_error = f"{type(exc).__name__}: {exc}"
                finished = iso_now()
                log.write(f"spawn_error: {spawn_error}\n")
                log.write(f"finished_at: {finished}\n")
                log.write("exit_code: None\n")
                log.write("timed_out: false\n")
                log.write("interrupted: false\n")
                log.flush()
                self.records.append(
                    CommandRecord(
                        index=index,
                        label=label,
                        command=normalized,
                        cwd=str(working_directory),
                        started_at=started,
                        finished_at=finished,
                        exit_code=None,
                        timed_out=False,
                        interrupted=False,
                        spawn_error=spawn_error,
                        timeout_seconds=timeout,
                        log=str(log_path),
                    )
                )
                raise RecoveryFailure(
                    f"{label} could not start: {spawn_error}. Inspect {log_path}."
                ) from exc
            assert process.stdout is not None

            def copy_output() -> None:
                try:
                    for line in process.stdout:
                        print(line, end="")
                        log.write(line)
                        log.flush()
                except (OSError, ValueError):
                    return

            reader = threading.Thread(target=copy_output, name=f"r0-log-{label}", daemon=True)
            reader.start()
            try:
                exit_code = self._wait_process(process, timeout)
            except subprocess.TimeoutExpired:
                timed_out = True
                self._terminate_process_tree(process)
                exit_code = process.returncode
            except BaseException:
                interrupted = True
                self._terminate_process_tree(process)
                exit_code = process.returncode
                raise
            finally:
                reader.join(timeout=TERMINATION_GRACE_SECONDS)
                try:
                    process.stdout.close()
                except OSError:
                    pass
                finished = iso_now()
                log.write(f"\nfinished_at: {finished}\n")
                log.write(f"exit_code: {exit_code}\n")
                log.write(f"timed_out: {str(timed_out).lower()}\n")
                log.write(f"interrupted: {str(interrupted).lower()}\n")
                log.write("spawn_error: none\n")
                log.flush()
                self.records.append(
                    CommandRecord(
                        index=index,
                        label=label,
                        command=normalized,
                        cwd=str(working_directory),
                        started_at=started,
                        finished_at=finished,
                        exit_code=exit_code,
                        timed_out=timed_out,
                        interrupted=interrupted,
                        spawn_error=None,
                        timeout_seconds=timeout,
                        log=str(log_path),
                    )
                )
        if timed_out:
            raise RecoveryFailure(f"{label} timed out after {timeout:g}s. Inspect {log_path}.")
        if exit_code != 0:
            raise RecoveryFailure(f"{label} failed with exit code {exit_code}. Inspect {log_path}.")

    def require_tools(self, names: Iterable[str]) -> None:
        missing = [name for name in names if shutil.which(name) is None]
        if missing:
            raise RecoveryFailure("Missing required tools on PATH: " + ", ".join(missing))

    def _archive_target(self, path: Path) -> Path:
        archive = Path(str(path) + f".previous-{self.run_stamp}")
        suffix = 1
        while archive.exists():
            archive = Path(str(path) + f".previous-{self.run_stamp}-{suffix}")
            suffix += 1
        protected = [self.source, self.worktree, self.build_root, self.release_root, self.evidence_root]
        for item in protected:
            if item != path and paths_overlap(archive, item):
                raise RecoveryFailure(f"Archive target {archive} would overlap protected path {item}.")
        return archive

    def archive_directory(self, path: Path, label: str) -> None:
        if not path.exists():
            path.mkdir(parents=True, exist_ok=True)
            return
        if not path.is_dir():
            raise RecoveryFailure(f"{label} exists but is not a directory: {path}")
        if not any(path.iterdir()):
            return
        archive = self._archive_target(path)
        print(f"Preserving existing {label} at {archive}")
        path.rename(archive)
        path.mkdir(parents=True, exist_ok=True)

    def assert_clean(self, repository: Path, label: str) -> None:
        result = self.capture(
            ["git", "-C", repository, "status", "--porcelain", "--untracked-files=all"],
            cwd=repository,
        )
        if result.stdout.strip():
            raise RecoveryFailure(
                f"{label} has unexplained changes:\n{result.stdout}\n"
                "R0 is one-shot. Preserve and review the existing checkpoint, then use a new clean "
                "worktree/output set for a new admitted revision. Do not discard or overwrite evidence."
            )

    def establish_worktree(self) -> None:
        self.assert_clean(self.source, "Source repository")
        if not self.skip_fetch:
            self.capture(["git", "-C", self.source, "fetch", "--all", "--prune"])

        base_head = self.capture(
            ["git", "-C", self.source, "rev-parse", self.base_ref]
        ).stdout.strip()
        ancestor = self.capture(
            ["git", "-C", self.source, "merge-base", "--is-ancestor", REQUIRED_BASE, base_head],
            check=False,
        )
        if ancestor.returncode != 0:
            raise RecoveryFailure(
                f"{self.base_ref} does not contain required M10 baseline {REQUIRED_BASE}."
            )
        self.admitted_revision = base_head

        branch_ref = f"refs/heads/{self.branch}"
        branch_exists = self.capture(
            ["git", "-C", self.source, "show-ref", "--verify", "--quiet", branch_ref],
            check=False,
        ).returncode == 0
        if branch_exists:
            branch_head = self.capture(
                ["git", "-C", self.source, "rev-parse", branch_ref]
            ).stdout.strip()
            if branch_head != base_head:
                raise RecoveryFailure(
                    f"Recovery branch {self.branch} is stale at {branch_head}; admitted revision is "
                    f"{base_head}. Preserve the old checkpoint and use a new branch/worktree."
                )

        if not self.worktree.exists():
            self.worktree.parent.mkdir(parents=True, exist_ok=True)
            if branch_exists:
                command = ["git", "-C", self.source, "worktree", "add", self.worktree, self.branch]
            else:
                command = [
                    "git", "-C", self.source, "worktree", "add", "-b", self.branch,
                    self.worktree, base_head,
                ]
            self.capture(command)
        elif not self.worktree.is_dir():
            raise RecoveryFailure(f"R0 worktree path exists but is not a directory: {self.worktree}")

        top = windows_path(
            self.capture(
                ["git", "-C", self.worktree, "rev-parse", "--show-toplevel"], cwd=self.worktree
            ).stdout.strip()
        )
        if canonical_path(top) != canonical_path(self.worktree):
            raise RecoveryFailure(f"Configured R0 worktree is not its Git top level: {self.worktree}")
        branch = self.capture(
            ["git", "-C", self.worktree, "branch", "--show-current"], cwd=self.worktree
        ).stdout.strip()
        if branch != self.branch:
            raise RecoveryFailure(f"R0 worktree is on {branch!r}, expected {self.branch!r}.")
        self.assert_clean(self.worktree, "R0 worktree")
        actual_head = self.current_head()
        if actual_head != base_head:
            raise RecoveryFailure(
                f"R0 worktree HEAD {actual_head} does not match admitted revision {base_head}. "
                "Preserve the old checkpoint and use a new clean branch/worktree."
            )
        self.heartbeat = self.worktree / "Docs" / "Agents" / "LOOP_HEARTBEAT.json"

    def write_milestone_report(
        self, number: int, capability: str, domain_test: str, smoke_test: str, head: str
    ) -> Path:
        path = self.worktree / "Docs" / "QA" / f"MILESTONE-{number}.md"
        path.parent.mkdir(parents=True, exist_ok=True)
        content = f"""# Implementation M{number} QA - {capability}

Recovery run started: {self.run_started_at}  
Historical filename/date lineage: August 11, 2026  
Verified commit: {head}

## Scope

This report covers the later implementation task labeled M{number}: **{capability}**. It does not redefine the original product-backlog milestone numbering.

## Fresh recovery evidence

- Fresh Visual Studio 2022 x64 Debug and Release builds completed successfully.
- Complete Debug and Release CTest runs passed, including {domain_test} and {smoke_test}.
- The packaged M10 candidate passed its native package smoke from the delivery directory.
- Command logs and exit codes are stored under {self.evidence_root}.

## Not claimed

This recovery does not claim clean-machine dependency acceptance, the full RPG, final art, complete National Mall, Shadow Summon, Enemy Set, dungeon backlog, or engine parity.

## Result

**PASS for the bounded implementation M{number} contract, pending independent R0 package review.**
"""
        path.write_text(content, encoding="utf-8")
        return path

    def append_once(self, path: Path, marker: str, text: str) -> None:
        current = path.read_text(encoding="utf-8")
        if marker not in current:
            with path.open("a", encoding="utf-8", newline="\n") as stream:
                stream.write("\n" + text.strip() + "\n")

    def write_recovery_documents(self, head: str) -> Path:
        self.write_milestone_report(8, "Thought Commands", "ThoughtCommandsTests", "M8RuntimeSmoke", head)
        self.write_milestone_report(9, "Landmark Interaction", "LandmarkInteractionTests", "M9RuntimeSmoke", head)
        self.write_milestone_report(10, "Landmark Encounter Loop", "LandmarkEncounterTests", "M10RuntimeSmoke", head)

        recovery = self.worktree / "Docs" / "QA" / "RECOVERY-2026-08-11.md"
        recovery_content = f"""# R0 Recovery Evidence

Historical filename retained from August 11, 2026.  
Recovery run started: {self.run_started_at}  
Verified commit: {head}  
Worktree: {self.worktree}  
External build tree: {self.build_root}  
Release package: {self.release_root}

## Automated gates completed

1. Clean-source, exact-revision, path-layout, and dedicated-worktree checks.
2. Visual Studio 2022 x64 configure.
3. Debug and Release builds with complete local CTest runs.
4. Static milestone verifiers and git diff checks.
5. Package construction, SHA-256 manifest, and native M10 package smoke.

All command logs, timestamps, deadlines, and exit codes are stored under {self.evidence_root}.

## Remaining gates

A separate reviewer must verify provenance, logs, manifest accuracy, allowed-file compliance, package launch evidence, unsupported-claim absence, and clean-machine runtime dependency requirements. This run does not establish UE5/Unity parity.
"""
        recovery.write_text(recovery_content, encoding="utf-8")

        review = self.worktree / "Docs" / "Reviews" / "R0-independent-review.md"
        review.parent.mkdir(parents=True, exist_ok=True)
        review_content = f"""# R0 Independent Review

Status: **PENDING INDEPENDENT REVIEW**  
Run started: {self.run_started_at}  
Candidate commit: {head}  
Candidate package: {self.release_root}

- [ ] allowed-file compliance
- [ ] exact admitted revision and package provenance
- [ ] Debug and Release evidence
- [ ] complete native RuntimeSmoke evidence
- [ ] package-specific M10 smoke evidence
- [ ] SHA-256 manifest accuracy
- [ ] launch instructions, controls, and limitations
- [ ] clean-machine runtime dependency verification
- [ ] no hidden source or dependency change
- [ ] milestone-number reconciliation
- [ ] no unsupported product or engine-parity claim

Recommendation: **PENDING**

The coordinator must not change the loop from review to complete until a separate reviewer records an evidence-backed recommendation here.
"""
        review.write_text(review_content, encoding="utf-8")

        decision_marker = f"## R0 M10 release-candidate recovery: {head}"
        decision_text = f"""{decision_marker}

Run started: {self.run_started_at}.

Decision: verify and package the accepted custom C++17 M10 baseline before starting another feature. The recovery uses an exact admitted revision, dedicated worktree, disjoint external outputs, and bounded child commands.

Evidence: Debug and Release builds, complete local CTest runs including native RuntimeSmoke tests, static verifiers, package-specific M10 smoke, and a SHA-256 manifest passed for commit {head}.

Consequence: the loop moves to review. No new feature begins before independent acceptance; clean-machine dependency verification remains separate.
"""
        self.append_once(self.worktree / "Docs" / "Decision-Log.md", decision_marker, decision_text)

        milestone_marker = "## Later implementation-label reconciliation"
        milestone_text = f"""{milestone_marker}

The staged backlog remains the product roadmap. Later implementation-task numbering does not rewrite it.

| Later implementation label | Delivered bounded capability | Product-backlog relationship |
|---|---|---|
| implementation M8 | Thought Commands | Partial bounded implementation of backlog M8 |
| implementation M9 | Landmark Interaction | Additional vertical-slice capability, not backlog M9 Shadow Summon |
| implementation M10 | Landmark Encounter Loop | Additional vertical-slice capability, not backlog M10 Enemy Set |

The R0 package represents the current implementation M10 baseline. Shadow Summon, Enemy Set, dungeon work, final art, and full National Mall completion remain future work unless separately verified.
"""
        self.append_once(self.worktree / "Docs" / "Planning" / "MILESTONES.md", milestone_marker, milestone_text)
        return recovery

    def write_package_readme(self, head: str) -> Path:
        path = self.release_root / "README-M10-RC.md"
        content = f"""# AstralGame M10 Release Candidate

Build run started: {self.run_started_at}  
Verified commit: {head}

## Launch

Run AstralGame.exe from this directory on the tested Windows host. Clean-machine MSVC runtime/dependency acceptance remains pending independent packaging verification.

## Controls

- W, A, S, D: movement
- J, K: light and heavy attacks
- Q: Shadow Dash
- L: Fatal Strike
- left Shift: Guard
- 1 through 5: bounded Thought Commands
- E: landmark interaction
- Escape: exit

## Verified scope

This is the bounded custom C++17 Win32/GDI prototype through implementation M10. It is not a claim that the full RPG, production engine, clean-machine package, or UE5/Unity parity is finished.

## Evidence

See RECOVERY-2026-08-11.md, R0-COMMAND-EVIDENCE.json, the evidence directory, and MANIFEST-M10-RC.json.
"""
        path.write_text(content, encoding="utf-8")
        return path

    def write_manifest(self, head: str) -> Path:
        manifest_path = self.release_root / "MANIFEST-M10-RC.json"
        entries = []
        for file in sorted(path for path in self.release_root.rglob("*") if path.is_file()):
            if file == manifest_path:
                continue
            entries.append(
                {
                    "path": file.relative_to(self.release_root).as_posix(),
                    "bytes": file.stat().st_size,
                    "sha256": hashlib.sha256(file.read_bytes()).hexdigest(),
                }
            )
        manifest_path.write_text(
            json.dumps(
                {
                    "generated_at": iso_now(),
                    "run_started_at": self.run_started_at,
                    "commit": head,
                    "package_root": str(self.release_root),
                    "files": entries,
                },
                indent=2,
            ) + "\n",
            encoding="utf-8",
        )
        return manifest_path

    def assert_allowed_changes(self) -> None:
        result = self.capture(
            ["git", "-C", self.worktree, "status", "--porcelain", "--untracked-files=all"],
            cwd=self.worktree,
        )
        unexpected = []
        for line in result.stdout.splitlines():
            if not line.strip():
                continue
            path = line[3:].strip().strip('"').replace("\\", "/")
            if " -> " in path:
                path = path.split(" -> ")[-1]
            if path not in ALLOWED_CHANGES:
                unexpected.append(line)
        if unexpected:
            raise RecoveryFailure("Unexpected repository changes were detected:\n" + "\n".join(unexpected))

    def execute(self) -> None:
        self.validate_path_layout()
        if os.name != "nt":
            raise RecoveryFailure("R0 must run on the registered Windows Agent Studio host.")
        self.require_tools(("git", "cmake", "ctest", "python"))
        if not self.source.is_dir():
            raise RecoveryFailure(f"Source repository does not exist: {self.source}")

        self.establish_worktree()
        head = self.current_head()
        if head is None or head != self.admitted_revision:
            raise RecoveryFailure("Unable to establish the exact admitted R0 worktree revision.")
        self.set_heartbeat(
            "running",
            last_result=f"Dedicated clean worktree established at admitted revision {head}",
            next_action="Run the bounded Debug and Release gates, then package the candidate.",
        )

        self.archive_directory(self.build_root, "build tree")
        self.archive_directory(self.release_root, "release tree")
        self.archive_directory(self.evidence_root, "evidence tree")

        for label, command in (
            ("git-version", ["git", "--version"]),
            ("cmake-version", ["cmake", "--version"]),
            ("ctest-version", ["ctest", "--version"]),
            ("python-version", ["python", "--version"]),
        ):
            self.run_command(label, command, cwd=self.worktree, timeout_seconds=self.capture_timeout)

        self.run_command(
            "configure-vs2022-x64",
            ["cmake", "-S", self.worktree, "-B", self.build_root, "-G", "Visual Studio 17 2022", "-A", "x64"],
        )
        self.run_command("build-debug", ["cmake", "--build", self.build_root, "--config", "Debug", "--parallel"])
        self.run_command(
            "ctest-debug-full",
            ["ctest", "--test-dir", self.build_root, "-C", "Debug", "--output-on-failure", "--no-tests=error"],
        )
        self.run_command("build-release", ["cmake", "--build", self.build_root, "--config", "Release", "--parallel"])
        self.run_command(
            "ctest-release-full",
            ["ctest", "--test-dir", self.build_root, "-C", "Release", "--output-on-failure", "--no-tests=error"],
        )
        for number in (1, 2, 3):
            self.run_command(f"verify-milestone-{number}", ["python", f"Scripts/verify_milestone{number}.py"])
        self.run_command("git-diff-check-prepackage", ["git", "diff", "--check"])

        release_exe = self.build_root / "Release" / "AstralGame.exe"
        package_smoke = self.build_root / "Release" / "M10RuntimeSmoke.exe"
        if not release_exe.is_file() or not package_smoke.is_file():
            raise RecoveryFailure(f"Release output is incomplete: {release_exe} or {package_smoke} is missing.")
        shutil.copy2(release_exe, self.release_root / "AstralGame.exe")
        self.write_package_readme(head)
        self.run_command(
            "m10-package-runtime-smoke",
            [package_smoke, self.release_root / "AstralGame.exe", self.release_root],
            cwd=self.release_root,
        )

        recovery = self.write_recovery_documents(head)
        shutil.copy2(recovery, self.release_root / recovery.name)
        self.run_command("git-diff-check-final", ["git", "diff", "--check"])
        self.assert_allowed_changes()

        command_evidence = self.release_root / "R0-COMMAND-EVIDENCE.json"
        command_evidence.write_text(
            json.dumps(
                {
                    "generated_at": iso_now(),
                    "run_started_at": self.run_started_at,
                    "historical_deadline": HISTORICAL_DEADLINE,
                    "commit": head,
                    "admitted_revision": self.admitted_revision,
                    "base_ref": self.base_ref,
                    "worktree": str(self.worktree),
                    "build_root": str(self.build_root),
                    "release_root": str(self.release_root),
                    "evidence_root": str(self.evidence_root),
                    "command_timeout_seconds": self.command_timeout,
                    "capture_timeout_seconds": self.capture_timeout,
                    "commands": [asdict(record) for record in self.records],
                },
                indent=2,
            ) + "\n",
            encoding="utf-8",
        )
        package_evidence = self.release_root / "evidence"
        package_evidence.mkdir(parents=True, exist_ok=True)
        for log in self.evidence_root.glob("*.log"):
            shutil.copy2(log, package_evidence / log.name)

        manifest = self.write_manifest(head)
        repo_release = self.worktree / "Release"
        repo_release.mkdir(parents=True, exist_ok=True)
        shutil.copy2(self.release_root / "README-M10-RC.md", repo_release / "README-M10-RC.md")
        shutil.copy2(manifest, repo_release / "MANIFEST-M10-RC.json")
        self.assert_allowed_changes()

        self.set_heartbeat(
            "review",
            last_result=(
                "R0 automated build, complete native tests, evidence reconciliation, packaging, "
                f"manifest, and package smoke passed for admitted revision {head}. Candidate: {self.release_root}"
            ),
            blocker="Independent review and clean-machine runtime dependency verification are still required.",
            next_action=(
                "Assign one independent reviewer to inspect Docs/Reviews/R0-independent-review.md, the command logs, "
                "package manifest, and runtime dependencies. Commit accepted evidence; do not rerun this dirty worktree in place."
            ),
        )
        print("\nR0 AUTOMATED GATE: PASS")
        print(f"Candidate package: {self.release_root}")
        print(f"Evidence: {self.evidence_root}")
        print(f"Worktree: {self.worktree}")
        print("Loop status: review. Independent acceptance remains required.")


def main() -> int:
    runner: R0Runner | None = None
    try:
        runner = R0Runner(parse_args())
        runner.execute()
        return 0
    except Exception as exc:
        if runner is not None:
            try:
                runner.set_heartbeat(
                    "blocked",
                    current_command=runner.last_command,
                    last_result=(
                        "R0 stopped at the first deterministic failure. "
                        f"Latest log: {runner.last_log}"
                    ),
                    blocker=str(exc),
                    next_action=(
                        "Inspect the named log and preserve the current checkpoint. Change one material condition before retrying. "
                        "If tracked evidence exists, use a new clean worktree/output set rather than overwriting or discarding it."
                    ),
                )
            except Exception as heartbeat_error:
                print(f"WARNING: heartbeat update failed: {heartbeat_error}", file=sys.stderr)
        print(f"R0 AUTOMATED GATE: BLOCKED\n{exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
