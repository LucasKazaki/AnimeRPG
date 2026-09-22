import hashlib,json,subprocess,sys,tempfile,unittest,zlib,struct
from pathlib import Path
from generate_color_value_calibration import build
from verify_color_value_calibration import verify

SOURCE=Path(__file__).parent.parent/"Content/Calibration/ColorValue/color-roles.json"
class Tests(unittest.TestCase):
    def setUp(self):
        self.t=tempfile.TemporaryDirectory(); self.addCleanup(self.t.cleanup); self.root=Path(self.t.name)/"out"; self.root.mkdir(); self.files,self.manifest=build(SOURCE)
        for p,d in self.files.items(): (self.root/p).write_bytes(d)
        (self.root/"manifest.json").write_bytes(self.manifest); self.pin=Path(self.t.name)/"pin.json"; self.pin.write_bytes(self.manifest)
    def ok(self): return verify(self.root,SOURCE,self.pin)
    def rehash(self,path):
        m=json.loads((self.root/"manifest.json").read_text()); d=(self.root/path).read_bytes()
        for r in m["files"]:
            if r["path"]==path:r.update(bytes=len(d),sha256=hashlib.sha256(d).hexdigest())
        b=(json.dumps(m,indent=2,sort_keys=True)+"\n").encode(); (self.root/"manifest.json").write_bytes(b); self.pin.write_bytes(b)
    def test_valid(self): self.assertEqual(self.ok()["png_files"],3)
    def test_palette_corruption_even_rehashed(self):
        p=self.root/"palette_card.png"; d=bytearray(p.read_bytes()); d[-5]^=1; p.write_bytes(d); self.rehash("palette_card.png")
        with self.assertRaises(ValueError): self.ok()
    def test_lut_semantic_corruption(self):
        # Replace generated LUT with palette PNG, then rehash. Dimensions/semantics still reject.
        (self.root/"neutral_lut_16.png").write_bytes((self.root/"palette_card.png").read_bytes()); self.rehash("neutral_lut_16.png")
        with self.assertRaises(ValueError): self.ok()
    def test_stale_pin(self):
        m=json.loads((self.root/"manifest.json").read_text()); m["status"]="changed"; (self.root/"manifest.json").write_text(json.dumps(m,indent=2,sort_keys=True)+"\n")
        with self.assertRaisesRegex(ValueError,"expected manifest"): self.ok()
    def test_bad_source_contrast(self):
        bad=Path(self.t.name)/"bad.json"; s=json.loads(SOURCE.read_text()); s["roles"][0]["hex"]="#20242A"; bad.write_text(json.dumps(s)); files,man=build(bad)
        out=Path(self.t.name)/"badout"; out.mkdir(); [ (out/p).write_bytes(d) for p,d in files.items() ]; (out/"manifest.json").write_bytes(man)
        with self.assertRaisesRegex(ValueError,"contrast"): verify(out,bad,None)
    def test_generator_refuses_overwrite(self):
        script=Path(__file__).with_name("generate_color_value_calibration.py")
        r=subprocess.run([sys.executable,str(script),"--source",str(SOURCE),"--output",str(self.root)],capture_output=True)
        self.assertNotEqual(r.returncode,0)
    def test_exact_check(self):
        script=Path(__file__).with_name("generate_color_value_calibration.py")
        subprocess.run([sys.executable,str(script),"--source",str(SOURCE),"--output",str(self.root),"--check"],check=True,capture_output=True)
if __name__=="__main__": unittest.main()
