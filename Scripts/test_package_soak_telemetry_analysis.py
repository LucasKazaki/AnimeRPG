#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

import analyze_package_soak_telemetry as mod

COMMIT = "1" * 40
EXE_HASH = "2" * 64


def write_fixture(root: Path, *, samples: list[dict] | None = None, passed: bool = True) -> tuple[Path, Path]:
    telemetry = root / "telemetry.jsonl"
    if samples is None:
        samples = []
        for i, elapsed in enumerate((0.5, 10.5, 20.5, 30.5)):
            samples.append({
                "timestamp": f"2026-09-20T20:00:{i:02d}-04:00",
                "elapsed_seconds": elapsed,
                "working_set_bytes": 100 + i * 10,
                "peak_working_set_bytes": 150 + i * 10,
                "private_usage_bytes": 200 + i * 20,
                "pagefile_usage_bytes": 200 + i * 20,
                "peak_pagefile_usage_bytes": 240 + i * 20,
                "handle_count": 20 + i,
                "gdi_objects": 5 + i,
                "user_objects": 4 + i,
            })
    telemetry.write_text("".join(json.dumps(s, sort_keys=True) + "\n" for s in samples), encoding="utf-8")
    telemetry_hash = hashlib.sha256(telemetry.read_bytes()).hexdigest()
    summary = {"sample_count": len(samples)}
    for metric in mod.METRICS:
        values = [s[metric] for s in samples]
        summary[metric] = {
            "first": values[0], "last": values[-1], "min": min(values), "max": max(values),
            "delta_first_to_last": values[-1] - values[0],
        }
    receipt = {
        "schema_version": 1,
        "commit": COMMIT,
        "AstralGame_sha256": EXE_HASH,
        "requested_duration_seconds": 30.0,
        "elapsed_seconds": 31.0,
        "telemetry_sha256": telemetry_hash,
        "telemetry_summary": summary,
        "passed": passed,
        "acceptance": {
            "required_24h_duration_observed": False,
            "required_24h_soak_verified": False,
            "ram_budget_verified": False,
            "vram_budget_verified": False,
            "frame_time_budget_verified": False,
            "memory_leak_free_verified": False,
            "clean_machine_compatibility_verified": False,
            "owned_interactive_desktop_verified": False,
            "independent_acceptance": False,
        },
    }
    receipt_path = root / "soak.json"
    receipt_path.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    return receipt_path, telemetry


class SoakTelemetryAnalysisTests(unittest.TestCase):
    def test_valid_fixture_binds_and_reports_descriptive_stats(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            receipt, telemetry = write_fixture(Path(td))
            result = mod.analyze(receipt, telemetry, expected_commit=COMMIT, expected_executable_sha256=EXE_HASH)
            self.assertTrue(result["acceptance"]["telemetry_integrity_verified"])
            self.assertTrue(result["acceptance"]["soak_receipt_binding_verified"])
            self.assertFalse(result["acceptance"]["memory_leak_free_verified"])
            self.assertEqual(result["sample_count"], 4)
            self.assertEqual(result["metrics"]["private_usage_bytes"]["delta_first_to_last"], 60)
            self.assertGreater(result["metrics"]["private_usage_bytes"]["ols_slope_per_hour"], 0)
            self.assertEqual(result["sample_intervals"]["count"], 3)

    def test_hash_mismatch_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            receipt, telemetry = write_fixture(Path(td))
            telemetry.write_text(telemetry.read_text(encoding="utf-8") + " ", encoding="utf-8")
            with self.assertRaisesRegex(mod.SoakAnalysisError, "SHA-256"):
                mod.analyze(receipt, telemetry)

    def test_summary_mismatch_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            receipt, telemetry = write_fixture(Path(td))
            data = json.loads(receipt.read_text(encoding="utf-8"))
            data["telemetry_summary"]["private_usage_bytes"]["max"] += 1
            receipt.write_text(json.dumps(data), encoding="utf-8")
            with self.assertRaisesRegex(mod.SoakAnalysisError, "private_usage_bytes"):
                mod.analyze(receipt, telemetry)

    def test_non_monotonic_elapsed_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            samples = []
            for i, elapsed in enumerate((1.0, 1.0)):
                samples.append({
                    "timestamp": str(i), "elapsed_seconds": elapsed,
                    "working_set_bytes": 1, "peak_working_set_bytes": 1, "private_usage_bytes": 1,
                    "pagefile_usage_bytes": 1, "peak_pagefile_usage_bytes": 1, "handle_count": 1,
                    "gdi_objects": 1, "user_objects": 1,
                })
            receipt, telemetry = write_fixture(root, samples=samples)
            with self.assertRaisesRegex(mod.SoakAnalysisError, "strictly increase"):
                mod.analyze(receipt, telemetry)

    def test_negative_or_boolean_metric_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            receipt, telemetry = write_fixture(root)
            lines = [json.loads(x) for x in telemetry.read_text(encoding="utf-8").splitlines()]
            lines[1]["handle_count"] = True
            telemetry.write_text("".join(json.dumps(s) + "\n" for s in lines), encoding="utf-8")
            data = json.loads(receipt.read_text(encoding="utf-8"))
            data["telemetry_sha256"] = hashlib.sha256(telemetry.read_bytes()).hexdigest()
            receipt.write_text(json.dumps(data), encoding="utf-8")
            with self.assertRaisesRegex(mod.SoakAnalysisError, "handle_count"):
                mod.analyze(receipt, telemetry)

    def test_false_acceptance_claim_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            receipt, telemetry = write_fixture(Path(td))
            data = json.loads(receipt.read_text(encoding="utf-8"))
            data["acceptance"]["memory_leak_free_verified"] = True
            receipt.write_text(json.dumps(data), encoding="utf-8")
            with self.assertRaisesRegex(mod.SoakAnalysisError, "memory_leak_free_verified"):
                mod.analyze(receipt, telemetry)

    def test_inconsistent_24h_observation_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            receipt, telemetry = write_fixture(Path(td))
            data = json.loads(receipt.read_text(encoding="utf-8"))
            data["acceptance"]["required_24h_duration_observed"] = True
            receipt.write_text(json.dumps(data), encoding="utf-8")
            with self.assertRaisesRegex(mod.SoakAnalysisError, "24-hour"):
                mod.analyze(receipt, telemetry)

    def test_expected_identity_mismatch_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            receipt, telemetry = write_fixture(Path(td))
            with self.assertRaisesRegex(mod.SoakAnalysisError, "expected commit"):
                mod.analyze(receipt, telemetry, expected_commit="a" * 40)
            with self.assertRaisesRegex(mod.SoakAnalysisError, "expected hash"):
                mod.analyze(receipt, telemetry, expected_executable_sha256="b" * 64)

    def test_cli_writes_json_and_preserves_claim_guards(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            receipt, telemetry = write_fixture(root)
            out = root / "analysis.json"
            proc = subprocess.run(
                [sys.executable, str(Path(mod.__file__)), str(receipt), str(telemetry),
                 "--expected-commit", COMMIT, "--expected-executable-sha256", EXE_HASH,
                 "--json", str(out)],
                capture_output=True, text=True, timeout=10,
            )
            self.assertEqual(proc.returncode, 0, proc.stderr)
            data = json.loads(out.read_text(encoding="utf-8"))
            self.assertTrue(data["passed"])
            self.assertFalse(data["acceptance"]["required_24h_soak_verified"])
            self.assertFalse(data["acceptance"]["independent_acceptance"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
