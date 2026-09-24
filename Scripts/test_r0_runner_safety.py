#!/usr/bin/env python3
from __future__ import annotations

import argparse
import importlib.util
import os
import subprocess
import sys
import tempfile
import time
import unittest
from pathlib import Path
from unittest import mock

SCRIPT = Path(__file__).with_name("invoke_r0_release_candidate.py")
spec = importlib.util.spec_from_file_location("r0_runner", SCRIPT)
assert spec and spec.loader
r0 = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = r0
spec.loader.exec_module(r0)


def git(*args: str, cwd: Path) -> str:
    result = subprocess.run(
        ["git", *args], cwd=cwd, text=True, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, check=True
    )
    return result.stdout.strip()


def make_args(root: Path, *, branch: str = "test/r0", base_ref: str = "HEAD") -> argparse.Namespace:
    return argparse.Namespace(
        source_repository=root / "source",
        worktree=root / "worktree",
        build_root=root / "build",
        release_root=root / "release",
        evidence_root=root / "evidence",
        recovery_branch=branch,
        base_ref=base_ref,
        skip_fetch=True,
        command_timeout_seconds=1.0,
        capture_timeout_seconds=1.0,
    )


def init_repo(path: Path) -> tuple[str, str]:
    path.mkdir(parents=True)
    git("init", cwd=path)
    git("config", "user.email", "r0-test@example.invalid", cwd=path)
    git("config", "user.name", "R0 Safety Test", cwd=path)
    (path / "tracked.txt").write_text("first\n", encoding="utf-8")
    git("add", "tracked.txt", cwd=path)
    git("commit", "-m", "first", cwd=path)
    first = git("rev-parse", "HEAD", cwd=path)
    (path / "tracked.txt").write_text("second\n", encoding="utf-8")
    git("commit", "-am", "second", cwd=path)
    second = git("rev-parse", "HEAD", cwd=path)
    return first, second


class R0RunnerSafetyTests(unittest.TestCase):
    def test_rejects_source_output_overlap_before_mutation(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            args = make_args(root)
            args.source_repository.mkdir()
            marker = args.source_repository / "keep.txt"
            marker.write_text("keep", encoding="utf-8")
            args.build_root = args.source_repository / "build"
            runner = r0.R0Runner(args)
            with self.assertRaisesRegex(r0.RecoveryFailure, "Unsafe path layout"):
                runner.validate_path_layout()
            self.assertEqual(marker.read_text(encoding="utf-8"), "keep")
            self.assertFalse(args.build_root.exists())

    def test_rejects_output_root_ancestor_descendant_overlap(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            args = make_args(root)
            args.release_root = args.build_root / "release"
            runner = r0.R0Runner(args)
            with self.assertRaisesRegex(r0.RecoveryFailure, "build root.*overlaps release root"):
                runner.validate_path_layout()

    def test_every_configured_root_pair_rejects_equality_and_nesting(self) -> None:
        names = ["source_repository", "worktree", "build_root", "release_root", "evidence_root"]
        for first_index, first in enumerate(names):
            for second in names[first_index + 1 :]:
                with self.subTest(first=first, second=second, relation="equal"):
                    with tempfile.TemporaryDirectory() as temp:
                        root = Path(temp)
                        args = make_args(root)
                        shared = root / "shared"
                        setattr(args, first, shared)
                        setattr(args, second, shared)
                        with self.assertRaises(r0.RecoveryFailure):
                            r0.R0Runner(args).validate_path_layout()
                with self.subTest(first=first, second=second, relation="nested"):
                    with tempfile.TemporaryDirectory() as temp:
                        root = Path(temp)
                        args = make_args(root)
                        parent = root / "shared"
                        setattr(args, first, parent)
                        setattr(args, second, parent / "nested")
                        with self.assertRaises(r0.RecoveryFailure):
                            r0.R0Runner(args).validate_path_layout()

    def test_rejects_symlink_alias_overlap_when_supported(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            args = make_args(root)
            args.source_repository.mkdir()
            target = root / "actual-output"
            target.mkdir()
            alias = root / "alias-output"
            try:
                alias.symlink_to(target, target_is_directory=True)
            except (OSError, NotImplementedError):
                self.skipTest("directory symlinks unavailable")
            args.build_root = target
            args.release_root = alias / "nested"
            runner = r0.R0Runner(args)
            with self.assertRaisesRegex(r0.RecoveryFailure, "overlaps"):
                runner.validate_path_layout()

    def test_archive_preserves_existing_output(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            args = make_args(root)
            runner = r0.R0Runner(args)
            runner.validate_path_layout()
            runner.build_root.mkdir()
            (runner.build_root / "old.txt").write_text("old", encoding="utf-8")
            runner.archive_directory(runner.build_root, "build tree")
            self.assertTrue(runner.build_root.is_dir())
            self.assertFalse(any(runner.build_root.iterdir()))
            archives = list(root.glob("build.previous-*"))
            self.assertEqual(len(archives), 1)
            self.assertEqual((archives[0] / "old.txt").read_text(encoding="utf-8"), "old")

    def test_base_ref_missing_required_baseline_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            args = make_args(root)
            first, _ = init_repo(args.source_repository)
            unrelated = "0" * 40
            runner = r0.R0Runner(args)
            with mock.patch.object(r0, "REQUIRED_BASE", unrelated):
                with self.assertRaisesRegex(r0.RecoveryFailure, "does not contain required M10 baseline"):
                    runner.establish_worktree()
            self.assertFalse(args.worktree.exists())
            self.assertTrue(first)

    def test_stale_recovery_branch_fails_before_worktree_build(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            args = make_args(root)
            first, second = init_repo(args.source_repository)
            git("branch", args.recovery_branch, first, cwd=args.source_repository)
            runner = r0.R0Runner(args)
            runner.validate_path_layout()
            with mock.patch.object(r0, "REQUIRED_BASE", first):
                with self.assertRaisesRegex(r0.RecoveryFailure, "Recovery branch .* is stale"):
                    runner.establish_worktree()
            self.assertFalse(args.worktree.exists())
            self.assertEqual(git("rev-parse", "HEAD", cwd=args.source_repository), second)

    def test_exact_recovery_branch_and_worktree_are_accepted(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            args = make_args(root)
            first, second = init_repo(args.source_repository)
            git("branch", args.recovery_branch, second, cwd=args.source_repository)
            git("worktree", "add", str(args.worktree), args.recovery_branch, cwd=args.source_repository)
            runner = r0.R0Runner(args)
            runner.validate_path_layout()
            with mock.patch.object(r0, "REQUIRED_BASE", first):
                runner.establish_worktree()
            self.assertEqual(runner.admitted_revision, second)
            self.assertEqual(runner.current_head(), second)

    def test_dirty_checkpoint_refuses_in_place_rerun(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            args = make_args(root)
            first, second = init_repo(args.source_repository)
            git("branch", args.recovery_branch, second, cwd=args.source_repository)
            git("worktree", "add", str(args.worktree), args.recovery_branch, cwd=args.source_repository)
            (args.worktree / "tracked.txt").write_text("evidence changed\n", encoding="utf-8")
            runner = r0.R0Runner(args)
            with mock.patch.object(r0, "REQUIRED_BASE", first):
                with self.assertRaisesRegex(r0.RecoveryFailure, "R0 is one-shot"):
                    runner.establish_worktree()
            self.assertEqual((args.worktree / "tracked.txt").read_text(encoding="utf-8"), "evidence changed\n")

    def _tree_sleep_command(self, sentinel: Path, ready: Path | None = None) -> list[str]:
        child_code = (
            "import pathlib,time; time.sleep(2.0); "
            f"pathlib.Path({str(sentinel)!r}).write_text('escaped', encoding='utf-8')"
        )
        ready_code = ""
        if ready is not None:
            ready_code = (
                f"pathlib.Path({str(ready)!r}).write_text('descendant-started', encoding='utf-8'); "
            )
        parent_code = (
            "import pathlib,subprocess,sys,time; "
            f"subprocess.Popen([sys.executable, '-c', {child_code!r}]); "
            + ready_code
            + "print('partial-output-marker', flush=True); time.sleep(30)"
        )
        return [sys.executable, "-c", parent_code]

    def test_run_command_timeout_retains_partial_log_and_kills_descendant(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            args = make_args(root)
            args.source_repository.mkdir()
            args.worktree.mkdir()
            runner = r0.R0Runner(args)
            sentinel = root / "escaped.txt"
            with self.assertRaisesRegex(r0.RecoveryFailure, "timed out"):
                runner.run_command(
                    "timeout-fixture", self._tree_sleep_command(sentinel),
                    cwd=args.worktree, timeout_seconds=0.4,
                )
            log = (args.evidence_root / "01-timeout-fixture.log").read_text(encoding="utf-8")
            self.assertIn("partial-output-marker", log)
            self.assertIn("timed_out: true", log)
            time.sleep(2.4)
            self.assertFalse(sentinel.exists(), "a descendant survived the timeout cleanup")
            self.assertTrue(runner.records[0].timed_out)

    def test_capture_timeout_kills_descendant(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            args = make_args(root)
            args.source_repository.mkdir()
            runner = r0.R0Runner(args)
            sentinel = root / "capture-escaped.txt"
            with self.assertRaisesRegex(r0.RecoveryFailure, "Command timed out"):
                runner.capture(self._tree_sleep_command(sentinel), timeout_seconds=0.4)
            time.sleep(2.4)
            self.assertFalse(sentinel.exists(), "a capture descendant survived timeout cleanup")

    def test_capture_timeout_cleanup_drain_remains_bounded(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            args = make_args(root)
            args.source_repository.mkdir()
            runner = r0.R0Runner(args)
            process = mock.Mock()
            process.returncode = -9
            process.communicate.side_effect = [
                subprocess.TimeoutExpired(["fixture"], 0.1, output="initial-partial\n"),
                subprocess.TimeoutExpired(
                    ["fixture"], r0.TERMINATION_GRACE_SECONDS, output="cleanup-partial\n"
                ),
                ("final-output\n", None),
            ]
            with mock.patch.object(r0.subprocess, "Popen", return_value=process), mock.patch.object(
                runner, "_terminate_process_tree"
            ):
                with self.assertRaisesRegex(r0.RecoveryFailure, "Command timed out after 0.1s"):
                    runner.capture(["fixture"], timeout_seconds=0.1)
            self.assertEqual(
                [call.kwargs.get("timeout") for call in process.communicate.call_args_list],
                [0.1, r0.TERMINATION_GRACE_SECONDS, r0.TERMINATION_GRACE_SECONDS],
            )
            process.kill.assert_called_once_with()

    def test_capture_timeout_cleanup_exhaustion_fails_closed(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            args = make_args(root)
            args.source_repository.mkdir()
            runner = r0.R0Runner(args)
            process = mock.Mock()
            process.returncode = None
            process.communicate.side_effect = [
                subprocess.TimeoutExpired(["fixture"], 0.1, output="initial-partial\n"),
                subprocess.TimeoutExpired(
                    ["fixture"], r0.TERMINATION_GRACE_SECONDS, output="cleanup-partial\n"
                ),
                subprocess.TimeoutExpired(
                    ["fixture"], r0.TERMINATION_GRACE_SECONDS, output="final-partial\n"
                ),
            ]
            with mock.patch.object(r0.subprocess, "Popen", return_value=process), mock.patch.object(
                runner, "_terminate_process_tree"
            ):
                with self.assertRaisesRegex(
                    r0.RecoveryFailure, "cleanup remained incomplete"
                ) as failure:
                    runner.capture(["fixture"], timeout_seconds=0.1)
            self.assertIn("bounded pipe-drain budget", str(failure.exception))
            self.assertIn("final-partial", str(failure.exception))
            self.assertEqual(len(process.communicate.call_args_list), 3)
            process.kill.assert_called_once_with()

    def test_capture_rejects_non_positive_override_before_spawn(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            args = make_args(root)
            args.source_repository.mkdir()
            runner = r0.R0Runner(args)
            with mock.patch.object(r0.subprocess, "Popen") as popen:
                with self.assertRaisesRegex(r0.RecoveryFailure, "non-positive timeout"):
                    runner.capture(["fixture"], timeout_seconds=0)
                popen.assert_not_called()

    def test_interrupted_run_command_cleans_owned_tree_and_records_interrupt(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            args = make_args(root)
            args.source_repository.mkdir()
            args.worktree.mkdir()
            runner = r0.R0Runner(args)
            sentinel = root / "interrupt-escaped.txt"
            ready = root / "interrupt-descendant-started.txt"

            def interrupt_after_descendant_started(process: subprocess.Popen[str], timeout: float) -> int:
                deadline = time.monotonic() + min(timeout, 5.0)
                while not ready.exists() and time.monotonic() < deadline:
                    if process.poll() is not None:
                        self.fail("interrupt fixture parent exited before descendant startup was confirmed")
                    time.sleep(0.01)
                self.assertTrue(ready.exists(), "interrupt fixture did not confirm descendant startup")
                raise KeyboardInterrupt

            with mock.patch.object(
                runner, "_wait_process", side_effect=interrupt_after_descendant_started
            ):
                with self.assertRaises(KeyboardInterrupt):
                    runner.run_command(
                        "interrupt-fixture", self._tree_sleep_command(sentinel, ready),
                        cwd=args.worktree, timeout_seconds=10.0,
                    )
            log = (args.evidence_root / "01-interrupt-fixture.log").read_text(encoding="utf-8")
            self.assertIn("interrupted: true", log)
            time.sleep(2.4)
            self.assertFalse(sentinel.exists(), "a descendant survived interrupted-run cleanup")
            self.assertTrue(runner.records[0].interrupted)

    def test_generated_docs_use_current_run_time_not_hardcoded_build_date(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            args = make_args(root)
            args.source_repository.mkdir()
            args.worktree.mkdir()
            args.release_root.mkdir()
            runner = r0.R0Runner(args)
            runner.write_milestone_report(8, "Fixture", "Unit", "Smoke", "abc123")
            runner.write_package_readme("abc123")
            milestone = (args.worktree / "Docs" / "QA" / "MILESTONE-8.md").read_text(encoding="utf-8")
            package = (args.release_root / "README-M10-RC.md").read_text(encoding="utf-8")
            self.assertIn(runner.run_started_at, milestone)
            self.assertIn(runner.run_started_at, package)
            self.assertNotIn("Build date: August 11, 2026", package)


if __name__ == "__main__":
    unittest.main(verbosity=2)
