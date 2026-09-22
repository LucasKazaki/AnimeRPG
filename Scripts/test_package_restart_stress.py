#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import run_package_restart_stress as stress
import run_package_runtime_smoke


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


class RestartStressTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.package = self.root / "package"
        self.package.mkdir()
        self.manifest = self.package / "MANIFEST.json"
        self.manifest.write_text("{}", encoding="utf-8")
        self.game = self.package / "AstralGame.exe"
        self.game.write_bytes(b"game")
        self.smoke = self.root / "M10RuntimeSmoke.exe"
        self.smoke.write_bytes(b"smoke")
        self.stress_root = self.root / "stress"
        self.commit = "a" * 40
        self.game_hash = digest(self.game)
        self.smoke_hash = digest(self.smoke)

    def tearDown(self):
        self.temp.cleanup()

    def _runner(self, fail_at=None, claim_native=False, calls=None):
        counter = {"value": 0}

        def run(*args, **kwargs):
            counter["value"] += 1
            index = counter["value"]
            runtime_root = Path(args[5])
            runtime_root.mkdir()
            if calls is not None:
                calls.append((index, runtime_root, kwargs))
            passed = index != fail_at
            return {
                "schema_version": 1,
                "passed": passed,
                "evidence_kind": "synthetic",
                "acceptance": {"package_launch_verified": claim_native and passed},
            }

        return run

    def _run(self, **kwargs):
        return stress.run_restart_sequence(
            self.manifest,
            self.package,
            self.commit,
            self.game_hash,
            self.smoke,
            self.smoke_hash,
            self.stress_root,
            iterations=kwargs.pop("iterations", 3),
            interval_seconds=kwargs.pop("interval_seconds", 0),
            timeout_seconds=kwargs.pop("timeout_seconds", 1),
            synthetic_contract_test=kwargs.pop("synthetic_contract_test", True),
            **kwargs,
        )

    def test_requires_bounded_iteration_count(self):
        for value in (1, stress.MAX_ITERATIONS + 1, True):
            with self.subTest(value=value):
                with self.assertRaises(stress.RestartStressError):
                    self._run(iterations=value, iteration_runner=self._runner())

    def test_requires_empty_disjoint_stress_root(self):
        inside = self.package / "stress"
        with self.assertRaises(stress.RestartStressError):
            stress.run_restart_sequence(
                self.manifest, self.package, self.commit, self.game_hash, self.smoke,
                self.smoke_hash, inside, iterations=2, interval_seconds=0,
                synthetic_contract_test=True, iteration_runner=self._runner(),
            )
        self.stress_root.mkdir()
        (self.stress_root / "old.txt").write_text("stale", encoding="utf-8")
        with self.assertRaises(stress.RestartStressError):
            self._run(iteration_runner=self._runner())

    def test_hash_mismatch_fails_before_creating_stress_root(self):
        with self.assertRaises(stress.RestartStressError):
            stress.run_restart_sequence(
                self.manifest, self.package, self.commit, "0" * 64, self.smoke,
                self.smoke_hash, self.stress_root, iterations=2, interval_seconds=0,
                synthetic_contract_test=True, iteration_runner=self._runner(),
            )
        self.assertFalse(self.stress_root.exists())

    def test_successful_contract_sequence_writes_unique_receipts(self):
        calls = []
        result = self._run(iteration_runner=self._runner(calls=calls))
        self.assertTrue(result["passed"])
        self.assertEqual(result["completed_iterations"], 3)
        self.assertIsNone(result["failed_iteration"])
        self.assertFalse(result["acceptance"]["repeated_native_package_launch_sequence_verified"])
        self.assertFalse(result["acceptance"]["required_24h_soak_verified"])
        self.assertEqual(len(calls), 3)
        runtime_roots = [entry[1] for entry in calls]
        self.assertEqual(len(set(runtime_roots)), 3)
        for index in range(1, 4):
            receipt = self.stress_root / f"iteration-{index:04d}.json"
            self.assertTrue(receipt.is_file())
            self.assertTrue(json.loads(receipt.read_text())["passed"])

    def test_stops_at_first_failed_iteration(self):
        calls = []
        result = self._run(
            iterations=5,
            iteration_runner=self._runner(fail_at=3, calls=calls),
        )
        self.assertFalse(result["passed"])
        self.assertEqual(result["failed_iteration"], 3)
        self.assertEqual(result["completed_iterations"], 3)
        self.assertEqual(len(calls), 3)
        self.assertFalse((self.stress_root / "iteration-0004.json").exists())

    def test_injected_native_claim_cannot_upgrade_sequence(self):
        result = self._run(
            synthetic_contract_test=False,
            iteration_runner=self._runner(claim_native=True),
        )
        self.assertTrue(result["passed"])
        self.assertTrue(all(item["package_launch_verified"] for item in result["iterations"]))
        self.assertFalse(result["acceptance"]["repeated_native_package_launch_sequence_verified"])

    def test_sleeps_only_between_successful_iterations(self):
        sleeps = []
        result = self._run(
            iteration_runner=self._runner(),
            interval_seconds=0.25,
            sleeper=sleeps.append,
        )
        self.assertTrue(result["passed"])
        self.assertEqual(sleeps, [0.25, 0.25])

    def test_interrupt_preserves_prior_receipts(self):
        calls = {"value": 0}

        def runner(*args, **kwargs):
            calls["value"] += 1
            if calls["value"] == 2:
                raise KeyboardInterrupt()
            runtime_root = Path(args[5])
            runtime_root.mkdir()
            return {
                "schema_version": 1,
                "passed": True,
                "evidence_kind": "synthetic",
                "acceptance": {"package_launch_verified": False},
            }

        result = self._run(iteration_runner=runner)
        self.assertFalse(result["passed"])
        self.assertTrue(result["interrupted"])
        self.assertEqual(result["failed_iteration"], 2)
        self.assertEqual(result["completed_iterations"], 1)
        self.assertTrue((self.stress_root / "iteration-0001.json").exists())
        self.assertFalse((self.stress_root / "iteration-0002.json").exists())

    def test_interrupt_during_interval_preserves_completed_iteration(self):
        def sleeper(_seconds):
            raise KeyboardInterrupt()

        result = self._run(
            iterations=3,
            interval_seconds=0.25,
            iteration_runner=self._runner(),
            sleeper=sleeper,
        )
        self.assertFalse(result["passed"])
        self.assertTrue(result["interrupted"])
        self.assertEqual(result["failed_iteration"], 2)
        self.assertEqual(result["completed_iterations"], 1)
        self.assertTrue((self.stress_root / "iteration-0001.json").exists())

    def test_production_runner_is_the_default(self):
        self.assertIs(
            stress.run_restart_sequence.__kwdefaults__["iteration_runner"],
            run_package_runtime_smoke.verify_and_run,
        )

    def test_main_writes_summary_without_claiming_acceptance(self):
        summary = self.root / "summary.json"
        with mock.patch.object(stress, "run_restart_sequence") as patched:
            patched.return_value = {
                "schema_version": 1,
                "passed": True,
                "acceptance": {
                    "repeated_native_package_launch_sequence_verified": False,
                    "required_24h_soak_verified": False,
                },
            }
            code = stress.main([
                str(self.manifest), str(self.package),
                "--expected-commit", self.commit,
                "--expected-executable-sha256", self.game_hash,
                "--smoke-executable", str(self.smoke),
                "--expected-smoke-sha256", self.smoke_hash,
                "--stress-root", str(self.stress_root),
                "--iterations", "2",
                "--interval-seconds", "0",
                "--timeout-seconds", "1",
                "--synthetic-contract-test",
                "--json", str(summary),
            ])
        self.assertEqual(code, 0)
        data = json.loads(summary.read_text())
        self.assertTrue(data["passed"])
        self.assertFalse(data["acceptance"]["repeated_native_package_launch_sequence_verified"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
