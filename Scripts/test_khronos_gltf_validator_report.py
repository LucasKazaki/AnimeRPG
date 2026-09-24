#!/usr/bin/env python3
from __future__ import annotations

import copy
import hashlib
import json
import tempfile
from pathlib import Path

import verify_khronos_gltf_validator_report as verify


def expect_fail(name, report, asset, sha, needle):
    try:
        verify.verify_report(report, asset_path=asset, expected_sha256=sha)
    except verify.VerificationError as exc:
        if needle not in str(exc):
            raise AssertionError(f"{name}: wrong failure {exc!r}") from exc
        return
    raise AssertionError(f"{name}: expected failure")


def main() -> int:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        asset = root / "mall_core_panel_blockout.gltf"
        asset.write_text('{"asset":{"version":"2.0"}}\n', encoding="utf-8")
        sha = hashlib.sha256(asset.read_bytes()).hexdigest()
        valid = {
            "uri": str(asset),
            "mimeType": "model/gltf+json",
            "validatorVersion": "2.0.0-dev.3.10",
            "issues": {
                "numErrors": 0,
                "numWarnings": 0,
                "numInfos": 1,
                "numHints": 0,
                "messages": [
                    {"code": "UNUSED_OBJECT", "severity": 2, "message": "informational fixture", "pointer": "/meshes/0"}
                ],
                "truncated": False,
            },
            "info": {
                "version": "2.0",
                "resources": [{"pointer": "/buffers/0", "storage": "data-uri", "byteLength": 4}],
            },
        }

        receipt = verify.verify_report(valid, asset_path=asset, expected_sha256=sha)
        assert receipt["errors"] == 0 and receipt["warnings"] == 0

        cases = []
        r = copy.deepcopy(valid); r["issues"]["numErrors"] = True; cases.append(("bool count", r, "JSON integer"))
        r = copy.deepcopy(valid); r["issues"]["numErrors"] = 1; r["issues"]["numInfos"] = 0; r["issues"]["messages"][0]["severity"] = 0; cases.append(("validator error", r, "reported 1 error"))
        r = copy.deepcopy(valid); r["issues"]["numWarnings"] = 1; r["issues"]["numInfos"] = 0; r["issues"]["messages"][0]["severity"] = 1; cases.append(("warning gate", r, "warning(s), limit is 0"))
        r = copy.deepcopy(valid); r["issues"]["truncated"] = True; cases.append(("truncated", r, "truncated"))
        r = copy.deepcopy(valid); r["uri"] = "different.gltf"; cases.append(("wrong uri", r, "does not identify"))
        r = copy.deepcopy(valid); r["validatorVersion"] = "not-semver"; cases.append(("bad version", r, "semver"))
        r = copy.deepcopy(valid); r["issues"]["numInfos"] = 0; cases.append(("count mismatch", r, "summary does not match"))
        r = copy.deepcopy(valid); r["issues"]["messages"][0]["severity"] = True; cases.append(("bool severity", r, "JSON integer"))
        r = copy.deepcopy(valid); r["info"]["version"] = "1.0"; cases.append(("wrong gltf version", r, "must be '2.0'"))
        r = copy.deepcopy(valid); r["info"]["resources"][0] = {"pointer": "/buffers/0", "storage": "external", "uri": "buf.bin"}; cases.append(("external resource", r, "is external"))
        r = copy.deepcopy(valid); del r["info"]["resources"][0]["storage"]; cases.append(("missing storage", r, "required to prove self-contained"))
        r = copy.deepcopy(valid); r["info"]["resources"][0]["storage"] = True; cases.append(("bool storage", r, "known Khronos storage string"))
        r = copy.deepcopy(valid); r["info"]["resources"][0]["storage"] = "mystery"; cases.append(("unknown storage", r, "known Khronos storage string"))
        r = copy.deepcopy(valid); r["unexpected"] = 1; cases.append(("unknown root", r, "unknown fields"))
        r = copy.deepcopy(valid); r["issues"]["messages"][0]["offset"] = 5; cases.append(("pointer and offset", r, "exactly one"))

        for name, report, needle in cases:
            expect_fail(name, report, asset, sha, needle)

        expect_fail("wrong hash", valid, asset, "0" * 64, "SHA-256 mismatch")

        report_path = root / "report.json"
        report_path.write_text(json.dumps(valid), encoding="utf-8")
        rc = verify.main([
            "--asset", str(asset),
            "--report", str(report_path),
            "--expected-sha256", sha,
        ])
        assert rc == 0

    print("PASS: 18/18 Khronos glTF report adapter tests")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
