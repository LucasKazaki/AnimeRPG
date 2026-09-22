#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import json
import os
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).with_name("inspect_pe_dependencies.py")
spec = importlib.util.spec_from_file_location("inspect_pe_dependencies", SCRIPT)
module = importlib.util.module_from_spec(spec)
assert spec.loader is not None
sys.modules[spec.name] = module
spec.loader.exec_module(module)


def make_pe64(imports: list[str], delay_imports: list[str] | None = None) -> bytes:
    delay_imports = delay_imports or []
    file_alignment = 0x200
    section_rva = 0x1000
    section_raw = 0x200
    data = bytearray(0x1000)
    data[0:2] = b"MZ"
    pe = 0x80
    struct.pack_into("<I", data, 0x3C, pe)
    data[pe:pe+4] = b"PE\0\0"
    coff = pe + 4
    struct.pack_into("<HHIIIHH", data, coff, 0x8664, 1, 0, 0, 0, 0xF0, 0x22)
    optional = coff + 20
    struct.pack_into("<H", data, optional, 0x20B)
    struct.pack_into("<Q", data, optional + 24, 0x140000000)
    struct.pack_into("<I", data, optional + 32, 0x1000)  # section alignment
    struct.pack_into("<I", data, optional + 36, file_alignment)
    struct.pack_into("<I", data, optional + 56, 0x2000)  # size of image
    struct.pack_into("<I", data, optional + 60, 0x200)   # headers
    struct.pack_into("<I", data, optional + 108, 16)    # number of directories
    section_table = optional + 0xF0
    name = b".rdata\0\0"
    data[section_table:section_table+8] = name
    struct.pack_into("<IIII", data, section_table + 8, 0x800, section_rva, 0x800, section_raw)

    cursor = 0
    import_desc_rva = 0
    import_desc_size = 0
    if imports:
        import_desc_rva = section_rva + cursor
        import_desc_size = (len(imports) + 1) * 20
        names_cursor = cursor + import_desc_size
        for i, dll in enumerate(imports):
            encoded = dll.encode("ascii") + b"\0"
            name_rva = section_rva + names_cursor
            struct.pack_into("<IIIII", data, section_raw + cursor + i*20, 0, 0, 0, name_rva, 1)
            data[section_raw + names_cursor:section_raw + names_cursor + len(encoded)] = encoded
            names_cursor += len(encoded)
        cursor = (names_cursor + 15) & ~15

    delay_desc_rva = 0
    delay_desc_size = 0
    if delay_imports:
        delay_desc_rva = section_rva + cursor
        delay_desc_size = (len(delay_imports) + 1) * 32
        names_cursor = cursor + delay_desc_size
        for i, dll in enumerate(delay_imports):
            encoded = dll.encode("ascii") + b"\0"
            name_rva = section_rva + names_cursor
            struct.pack_into("<IIIIIIII", data, section_raw + cursor + i*32, 1, name_rva, 0, 0, 0, 0, 0, 0)
            data[section_raw + names_cursor:section_raw + names_cursor + len(encoded)] = encoded
            names_cursor += len(encoded)
        cursor = (names_cursor + 15) & ~15

    struct.pack_into("<II", data, optional + 112 + 8, import_desc_rva, import_desc_size)
    struct.pack_into("<II", data, optional + 112 + 13*8, delay_desc_rva, delay_desc_size)
    return bytes(data[:section_raw + 0x800])


class PETests(unittest.TestCase):
    def test_normal_and_delay_imports(self):
        image = make_pe64(["KERNEL32.dll", "VCRUNTIME140.dll"], ["USER32.dll"])
        parsed = module.parse_pe_imports(image)
        self.assertEqual(parsed.machine, "AMD64")
        self.assertEqual(parsed.format, "PE32+")
        self.assertEqual(set(parsed.imports), {"KERNEL32.dll", "VCRUNTIME140.dll"})
        self.assertEqual(set(parsed.delay_imports), {"USER32.dll"})

    def test_debug_runtime_detection_and_cli_exit(self):
        with tempfile.TemporaryDirectory() as temp:
            image = Path(temp) / "debug.exe"
            image.write_bytes(make_pe64(["ucrtbased.dll", "KERNEL32.dll"]))
            result = subprocess.run(
                [sys.executable, str(SCRIPT), str(image), "--fail-on-debug-runtime"],
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=False,
            )
            self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
            report = json.loads(result.stdout)
            self.assertEqual(report["analysis"]["debug_crt_imports"], ["ucrtbased.dll"])
            self.assertFalse(report["analysis"]["clean_machine_compatibility_verified"])

    def test_release_runtime_is_inventory_not_clean_machine_proof(self):
        with tempfile.TemporaryDirectory() as temp:
            image = Path(temp) / "release.exe"
            report_path = Path(temp) / "report.json"
            image.write_bytes(make_pe64(["MSVCP140.dll", "VCRUNTIME140_1.dll", "KERNEL32.dll"]))
            result = subprocess.run(
                [sys.executable, str(SCRIPT), str(image), "--json", str(report_path), "--fail-on-debug-runtime"],
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=False,
            )
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            report = json.loads(report_path.read_text())
            self.assertTrue(report["analysis"]["requires_vc_runtime_deployment_review"])
            self.assertFalse(report["analysis"]["clean_machine_compatibility_verified"])
            self.assertEqual(
                set(report["analysis"]["vc_runtime_imports"]),
                {"MSVCP140.dll", "VCRUNTIME140_1.dll"},
            )

    def test_rejects_malformed_inputs(self):
        for data in (b"", b"MZ" + b"\0" * 10, b"not a pe file" * 10):
            with self.assertRaises(module.PEFormatError):
                module.parse_pe_imports(data)

    @unittest.skipUnless(os.name == "nt", "real PE sanity check requires Windows")
    def test_real_python_executable_on_windows(self):
        report = module.inspect_file(Path(sys.executable))
        self.assertIn(report["machine"], {"AMD64", "ARM64", "I386"})
        self.assertGreater(len(report["all_imports"]), 0)
        self.assertEqual(report["analysis"]["debug_crt_imports"], [])


if __name__ == "__main__":
    unittest.main(verbosity=2)
