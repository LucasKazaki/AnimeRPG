#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from typing import Any

import run_package_continuous_soak as soak

COMMIT = "1" * 40


class FakeClock:
    def __init__(self) -> None:
        self.value = 0.0

    def monotonic(self) -> float:
        return self.value

    def sleep(self, seconds: float) -> None:
        self.value += max(0.0, seconds)

    def now(self) -> str:
        return f"synthetic+{self.value:.3f}s"


class FakeProcess:
    def __init__(self, *, pid: int = 4242, early_exit_at: float | None = None, clock: FakeClock | None = None) -> None:
        self.pid = pid
        self.returncode: int | None = None
        self.early_exit_at = early_exit_at
        self.clock = clock
        self.killed = False

    def poll(self) -> int | None:
        if self.returncode is None and self.early_exit_at is not None and self.clock is not None:
            if self.clock.value >= self.early_exit_at:
                self.returncode = 3
        return self.returncode

    def wait(self, timeout: float | None = None) -> int:
        if self.returncode is None:
            self.returncode = 0
        return self.returncode

    def kill(self) -> None:
        self.killed = True
        self.returncode = -9


class SoakTests(unittest.TestCase):
    def make_fixture(self, root: Path) -> tuple[Path, Path, Path, str]:
        package = root / "package"
        package.mkdir()
        exe = package / "AstralGame.exe"
        exe.write_bytes(b"astral-test-exe")
        manifest = package / "MANIFEST.json"
        manifest.write_text("{}\n", encoding="utf-8")
        runtime = root / "runtime"
        return package, manifest, runtime, soak._sha256(exe)

    @staticmethod
    def verifier(_manifest: Path, _package: Path, _commit: str, _hash: str) -> dict[str, Any]:
        return {"manifest_integrity_verified": True}

    @staticmethod
    def sampler(_pid: int) -> dict[str, int]:
        return {
            "working_set_bytes": 100,
            "peak_working_set_bytes": 120,
            "private_usage_bytes": 80,
            "pagefile_usage_bytes": 90,
            "peak_pagefile_usage_bytes": 110,
            "handle_count": 10,
            "gdi_objects": 3,
            "user_objects": 4,
        }

    def run_fake(self, root: Path, *, duration: float = 0.03, interval: float = 0.01,
                 process: FakeProcess | None = None, verifier: Any = None, sampler: Any = None,
                 window_probe: Any = None, graceful_closer: Any = None, terminator: Any = None):
        package, manifest, runtime, exe_hash = self.make_fixture(root)
        clock = FakeClock()
        proc = process or FakeProcess(clock=clock)
        proc.clock = clock

        def launcher(_exe: Path, _runtime: Path) -> FakeProcess:
            return proc

        def closer(_pid: int) -> int:
            proc.returncode = 0
            return 1

        def kill(p: FakeProcess) -> None:
            p.killed = True
            if p.returncode is None:
                p.returncode = -9

        return soak.run_continuous_soak(
            manifest,
            package,
            COMMIT,
            exe_hash,
            runtime,
            duration_seconds=duration,
            sample_interval_seconds=interval,
            window_wait_seconds=0.0,
            close_timeout_seconds=0.1,
            synthetic_contract_test=True,
            verifier=verifier or self.verifier,
            launcher=launcher,
            sampler=sampler or self.sampler,
            window_probe=window_probe or (lambda _pid, _timeout: True),
            graceful_closer=graceful_closer or closer,
            terminator=terminator or kill,
            sleeper=clock.sleep,
            monotonic=clock.monotonic,
            now=clock.now,
        )

    def test_successful_synthetic_run_retains_claim_guards_and_telemetry(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            result = self.run_fake(Path(tmp))
        self.assertTrue(result["passed"])
        self.assertEqual(result["telemetry_summary"]["sample_count"], 4)
        self.assertEqual(result["telemetry_summary"]["working_set_bytes"]["delta_first_to_last"], 0)
        self.assertFalse(result["acceptance"]["continuous_package_uptime_observed"])
        self.assertFalse(result["acceptance"]["required_24h_duration_observed"])
        self.assertFalse(result["acceptance"]["required_24h_soak_verified"])
        self.assertFalse(result["acceptance"]["ram_telemetry_observed"])
        self.assertFalse(result["acceptance"]["ram_budget_verified"])
        self.assertFalse(result["acceptance"]["vram_budget_verified"])
        self.assertFalse(result["acceptance"]["frame_time_budget_verified"])
        self.assertFalse(result["acceptance"]["memory_leak_free_verified"])

    def test_rejects_bad_revision_and_hash_before_launch(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            package, manifest, runtime, exe_hash = self.make_fixture(Path(tmp))
            with self.assertRaises(soak.ContinuousSoakError):
                soak.run_continuous_soak(
                    manifest, package, "bad", exe_hash, runtime,
                    duration_seconds=0.02, sample_interval_seconds=0.01,
                    synthetic_contract_test=True,
                )
            with self.assertRaises(soak.ContinuousSoakError):
                soak.run_continuous_soak(
                    manifest, package, COMMIT, "0" * 64, runtime,
                    duration_seconds=0.02, sample_interval_seconds=0.01,
                    synthetic_contract_test=True,
                )

    def test_rejects_unsafe_runtime_root(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            package, manifest, _runtime, exe_hash = self.make_fixture(Path(tmp))
            with self.assertRaises(soak.ContinuousSoakError):
                soak.run_continuous_soak(
                    manifest, package, COMMIT, exe_hash, package / "runtime",
                    duration_seconds=0.02, sample_interval_seconds=0.01,
                    synthetic_contract_test=True,
                    verifier=self.verifier,
                )

    def test_early_exit_fails_and_preserves_partial_samples(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            clocked = FakeProcess(early_exit_at=0.015)
            result = self.run_fake(Path(tmp), duration=0.04, interval=0.01, process=clocked)
            telemetry = Path(result["telemetry_path"])
            lines = telemetry.read_text(encoding="utf-8").splitlines()
        self.assertFalse(result["passed"])
        self.assertIn("exited before requested duration", result["failure"])
        self.assertGreaterEqual(len(lines), 2)
        self.assertFalse(result["acceptance"]["continuous_package_uptime_observed"])

    def test_sample_failure_fails_and_terminates_owned_process(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            proc = FakeProcess()
            calls = 0

            def failing_sampler(_pid: int) -> dict[str, int]:
                nonlocal calls
                calls += 1
                if calls == 2:
                    raise soak.ContinuousSoakError("fixture sample failure")
                return self.sampler(_pid)

            result = self.run_fake(Path(tmp), process=proc, sampler=failing_sampler)
        self.assertFalse(result["passed"])
        self.assertIn("fixture sample failure", result["failure"])
        self.assertTrue(proc.killed)

    def test_no_visible_window_fails_before_sampling(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            sample_calls = 0

            def sampler(_pid: int) -> dict[str, int]:
                nonlocal sample_calls
                sample_calls += 1
                return self.sampler(_pid)

            result = self.run_fake(
                Path(tmp), sampler=sampler, window_probe=lambda _pid, _timeout: False
            )
        self.assertFalse(result["passed"])
        self.assertIn("no visible AstralGame window", result["failure"])
        self.assertEqual(sample_calls, 0)

    def test_missing_graceful_close_fails(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            result = self.run_fake(
                Path(tmp), graceful_closer=lambda _pid: 0
            )
        self.assertFalse(result["passed"])
        self.assertIn("no visible window accepted WM_CLOSE", result["failure"])

    def test_post_soak_manifest_failure_invalidates_result(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            calls = 0

            def verifier(_manifest: Path, _package: Path, _commit: str, _hash: str) -> dict[str, Any]:
                nonlocal calls
                calls += 1
                if calls == 2:
                    raise soak.ContinuousSoakError("package changed")
                return {"manifest_integrity_verified": True}

            result = self.run_fake(Path(tmp), verifier=verifier)
        self.assertFalse(result["passed"])
        self.assertIn("post-soak package verification failed", result["failure"])
        self.assertFalse(result["acceptance"]["package_unchanged_after_soak"])

    def test_interruption_preserves_completed_telemetry(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            calls = 0

            def sampler(_pid: int) -> dict[str, int]:
                nonlocal calls
                calls += 1
                if calls == 3:
                    raise KeyboardInterrupt
                return self.sampler(_pid)

            result = self.run_fake(Path(tmp), duration=0.05, interval=0.01, sampler=sampler)
            lines = Path(result["telemetry_path"]).read_text(encoding="utf-8").splitlines()
        self.assertFalse(result["passed"])
        self.assertTrue(result["interrupted"])
        self.assertEqual(len(lines), 2)

    @unittest.skipUnless(os.name == "nt", "Windows telemetry API contract")
    def test_real_windows_process_sampler(self) -> None:
        proc = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(5)"])
        try:
            sample = soak._sample_windows_process(proc.pid)
        finally:
            proc.terminate()
            proc.wait(timeout=5)
        self.assertGreater(sample["working_set_bytes"], 0)
        self.assertGreater(sample["private_usage_bytes"], 0)
        self.assertGreater(sample["handle_count"], 0)
        self.assertGreaterEqual(sample["gdi_objects"], 0)
        self.assertGreaterEqual(sample["user_objects"], 0)

    def test_summary_does_not_invent_leak_or_budget_claims(self) -> None:
        samples = [
            {"working_set_bytes": 100, "peak_working_set_bytes": 100, "private_usage_bytes": 50,
             "pagefile_usage_bytes": 60, "peak_pagefile_usage_bytes": 60, "handle_count": 10,
             "gdi_objects": 2, "user_objects": 3},
            {"working_set_bytes": 180, "peak_working_set_bytes": 180, "private_usage_bytes": 90,
             "pagefile_usage_bytes": 100, "peak_pagefile_usage_bytes": 100, "handle_count": 14,
             "gdi_objects": 3, "user_objects": 5},
        ]
        summary = soak._summarize_samples(samples)
        self.assertEqual(summary["working_set_bytes"]["delta_first_to_last"], 80)
        self.assertEqual(summary["handle_count"]["max"], 14)
        self.assertNotIn("leak", json.dumps(summary).lower())


if __name__ == "__main__":
    unittest.main(verbosity=2)
