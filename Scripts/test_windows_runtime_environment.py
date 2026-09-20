#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import tempfile
import unittest
from pathlib import Path

import probe_windows_runtime_environment as probe


def dependency(image: Path, *, machine="AMD64", vc=None, debug=None):
    vc = list(vc or [])
    debug = list(debug or [])
    return {
        "schema_version": 1,
        "sha256": hashlib.sha256(image.read_bytes()).hexdigest(),
        "machine": machine,
        "analysis": {
            "vc_runtime_imports": vc,
            "debug_crt_imports": debug,
        },
    }


def plan(dep, *, vc=None, strategy="central_vc_redist", clean=False, reject=False):
    vc = list(vc if vc is not None else dep["analysis"]["vc_runtime_imports"])
    arch = {"AMD64": "x64", "I386": "x86", "ARM64": "arm64"}[dep["machine"]]
    return {
        "schema_version": 1,
        "source_dependency_report": {
            "image_sha256": dep["sha256"],
            "machine": dep["machine"],
            "architecture": arch,
        },
        "release_policy": {
            "clean_machine_compatibility_verified": clean,
            "reject_release": reject,
        },
        "vc_runtime": {
            "strategy": strategy if vc else "not_required_by_import_table",
            "architecture": arch if vc else None,
            "imports": vc,
        },
    }


class RuntimeEnvironmentProbeTests(unittest.TestCase):
    def test_present_runtime_files_generate_hashes_versions_and_no_launch_claim(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            image = root / "AstralGame.exe"
            image.write_bytes(b"astral-image")
            system32 = root / "Windows" / "System32"
            system32.mkdir(parents=True)
            for name, payload in {"MSVCP140.dll": b"cpp", "VCRUNTIME140.dll": b"runtime"}.items():
                (system32 / name).write_bytes(payload)
            dep = dependency(image, vc=["MSVCP140.dll", "VCRUNTIME140.dll"])
            result = probe.probe_runtime(
                dep,
                plan(dep),
                image,
                system_root=root / "Windows",
                version_reader=lambda path: "14.50.35719.0",
            )
            self.assertTrue(result["acceptance"]["preflight_passed"])
            self.assertFalse(result["acceptance"]["package_launch_verified"])
            self.assertFalse(result["acceptance"]["clean_machine_compatibility_verified"])
            self.assertFalse(result["vc_runtime"]["runtime_version_compatibility_verified"])
            self.assertEqual(len(result["vc_runtime"]["files"]), 2)
            self.assertTrue(all(item["sha256"] for item in result["vc_runtime"]["files"]))

    def test_missing_runtime_file_fails_preflight_without_fabricating_metadata(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            image = root / "AstralGame.exe"; image.write_bytes(b"astral")
            (root / "Windows" / "System32").mkdir(parents=True)
            dep = dependency(image, vc=["VCRUNTIME140_1.dll"])
            result = probe.probe_runtime(dep, plan(dep), image, system_root=root / "Windows", version_reader=lambda path: "14.0")
            self.assertFalse(result["acceptance"]["preflight_passed"])
            self.assertEqual(result["vc_runtime"]["missing_imports"], ["VCRUNTIME140_1.dll"])

    def test_metadata_failure_fails_preflight(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            image = root / "AstralGame.exe"; image.write_bytes(b"astral")
            dll = root / "Windows" / "System32" / "MSVCP140.dll"; dll.parent.mkdir(parents=True); dll.write_bytes(b"x")
            dep = dependency(image, vc=[dll.name])
            def broken(_):
                raise probe.RuntimeProbeError("no version")
            result = probe.probe_runtime(dep, plan(dep), image, system_root=root / "Windows", version_reader=broken)
            self.assertFalse(result["acceptance"]["preflight_passed"])
            self.assertEqual(result["vc_runtime"]["metadata_failures"], [dll.name])

    def test_image_hash_mismatch_is_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            image = root / "AstralGame.exe"; image.write_bytes(b"before")
            dep = dependency(image, vc=[])
            image.write_bytes(b"after")
            with self.assertRaises(probe.RuntimeProbeError):
                probe.validate_evidence(dep, plan(dep), image)

    def test_stale_plan_or_import_mismatch_is_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            image = root / "AstralGame.exe"; image.write_bytes(b"astral")
            dep = dependency(image, vc=["MSVCP140.dll"])
            wrong = plan(dep, vc=["VCRUNTIME140.dll"])
            with self.assertRaises(probe.RuntimeProbeError):
                probe.validate_evidence(dep, wrong, image)
            wrong = plan(dep)
            wrong["source_dependency_report"]["image_sha256"] = "0" * 64
            with self.assertRaises(probe.RuntimeProbeError):
                probe.validate_evidence(dep, wrong, image)

    def test_rejects_path_like_dll_names_and_unreviewed_strategy(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            image = root / "AstralGame.exe"; image.write_bytes(b"astral")
            dep = dependency(image, vc=["../VCRUNTIME140.dll"])
            with self.assertRaises(probe.RuntimeProbeError):
                probe.validate_evidence(dep, plan(dep), image)
            dep = dependency(image, vc=["VCRUNTIME140.dll"])
            with self.assertRaises(probe.RuntimeProbeError):
                probe.validate_evidence(dep, plan(dep, strategy="app_local"), image)

    def test_release_rejection_or_false_clean_claim_is_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            image = root / "AstralGame.exe"; image.write_bytes(b"astral")
            dep = dependency(image, vc=["VCRUNTIME140.dll"])
            with self.assertRaises(probe.RuntimeProbeError):
                probe.validate_evidence(dep, plan(dep, reject=True), image)
            with self.assertRaises(probe.RuntimeProbeError):
                probe.validate_evidence(dep, plan(dep, clean=True), image)

    def test_architecture_selects_expected_central_runtime_directory(self):
        root = Path("C:/Windows")
        self.assertEqual(probe.central_runtime_directory(root, "x64"), root / "System32")
        self.assertEqual(probe.central_runtime_directory(root, "arm64"), root / "System32")
        self.assertEqual(probe.central_runtime_directory(root, "x86"), root / "SysWOW64")


if __name__ == "__main__":
    unittest.main(verbosity=2)
