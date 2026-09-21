#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

import benchmark_manifest
import release_manifest
import verify_benchmark_run_control as run_control

COMMIT = "b" * 40


def receipt() -> dict:
    return {
        "schema_version": 3,
        "mode": "fixed_frame_count",
        "simulation_fixed_hz": 60,
        "warmup_frames": 120,
        "measured_frames": 3600,
        "total_frames": 3720,
        "completed_frames": 3720,
        "client_width_px": 1920,
        "client_height_px": 1080,
        "client_area_observations": 3720,
        "client_area_stable": True,
        "client_area_control": "environment_requested_and_verified",
        "window_mode": "windowed",
        "presentation_backend": "win32_gdi_window_dc",
        "vsync_control": "unavailable_in_gdi_path",
        "frame_pacing": "sleep_1ms_not_refresh_locked",
        "live_input": "suppressed",
        "termination": "exact_frame_limit",
        "performance_budget_verified": False,
        "comparative_parity_verified": False,
        "independent_acceptance": False,
    }


def spec() -> dict:
    return {
        "candidate": {"commit": COMMIT, "build_config": "Release"},
        "workload": {
            "id": "e14-procedural-3d-v1",
            "dimension": "3d",
            "fixture_description": "Procedural package-bound profiling fixture",
        },
        "run_protocol": {
            "width": 1920,
            "height": 1080,
            "window_mode": "windowed",
            "vsync": False,
            "warmup_seconds": 2.0,
            "sample_seconds": 60.0,
        },
        "environment": {
            "os": "Windows fixture",
            "cpu": "Fixture CPU",
            "logical_cpus": 8,
            "ram_bytes": 16 * 1024**3,
            "gpu": "Fixture GPU",
            "gpu_driver": "fixture-driver",
        },
        "reference_versions": {"unreal": "5.8", "unity": "6000.0"},
        "provenance": {
            "evidence_class": "hosted_contract_fixture",
            "machine_label": "fixture",
        },
        "evidence": [
            {"path": "benchmark-control.json", "role": "benchmark_run_control_json"},
            {"path": "astral.log", "role": "engine_log"},
        ],
    }


class Fixture:
    def __init__(self, root: Path):
        self.root = root
        self.package = root / "package"
        self.evidence = root / "evidence"
        self.package.mkdir()
        self.evidence.mkdir()
        (self.package / "AstralGame.exe").write_bytes(b"astral-run-control-fixture")
        self.release_manifest = self.package / "MANIFEST.json"
        release_manifest.write_release_manifest(self.package, self.release_manifest, COMMIT)
        self.receipt = self.evidence / "benchmark-control.json"
        self.write_receipt(receipt())
        (self.evidence / "astral.log").write_text("fixture\n", encoding="utf-8")
        self.manifest_path = root / "benchmark.json"
        self.rebuild()

    def write_receipt(self, data: dict):
        self.receipt.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")

    def rebuild(self, custom_spec: dict | None = None):
        manifest = benchmark_manifest.build_benchmark_manifest(
            custom_spec or spec(), self.package, self.release_manifest, self.evidence
        )
        self.manifest_path.write_text(
            json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
        )

    def verify(self):
        return run_control.verify(
            self.manifest_path, self.package, self.release_manifest, self.evidence
        )


class BenchmarkRunControlVerificationTests(unittest.TestCase):
    def test_valid_receipt_is_bound_to_descriptor(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            report = fx.verify()
            self.assertTrue(report["benchmark_run_control_verified"])
            self.assertTrue(report["benchmark_duration_protocol_coherent"])
            self.assertTrue(report["benchmark_presentation_policy_coherent"])
            self.assertEqual(report["simulation_fixed_hz"], 60)
            self.assertEqual(report["warmup_frames"], 120)
            self.assertEqual(report["measured_frames"], 3600)
            self.assertEqual(report["warmup_seconds"], 2.0)
            self.assertEqual(report["sample_seconds"], 60.0)
            self.assertEqual(report["completed_frames"], 3720)
            self.assertEqual(report["client_width_px"], 1920)
            self.assertEqual(report["client_height_px"], 1080)
            self.assertEqual(report["client_area_observations"], 3720)
            self.assertTrue(report["client_area_stable"])
            self.assertEqual(
                report["client_area_control"], "environment_requested_and_verified"
            )
            self.assertEqual(report["window_mode"], "windowed")
            self.assertFalse(report["vsync_requested"])
            self.assertEqual(report["presentation_backend"], "win32_gdi_window_dc")
            self.assertEqual(report["vsync_control"], "unavailable_in_gdi_path")
            self.assertEqual(report["frame_pacing"], "sleep_1ms_not_refresh_locked")
            self.assertEqual(report["live_input"], "suppressed")
            self.assertFalse(report["acceptance"]["comparative_parity_verified"])

    def test_vsync_true_is_rejected_after_manifest_rehash(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            incompatible = spec()
            incompatible["run_protocol"]["vsync"] = True
            fx.rebuild(incompatible)
            with self.assertRaises(run_control.BenchmarkRunControlError):
                fx.verify()

    def test_duration_mismatch_is_rejected_after_manifest_rehash(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            wrong_warmup = spec()
            wrong_warmup["run_protocol"]["warmup_seconds"] = 3.0
            fx.rebuild(wrong_warmup)
            with self.assertRaises(run_control.BenchmarkRunControlError):
                fx.verify()

        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            wrong_sample = spec()
            wrong_sample["run_protocol"]["sample_seconds"] = 59.0
            fx.rebuild(wrong_sample)
            with self.assertRaises(run_control.BenchmarkRunControlError):
                fx.verify()

    def test_fractional_fixed_step_duration_is_rejected_not_rounded(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            fractional = spec()
            fractional["run_protocol"]["sample_seconds"] = 60.01
            fx.rebuild(fractional)
            with self.assertRaises(run_control.BenchmarkRunControlError):
                fx.verify()

    def test_coherent_non_sixty_hz_protocol_is_not_hardcoded(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            data = receipt()
            data["simulation_fixed_hz"] = 120
            data["warmup_frames"] = 240
            data["measured_frames"] = 7200
            data["total_frames"] = 7440
            data["completed_frames"] = 7440
            data["client_area_observations"] = 7440
            fx.write_receipt(data)
            fx.rebuild()
            report = fx.verify()
            self.assertTrue(report["benchmark_duration_protocol_coherent"])
            self.assertEqual(report["simulation_fixed_hz"], 120)
            self.assertEqual(report["warmup_frames"], 240)
            self.assertEqual(report["measured_frames"], 7200)

    def test_evidence_mutation_is_rejected_before_interpretation(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            data = receipt()
            data["simulation_fixed_hz"] = 120
            fx.receipt.write_text(json.dumps(data), encoding="utf-8")
            with self.assertRaises(benchmark_manifest.BenchmarkManifestError):
                fx.verify()

    def test_missing_or_duplicate_role_is_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            missing = spec()
            missing["evidence"][0]["role"] = "other"
            fx.rebuild(missing)
            with self.assertRaises(run_control.BenchmarkRunControlError):
                fx.verify()

            duplicate_path = fx.evidence / "benchmark-control-2.json"
            duplicate_path.write_text(json.dumps(receipt()), encoding="utf-8")
            duplicate = spec()
            duplicate["evidence"].append(
                {"path": "benchmark-control-2.json", "role": "benchmark_run_control_json"}
            )
            fx.rebuild(duplicate)
            with self.assertRaises(run_control.BenchmarkRunControlError):
                fx.verify()

    def test_claim_laundering_is_rejected(self):
        for key in (
            "performance_budget_verified",
            "comparative_parity_verified",
            "independent_acceptance",
        ):
            with self.subTest(key=key):
                data = receipt()
                data[key] = True
                with self.assertRaises(run_control.BenchmarkRunControlError):
                    run_control.validate_receipt(data)

    def test_wrong_mode_input_termination_window_or_presentation_state_is_rejected(self):
        cases = (
            ("mode", "variable_time"),
            ("live_input", "enabled"),
            ("termination", "window_close"),
            ("window_mode", "borderless"),
            ("presentation_backend", "dxgi_swap_chain"),
            ("vsync_control", "enabled"),
            ("frame_pacing", "refresh_locked"),
            ("client_area_stable", False),
            ("client_area_control", "configured_contract"),
        )
        for key, value in cases:
            with self.subTest(key=key):
                data = receipt()
                data[key] = value
                with self.assertRaises(run_control.BenchmarkRunControlError):
                    run_control.validate_receipt(data)

    def test_old_receipt_schema_is_rejected(self):
        data = receipt()
        data["schema_version"] = 2
        with self.assertRaises(run_control.BenchmarkRunControlError):
            run_control.validate_receipt(data)

    def test_invalid_rate_counts_and_client_area_are_rejected(self):
        cases = (
            ("simulation_fixed_hz", 0),
            ("simulation_fixed_hz", 1001),
            ("simulation_fixed_hz", True),
            ("warmup_frames", -1),
            ("measured_frames", 0),
            ("total_frames", 3719),
            ("completed_frames", 3719),
            ("client_width_px", 0),
            ("client_width_px", 16385),
            ("client_width_px", True),
            ("client_height_px", 0),
            ("client_height_px", 16385),
            ("client_area_observations", 3719),
        )
        for key, value in cases:
            with self.subTest(key=key):
                data = receipt()
                data[key] = value
                with self.assertRaises(run_control.BenchmarkRunControlError):
                    run_control.validate_receipt(data)

    def test_protocol_client_area_mismatch_is_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            wrong_width = spec()
            wrong_width["run_protocol"]["width"] = 1919
            fx.rebuild(wrong_width)
            with self.assertRaises(run_control.BenchmarkRunControlError):
                fx.verify()

        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            wrong_height = spec()
            wrong_height["run_protocol"]["height"] = 1079
            fx.rebuild(wrong_height)
            with self.assertRaises(run_control.BenchmarkRunControlError):
                fx.verify()

        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            wrong_mode = spec()
            wrong_mode["run_protocol"]["window_mode"] = "borderless"
            fx.rebuild(wrong_mode)
            with self.assertRaises(run_control.BenchmarkRunControlError):
                fx.verify()

    def test_extra_or_missing_keys_are_rejected(self):
        data = receipt()
        data["extra"] = 1
        with self.assertRaises(run_control.BenchmarkRunControlError):
            run_control.validate_receipt(data)
        data = receipt()
        del data["presentation_backend"]
        with self.assertRaises(run_control.BenchmarkRunControlError):
            run_control.validate_receipt(data)

    def test_cli_writes_once_and_refuses_overwrite(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            out = Path(td) / "verification.json"
            script = Path(__file__).with_name("verify_benchmark_run_control.py")
            args = [
                sys.executable,
                str(script),
                str(fx.manifest_path),
                str(fx.package),
                str(fx.release_manifest),
                str(fx.evidence),
                "--json",
                str(out),
            ]
            first = subprocess.run(args, capture_output=True, text=True, timeout=15)
            self.assertEqual(first.returncode, 0, first.stderr)
            self.assertTrue(json.loads(out.read_text(encoding="utf-8"))[
                "benchmark_run_control_verified"
            ])
            second = subprocess.run(args, capture_output=True, text=True, timeout=15)
            self.assertNotEqual(second.returncode, 0)

    def test_receipt_file_size_is_bounded(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "large.json"
            path.write_bytes(b" " * (run_control.MAX_RECEIPT_BYTES + 1))
            with self.assertRaises(run_control.BenchmarkRunControlError):
                run_control._load_json(path, run_control.MAX_RECEIPT_BYTES, "receipt")


if __name__ == "__main__":
    unittest.main(verbosity=2)
