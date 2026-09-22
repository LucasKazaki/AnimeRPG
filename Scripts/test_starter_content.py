"""Offline regression tests; no renderer, model service, or Windows claims."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from generate_starter_content import build_pack
from verify_starter_content import verify


class StarterTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name)/"pack"
        self.files=build_pack()
        for p,data in self.files.items():
            dest=self.root/p;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data)

    def refresh_hash(self,path):
        manifest=json.loads((self.root/"manifest.json").read_text())
        data=(self.root/path).read_bytes()
        for r in manifest["files"]:
            if r["path"]==path:r.update(bytes=len(data),sha256=hashlib.sha256(data).hexdigest())
        (self.root/"manifest.json").write_text(json.dumps(manifest))

    def test_valid(self):
        self.assertEqual(verify(self.root)["meshes"],32)

    def test_deterministic(self):
        self.assertEqual(self.files,build_pack())

    def test_hash_corruption(self):
        (self.root/"Meshes/cube.mesh").write_text("corrupt")
        with self.assertRaises(ValueError):verify(self.root)

    def test_edge_index_even_after_rehash(self):
        p="Meshes/cube.mesh";f=self.root/p
        f.write_text(f.read_text().replace("edge 0 1","edge 0 999"));self.refresh_hash(p)
        with self.assertRaises(ValueError):verify(self.root)

    def test_nan_even_after_rehash(self):
        p="Meshes/cube.mesh";f=self.root/p
        f.write_text(f.read_text().replace("-0.500000","nan",1));self.refresh_hash(p)
        with self.assertRaises(ValueError):verify(self.root)

    def test_png_crc_even_after_rehash(self):
        p="Textures/flat_normal.png";f=self.root/p;data=bytearray(f.read_bytes());data[-1]^=1
        f.write_bytes(data);self.refresh_hash(p)
        with self.assertRaises(ValueError):verify(self.root)

    def test_unknown_file(self):
        (self.root/"surprise.txt").write_text("x")
        with self.assertRaises(ValueError):verify(self.root)

    def test_path_traversal(self):
        p=self.root/"manifest.json";m=json.loads(p.read_text());m["files"][0]["path"]="../outside"
        p.write_text(json.dumps(m))
        with self.assertRaises(ValueError):verify(self.root)

    def test_material_reference(self):
        p="materials.json";f=self.root/p;m=json.loads(f.read_text());m[0]["normal"]="missing.png"
        f.write_text(json.dumps(m));self.refresh_hash(p)
        with self.assertRaises(ValueError):verify(self.root)

    def test_symlink(self):
        try:(self.root/"link").symlink_to(self.root/"materials.json")
        except OSError:self.skipTest("Symlink creation unavailable")
        with self.assertRaises(ValueError):verify(self.root)

    def test_cli_no_overwrite_and_check(self):
        script=Path(__file__).with_name("generate_starter_content.py")
        before={p:p.read_bytes() for p in self.root.rglob("*") if p.is_file()}
        result=subprocess.run([sys.executable,str(script),"--output",str(self.root)],capture_output=True,timeout=15)
        self.assertNotEqual(result.returncode,0)
        self.assertEqual(before,{p:p.read_bytes() for p in self.root.rglob("*") if p.is_file()})
        subprocess.run([sys.executable,str(script),"--output",str(self.root),"--check"],check=True,capture_output=True,timeout=15)


if __name__=="__main__":unittest.main()
