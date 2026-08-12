#!/usr/bin/env python3
"""Run the bounded R0 recovery and package gate on the Windows Agent Studio host.

This script deliberately does not modify Engine/, Game/, Tests/, or CMakeLists.txt.
It stops at the first deterministic failure and leaves the loop in review after the
automated gate. A separate reviewer must still accept the evidence and package.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
from dataclasses import asdict, dataclass
from datetime import datetime
from pathlib import Path
from typing import Iterable, Sequence

DEADLINE = "2026-08-14T23:59:59-04:00"
REQUIRED_BASE = "181298f69b42899db83040d2d5b6c3e67804e242"
DEFAULT_BRANCH = "agent/r0-loop-recovery-2026-08-11"
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
    exit_code: int
    log: str


class RecoveryFailure(RuntimeError):
    pass


def iso_now() -> str:
    return datetime.now().astimezone().isoformat(timespec="seconds")


def windows_path(path: str | Path) -> Path:
    return Path(path).expanduser().resolve()


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
        self.heartbeat: Path | None = None
        self.records: list[CommandRecord] = []
        self.last_command: list[str] | None = None
        self.last_log: Path | None = None
        self.run_stamp = datetime.now().strftime("%Y%m%d-%H%M%S")

    def capture(
        self,
        command: Sequence[str | Path],
        *,
        cwd: Path | None = None,
        check: bool = True,
    ) -> subprocess.CompletedProcess[str]:
        normalized = [str(part) for part in command]
        result = subprocess.run(
            normalized,
            cwd=str(cwd or self.source),
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            errors="replace",
            check=False,
        )
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
            "deadline": DEADLINE,
            "status": status,
            "updated_at": iso_now(),
            "worktree": str(self.worktree),
            "branch": self.branch,
            "head": self.current_head(),
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
    ) -> None:
        normalized = [str(part) for part in command]
        working_directory = cwd or self.worktree
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
        log_path.parent.mkdir(parents=True, exist_ok=True)
        with log_path.open("w", encoding="utf-8", newline="\n") as log:
            log.write(f"label: {label}\n")
            log.write(f"started_at: {started}\n")
            log.write(f"working_directory: {working_directory}\n")
            log.write("command: " + subprocess.list2cmdline(normalized) + "\n\n")
            process = subprocess.Popen(
                normalized,
                cwd=str(working_directory),
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                errors="replace",
            )
            assert process.stdout is not None
            for line in process.stdout:
                print(line, end="")
                log.write(line)
            exit_code = process.wait()
            finished = iso_now()
            log.write(f"\nfinished_at: {finished}\nexit_code: {exit_code}\n")

        self.records.append(
            CommandRecord(
                index=index,
                label=label,
                command=normalized,
                cwd=str(working_directory),
                started_at=started,
                finished_at=finished,
                exit_code=exit_code,
                log=str(log_path),
            )
        )
        if exit_code != 0:
            raise RecoveryFailure(
                f"{label} failed with exit code {exit_code}. Inspect {log_path}."
            )

    def require_tools(self, names: Iterable[str]) -> None:
        missing = [name for name in names if shutil.which(name) is None]
        if missing:
            raise RecoveryFailure("Missing required tools on PATH: " + ", ".join(missing))

    def archive_directory(self, path: Path, label: str) -> None:
        if not path.exists():
            path.mkdir(parents=True, exist_ok=True)
            return
        if not any(path.iterdir()):
            return
        archive = Path(str(path) + f".previous-{self.run_stamp}")
        suffix = 1
        while archive.exists():
            archive = Path(str(path) + f".previous-{self.run_stamp}-{suffix}")
            suffix += 1
        print(f"Preserving existing {label} at {archive}")
        path.rename(archive)
        path.mkdir(parents=True, exist_ok=True)

    def assert_clean(self, repository: Path, label: str) -> None:
        result = self.capture(
            ["git", "-C", repository, "status", "--porcelain", "--untracked-files=all"],
            cwd=repository,
        )
        if result.stdout.strip():
            raise RecoveryFailure(f"{label} has unexplained changes:\n{result.stdout}")

    def establish_worktree(self) -> None:
        self.assert_clean(self.source, "Source repository")
        if not self.skip_fetch:
            self.capture(["git", "-C", self.source, "fetch", "--all", "--prune"])

        base_head = self.capture(
            ["git", "-C", self.source, "rev-parse", self.base_ref]
        ).stdout.strip()
        ancestor = self.capture(
            [
                "git",
                "-C",
                self.source,
                "merge-base",
                "--is-ancestor",
                REQUIRED_BASE,
                base_head,
            ],
            check=False,
        )
        if ancestor.returncode != 0:
            raise RecoveryFailure(
                f"{self.base_ref} does not contain required M10 baseline {REQUIRED_BASE}."
            )

        if not self.worktree.exists():
            self.worktree.parent.mkdir(parents=True, exist_ok=True)
            branch_exists = self.capture(
                [
                    "git",
                    "-C",
                    self.source,
                    "show-ref",
                    "--verify",
                    "--quiet",
                    f"refs/heads/{self.branch}",
                ],
                check=False,
            ).returncode == 0
            if branch_exists:
                command = ["git", "-C", self.source, "worktree", "add", self.worktree, self.branch]
            else:
                command = [
                    "git",
                    "-C",
                    self.source,
                    "worktree",
                    "add",
                    "-b",
                    self.branch,
                    self.worktree,
                    self.base_ref,
                ]
            self.capture(command)

        branch = self.capture(
            ["git", "-C", self.worktree, "branch", "--show-current"], cwd=self.worktree
        ).stdout.strip()
        if branch != self.branch:
            raise RecoveryFailure(
                f"R0 worktree is on {branch!r}, expected {self.branch!r}."
            )
        self.assert_clean(self.worktree, "R0 worktree")
        self.heartbeat = self.worktree / "Docs" / "Agents" / "LOOP_HEARTBEAT.json"

    def write_milestone_report(
        self,
        number: int,
        capability: str,
        domain_test: str,
        smoke_test: str,
        head: str,
    ) -> Path:
        path = self.worktree / "Docs" / "QA" / f"MILESTONE-{number}.md"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(
            f"""# Implementation M{number} QA - {capability}

Recovery date: August 11, 2026  
Verified commit: {head}

## Scope

This report covers the later implementation task labeled M{number}: **{capability}**. It does not redefine the original product-backlog milestone numbering.

## Fresh recovery evidence

- Fresh Visual Studio 2022 x64 Debug and Release builds completed successfully.
- Complete Debug and Release CTest runs passed, including {domain_test} and {smoke_test}.
- The packaged M10 candidate passed its native package smoke from the delivery directory.
- Command logs and exit codes are stored under {self.evidence_root}.

## Not claimed

This recovery does not claim that the full RPG, final art, complete National Mall, Shadow Summon, Enemy Set, or dungeon backlog is complete.

## Result

**PASS for the bounded implementation M{number} contract, pending independent R0 package review.**
""",
            encoding="utf-8",
        )
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
        recovery.write_text(
            f"""# R0 Recovery Evidence - August 11, 2026

Verified commit: {head}  
Worktree: {self.worktree}  
External build tree: {self.build_root}  
Release package: {self.release_root}

## Automated gates completed

1. Clean-source and dedicated-worktree checks.
2. Visual Studio 2022 x64 configure.
3. Debug and Release builds with complete local CTest runs.
4. Static milestone verifiers and git diff checks.
5. Package construction, SHA-256 manifest, and native M10 package smoke.

All command logs, timestamps, and exit codes are stored under {self.evidence_root}.

## Milestone-label reconciliation

- implementation M8: Thought Commands;
- implementation M9: Landmark Interaction;
- implementation M10: Landmark Encounter Loop.

These implementation labels do not replace product-backlog M9 Shadow Summon, product-backlog M10 Enemy Set, or later dungeon work.

## Remaining gate

A separate reviewer must verify provenance, logs, manifest accuracy, allowed-file compliance, package launch evidence, and unsupported-claim absence before the heartbeat can become complete.
""",
            encoding="utf-8",
        )

        review = self.worktree / "Docs" / "Reviews" / "R0-independent-review.md"
        review.parent.mkdir(parents=True, exist_ok=True)
        review.write_text(
            f"""# R0 Independent Review

Status: **PENDING INDEPENDENT REVIEW**  
Candidate commit: {head}  
Candidate package: {self.release_root}

- [ ] allowed-file compliance
- [ ] exact commit and package provenance
- [ ] Debug and Release evidence
- [ ] complete native RuntimeSmoke evidence
- [ ] package-specific M10 smoke evidence
- [ ] SHA-256 manifest accuracy
- [ ] launch instructions, controls, and limitations
- [ ] no hidden source or dependency change
- [ ] milestone-number reconciliation
- [ ] no unsupported product claim

Recommendation: **PENDING**

The coordinator must not change the loop from review to complete until a separate reviewer records an evidence-backed recommendation here.
""",
            encoding="utf-8",
        )

        decision_marker = "## 2026-08-11 - R0 M10 release-candidate recovery"
        self.append_once(
            self.worktree / "Docs" / "Decision-Log.md",
            decision_marker,
            f"""{decision_marker}

Decision: Verify and package the accepted custom C++17 M10 baseline before starting another feature. The recovery uses a dedicated worktree and external build tree and does not modify Engine, Game, Tests, or CMakeLists.txt.

Evidence: Fresh Debug and Release builds, complete local CTest runs including native RuntimeSmoke tests, static verifiers, package-specific M10 smoke, and a SHA-256 manifest passed for commit {head}.

Milestone reconciliation: implementation M8 is Thought Commands, implementation M9 is Landmark Interaction, and implementation M10 is Landmark Encounter Loop. These labels do not replace product-backlog M9 Shadow Summon or product-backlog M10 Enemy Set.

Consequence: the loop moves to review. No new feature begins before independent acceptance.
""",
        )

        milestone_marker = "## Later implementation-label reconciliation"
        self.append_once(
            self.worktree / "Docs" / "Planning" / "MILESTONES.md",
            milestone_marker,
            f"""{milestone_marker}

The staged backlog remains the product roadmap. Later implementation-task numbering does not rewrite it.

| Later implementation label | Delivered bounded capability | Product-backlog relationship |
|---|---|---|
| implementation M8 | Thought Commands | Partial bounded implementation of backlog M8 |
| implementation M9 | Landmark Interaction | Additional vertical-slice capability, not backlog M9 Shadow Summon |
| implementation M10 | Landmark Encounter Loop | Additional vertical-slice capability, not backlog M10 Enemy Set |

The R0 package represents the current implementation M10 baseline. Shadow Summon, Enemy Set, dungeon work, final art, and full National Mall completion remain future work unless separately verified.
""",
        )
        return recovery

    def write_package_readme(self, head: str) -> Path:
        path = self.release_root / "README-M10-RC.md"
        path.write_text(
            f"""# AstralGame M10 Release Candidate

Build date: August 11, 2026  
Verified commit: {head}

## Launch

Run AstralGame.exe from this directory on Windows 10 or Windows 11.

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

This is the bounded custom C++17 Win32/GDI prototype through implementation M10. It is not a claim that the full RPG, final art, complete National Mall, dungeon set, or production engine is finished.

## Evidence

See RECOVERY-2026-08-11.md, R0-COMMAND-EVIDENCE.json, the evidence directory, and MANIFEST-M10-RC.json.
""",
            encoding="utf-8",
        )
        return path

    def write_manifest(self) -> Path:
        manifest_path = self.release_root / "MANIFEST-M10-RC.json"
        entries = []
        for file in sorted(path for path in self.release_root.rglob("*") if path.is_file()):
            if file == manifest_path:
                continue
            digest = hashlib.sha256(file.read_bytes()).hexdigest()
            entries.append(
                {
                    "path": file.relative_to(self.release_root).as_posix(),
                    "bytes": file.stat().st_size,
                    "sha256": digest,
                }
            )
        manifest_path.write_text(
            json.dumps(
                {
                    "generated_at": iso_now(),
                    "package_root": str(self.release_root),
                    "files": entries,
                },
                indent=2,
            )
            + "\n",
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
            raise RecoveryFailure(
                "Unexpected repository changes were detected:\n" + "\n".join(unexpected)
            )

    def execute(self) -> None:
        if os.name != "nt":
            raise RecoveryFailure("R0 must run on the Windows Agent Studio host.")
        self.require_tools(("git", "cmake", "ctest", "python"))
        if not self.source.is_dir():
            raise RecoveryFailure(f"Source repository does not exist: {self.source}")

        self.establish_worktree()
        head = self.current_head()
        if head is None:
            raise RecoveryFailure("Unable to resolve the R0 worktree commit.")
        self.set_heartbeat(
            "running",
            last_result=f"Dedicated clean worktree established at {head}",
            next_action="Run the fresh Debug and Release gates, then package the candidate.",
        )

        self.archive_directory(self.build_root, "build tree")
        self.archive_directory(self.release_root, "release tree")
        self.archive_directory(self.evidence_root, "evidence tree")

        self.run_command(
            "configure-vs2022-x64",
            [
                "cmake",
                "-S",
                self.worktree,
                "-B",
                self.build_root,
                "-G",
                "Visual Studio 17 2022",
                "-A",
                "x64",
            ],
        )
        self.run_command(
            "build-debug",
            ["cmake", "--build", self.build_root, "--config", "Debug", "--parallel"],
        )
        self.run_command(
            "ctest-debug-full",
            ["ctest", "--test-dir", self.build_root, "-C", "Debug", "--output-on-failure"],
        )
        self.run_command(
            "build-release",
            ["cmake", "--build", self.build_root, "--config", "Release", "--parallel"],
        )
        self.run_command(
            "ctest-release-full",
            ["ctest", "--test-dir", self.build_root, "-C", "Release", "--output-on-failure"],
        )
        for number in (1, 2, 3):
            self.run_command(
                f"verify-milestone-{number}",
                ["python", f"Scripts/verify_milestone{number}.py"],
            )
        self.run_command("git-diff-check-prepackage", ["git", "diff", "--check"])

        release_exe = self.build_root / "Release" / "AstralGame.exe"
        package_smoke = self.build_root / "Release" / "M10RuntimeSmoke.exe"
        if not release_exe.is_file() or not package_smoke.is_file():
            raise RecoveryFailure(
                f"Release output is incomplete: {release_exe} or {package_smoke} is missing."
            )
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
                    "commit": head,
                    "worktree": str(self.worktree),
                    "build_root": str(self.build_root),
                    "release_root": str(self.release_root),
                    "commands": [asdict(record) for record in self.records],
                },
                indent=2,
            )
            + "\n",
            encoding="utf-8",
        )
        package_evidence = self.release_root / "evidence"
        package_evidence.mkdir(parents=True, exist_ok=True)
        for log in self.evidence_root.glob("*.log"):
            shutil.copy2(log, package_evidence / log.name)

        manifest = self.write_manifest()
        repo_release = self.worktree / "Release"
        repo_release.mkdir(parents=True, exist_ok=True)
        shutil.copy2(self.release_root / "README-M10-RC.md", repo_release / "README-M10-RC.md")
        shutil.copy2(manifest, repo_release / "MANIFEST-M10-RC.json")
        self.assert_allowed_changes()

        self.set_heartbeat(
            "review",
            last_result=(
                "R0 automated build, complete native tests, evidence reconciliation, "
                f"packaging, manifest, and package smoke passed. Candidate: {self.release_root}"
            ),
            blocker="Independent review is still required before completion.",
            next_action=(
                "Assign one independent reviewer to inspect Docs/Reviews/R0-independent-review.md, "
                "the command logs, and the package manifest, then commit accepted evidence."
            ),
        )
        print("\nR0 AUTOMATED GATE: PASS")
        print(f"Candidate package: {self.release_root}")
        print(f"Evidence: {self.evidence_root}")
        print(f"Worktree: {self.worktree}")
        print("Loop status: review. Independent acceptance remains required.")


def main() -> int:
    runner = R0Runner(parse_args())
    try:
        runner.execute()
        return 0
    except Exception as exc:  # exact blocker is persisted before exit
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
                    "Inspect the named log, change one material condition, and resume from the "
                    "preserved checkpoint. Do not repeat the identical failed command."
                ),
            )
        except Exception as heartbeat_error:
            print(f"WARNING: heartbeat update failed: {heartbeat_error}", file=sys.stderr)
        print(f"R0 AUTOMATED GATE: BLOCKED\n{exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
