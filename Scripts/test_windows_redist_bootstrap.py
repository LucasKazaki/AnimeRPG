#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

import plan_windows_redist_bootstrap as target

SHA = "a" * 64


def prereq(arch: str = "x64") -> dict:
    return {
        "schema_version": 1,
        "source_dependency_report": {"image_sha256": SHA, "machine": "AMD64", "architecture": arch},
        "release_policy": {
            "debug_runtime_imports": [], "reject_release": False, "requires_vc_runtime": True,
            "clean_machine_compatibility_verified": False, "clean_machine_launch_required": True,
        },
        "vc_runtime": {
            "strategy": "central_vc_redist", "architecture": arch,
            "imports": ["MSVCP140.dll"], "installer_bundled": False, "installer_executed": False,
            "app_local_fallback_selected": False, "static_runtime_switch_selected": False,
        },
    }


def compatibility(version: str = "14.44.35207") -> dict:
    return {
        "schema_version": 1,
        "package_sha256": SHA,
        "build_toolchain": {"vc_tools_version": version},
        "compatibility": {
            "runtime_file_version_floor_satisfied": True,
            "runtime_version_compatibility_verified": True,
            "supported_redist_installation_verified": False,
        },
        "acceptance": {
            "package_launch_verified": False,
            "clean_machine_compatibility_verified": False,
            "independent_acceptance": False,
        },
    }


def registry(arch: str = "x64", version: str = "v14.51.36247.0") -> dict:
    return {
        "architecture": arch,
        "version": version,
        "installed": 1,
        "components": {"Major": 14, "Minor": 51, "Bld": 36247, "Rbld": 0},
        "source": "fixture registry",
    }


class BootstrapTests(unittest.TestCase):
    def test_newer_registered_redist_skips_install(self) -> None:
        result = target.build_bootstrap_plan(prereq(), compatibility(), registry())
        self.assertEqual(result["bootstrap"]["action"], "skip_install")
        self.assertTrue(result["registered_v14_redist"]["version_floor_satisfied"])
        self.assertFalse(result["bootstrap"]["installer_executed"])
        self.assertFalse(result["acceptance"]["clean_machine_compatibility_verified"])

    def test_older_registered_redist_requires_install(self) -> None:
        old = registry(version="14.40.33816.0")
        old["components"] = {"Major": 14, "Minor": 40, "Bld": 33816, "Rbld": 0}
        result = target.build_bootstrap_plan(prereq(), compatibility(), old)
        self.assertEqual(result["bootstrap"]["action"], "install_latest_supported")
        self.assertFalse(result["registered_v14_redist"]["version_floor_satisfied"])

    def test_missing_registration_requires_install(self) -> None:
        result = target.build_bootstrap_plan(prereq(), compatibility(), None)
        self.assertEqual(result["bootstrap"]["action"], "install_latest_supported")
        self.assertIsNone(result["registered_v14_redist"])

    def test_architecture_maps_to_official_permalink(self) -> None:
        for arch, suffix in (("x64", "vc_redist.x64.exe"), ("x86", "vc_redist.x86.exe"), ("arm64", "vc_redist.arm64.exe")):
            p = prereq(arch)
            result = target.build_bootstrap_plan(p, compatibility(), None)
            self.assertTrue(result["bootstrap"]["official_latest_supported_permalink"].endswith(suffix))
            self.assertEqual(result["bootstrap"]["expected_installer_filename"], suffix)

    def test_rejects_stale_or_false_claims(self) -> None:
        c = compatibility(); c["package_sha256"] = "b" * 64
        with self.assertRaises(target.BootstrapPlanError):
            target.build_bootstrap_plan(prereq(), c, registry())
        c = compatibility(); c["compatibility"]["supported_redist_installation_verified"] = True
        with self.assertRaises(target.BootstrapPlanError):
            target.build_bootstrap_plan(prereq(), c, registry())
        p = prereq(); p["vc_runtime"]["installer_executed"] = True
        with self.assertRaises(target.BootstrapPlanError):
            target.build_bootstrap_plan(p, compatibility(), registry())

    def test_rejects_registry_component_inconsistency_and_wrong_arch(self) -> None:
        bad = registry(); bad["components"]["Minor"] = 50
        with self.assertRaises(target.BootstrapPlanError):
            target.build_bootstrap_plan(prereq(), compatibility(), bad)
        with self.assertRaises(target.BootstrapPlanError):
            target.build_bootstrap_plan(prereq(), compatibility(), registry(arch="x86"))

    def test_equal_version_skips_install(self) -> None:
        same = registry(version="14.44.35207.0")
        same["components"] = {"Major": 14, "Minor": 44, "Bld": 35207, "Rbld": 0}
        result = target.build_bootstrap_plan(prereq(), compatibility(), same)
        self.assertEqual(result["bootstrap"]["action"], "skip_install")

    def test_cli_fixture_writes_bound_json(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            p = root / "prereq.json"; c = root / "compat.json"; r = root / "registry.json"; out = root / "out.json"
            p.write_text(json.dumps(prereq()), encoding="utf-8")
            c.write_text(json.dumps(compatibility()), encoding="utf-8")
            r.write_text(json.dumps(registry()), encoding="utf-8")
            completed = subprocess.run(
                [sys.executable, str(Path(__file__).with_name("plan_windows_redist_bootstrap.py")), str(p), str(c), "--registry-snapshot", str(r), "--json", str(out)],
                text=True, capture_output=True, check=False,
            )
            self.assertEqual(completed.returncode, 0, completed.stderr)
            data = json.loads(out.read_text(encoding="utf-8"))
            self.assertEqual(data["package_sha256"], SHA)
            self.assertEqual(data["bootstrap"]["action"], "skip_install")
            self.assertFalse(data["bootstrap"]["installer_downloaded"])

    def test_non_windows_live_probe_refuses_portable_claim(self) -> None:
        if sys.platform == "win32":
            self.skipTest("portable refusal only applies off Windows")
        with self.assertRaises(target.BootstrapPlanError):
            target._read_registry_record("x64")


if __name__ == "__main__":
    unittest.main(verbosity=2)
