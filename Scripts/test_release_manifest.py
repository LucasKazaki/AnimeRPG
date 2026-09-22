#!/usr/bin/env python3
from __future__ import annotations
import hashlib, json, subprocess, sys, tempfile, unittest
from pathlib import Path
import release_manifest as target
COMMIT="1"*40

def digest(p): return hashlib.sha256(p.read_bytes()).hexdigest()
class Tests(unittest.TestCase):
    def pkg(self, root):
        p=root/"package"; p.mkdir(); (p/"AstralGame.exe").write_bytes(b"MZ-test"); (p/"README.txt").write_text("Astral\n"); (p/"evidence").mkdir(); (p/"evidence"/"build.log").write_text("pass\n"); return p
    def test_roundtrip_and_claim_boundaries(self):
        with tempfile.TemporaryDirectory() as t:
            p=self.pkg(Path(t)); m=p/"MANIFEST.json"; data=target.write_release_manifest(p,m,COMMIT)
            r=target.verify_manifest(data,p,manifest_path=m,expected_commit=COMMIT,expected_executable_sha256=digest(p/"AstralGame.exe"))
            self.assertEqual(r["files_verified"],3); self.assertTrue(r["acceptance"]["package_manifest_verified"]); self.assertFalse(r["acceptance"]["package_launch_verified"])
    def test_tamper_missing_extra(self):
        with tempfile.TemporaryDirectory() as t:
            p=self.pkg(Path(t)); m=p/"MANIFEST.json"; data=target.write_release_manifest(p,m,COMMIT)
            (p/"README.txt").write_text("changed")
            with self.assertRaises(target.ManifestError): target.verify_manifest(data,p,manifest_path=m)
            (p/"README.txt").write_text("Astral\n"); (p/"evidence"/"build.log").unlink()
            with self.assertRaises(target.ManifestError): target.verify_manifest(data,p,manifest_path=m)
            (p/"evidence"/"build.log").write_text("pass\n"); (p/"extra").write_text("x")
            with self.assertRaises(target.ManifestError): target.verify_manifest(data,p,manifest_path=m)
    def test_unsafe_and_case_collision_entries(self):
        with tempfile.TemporaryDirectory() as t:
            p=self.pkg(Path(t))
            for bad in ("../x","/x","C:/x",r"bad\x"):
                d={"commit":COMMIT,"files":[{"path":bad,"bytes":0,"sha256":"0"*64}]}
                with self.assertRaises(target.ManifestError): target.verify_manifest(d,p)
            d={"commit":COMMIT,"files":[{"path":"README.txt","bytes":0,"sha256":"0"*64},{"path":"readme.TXT","bytes":0,"sha256":"0"*64}]}
            with self.assertRaises(target.ManifestError): target.verify_manifest(d,p)
    def test_v2_aggregate_and_commit_binding(self):
        with tempfile.TemporaryDirectory() as t:
            p=self.pkg(Path(t)); m=p/"MANIFEST.json"; data=target.write_release_manifest(p,m,COMMIT)
            bad=json.loads(json.dumps(data)); bad["entries_sha256"]="0"*64
            with self.assertRaises(target.ManifestError): target.verify_manifest(bad,p,manifest_path=m)
            with self.assertRaises(target.ManifestError): target.verify_manifest(data,p,manifest_path=m,expected_commit="2"*40)
            with self.assertRaises(target.ManifestError): target.verify_manifest(data,p,manifest_path=m,expected_executable_sha256="0"*64)
    def test_historical_v1_and_recorded_root_not_trusted(self):
        with tempfile.TemporaryDirectory() as t:
            p=self.pkg(Path(t)); files=[]
            for f in sorted(x for x in p.rglob("*") if x.is_file()): files.append({"path":f.relative_to(p).as_posix(),"bytes":f.stat().st_size,"sha256":digest(f)})
            d={"commit":COMMIT,"package_root":r"C:\historical","files":files}; r=target.verify_manifest(d,p)
            self.assertEqual(r["manifest_schema_version"],1); self.assertEqual(r["package_root_verified"],str(p.resolve()))
    def test_symlink_rejected_if_available(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t); p=self.pkg(root); outside=root/"outside"; outside.write_text("x"); link=p/"link"
            try: link.symlink_to(outside)
            except (OSError,NotImplementedError): self.skipTest("symlinks unavailable")
            with self.assertRaises(target.ManifestError): target.build_manifest(p,COMMIT)
    def test_cli_create_verify_and_failure(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t); p=self.pkg(root); m=p/"MANIFEST.json"; report=root/"report.json"; s=Path(__file__).with_name("release_manifest.py")
            a=subprocess.run([sys.executable,str(s),"create",str(p),str(m),"--commit",COMMIT],capture_output=True,text=True); self.assertEqual(a.returncode,0,a.stderr)
            b=subprocess.run([sys.executable,str(s),"verify",str(m),str(p),"--expected-commit",COMMIT,"--expected-executable-sha256",digest(p/"AstralGame.exe"),"--json",str(report)],capture_output=True,text=True); self.assertEqual(b.returncode,0,b.stderr)
            self.assertTrue(json.loads(report.read_text())["manifest_integrity_verified"]); (p/"AstralGame.exe").write_bytes(b"bad")
            c=subprocess.run([sys.executable,str(s),"verify",str(m),str(p)],capture_output=True,text=True); self.assertNotEqual(c.returncode,0)
if __name__=="__main__": unittest.main(verbosity=2)
