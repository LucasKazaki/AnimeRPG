#!/usr/bin/env python3
from __future__ import annotations

import json
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

import diagnose_pe_reproducibility as diag


def make_pe(
    *,
    timestamp: int = 1,
    checksum: int = 0,
    guid_byte: int = 1,
    pdb_path: str = r"C:\\a\\Astral.pdb",
    repro: bool = False,
    payload_byte: int = 0x41,
) -> bytes:
    data = bytearray(0x400)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 0x3C, 0x80)
    data[0x80:0x84] = b"PE\0\0"
    coff = 0x84
    struct.pack_into("<H", data, coff, 0x8664)
    struct.pack_into("<H", data, coff + 2, 1)
    struct.pack_into("<I", data, coff + 4, timestamp)
    struct.pack_into("<H", data, coff + 16, 240)
    optional = coff + 20
    struct.pack_into("<H", data, optional, 0x20B)
    struct.pack_into("<I", data, optional + 64, checksum)
    struct.pack_into("<I", data, optional + 108, 16)
    debug_rva = 0x1000
    debug_size = 56 if repro else 28
    struct.pack_into("<II", data, optional + 112 + 6 * 8, debug_rva, debug_size)

    section = optional + 240
    data[section : section + 8] = b".rdata\0\0"
    struct.pack_into("<I", data, section + 8, 0x200)
    struct.pack_into("<I", data, section + 12, 0x1000)
    struct.pack_into("<I", data, section + 16, 0x200)
    struct.pack_into("<I", data, section + 20, 0x200)

    cv_ptr = 0x260
    cv = b"RSDS" + bytes([guid_byte]) * 16 + struct.pack("<I", 1) + pdb_path.encode("utf-8") + b"\0"
    struct.pack_into("<IIHHIIII", data, 0x200, 0, timestamp, 0, 0, 2, len(cv), 0x1060, cv_ptr)
    data[cv_ptr : cv_ptr + len(cv)] = cv
    if repro:
        struct.pack_into("<IIHHIIII", data, 0x21C, 0, 0, 0, 0, 16, 0, 0, 0)
    data[0x350] = payload_byte
    return bytes(data)


class DiagnosticTests(unittest.TestCase):
    def test_identical(self) -> None:
        data = make_pe(repro=True)
        report = diag.compare_bytes(data, data)
        self.assertEqual(report["comparison"]["classification"], "identical")
        self.assertTrue(report["comparison"]["byte_reproducibility_observed_for_supplied_files"])
        self.assertTrue(report["image_a"]["reproducible_debug_entry_present"])
        self.assertFalse(report["comparison"]["deterministic_linker_contract_established"])
        self.assertFalse(report["comparison"]["cross_machine_reproducibility_verified"])
        self.assertFalse(report["comparison"]["independent_acceptance"])

    def test_timestamp_only_is_known_metadata(self) -> None:
        report = diag.compare_bytes(make_pe(timestamp=1), make_pe(timestamp=2))
        self.assertEqual(report["comparison"]["classification"], "recognized_pe_metadata_only")
        self.assertIn("coff_timestamp", report["comparison"]["changing_field_classes"])
        self.assertIn("debug_directory_timestamp", report["comparison"]["changing_field_classes"])
        self.assertFalse(report["comparison"]["unclassified_changes_present"])

    def test_codeview_identity_and_path_are_known_metadata(self) -> None:
        a = make_pe(guid_byte=1, pdb_path=r"C:\\one\\Astral.pdb")
        b = make_pe(guid_byte=2, pdb_path=r"C:\\two\\Astral.pdb")
        report = diag.compare_bytes(a, b)
        self.assertEqual(report["comparison"]["classification"], "recognized_pe_metadata_only")
        self.assertIn("codeview_data", report["comparison"]["changing_field_classes"])
        self.assertNotEqual(report["image_a"]["debug_entries"][0]["guid"], report["image_b"]["debug_entries"][0]["guid"])

    def test_payload_change_is_not_laundered_as_metadata(self) -> None:
        report = diag.compare_bytes(make_pe(payload_byte=0x41), make_pe(payload_byte=0x42))
        self.assertEqual(report["comparison"]["classification"], "payload_or_unclassified_bytes_changed")
        self.assertTrue(report["comparison"]["unclassified_changes_present"])
        self.assertFalse(report["comparison"]["deterministic_linker_contract_established"])

    def test_checksum_is_known_metadata(self) -> None:
        report = diag.compare_bytes(make_pe(checksum=1), make_pe(checksum=2))
        self.assertEqual(report["comparison"]["classification"], "recognized_pe_metadata_only")
        self.assertIn("optional_header_checksum", report["comparison"]["changing_field_classes"])

    def test_malformed_rejected(self) -> None:
        with self.assertRaises(diag.PEFormatError):
            diag.compare_bytes(b"not pe", b"not pe")

    def test_cli_json_and_claim_boundaries(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            a = root / "a.exe"
            b = root / "b.exe"
            out = root / "report.json"
            a.write_bytes(make_pe(timestamp=1))
            b.write_bytes(make_pe(timestamp=2))
            proc = subprocess.run(
                [sys.executable, str(Path(diag.__file__).resolve()), str(a), str(b), "--json", str(out)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=False,
            )
            self.assertEqual(proc.returncode, 0, proc.stderr)
            payload = json.loads(out.read_text(encoding="utf-8"))
            self.assertEqual(payload["comparison"]["classification"], "recognized_pe_metadata_only")
            self.assertFalse(payload["comparison"]["cross_machine_reproducibility_verified"])
            self.assertFalse(payload["comparison"]["independent_acceptance"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
