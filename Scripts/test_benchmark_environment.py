#!/usr/bin/env python3
from __future__ import annotations

import argparse
import copy
import hashlib
import json
import tempfile
import unittest
from pathlib import Path

import benchmark_environment as be

COMMIT = "1" * 40
EXE = "2" * 64
RECEIPT_SHA = "3" * 64
EVIDENCE_PATH = "native/windows-environment.json"


def payload():
    return {
        "operating_system": {"Caption": "Microsoft Windows 11 Pro", "Version": "10.0.26100", "BuildNumber": "26100", "OSArchitecture": "64-bit"},
        "computer_system": {"Manufacturer": "Example Corp", "Model": "Example Model", "NumberOfLogicalProcessors": 16, "TotalPhysicalMemory": 68719476736},
        "processors": [{"Name": "Example CPU", "Manufacturer": "Example", "NumberOfCores": 8, "NumberOfLogicalProcessors": 16}],
        "video_controllers": [{"Name": "Example GPU", "DriverVersion": "32.0.15.9999", "PNPDeviceID": "PCI\\VEN_0000", "AdapterRAM": 17179869184, "VideoProcessor": "Example GPU"}],
    }


def receipt(source="contract_fixture"):
    return be.normalize_cim_payload(payload(), "fixture-machine", source=source)


def manifest(r):
    return {
        "candidate": {"commit": COMMIT, "executable_sha256": EXE},
        "environment": copy.deepcopy(r["benchmark_environment"]),
        "provenance": {"evidence_class": "hosted_contract_fixture", "machine_label": r["machine_label"]},
        "evidence": [{"path": EVIDENCE_PATH, "role": "windows_benchmark_environment_json", "bytes": 123, "sha256": RECEIPT_SHA}],
        "benchmark_descriptor_sha256": "4" * 64,
    }


class BenchmarkEnvironmentTests(unittest.TestCase):
    def test_normalizes_expected_environment(self):
        r = receipt()
        self.assertEqual(r["benchmark_environment"]["logical_cpus"], 16)
        self.assertEqual(r["benchmark_environment"]["ram_bytes"], 68719476736)
        self.assertEqual(r["benchmark_environment"]["gpu"], "Example GPU")
        self.assertIn("26100", r["benchmark_environment"]["os"])
        self.assertTrue(all(v is False for v in r["acceptance"].values()))

    def test_multiple_gpus_are_sorted_and_driver_bound(self):
        p = payload()
        p["video_controllers"] = [
            {"Name": "Zeta GPU", "DriverVersion": "2.0", "PNPDeviceID": "PCI\\Z", "AdapterRAM": None, "VideoProcessor": None},
            {"Name": "Alpha GPU", "DriverVersion": "1.0", "PNPDeviceID": "PCI\\A", "AdapterRAM": 1, "VideoProcessor": "Alpha"},
        ]
        r = be.normalize_cim_payload(p, "m", source="contract_fixture")
        self.assertEqual(r["benchmark_environment"]["gpu"], "Alpha GPU; Zeta GPU")
        self.assertEqual(r["benchmark_environment"]["gpu_driver"], "Alpha GPU=1.0; Zeta GPU=2.0")

    def test_rejects_boolean_integer_laundering(self):
        p = payload()
        p["computer_system"]["NumberOfLogicalProcessors"] = True
        with self.assertRaises(be.EnvironmentError):
            be.normalize_cim_payload(p, "m", source="contract_fixture")

    def test_rejects_missing_gpu(self):
        p = payload()
        p["video_controllers"] = []
        with self.assertRaises(be.EnvironmentError):
            be.normalize_cim_payload(p, "m", source="contract_fixture")

    def test_raw_environment_tamper_rejected(self):
        r = receipt()
        r["raw"]["computer_system"]["total_physical_memory_bytes"] += 1
        with self.assertRaises(be.EnvironmentError):
            be.validate_receipt(r)

    def test_receipt_claim_boundary_is_immutable(self):
        r = receipt()
        r["acceptance"]["performance_budget_verified"] = True
        with self.assertRaises(be.EnvironmentError):
            be.validate_receipt(r)

    def test_hosted_fixture_binding_passes(self):
        r = receipt()
        m = manifest(r)
        report = be.verify_binding(m, r, receipt_sha256=RECEIPT_SHA, evidence_path=EVIDENCE_PATH, expected_commit=COMMIT, expected_executable_sha256=EXE)
        self.assertTrue(report["verification"]["manifest_environment_exact_match"])
        self.assertFalse(report["acceptance"]["environment_binding_verified"])

    def test_local_native_rejects_fixture_source(self):
        r = receipt()
        m = manifest(r)
        m["provenance"]["evidence_class"] = "local_native"
        with self.assertRaises(be.EnvironmentError):
            be.verify_binding(m, r, receipt_sha256=RECEIPT_SHA, evidence_path=EVIDENCE_PATH, expected_commit=COMMIT, expected_executable_sha256=EXE)

    def test_local_native_accepts_windows_source_contract(self):
        r = receipt(source="Windows_CIM_GetCimInstance")
        m = manifest(r)
        m["provenance"]["evidence_class"] = "local_native"
        report = be.verify_binding(m, r, receipt_sha256=RECEIPT_SHA, evidence_path=EVIDENCE_PATH, expected_commit=COMMIT, expected_executable_sha256=EXE)
        self.assertTrue(report["verification"]["native_source_required_and_verified"])

    def test_environment_mismatch_rejected(self):
        r = receipt()
        m = manifest(r)
        m["environment"]["ram_bytes"] += 1
        with self.assertRaises(be.EnvironmentError):
            be.verify_binding(m, r, receipt_sha256=RECEIPT_SHA, evidence_path=EVIDENCE_PATH, expected_commit=COMMIT, expected_executable_sha256=EXE)

    def test_machine_label_mismatch_rejected(self):
        r = receipt()
        m = manifest(r)
        m["provenance"]["machine_label"] = "other"
        with self.assertRaises(be.EnvironmentError):
            be.verify_binding(m, r, receipt_sha256=RECEIPT_SHA, evidence_path=EVIDENCE_PATH, expected_commit=COMMIT, expected_executable_sha256=EXE)

    def test_environment_hash_mismatch_rejected(self):
        r = receipt()
        m = manifest(r)
        with self.assertRaises(be.EnvironmentError):
            be.verify_binding(m, r, receipt_sha256="5" * 64, evidence_path=EVIDENCE_PATH, expected_commit=COMMIT, expected_executable_sha256=EXE)

    def test_duplicate_environment_role_rejected(self):
        r = receipt()
        m = manifest(r)
        m["evidence"].append(copy.deepcopy(m["evidence"][0]))
        with self.assertRaises(be.EnvironmentError):
            be.verify_binding(m, r, receipt_sha256=RECEIPT_SHA, evidence_path=EVIDENCE_PATH, expected_commit=COMMIT, expected_executable_sha256=EXE)

    def test_write_is_exclusive_and_round_trips(self):
        r = receipt()
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "env.json"
            be.write_fresh_json(path, r)
            loaded = be.load_receipt(path)
            self.assertEqual(loaded["benchmark_environment"], r["benchmark_environment"])
            with self.assertRaises(be.EnvironmentError):
                be.write_fresh_json(path, r)


@unittest.skipIf(be.benchmark_manifest is None, "repository benchmark_manifest module unavailable in partial fixture")
class ProductionManifestBindingTests(unittest.TestCase):
    def setUp(self):
        import release_manifest
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.package = self.root / "package"
        self.evidence = self.root / "evidence"
        self.package.mkdir(); self.evidence.mkdir()
        self.commit = "a" * 40
        (self.package / "AstralGame.exe").write_bytes(b"MZ bounded environment fixture\n")
        self.release = self.root / "release.json"
        release_manifest.write_release_manifest(self.package, self.release, self.commit)
        self.receipt_path = self.evidence / "windows-environment.json"
        self.receipt = be.normalize_cim_payload(payload(), "fixture-machine", source="contract_fixture")
        be.write_fresh_json(self.receipt_path, self.receipt)
        self.spec = {
            "candidate": {"commit": self.commit, "build_config": "Release"},
            "workload": {"id": "fixture-idle-3d", "dimension": "3d", "fixture_description": "Procedural idle fixture for environment binding contract testing."},
            "run_protocol": {"width": 1920, "height": 1080, "window_mode": "windowed", "vsync": False, "warmup_seconds": 1, "sample_seconds": 5},
            "environment": copy.deepcopy(self.receipt["benchmark_environment"]),
            "reference_versions": {"unreal": "5.8", "unity": "6.0"},
            "provenance": {"evidence_class": "hosted_contract_fixture", "machine_label": "fixture-machine"},
            "evidence": [{"path": "windows-environment.json", "role": "windows_benchmark_environment_json"}],
        }
        self.manifest = be.benchmark_manifest.build_benchmark_manifest(self.spec, self.package, self.release, self.evidence)
        self.exe_sha = self.manifest["candidate"]["executable_sha256"]
        self.manifest_path = self.root / "benchmark.json"
        self.manifest_path.write_text(json.dumps(self.manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    def tearDown(self):
        self.tmp.cleanup()

    def args(self, output_name="binding.json"):
        return argparse.Namespace(
            benchmark_manifest=str(self.manifest_path),
            package_root=str(self.package),
            release_manifest=str(self.release),
            evidence_root=str(self.evidence),
            environment_receipt=str(self.receipt_path),
            expected_commit=self.commit,
            expected_executable_sha256=self.exe_sha,
            output=str(self.root / output_name),
        )

    def test_real_production_manifest_binding(self):
        result = be.verify_manifest_bound_environment(self.args())
        self.assertEqual(result["candidate_commit"], self.commit)
        self.assertEqual(result["environment"], self.receipt["benchmark_environment"])
        self.assertEqual(result["environment_receipt"]["sha256"], hashlib.sha256(self.receipt_path.read_bytes()).hexdigest())
        self.assertTrue(all(value is False for value in result["acceptance"].values()))

    def test_production_manifest_rejects_tampered_receipt(self):
        data = json.loads(self.receipt_path.read_text(encoding="utf-8"))
        data["raw"]["computer_system"]["model"] = "tampered model"
        self.receipt_path.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        with self.assertRaises(be.EnvironmentError):
            be.verify_manifest_bound_environment(self.args())

    def test_production_binding_requires_exact_candidate(self):
        args = self.args()
        args.expected_commit = "b" * 40
        with self.assertRaises(be.EnvironmentError):
            be.verify_manifest_bound_environment(args)


if __name__ == "__main__":
    unittest.main(verbosity=2)
