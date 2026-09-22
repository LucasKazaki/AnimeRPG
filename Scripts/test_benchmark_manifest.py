#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

import benchmark_manifest
import release_manifest

COMMIT = "a" * 40


def valid_spec() -> dict:
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
            "warmup_seconds": 5.0,
            "sample_seconds": 30.0,
        },
        "environment": {
            "os": "Windows test fixture",
            "cpu": "Fixture CPU",
            "logical_cpus": 8,
            "ram_bytes": 16 * 1024**3,
            "gpu": "Fixture GPU",
            "gpu_driver": "fixture-driver-1",
        },
        "reference_versions": {"unreal": "5.8", "unity": "6000.0"},
        "provenance": {"evidence_class": "hosted_contract_fixture", "machine_label": "fixture"},
        "evidence": [
            {"path": "telemetry.jsonl", "role": "process_telemetry"},
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
        (self.package / "AstralGame.exe").write_bytes(b"astral-release-fixture")
        (self.package / "README.txt").write_text("fixture\n", encoding="utf-8")
        self.release_manifest = self.package / "MANIFEST.json"
        release_manifest.write_release_manifest(self.package, self.release_manifest, COMMIT)
        (self.evidence / "telemetry.jsonl").write_text('{"sample":1}\n', encoding="utf-8")
        (self.evidence / "astral.log").write_text("fixture log\n", encoding="utf-8")

    def build(self, spec: dict | None = None) -> dict:
        return benchmark_manifest.build_benchmark_manifest(
            spec or valid_spec(), self.package, self.release_manifest, self.evidence
        )

    def verify(self, manifest: dict) -> dict:
        return benchmark_manifest.verify_benchmark_manifest(
            manifest, self.package, self.release_manifest, self.evidence
        )


class BenchmarkManifestTests(unittest.TestCase):
    def test_round_trip_binds_package_protocol_environment_and_evidence(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            manifest = fx.build()
            report = fx.verify(manifest)
            self.assertTrue(report["benchmark_manifest_verified"])
            self.assertEqual(report["candidate_commit"], COMMIT)
            self.assertEqual(report["evidence_files_verified"], 2)
            self.assertFalse(report["acceptance"]["performance_budget_verified"])
            self.assertFalse(report["acceptance"]["comparative_parity_verified"])

    def test_release_package_mutation_is_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            manifest = fx.build()
            (fx.package / "AstralGame.exe").write_bytes(b"mutated")
            with self.assertRaises(benchmark_manifest.BenchmarkManifestError):
                fx.verify(manifest)

    def test_evidence_mutation_is_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            manifest = fx.build()
            (fx.evidence / "telemetry.jsonl").write_text('{"sample":2}\n', encoding="utf-8")
            with self.assertRaises(benchmark_manifest.BenchmarkManifestError):
                fx.verify(manifest)

    def test_commit_mismatch_is_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            spec = valid_spec()
            spec["candidate"]["commit"] = "b" * 40
            with self.assertRaises(benchmark_manifest.BenchmarkManifestError):
                fx.build(spec)

    def test_invalid_protocol_and_environment_are_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            bad_specs = []
            bad = valid_spec(); bad["run_protocol"]["width"] = 0; bad_specs.append(bad)
            bad = valid_spec(); bad["run_protocol"]["sample_seconds"] = 0; bad_specs.append(bad)
            bad = valid_spec(); bad["run_protocol"]["vsync"] = 1; bad_specs.append(bad)
            bad = valid_spec(); bad["environment"]["gpu_driver"] = ""; bad_specs.append(bad)
            bad = valid_spec(); bad["workload"]["dimension"] = "hybrid"; bad_specs.append(bad)
            for spec in bad_specs:
                with self.subTest(spec=spec):
                    with self.assertRaises(benchmark_manifest.BenchmarkManifestError):
                        fx.build(spec)

    def test_unsafe_and_case_colliding_evidence_paths_are_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            bad = valid_spec(); bad["evidence"][0]["path"] = "../escape.jsonl"
            with self.assertRaises(benchmark_manifest.BenchmarkManifestError):
                fx.build(bad)
            bad = valid_spec(); bad["evidence"] = [
                {"path": "astral.log", "role": "a"},
                {"path": "ASTRAL.LOG", "role": "b"},
            ]
            with self.assertRaises(benchmark_manifest.BenchmarkManifestError):
                fx.build(bad)

    @unittest.skipUnless(hasattr(os, "symlink"), "symlink unavailable")
    def test_symlinked_evidence_is_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            target = fx.evidence / "target.log"
            target.write_text("target\n", encoding="utf-8")
            link = fx.evidence / "linked.log"
            try:
                os.symlink(target, link)
            except OSError as exc:
                self.skipTest(f"symlink creation unavailable: {exc}")
            spec = valid_spec()
            spec["evidence"] = [{"path": "linked.log", "role": "linked"}]
            with self.assertRaises(benchmark_manifest.BenchmarkManifestError):
                fx.build(spec)

    def test_false_acceptance_claims_and_descriptor_tampering_are_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            manifest = fx.build()
            manifest["acceptance"]["performance_budget_verified"] = True
            with self.assertRaises(benchmark_manifest.BenchmarkManifestError):
                fx.verify(manifest)
            manifest = fx.build()
            manifest["environment"]["gpu"] = "different"
            with self.assertRaises(benchmark_manifest.BenchmarkManifestError):
                fx.verify(manifest)

    def test_release_manifest_hash_tampering_is_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            manifest = fx.build()
            manifest["candidate"]["release_manifest_sha256"] = "0" * 64
            with self.assertRaises(benchmark_manifest.BenchmarkManifestError):
                fx.verify(manifest)

    def test_bool_laundering_in_verification_fields_is_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            manifest = fx.build()
            manifest["acceptance"]["performance_budget_verified"] = 0
            with self.assertRaises(benchmark_manifest.BenchmarkManifestError):
                fx.verify(manifest)
            manifest = fx.build()
            manifest["candidate"]["package_files_verified"] = True
            with self.assertRaises(benchmark_manifest.BenchmarkManifestError):
                fx.verify(manifest)
            manifest = fx.build()
            manifest["evidence"][0]["bytes"] = False
            with self.assertRaises(benchmark_manifest.BenchmarkManifestError):
                fx.verify(manifest)

    @unittest.skipUnless(hasattr(os, "symlink"), "symlink unavailable")
    def test_intermediate_symlink_evidence_path_is_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            fx = Fixture(Path(td))
            real = fx.evidence / "real"
            real.mkdir()
            (real / "sample.log").write_text("sample\n", encoding="utf-8")
            alias = fx.evidence / "alias"
            try:
                os.symlink(real, alias, target_is_directory=True)
            except OSError as exc:
                self.skipTest(f"directory symlink creation unavailable: {exc}")
            spec = valid_spec()
            spec["evidence"] = [{"path": "alias/sample.log", "role": "linked"}]
            with self.assertRaises(benchmark_manifest.BenchmarkManifestError):
                fx.build(spec)

    def test_cli_round_trip(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            fx = Fixture(root)
            spec_path = root / "spec.json"
            out = root / "benchmark.json"
            report = root / "verify.json"
            spec_path.write_text(json.dumps(valid_spec()), encoding="utf-8")
            script = Path(__file__).with_name("benchmark_manifest.py")
            create = subprocess.run(
                [sys.executable, str(script), "create", str(spec_path), str(fx.package), str(fx.release_manifest), str(fx.evidence), str(out)],
                capture_output=True, text=True, timeout=15,
            )
            self.assertEqual(create.returncode, 0, create.stderr)
            verify = subprocess.run(
                [sys.executable, str(script), "verify", str(out), str(fx.package), str(fx.release_manifest), str(fx.evidence), "--json", str(report)],
                capture_output=True, text=True, timeout=15,
            )
            self.assertEqual(verify.returncode, 0, verify.stderr)
            data = json.loads(report.read_text(encoding="utf-8"))
            self.assertTrue(data["benchmark_manifest_verified"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
