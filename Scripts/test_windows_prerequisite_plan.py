#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

import plan_windows_prerequisites as policy


def report(*, machine="AMD64", vc=None, debug=None, clean=False):
    vc = list(vc or [])
    debug = list(debug or [])
    api = ["api-ms-win-crt-runtime-l1-1-0.dll"]
    all_imports = ["KERNEL32.dll", *api, *vc, *debug]
    return {
        "schema_version": 1,
        "sha256": "a" * 64,
        "machine": machine,
        "all_imports": all_imports,
        "analysis": {
            "debug_crt_imports": debug,
            "vc_runtime_imports": vc,
            "ucrt_imports": [],
            "api_set_imports": api,
            "clean_machine_compatibility_verified": clean,
        },
    }


class PrerequisitePlanTests(unittest.TestCase):
    def test_vc_runtime_selects_central_redist_without_claiming_bundle(self):
        plan = policy.build_plan(report(vc=["MSVCP140.dll", "VCRUNTIME140_1.dll"]))
        self.assertTrue(plan["release_policy"]["requires_vc_runtime"])
        self.assertFalse(plan["release_policy"]["clean_machine_compatibility_verified"])
        self.assertEqual(plan["vc_runtime"]["strategy"], "central_vc_redist")
        self.assertEqual(plan["vc_runtime"]["architecture"], "x64")
        self.assertFalse(plan["vc_runtime"]["installer_bundled"])
        self.assertFalse(plan["vc_runtime"]["installer_executed"])
        self.assertIn("learn.microsoft.com", plan["vc_runtime"]["official_download_reference"])

    def test_no_vc_imports_does_not_invent_a_redist_requirement(self):
        plan = policy.build_plan(report())
        self.assertFalse(plan["release_policy"]["requires_vc_runtime"])
        self.assertEqual(plan["vc_runtime"]["strategy"], "not_required_by_import_table")
        self.assertIsNone(plan["vc_runtime"]["architecture"])
        self.assertTrue(plan["release_policy"]["clean_machine_launch_required"])

    def test_machine_maps_to_installer_architecture(self):
        self.assertEqual(
            policy.build_plan(report(machine="I386"))["source_dependency_report"]["architecture"],
            "x86",
        )
        self.assertEqual(
            policy.build_plan(report(machine="ARM64"))["source_dependency_report"]["architecture"],
            "arm64",
        )

    def test_debug_runtime_rejects_release(self):
        plan = policy.build_plan(report(debug=["VCRUNTIME140D.dll"]))
        self.assertTrue(plan["release_policy"]["reject_release"])

    def test_unknown_machine_is_rejected(self):
        with self.assertRaises(policy.PrerequisitePlanError):
            policy.build_plan(report(machine="UNKNOWN"))

    def test_analysis_cannot_reference_missing_import(self):
        data = report(vc=["MSVCP140.dll"])
        data["all_imports"].remove("MSVCP140.dll")
        with self.assertRaises(policy.PrerequisitePlanError):
            policy.build_plan(data)

    def test_dependency_report_cannot_claim_clean_machine_verification(self):
        with self.assertRaises(policy.PrerequisitePlanError):
            policy.build_plan(report(clean=True))

    def test_cli_writes_json_and_debug_mode_returns_two(self):
        root = Path(__file__).resolve().parent
        script = root / "plan_windows_prerequisites.py"
        with tempfile.TemporaryDirectory() as temp:
            temp_path = Path(temp)
            dependency = temp_path / "deps.json"
            output = temp_path / "plan.json"
            dependency.write_text(json.dumps(report(vc=["MSVCP140.dll"])), encoding="utf-8")
            completed = subprocess.run(
                [sys.executable, script, dependency, "--json", output, "--fail-on-debug-runtime"],
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(completed.returncode, 0, completed.stderr)
            parsed = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(parsed["vc_runtime"]["strategy"], "central_vc_redist")

            dependency.write_text(json.dumps(report(debug=["MSVCP140D.dll"])), encoding="utf-8")
            completed = subprocess.run(
                [sys.executable, script, dependency, "--fail-on-debug-runtime"],
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(completed.returncode, 2)


if __name__ == "__main__":
    unittest.main(verbosity=2)
