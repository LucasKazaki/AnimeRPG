import json,subprocess,sys,tempfile,unittest
from pathlib import Path
from generate_asymmetric_calibration_gltf import build
from verify_asymmetric_calibration_gltf import verify

class T(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory(); self.addCleanup(self.tmp.cleanup); self.r=Path(self.tmp.name)
        a,m=build(); (self.r/"asymmetric_surface.gltf").write_bytes(a); (self.r/"manifest.json").write_bytes(m)
    def ok(self): return verify(self.r/"asymmetric_surface.gltf",self.r/"manifest.json")
    def mutate(self,fn):
        p=self.r/"asymmetric_surface.gltf"; g=json.loads(p.read_text()); fn(g); p.write_text(json.dumps(g))
    def test_valid(self): self.assertEqual(self.ok()["block_vertices"],24)
    def test_bad_contract(self):
        self.mutate(lambda g:g["extras"]["astral_contract"].update({"up":"+Z"}))
        with self.assertRaises(ValueError): self.ok()
    def test_bad_material(self):
        self.mutate(lambda g:g["meshes"][0]["primitives"][0].update({"material":1}))
        with self.assertRaises(ValueError): self.ok()
    def test_bad_sampler(self):
        self.mutate(lambda g:g["samplers"][0].update({"wrapS":10497}))
        with self.assertRaises(ValueError): self.ok()
    def test_manifest_detects_reformat(self):
        p=self.r/"asymmetric_surface.gltf"; g=json.loads(p.read_text()); p.write_text(json.dumps(g))
        with self.assertRaises(ValueError): self.ok()
    def test_generator_refuses_overwrite(self):
        script=Path(__file__).with_name("generate_asymmetric_calibration_gltf.py")
        q=subprocess.run([sys.executable,str(script),"--output",str(self.r)],capture_output=True)
        self.assertNotEqual(q.returncode,0)
if __name__=="__main__": unittest.main()
