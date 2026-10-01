"""Bounded regression tests for Astral material source pack v2."""
from pathlib import Path
import hashlib
import json
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib

from generate_starter_materials_v2 import build_pack, png_rgb
from verify_starter_materials_v2 import decode_png, verify


class MaterialPackTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.template_files = build_pack()

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name) / "pack"
        for rel, data in self.template_files.items():
            f = self.root / rel
            f.parent.mkdir(parents=True, exist_ok=True)
            f.write_bytes(data)

    def rehash(self, rel):
        mpath = self.root / "manifest.json"
        m = json.loads(mpath.read_text())
        data = (self.root / rel).read_bytes()
        for r in m["files"]:
            if r["path"] == rel:
                r["bytes"] = len(data)
                r["sha256"] = hashlib.sha256(data).hexdigest()
        mpath.write_text(json.dumps(m))

    def test_valid_pack(self):
        self.assertEqual(verify(self.root), {"materials": 4, "png_files": 16})

    def test_payload_corruption_is_rejected(self):
        f = self.root / "limestone/limestone_basecolor.png"
        b = bytearray(f.read_bytes()); b[-1] ^= 1; f.write_bytes(b)
        with self.assertRaises(ValueError): verify(self.root)

    def test_single_unsampled_metal_error_is_rejected_even_with_fresh_hash(self):
        rel = "asphalt/asphalt_orm.png"
        f = self.root / rel
        w, h, rows = decode_png(f.read_bytes())
        mutable = [bytearray(row) for row in rows]
        mutable[1][1*3 + 2] = 127
        f.write_bytes(png_rgb(w, h, lambda x, y: tuple(mutable[y][x*3:x*3+3])))
        self.rehash(rel)
        with self.assertRaises(ValueError): verify(self.root)

    def test_periodic_sampling_uses_unique_final_texel(self):
        _, _, rows = decode_png(self.template_files["limestone/limestone_height.png"])
        self.assertNotEqual(rows[0], rows[-1])
        self.assertNotEqual(b"".join(row[:3] for row in rows), b"".join(row[-3:] for row in rows))

    def test_png_inflate_bomb_is_bounded_and_rejected(self):
        rel = "limestone/limestone_basecolor.png"
        f = self.root / rel
        def chunk(kind, body):
            return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind+body) & 0xffffffff)
        ihdr = struct.pack(">IIBBBBB", 1, 1, 8, 2, 0, 0, 0)
        bomb = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(b"\0" * 2_000_000, 9)) + chunk(b"IEND", b"")
        f.write_bytes(bomb)
        self.rehash(rel)
        with self.assertRaises(ValueError): verify(self.root)

    def test_manifest_traversal_is_rejected(self):
        p = self.root/"manifest.json"
        m = json.loads(p.read_text())
        m["files"][0]["path"] = "../outside.png"
        p.write_text(json.dumps(m))
        with self.assertRaises(ValueError): verify(self.root)

    def test_cli_refuses_overwrite_and_exact_check_passes(self):
        script = Path(__file__).with_name("generate_starter_materials_v2.py")
        r = subprocess.run([sys.executable, str(script), "--output", str(self.root)], capture_output=True, timeout=15)
        self.assertNotEqual(r.returncode, 0)
        subprocess.run([sys.executable, str(script), "--output", str(self.root), "--check"], check=True, capture_output=True, timeout=20)

if __name__ == "__main__": unittest.main()
