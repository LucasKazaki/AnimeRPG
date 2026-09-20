#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import os
import sys
import tempfile
import unittest
from pathlib import Path

import run_package_runtime_smoke as target

COMMIT = "a" * 40


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


class RuntimeSmokeTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="astral-runtime-smoke-")
        self.root = Path(self.temp.name)
        self.package = self.root / "package"
        self.package.mkdir()
        self.game = self.package / "AstralGame.exe"
        self.game.write_bytes(b"astral-fixture")
        self.manifest = self.package / "MANIFEST.json"
        self.manifest.write_text("{}", encoding="utf-8")
        self.smoke = self.root / "M10RuntimeSmoke.exe"
        self.smoke.write_bytes(b"smoke-fixture")
        self.runtime = self.root / "runtime"

    def tearDown(self):
        self.temp.cleanup()

    def verifier(self, manifest, package, commit, game_hash):
        self.assertEqual(manifest, self.manifest.resolve())
        self.assertEqual(package, self.package.resolve())
        self.assertEqual(commit, COMMIT)
        self.assertEqual(game_hash, digest(self.game))
        return {"manifest_integrity_verified": True, "commit": commit}

    def executor(self, command, cwd, timeout, output_path):
        self.assertEqual(Path(command[0]), self.smoke.resolve())
        self.assertEqual(Path(command[-2]), self.game.resolve())
        self.assertEqual(Path(command[-1]), self.runtime.resolve())
        self.assertEqual(cwd, self.runtime.resolve())
        output_path.write_text("fixture pass\n", encoding="utf-8")
        return {"return_code": 0, "timed_out": False, "output": target.NATIVE_PASS_MARKER + "\n", "output_bytes": len(target.NATIVE_PASS_MARKER) + 1, "output_limit_exceeded": False}

    def run_success(self, **kwargs):
        return target.verify_and_run(
            self.manifest,
            self.package,
            COMMIT,
            digest(self.game),
            self.smoke,
            self.runtime,
            verifier=self.verifier,
            executor=self.executor,
            expected_smoke_sha256=digest(self.smoke),
            platform_name="nt",
            **kwargs,
        )

    def test_native_success_binds_package_and_rechecks_manifest(self):
        result = self.run_success()
        self.assertTrue(result["passed"])
        self.assertTrue(result["acceptance"]["smoke_process_passed"])
        self.assertTrue(result["acceptance"]["package_unchanged_after_smoke"])
        self.assertFalse(result["acceptance"]["package_launch_verified"])
        self.assertEqual(result["evidence_kind"], "native_contract_test")
        self.assertFalse(result["acceptance"]["clean_machine_compatibility_verified"])
        self.assertFalse(result["acceptance"]["owned_interactive_desktop_verified"])
        self.assertFalse(result["acceptance"]["independent_acceptance"])

    def test_synthetic_mode_never_claims_launch(self):
        result = self.run_success(synthetic_contract_test=True)
        self.assertTrue(result["passed"])
        self.assertFalse(result["acceptance"]["package_launch_verified"])
        self.assertEqual(result["evidence_kind"], "synthetic_contract")

    def test_nonzero_timeout_or_oversize_output_fails(self):
        for override in (
            {"return_code": 7, "timed_out": False, "output_limit_exceeded": False},
            {"return_code": -9, "timed_out": True, "output_limit_exceeded": False},
            {"return_code": 0, "timed_out": False, "output_limit_exceeded": True},
        ):
            def bad_executor(command, cwd, timeout, output_path, override=override):
                output_path.write_text("bad\n", encoding="utf-8")
                return {**override, "output": "bad\n", "output_bytes": 4}
            runtime = self.root / ("runtime-" + str(abs(hash(tuple(sorted(override.items()))))))
            result = target.verify_and_run(
                self.manifest, self.package, COMMIT, digest(self.game), self.smoke, runtime,
                verifier=self.verifier, executor=bad_executor,
                expected_smoke_sha256=digest(self.smoke), platform_name="nt",
            )
            self.assertFalse(result["passed"])
            self.assertFalse(result["acceptance"]["package_launch_verified"])

    def test_native_success_requires_pass_marker(self):
        def no_marker_executor(command, cwd, timeout, output_path):
            output_path.write_text("clean exit without marker\n", encoding="utf-8")
            return {
                "return_code": 0,
                "timed_out": False,
                "output": "clean exit without marker\n",
                "output_bytes": 26,
                "output_limit_exceeded": False,
            }
        result = target.verify_and_run(
            self.manifest, self.package, COMMIT, digest(self.game), self.smoke, self.runtime,
            verifier=self.verifier, executor=no_marker_executor,
            expected_smoke_sha256=digest(self.smoke), platform_name="nt",
        )
        self.assertFalse(result["passed"])
        self.assertFalse(result["native_pass_marker_verified"])
        self.assertFalse(result["acceptance"]["package_launch_verified"])

    def test_post_launch_package_mutation_fails(self):
        calls = 0
        def mutate_verifier(manifest, package, commit, game_hash):
            nonlocal calls
            calls += 1
            if calls == 2:
                raise target.RuntimeSmokeError("package changed")
            return {"manifest_integrity_verified": True}
        result = target.verify_and_run(
            self.manifest, self.package, COMMIT, digest(self.game), self.smoke, self.runtime,
            verifier=mutate_verifier, executor=self.executor,
            expected_smoke_sha256=digest(self.smoke), platform_name="nt",
        )
        self.assertFalse(result["passed"])
        self.assertFalse(result["acceptance"]["package_unchanged_after_smoke"])
        self.assertEqual(result["post_launch_manifest_error"], "package changed")

    def test_runtime_root_must_be_fresh_and_disjoint(self):
        with self.assertRaises(target.RuntimeSmokeError):
            target.verify_and_run(
                self.manifest, self.package, COMMIT, digest(self.game), self.smoke,
                self.package / "runtime", verifier=self.verifier, executor=self.executor,
                expected_smoke_sha256=digest(self.smoke), platform_name="nt",
            )
        if hasattr(os, "symlink"):
            target_dir = self.root / "runtime-target"
            target_dir.mkdir()
            runtime_link = self.root / "runtime-link"
            try:
                runtime_link.symlink_to(target_dir, target_is_directory=True)
            except OSError:
                runtime_link = None
            if runtime_link is not None:
                with self.assertRaises(target.RuntimeSmokeError):
                    target.verify_and_run(
                        self.manifest, self.package, COMMIT, digest(self.game), self.smoke,
                        runtime_link, verifier=self.verifier, executor=self.executor,
                        expected_smoke_sha256=digest(self.smoke), platform_name="nt",
                    )
        self.runtime.mkdir()
        (self.runtime / "stale.txt").write_text("stale", encoding="utf-8")
        with self.assertRaises(target.RuntimeSmokeError):
            target.verify_and_run(
                self.manifest, self.package, COMMIT, digest(self.game), self.smoke,
                self.runtime, verifier=self.verifier, executor=self.executor,
                expected_smoke_sha256=digest(self.smoke), platform_name="nt",
            )

    def test_smoke_executable_must_be_real_and_outside_package(self):
        inside = self.package / "smoke.exe"
        inside.write_bytes(b"bad")
        with self.assertRaises(target.RuntimeSmokeError):
            target.verify_and_run(
                self.manifest, self.package, COMMIT, digest(self.game), inside,
                self.runtime, verifier=self.verifier, executor=self.executor,
                expected_smoke_sha256=digest(inside), platform_name="nt",
            )
        if hasattr(os, "symlink"):
            link = self.root / "smoke-link"
            try:
                link.symlink_to(self.smoke)
            except OSError:
                return
            with self.assertRaises(target.RuntimeSmokeError):
                target.verify_and_run(
                    self.manifest, self.package, COMMIT, digest(self.game), link,
                    self.runtime, verifier=self.verifier, executor=self.executor,
                    expected_smoke_sha256=digest(self.smoke), platform_name="nt",
                )

    def test_native_mode_rejects_prefix_args_wrong_name_and_hash(self):
        with self.assertRaises(target.RuntimeSmokeError):
            self.run_success(smoke_prefix_args=["fake.py"])
        wrong_name = self.root / "OtherSmoke.exe"
        wrong_name.write_bytes(self.smoke.read_bytes())
        with self.assertRaises(target.RuntimeSmokeError):
            target.verify_and_run(
                self.manifest, self.package, COMMIT, digest(self.game), wrong_name, self.runtime,
                verifier=self.verifier, executor=self.executor,
                expected_smoke_sha256=digest(wrong_name), platform_name="nt",
            )
        with self.assertRaises(target.RuntimeSmokeError):
            target.verify_and_run(
                self.manifest, self.package, COMMIT, digest(self.game), self.smoke, self.runtime,
                verifier=self.verifier, executor=self.executor,
                expected_smoke_sha256="0" * 64, platform_name="nt",
            )

    def test_real_timeout_cleanup_and_bounded_output(self):
        sleeper = self.root / "sleep.py"
        sentinel = self.root / "escaped.txt"
        escaped = str(sentinel).replace("\\", "\\\\")
        if os.name == "nt":
            body = (
                "import pathlib,time\n"
                "time.sleep(1.2)\n"
                f"pathlib.Path(r'{escaped}').write_text('escaped')\n"
                "time.sleep(10)\n"
            )
        else:
            body = (
                "import pathlib, subprocess, sys, time\n"
                f"subprocess.Popen([sys.executable, '-c', \"import pathlib,time; time.sleep(1.2); pathlib.Path(r'{escaped}').write_text('escaped')\"])\n"
                "time.sleep(10)\n"
            )
        sleeper.write_text(body, encoding="utf-8")
        runtime = self.root / "runtime-timeout"
        runtime.mkdir()
        output = runtime / "out.log"
        result = target._execute_smoke(
            [sys.executable, str(sleeper)], runtime, 0.2, output
        )
        self.assertTrue(result["timed_out"])
        import time
        time.sleep(1.5)
        self.assertFalse(sentinel.exists(), "descendant escaped timeout cleanup")

    def test_release_manifest_integration_when_module_available(self):
        try:
            import release_manifest
        except ImportError:
            self.skipTest("repository release_manifest.py not present in sandbox fixture")
        manifest = self.package / "MANIFEST.json"
        manifest.unlink()
        release_manifest.write_release_manifest(self.package, manifest, COMMIT)
        fixture = self.root / "synthetic_smoke.py"
        fixture.write_text(
            "import pathlib, sys\n"
            "game=pathlib.Path(sys.argv[-2]); runtime=pathlib.Path(sys.argv[-1])\n"
            "assert game.is_file(); runtime.joinpath('fixture.txt').write_text('ok')\n"
            "print('synthetic contract pass')\n",
            encoding="utf-8",
        )
        result = target.verify_and_run(
            manifest, self.package, COMMIT, digest(self.game), Path(sys.executable),
            self.runtime, smoke_prefix_args=[str(fixture)], synthetic_contract_test=True,
        )
        self.assertTrue(result["passed"])
        self.assertTrue(result["acceptance"]["package_unchanged_after_smoke"])
        self.assertFalse(result["acceptance"]["package_launch_verified"])

    def test_timeout_bounds(self):
        for timeout in (0, target.MAX_TIMEOUT_SECONDS + 1):
            runtime = self.root / f"runtime-{timeout}"
            with self.assertRaises(target.RuntimeSmokeError):
                target.verify_and_run(
                    self.manifest, self.package, COMMIT, digest(self.game), self.smoke,
                    runtime, timeout_seconds=timeout, verifier=self.verifier, executor=self.executor,
                    expected_smoke_sha256=digest(self.smoke), platform_name="nt",
                )


if __name__ == "__main__":
    unittest.main(verbosity=2)
