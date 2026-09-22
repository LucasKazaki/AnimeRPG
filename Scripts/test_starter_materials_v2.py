"""Bounded regression tests for Astral material source pack v2."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys
import tempfile
import unittest

from generate_starter_materials_v2 import build_pack
from verify_starter_materials_v2 import verify

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

    def test_semantic_metal_error_is_rejected_even_with_fresh_hash(self):
        rel = "asphalt/asphalt_orm.png"
        src = self.root/"brushed_steel/brushed_steel_orm.png"
        (self.root/rel).write_bytes(src.read_bytes()); self.rehash(rel)
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
