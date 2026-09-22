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
import verify_benchmark_capture_pair as pair

COMMIT = "c" * 40


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


def capture_receipt(profiled: bool) -> dict:
    def frame(name: str) -> dict:
        if profiled:
            return {
                "state": "enabled",
                "output_path": f"{name}.csv",
                "warmup_frames": 120,
                "max_samples": 3600,
            }
        return {
            "state": "not_requested",
            "output_path": None,
            "warmup_frames": None,
            "max_samples": None,
        }

    memory = (
        {
            "state": "enabled",
            "output_path": "memory.csv",
            "warmup_frames": 120,
            "sample_every_frames": 10,
            "max_samples": 360,
            "sample_source": "windows_process_counters",
        }
        if profiled
        else {
            "state": "not_requested",
            "output_path": None,
            "warmup_frames": None,
            "sample_every_frames": None,
            "max_samples": None,
            "sample_source": None,
        }
    )
    return {
        "schema_version": 1,
        "capture_state_semantics": "post_configuration_pre_frame_loop",
        "environment_scope": "known_astral_profiling_controls_only",
        "raw_environment_dumped": False,
        "run_control": {
            "simulation_fixed_hz": 60,
            "warmup_frames": 120,
            "measured_frames": 3600,
            "total_frames": 3720,
            "client_width_px": 1920,
            "client_height_px": 1080,
            "window_mode": "windowed",
            "vsync_requested": False,
            "presentation_backend": "win32_gdi_window_dc",
            "vsync_control": "unavailable_in_gdi_path",
            "frame_pacing": "sleep_1ms_not_refresh_locked",
            "live_input": "suppressed",
            "termination": "exact_frame_limit",
        },
        "streams": {
            "cpu_frame_timing": frame("frame"),
            "cpu_phase_timing": frame("phase"),
            "process_memory": memory,
        },
        "claim_boundaries": {
            "instrumentation_overhead_verified": False,
            "performance_budget_verified": False,
            "gpu_timing_verified": False,
            "comparative_parity_verified": False,
            "independent_acceptance": False,
        },
    }


def spec(profiled: bool) -> dict:
    evidence = [
        {"path": "benchmark-control.json", "role": "benchmark_run_control_json"},
        {"path": "capture-state.json", "role": "profiling_capture_state_json"},
        {"path": "astral.log", "role": "engine_log"},
    ]
    if profiled:
        evidence.extend([
            {"path": "frame.csv", "role": "cpu_frame_timing_csv"},
            {"path": "phase.csv", "role": "cpu_phase_timing_csv"},
            {"path": "memory.csv", "role": "process_memory_csv"},
        ])
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
            "machine_label": "fixture-machine",
        },
        "evidence": evidence,
    }


class Fixture:
    def __init__(self, root: Path):
        self.root = root
        self.package = root / "package"
        self.profiled_evidence = root / "profiled-evidence"
        self.control_evidence = root / "control-evidence"
        self.package.mkdir()
        self.profiled_evidence.mkdir()
        self.control_evidence.mkdir()

        (self.package / "AstralGame.exe").write_bytes(b"capture-pair-fixture")
        self.release_manifest = self.package / "MANIFEST.json"
        release_manifest.write_release_manifest(
            self.package, self.release_manifest, COMMIT
        )

        self._write_evidence(self.profiled_evidence, profiled=True)
        self._write_evidence(self.control_evidence, profiled=False)
        self.profiled_manifest = root / "profiled.json"
        self.control_manifest = root / "control.json"
        self.rebuild_profiled()
        self.rebuild_control()

    @staticmethod
    def _write_evidence(root: Path, profiled: bool) -> None:
        (root / "benchmark-control.json").write_text(
            json.dumps(receipt(), indent=2) + "\n", encoding="utf-8"
        )
        (root / "capture-state.json").write_text(
            json.dumps(capture_receipt(profiled), indent=2) + "\n", encoding="utf-8"
        )
        (root / "astral.log").write_text("fixture\n", encoding="utf-8")
        if profiled:
            (root / "frame.csv").write_text("fixture-frame\n", encoding="utf-8")
            (root / "phase.csv").write_text("fixture-phase\n", encoding="utf-8")
            (root / "memory.csv").write_text("fixture-memory\n", encoding="utf-8")

    def _write_manifest(
        self, path: Path, evidence_root: Path, custom_spec: dict
    ) -> None:
        manifest = benchmark_manifest.build_benchmark_manifest(
            custom_spec, self.package, self.release_manifest, evidence_root
        )
        path.write_text(
            json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
        )

    def rebuild_profiled(self, custom_spec: dict | None = None) -> None:
        self._write_manifest(
            self.profiled_manifest,
            self.profiled_evidence,
            custom_spec or spec(True),
        )

    def rebuild_control(self, custom_spec: dict | None = None) -> None:
        self._write_manifest(
            self.control_manifest,
            self.control_evidence,
            custom_spec or spec(False),
        )

    def verify(self) -> dict:
        return pair.verify_capture_pair(
            self.profiled_manifest,
            self.control_manifest,
            self.package,
            self.release_manifest,
            self.profiled_evidence,
            self.control_evidence,
        )


class CapturePairVerificationTests(unittest.TestCase):
    def test_valid_matched_pair_is_accepted_without_overhead_claim(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            report = fx.verify()
            self.assertTrue(report["capture_pairing_verified"])
            self.assertTrue(report["startup_capture_state_verified"])
            self.assertEqual(report["candidate_commit"], COMMIT)
            self.assertEqual(
                report["profiled_capture_roles"], sorted(pair.PROFILE_ROLES)
            )
            self.assertEqual(report["control_capture_roles"], [])
            self.assertEqual(
                report["profiled_startup_streams"]["cpu_frame_timing"]["state"],
                "enabled",
            )
            self.assertEqual(
                report["control_startup_streams"]["cpu_frame_timing"]["state"],
                "not_requested",
            )
            self.assertFalse(
                report["acceptance"]["instrumentation_overhead_verified"]
            )
            self.assertFalse(report["acceptance"]["performance_budget_verified"])
            self.assertFalse(report["acceptance"]["comparative_parity_verified"])

    def test_profiled_manifest_requires_each_capture_role_exactly_once(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            missing = spec(True)
            missing["evidence"] = [
                item for item in missing["evidence"]
                if item["role"] != "process_memory_csv"
            ]
            fx.rebuild_profiled(missing)
            with self.assertRaises(pair.CapturePairError):
                fx.verify()

        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            (fx.profiled_evidence / "frame-2.csv").write_text(
                "second-frame\n", encoding="utf-8"
            )
            duplicate = spec(True)
            duplicate["evidence"].append(
                {"path": "frame-2.csv", "role": "cpu_frame_timing_csv"}
            )
            fx.rebuild_profiled(duplicate)
            with self.assertRaises(pair.CapturePairError):
                fx.verify()

    def test_startup_capture_state_is_required_and_must_match_mode(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            missing = spec(True)
            missing["evidence"] = [
                item for item in missing["evidence"]
                if item["role"] != "profiling_capture_state_json"
            ]
            fx.rebuild_profiled(missing)
            with self.assertRaises(pair.CapturePairError):
                fx.verify()

        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            wrong_mode = capture_receipt(False)
            (fx.profiled_evidence / "capture-state.json").write_text(
                json.dumps(wrong_mode, indent=2) + "\n", encoding="utf-8"
            )
            fx.rebuild_profiled()
            with self.assertRaises(pair.CapturePairError):
                fx.verify()

    def test_control_manifest_must_omit_profile_capture_roles(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            (fx.control_evidence / "frame.csv").write_text(
                "control-frame\n", encoding="utf-8"
            )
            contaminated = spec(False)
            contaminated["evidence"].append(
                {"path": "frame.csv", "role": "cpu_frame_timing_csv"}
            )
            fx.rebuild_control(contaminated)
            with self.assertRaises(pair.CapturePairError):
                fx.verify()

    def test_workload_reference_environment_and_machine_mismatches_are_rejected(self):
        mutations = (
            ("workload", lambda s: s["workload"].__setitem__(
                "fixture_description", "different fixture"
            )),
            ("reference", lambda s: s["reference_versions"].__setitem__(
                "unity", "6000.1"
            )),
            ("environment", lambda s: s["environment"].__setitem__(
                "logical_cpus", 16
            )),
            ("machine", lambda s: s["provenance"].__setitem__(
                "machine_label", "other-machine"
            )),
        )
        for label, mutate in mutations:
            with self.subTest(label=label), tempfile.TemporaryDirectory() as td:
                fx = Fixture(Path(td))
                changed = spec(False)
                mutate(changed)
                fx.rebuild_control(changed)
                with self.assertRaises(pair.CapturePairError):
                    fx.verify()

    def test_run_protocol_and_run_control_mismatch_is_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            changed = spec(False)
            changed["run_protocol"]["sample_seconds"] = 59.0
            fx.rebuild_control(changed)
            with self.assertRaises(pair.CapturePairError):
                fx.verify()

        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            data = receipt()
            data["client_area_observations"] = 3719
            (fx.control_evidence / "benchmark-control.json").write_text(
                json.dumps(data), encoding="utf-8"
            )
            fx.rebuild_control()
            with self.assertRaises(pair.CapturePairError):
                fx.verify()

    def test_evidence_tampering_is_rejected_before_pairing(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            (fx.profiled_evidence / "frame.csv").write_text(
                "tampered\n", encoding="utf-8"
            )
            with self.assertRaises(pair.CapturePairError):
                fx.verify()

    def test_cli_writes_once_and_refuses_overwrite(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            output = Path(td) / "pair.json"
            script = Path(__file__).with_name("verify_benchmark_capture_pair.py")
            args = [
                sys.executable,
                str(script),
                "--profiled-manifest",
                str(fx.profiled_manifest),
                "--control-manifest",
                str(fx.control_manifest),
                "--package-root",
                str(fx.package),
                "--release-manifest",
                str(fx.release_manifest),
                "--profiled-evidence-root",
                str(fx.profiled_evidence),
                "--control-evidence-root",
                str(fx.control_evidence),
                "--output",
                str(output),
            ]
            first = subprocess.run(args, capture_output=True, text=True, timeout=15)
            self.assertEqual(first.returncode, 0, first.stderr)
            self.assertTrue(
                json.loads(output.read_text(encoding="utf-8"))[
                    "capture_pairing_verified"
                ]
            )
            second = subprocess.run(args, capture_output=True, text=True, timeout=15)
            self.assertNotEqual(second.returncode, 0)

    def test_manifest_size_and_output_parent_are_bounded(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            huge = root / "huge.json"
            huge.write_bytes(b" " * (pair.MAX_JSON_BYTES + 1))
            with self.assertRaises(pair.CapturePairError):
                pair._load_json(huge, "huge")

            with self.assertRaises(pair.CapturePairError):
                pair._write_new_json(root / "missing" / "out.json", {"ok": True})


if __name__ == "__main__":
    unittest.main(verbosity=2)
