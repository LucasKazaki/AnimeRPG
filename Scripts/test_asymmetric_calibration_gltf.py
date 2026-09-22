import base64,hashlib,json,subprocess,sys,tempfile,unittest
from pathlib import Path
from generate_asymmetric_calibration_gltf import build
from verify_asymmetric_calibration_gltf import verify

class T(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory(); self.addCleanup(self.tmp.cleanup); self.r=Path(self.tmp.name)
        a,m=build(); (self.r/"asymmetric_surface.gltf").write_bytes(a); (self.r/"manifest.json").write_bytes(m); (self.r/"expected-manifest.json").write_bytes(m)
    def ok(self,pin=True): return verify(self.r/"asymmetric_surface.gltf",self.r/"manifest.json",self.r/"expected-manifest.json" if pin else None)
    def rehash(self):
        p=self.r/"asymmetric_surface.gltf"; raw=p.read_bytes(); mp=self.r/"manifest.json"; m=json.loads(mp.read_text()); m["files"][0].update(bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest()); mp.write_text(json.dumps(m,indent=2,sort_keys=True)+"\n")
    def mutate(self,fn,rehash=True):
        p=self.r/"asymmetric_surface.gltf"; g=json.loads(p.read_text()); fn(g); p.write_text(json.dumps(g,indent=2,sort_keys=True)+"\n")
        if rehash: self.rehash()
    def test_valid_and_expected_manifest(self): self.assertEqual(self.ok()["block_vertices"],24)
    def test_bad_contract_after_rehash(self):
        self.mutate(lambda g:g["extras"]["astral_contract"].update({"up":"+Z"}))
        with self.assertRaisesRegex(ValueError,"contract"): self.ok(False)
    def test_bad_material_after_rehash(self):
        self.mutate(lambda g:g["meshes"][0]["primitives"][0].update({"material":1}))
        with self.assertRaisesRegex(ValueError,"block primitive"): self.ok(False)
    def test_bad_sampler_after_rehash(self):
        self.mutate(lambda g:g["samplers"][0].update({"wrapS":10497}))
        with self.assertRaisesRegex(ValueError,"sampler"): self.ok(False)
    def test_marker_winding_and_tangent_contract(self):
        self.mutate(lambda g:g["accessors"][g["meshes"][1]["primitives"][0]["indices"]].update({"count":2}))
        with self.assertRaisesRegex(ValueError,"marker counts"): self.ok(False)
    def test_accessor_cannot_escape_buffer_view(self):
        self.mutate(lambda g:g["bufferViews"][g["accessors"][g["meshes"][1]["primitives"][0]["attributes"]["POSITION"]]["bufferView"]].update({"byteLength":1}))
        with self.assertRaisesRegex(ValueError,"accessor within bufferView"): self.ok(False)
    def test_scene_node_mesh_ownership(self):
        self.mutate(lambda g:(g["nodes"][0].update({"mesh":1}),g["nodes"][1].update({"mesh":0})))
        with self.assertRaisesRegex(ValueError,"node mesh ownership"): self.ok(False)
    def test_scene_membership(self):
        self.mutate(lambda g:g["scenes"][0].update({"nodes":[1]}))
        with self.assertRaisesRegex(ValueError,"scene nodes"): self.ok(False)
    def test_corrupt_png_crc_after_rehash(self):
        def corrupt(g):
            prefix="data:image/png;base64,"; data=bytearray(base64.b64decode(g["images"][0]["uri"][len(prefix):])); data[29]^=1; g["images"][0]["uri"]=prefix+base64.b64encode(data).decode()
        self.mutate(corrupt)
        with self.assertRaisesRegex(ValueError,"png crc"): self.ok(False)
    def test_truncated_png_after_rehash(self):
        def truncate(g):
            prefix="data:image/png;base64,"; data=base64.b64decode(g["images"][0]["uri"][len(prefix):]); g["images"][0]["uri"]=prefix+base64.b64encode(data[:33]).decode()
        self.mutate(truncate)
        with self.assertRaises(ValueError): self.ok(False)
    def test_expected_manifest_detects_stale_pin(self):
        self.mutate(lambda g:g["asset"].update({"generator":"changed-generator"}))
        # Semantically still valid enough until the final pin check; generated manifest is rehashed.
        with self.assertRaisesRegex(ValueError,"expected manifest pin"): self.ok(True)
    def test_manifest_detects_unrehased_reformat(self):
        p=self.r/"asymmetric_surface.gltf"; g=json.loads(p.read_text()); p.write_text(json.dumps(g))
        with self.assertRaisesRegex(ValueError,"manifest"): self.ok(False)
    def test_generator_refuses_overwrite(self):
        script=Path(__file__).with_name("generate_asymmetric_calibration_gltf.py")
        q=subprocess.run([sys.executable,str(script),"--output",str(self.r)],capture_output=True)
        self.assertNotEqual(q.returncode,0)
if __name__=="__main__": unittest.main()
