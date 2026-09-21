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
import verify_profiling_capture_state as capture_state

COMMIT = "d" * 40


def run_control_receipt() -> dict:
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

    memory = {
        "state": "enabled",
        "output_path": "memory.csv",
        "warmup_frames": 120,
        "sample_every_frames": 10,
        "max_samples": 360,
        "sample_source": "windows_process_counters",
    } if profiled else {
        "state": "not_requested",
        "output_path": None,
        "warmup_frames": None,
        "sample_every_frames": None,
        "max_samples": None,
        "sample_source": None,
    }
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
            "fixture_description": "Capture-state verification fixture",
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
    def __init__(self, root: Path, profiled: bool):
        self.root = root
        self.package = root / "package"
        self.evidence = root / "evidence"
        self.package.mkdir()
        self.evidence.mkdir()
        (self.package / "AstralGame.exe").write_bytes(b"capture-state-fixture")
        self.release_manifest = self.package / "MANIFEST.json"
        release_manifest.write_release_manifest(self.package, self.release_manifest, COMMIT)
        (self.evidence / "benchmark-control.json").write_text(
            json.dumps(run_control_receipt(), indent=2) + "\n", encoding="utf-8"
        )
        (self.evidence / "capture-state.json").write_text(
            json.dumps(capture_receipt(profiled), indent=2) + "\n", encoding="utf-8"
        )
        (self.evidence / "astral.log").write_text("fixture\n", encoding="utf-8")
        if profiled:
            (self.evidence / "frame.csv").write_text("frame\n", encoding="utf-8")
            (self.evidence / "phase.csv").write_text("phase\n", encoding="utf-8")
            (self.evidence / "memory.csv").write_text("memory\n", encoding="utf-8")
        self.manifest = root / "benchmark.json"
        self.rebuild(profiled)

    def rebuild(self, profiled: bool, custom_spec: dict | None = None) -> None:
        manifest = benchmark_manifest.build_benchmark_manifest(
            custom_spec or spec(profiled), self.package, self.release_manifest, self.evidence
        )
        self.manifest.write_text(
            json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
        )

    def rewrite_receipt(self, receipt: dict, profiled: bool) -> None:
        (self.evidence / "capture-state.json").write_text(
            json.dumps(receipt, indent=2) + "\n", encoding="utf-8"
        )
        self.rebuild(profiled)

    def verify(self, mode: str) -> dict:
        return capture_state.verify(
            self.manifest, self.package, self.release_manifest, self.evidence, mode
        )


class ProfilingCaptureStateTests(unittest.TestCase):
    def test_profiled_and_control_receipts_validate(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td), True)
            report = fx.verify("profiled")
            self.assertTrue(report["profiling_capture_state_verified"])
            self.assertTrue(report["acceptance"]["capture_state_binding_verified"])
            self.assertFalse(report["acceptance"]["instrumentation_overhead_verified"])
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td), False)
            report = fx.verify("control")
            self.assertEqual(report["expected_mode"], "control")

    def test_profiled_requires_enabled_streams_and_matching_paths(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td), True)
            receipt = capture_receipt(True)
            receipt["streams"]["cpu_phase_timing"]["state"] = "not_requested"
            receipt["streams"]["cpu_phase_timing"]["output_path"] = None
            receipt["streams"]["cpu_phase_timing"]["warmup_frames"] = None
            receipt["streams"]["cpu_phase_timing"]["max_samples"] = None
            fx.rewrite_receipt(receipt, True)
            with self.assertRaises(capture_state.ProfilingCaptureStateError):
                fx.verify("profiled")
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td), True)
            receipt = capture_receipt(True)
            receipt["streams"]["cpu_frame_timing"]["output_path"] = "other.csv"
            fx.rewrite_receipt(receipt, True)
            with self.assertRaises(capture_state.ProfilingCaptureStateError):
                fx.verify("profiled")

    def test_control_requires_all_streams_not_requested(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td), False)
            receipt = capture_receipt(False)
            receipt["streams"]["cpu_frame_timing"] = {
                "state": "enabled",
                "output_path": "frame.csv",
                "warmup_frames": 120,
                "max_samples": 3600,
            }
            fx.rewrite_receipt(receipt, False)
            with self.assertRaises(capture_state.ProfilingCaptureStateError):
                fx.verify("control")

    def test_warmup_capacity_stride_and_source_are_bound(self):
        mutations = [
            lambda r: r["streams"]["cpu_frame_timing"].__setitem__("warmup_frames", 0),
            lambda r: r["streams"]["cpu_phase_timing"].__setitem__("max_samples", 3599),
            lambda r: r["streams"]["process_memory"].__setitem__("max_samples", 359),
            lambda r: r["streams"]["process_memory"].__setitem__(
                "sample_source", "caller_supplied_contract_sample"
            ),
        ]
        for mutate in mutations:
            with tempfile.TemporaryDirectory() as td:
                fx = Fixture(Path(td), True)
                receipt = capture_receipt(True)
                mutate(receipt)
                fx.rewrite_receipt(receipt, True)
                with self.assertRaises(capture_state.ProfilingCaptureStateError):
                    fx.verify("profiled")

    def test_run_control_mismatch_is_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td), True)
            receipt = capture_receipt(True)
            receipt["run_control"]["client_width_px"] = 1919
            fx.rewrite_receipt(receipt, True)
            with self.assertRaises(capture_state.ProfilingCaptureStateError):
                fx.verify("profiled")

    def test_receipt_role_and_claim_boundaries_are_strict(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td), True)
            changed = spec(True)
            changed["evidence"] = [
                item for item in changed["evidence"]
                if item["role"] != "profiling_capture_state_json"
            ]
            fx.rebuild(True, changed)
            with self.assertRaises(capture_state.ProfilingCaptureStateError):
                fx.verify("profiled")
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td), True)
            receipt = capture_receipt(True)
            receipt["claim_boundaries"]["instrumentation_overhead_verified"] = True
            fx.rewrite_receipt(receipt, True)
            with self.assertRaises(capture_state.ProfilingCaptureStateError):
                fx.verify("profiled")

    def test_tampering_is_rejected_by_manifest_hash(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td), True)
            (fx.evidence / "capture-state.json").write_text("{}\n", encoding="utf-8")
            with self.assertRaises(capture_state.ProfilingCaptureStateError):
                fx.verify("profiled")

    def test_cli_writes_once(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td), True)
            out = Path(td) / "report.json"
            script = Path(__file__).with_name("verify_profiling_capture_state.py")
            args = [
                sys.executable, str(script), str(fx.manifest), str(fx.package),
                str(fx.release_manifest), str(fx.evidence),
                "--expected-mode", "profiled", "--json", str(out),
            ]
            first = subprocess.run(args, capture_output=True, text=True, timeout=15)
            self.assertEqual(first.returncode, 0, first.stderr)
            second = subprocess.run(args, capture_output=True, text=True, timeout=15)
            self.assertNotEqual(second.returncode, 0)


if __name__ == "__main__":
    unittest.main(verbosity=2)
