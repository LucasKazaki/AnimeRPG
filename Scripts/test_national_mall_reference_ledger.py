from __future__ import annotations

import copy
import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VERIFIER = ROOT / "Scripts/verify_national_mall_reference_ledger.py"
LEDGER = ROOT / "Content/Reference/NationalMall/reference-ledger.json"


def run(path: Path, expect_ok: bool = True) -> subprocess.CompletedProcess[str]:
    proc = subprocess.run([sys.executable, str(VERIFIER), str(path)], capture_output=True, text=True)
    if expect_ok and proc.returncode != 0:
        raise AssertionError(proc.stderr or proc.stdout)
    if not expect_ok and proc.returncode == 0:
        raise AssertionError("expected verifier rejection")
    return proc


def mutate(fn) -> subprocess.CompletedProcess[str]:
    data = json.loads(LEDGER.read_text(encoding="utf-8"))
    fn(data)
    with tempfile.TemporaryDirectory() as td:
        path = Path(td) / "ledger.json"
        path.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        return run(path, expect_ok=False)


def test_valid() -> None:
    proc = run(LEDGER)
    assert "11 authoritative" in proc.stdout


def test_unknown_root_field_rejected() -> None:
    proc = mutate(lambda d: d.__setitem__("runtime_verified", True))
    assert "root keys" in proc.stderr


def test_unknown_entry_field_rejected() -> None:
    proc = mutate(lambda d: d["entries"][0].__setitem__("image_url", "data:image/png;base64,AAAA"))
    assert "entry keys" in proc.stderr


def test_lincoln_derived_measurement_mismatch_rejected() -> None:
    def change(d):
        entry = next(e for e in d["entries"] if e["id"] == "lincoln-memorial-materials")
        entry["measurements"][0]["value"] = 1
    proc = mutate(change)
    assert "derived measurement value" in proc.stderr


def test_washington_derived_measurement_mismatch_rejected() -> None:
    def change(d):
        entry = next(e for e in d["entries"] if e["id"] == "washington-monument-massing")
        entry["measurements"][0]["value"] = 1
    proc = mutate(change)
    assert "derived measurement value" in proc.stderr


def test_schema_bool_rejected() -> None:
    proc = mutate(lambda d: d.__setitem__("schema_version", True))
    assert "schema_version" in proc.stderr


def test_false_runtime_status_rejected() -> None:
    proc = mutate(lambda d: d.__setitem__("status", "runtime_verified"))
    assert "status" in proc.stderr


def test_duplicate_id_rejected() -> None:
    def change(d):
        d["entries"][1]["id"] = d["entries"][0]["id"]
    proc = mutate(change)
    assert "entry id" in proc.stderr


def test_non_https_rejected() -> None:
    proc = mutate(lambda d: d["entries"][0].__setitem__("url", "http://www.nps.gov/places/000/national-mall.htm"))
    assert "authoritative https url" in proc.stderr


def test_unapproved_host_rejected() -> None:
    proc = mutate(lambda d: d["entries"][0].__setitem__("url", "https://example.com/mall"))
    assert "authoritative https url" in proc.stderr



def test_pdf_kind_requires_pdf_url() -> None:
    proc = mutate(lambda d: d["entries"][0].__setitem__("source_kind", "official_pdf"))
    assert "pdf source url" in proc.stderr


def test_embedded_media_rejected() -> None:
    proc = mutate(lambda d: d["entries"][0].__setitem__("embedded_media", True))
    assert "embedded_media" in proc.stderr


def test_missing_facts_rejected() -> None:
    proc = mutate(lambda d: d["entries"][0].__setitem__("facts", []))
    assert "facts" in proc.stderr


def test_measurement_bool_rejected() -> None:
    proc = mutate(lambda d: d["entries"][0]["measurements"][0].__setitem__("value", True))
    assert "measurement value" in proc.stderr


def test_unknown_unit_rejected() -> None:
    proc = mutate(lambda d: d["entries"][0]["measurements"][0].__setitem__("unit", "meters"))
    assert "measurement unit" in proc.stderr


def test_unknown_source_relation_rejected() -> None:
    proc = mutate(lambda d: d["entries"][0]["measurements"][0].__setitem__("source_relation", "guessed"))
    assert "measurement provenance" in proc.stderr


def test_missing_required_coverage_rejected() -> None:
    proc = mutate(lambda d: d.__setitem__("required_coverage", ["axis_scale", "materials"]))
    assert "required_coverage" in proc.stderr


TESTS = [
    test_valid,
    test_unknown_root_field_rejected,
    test_unknown_entry_field_rejected,
    test_lincoln_derived_measurement_mismatch_rejected,
    test_washington_derived_measurement_mismatch_rejected,
    test_schema_bool_rejected,
    test_false_runtime_status_rejected,
    test_duplicate_id_rejected,
    test_non_https_rejected,
    test_unapproved_host_rejected,
    test_pdf_kind_requires_pdf_url,
    test_embedded_media_rejected,
    test_missing_facts_rejected,
    test_measurement_bool_rejected,
    test_unknown_unit_rejected,
    test_unknown_source_relation_rejected,
    test_missing_required_coverage_rejected,
]


def main() -> None:
    failures = []
    for test in TESTS:
        try:
            test()
        except Exception as exc:
            failures.append((test.__name__, str(exc)))
    if failures:
        for name, message in failures:
            print(f"FAIL {name}: {message}")
        raise SystemExit(1)
    print(f"PASS: {len(TESTS)}/{len(TESTS)} National Mall reference-ledger tests")


if __name__ == "__main__":
    main()
