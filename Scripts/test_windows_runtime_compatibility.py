#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

import audit_windows_runtime_compatibility as audit


def fixture(version: str = "14.44.35211.0") -> dict:
    sha = "a" * 64
    names = ["MSVCP140.dll", "VCRUNTIME140.dll", "VCRUNTIME140_1.dll"]
    return {
        "schema_version": 1,
        "package": {"sha256": sha, "architecture": "x64"},
        "vc_runtime": {
            "strategy": "central_vc_redist",
            "required_imports": names,
            "files": [
                {"name": name, "present": True, "file_version": version, "sha256": "b" * 64, "size_bytes": 1}
                for name in names
            ],
            "missing_imports": [],
            "metadata_failures": [],
            "all_required_files_present": True,
            "all_present_files_versioned": True,
            "runtime_version_compatibility_verified": False,
        },
        "acceptance": {
            "preflight_passed": True,
            "package_launch_verified": False,
            "clean_machine_compatibility_verified": False,
            "independent_acceptance": False,
        },
    }


class CompatibilityTests(unittest.TestCase):
    def test_equal_or_newer_runtime_satisfies_floor(self) -> None:
        result = audit.audit_runtime_compatibility(fixture(), "14.44.35207")
        self.assertTrue(result["compatibility"]["runtime_file_version_floor_satisfied"])
        self.assertTrue(result["compatibility"]["runtime_version_compatibility_verified"])
        self.assertFalse(result["compatibility"]["supported_redist_installation_verified"])
        self.assertFalse(result["acceptance"]["package_launch_verified"])

    def test_older_runtime_fails_floor_without_false_acceptance(self) -> None:
        result = audit.audit_runtime_compatibility(fixture("14.44.35100.0"), "14.44.35207")
        self.assertFalse(result["compatibility"]["runtime_file_version_floor_satisfied"])
        self.assertFalse(result["compatibility"]["runtime_version_compatibility_verified"])
        self.assertFalse(result["acceptance"]["clean_machine_compatibility_verified"])

    def test_major_minor_comparison_is_numeric(self) -> None:
        self.assertFalse(audit.audit_runtime_compatibility(fixture("14.9.99999.0"), "14.44.1")["compatibility"]["runtime_file_version_floor_satisfied"])
        self.assertTrue(audit.audit_runtime_compatibility(fixture("14.50.1.0"), "14.44.99999")["compatibility"]["runtime_file_version_floor_satisfied"])

    def test_malformed_versions_are_rejected(self) -> None:
        for value in ("19.44", "14.44.x", "14.44.1.2.3", "-14.44.1", ""):
            with self.assertRaises(audit.RuntimeCompatibilityError):
                audit.parse_version(value, "version")

    def test_inconsistent_preflight_is_rejected(self) -> None:
        data = fixture()
        data["vc_runtime"]["files"].pop()
        with self.assertRaises(audit.RuntimeCompatibilityError):
            audit.audit_runtime_compatibility(data, "14.44.35207")
        data = fixture()
        data["acceptance"]["clean_machine_compatibility_verified"] = True
        with self.assertRaises(audit.RuntimeCompatibilityError):
            audit.audit_runtime_compatibility(data, "14.44.35207")

    def test_duplicate_runtime_entries_are_rejected(self) -> None:
        data = fixture()
        data["vc_runtime"]["files"].append(dict(data["vc_runtime"]["files"][0]))
        with self.assertRaises(audit.RuntimeCompatibilityError):
            audit.audit_runtime_compatibility(data, "14.44.35207")

    def test_cli_exit_codes_and_json(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            source = root / "preflight.json"
            output = root / "audit.json"
            source.write_text(json.dumps(fixture()), encoding="utf-8")
            command = [sys.executable, str(Path(__file__).with_name("audit_windows_runtime_compatibility.py")),
                       str(source), "--vc-tools-version", "14.44.35207", "--json", str(output),
                       "--fail-on-incompatible"]
            ok = subprocess.run(command, text=True, capture_output=True, check=False)
            self.assertEqual(ok.returncode, 0, ok.stderr)
            result = json.loads(output.read_text(encoding="utf-8"))
            self.assertTrue(result["compatibility"]["runtime_file_version_floor_satisfied"])

            source.write_text(json.dumps(fixture("14.44.1.0")), encoding="utf-8")
            bad = subprocess.run(command, text=True, capture_output=True, check=False)
            self.assertEqual(bad.returncode, 2, bad.stderr)


if __name__ == "__main__":
    unittest.main(verbosity=2)
