import hashlib,json,subprocess,sys,tempfile,unittest,zlib,struct
from pathlib import Path
from generate_color_value_calibration import build
from verify_color_value_calibration import verify

REPO=Path(__file__).parent.parent
SOURCE=REPO/"Content/Calibration/ColorValue/color-roles.json"
GENERATED=REPO/"Content/Calibration/ColorValue/Generated"
EXPECTED=REPO/"Content/Calibration/ColorValue/expected-manifest.json"

def png_chunk(kind,data):
    return struct.pack(">I",len(data))+kind+data+struct.pack(">I",zlib.crc32(kind+data)&0xffffffff)

def mutate_rgb_pixel(data,x,y,new_rgb):
    assert data[:8]==b"\x89PNG\r\n\x1a\n"
    off=8; ihdr=None; idat=b""
    while off<len(data):
        n=struct.unpack(">I",data[off:off+4])[0]; kind=data[off+4:off+8]; body=data[off+8:off+8+n]
        if kind==b"IHDR": ihdr=body
        elif kind==b"IDAT": idat+=body
        off+=12+n
    w,h,depth,color,comp,filt,inter=struct.unpack(">IIBBBBB",ihdr)
    raw=bytearray(zlib.decompress(idat)); stride=1+3*w; pos=y*stride+1+3*x; raw[pos:pos+3]=bytes(new_rgb)
    return b"\x89PNG\r\n\x1a\n"+png_chunk(b"IHDR",ihdr)+png_chunk(b"IDAT",zlib.compress(bytes(raw),9))+png_chunk(b"IEND",b"")

def replace_ihdr_dims(data,w,h):
    assert data[12:16]==b"IHDR"; old=13
    body=struct.pack(">IIBBBBB",w,h,8,2,0,0,0); rest=data[8+12+old:]
    return data[:8]+png_chunk(b"IHDR",body)+rest

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
    def test_no_stale_derived_pack_is_tracked(self):
        self.assertFalse(GENERATED.exists(), "derived PNG pack must be generated outside the source tree")
    def test_fresh_pack_matches_pinned_manifest(self):
        self.assertEqual(self.manifest,EXPECTED.read_bytes())
        self.assertEqual(verify(self.root,SOURCE,EXPECTED)["png_files"],3)
    def test_valid(self): self.assertEqual(self.ok()["png_files"],3)
    def test_palette_unsampled_swatch_corruption_even_rehashed(self):
        p=self.root/"palette_card.png"; p.write_bytes(mutate_rgb_pixel(p.read_bytes(),17,45,(1,2,3))); self.rehash("palette_card.png")
        with self.assertRaisesRegex(ValueError,"palette semantic"): self.ok()
    def test_palette_preview_corruption_even_rehashed(self):
        p=self.root/"palette_card.png"; p.write_bytes(mutate_rgb_pixel(p.read_bytes(),9,190,(4,5,6))); self.rehash("palette_card.png")
        with self.assertRaisesRegex(ValueError,"palette semantic"): self.ok()
    def test_ramp_unsampled_corruption_even_rehashed(self):
        p=self.root/"value_ramp_16.png"; p.write_bytes(mutate_rgb_pixel(p.read_bytes(),7,63,(9,9,9))); self.rehash("value_ramp_16.png")
        with self.assertRaisesRegex(ValueError,"value ramp semantic"): self.ok()
    def test_lut_semantic_corruption(self):
        (self.root/"neutral_lut_16.png").write_bytes((self.root/"palette_card.png").read_bytes()); self.rehash("neutral_lut_16.png")
        with self.assertRaises(ValueError): self.ok()
    def test_huge_png_dimensions_rejected_before_inflate(self):
        p=self.root/"value_ramp_16.png"; p.write_bytes(replace_ihdr_dims(p.read_bytes(),4096,4096)); self.rehash("value_ramp_16.png")
        with self.assertRaisesRegex(ValueError,"png dimensions|png inflate limit"): self.ok()
    def test_stale_pin(self):
        m=json.loads((self.root/"manifest.json").read_text()); m["status"]="changed"; (self.root/"manifest.json").write_text(json.dumps(m,indent=2,sort_keys=True)+"\n")
        with self.assertRaisesRegex(ValueError,"expected manifest"): self.ok()
    def test_ui_screening_manifest_corruption_even_repinned(self):
        m=json.loads((self.root/"manifest.json").read_text()); m["ui_screening"]=[{"foreground":"fake","background":"graphite","ratio":99.0,"minimum_ratio":1.0}]
        b=(json.dumps(m,indent=2,sort_keys=True)+"\n").encode(); (self.root/"manifest.json").write_bytes(b); self.pin.write_bytes(b)
        with self.assertRaisesRegex(ValueError,"ui screening manifest"): self.ok()
    def test_bad_source_contrast(self):
        bad=Path(self.t.name)/"bad.json"; s=json.loads(SOURCE.read_text()); s["roles"][0]["hex"]="#20242A"; bad.write_text(json.dumps(s)); files,man=build(bad)
        out=Path(self.t.name)/"badout"; out.mkdir(); [ (out/p).write_bytes(d) for p,d in files.items() ]; (out/"manifest.json").write_bytes(man)
        with self.assertRaisesRegex(ValueError,"contrast"): verify(out,bad,None)
    def test_linear_luminance_preview(self):
        files,_=build(SOURCE); data=files["palette_card.png"]; off=8; idat=b""; w=h=None
        while off<len(data):
            n=struct.unpack(">I",data[off:off+4])[0]; k=data[off+4:off+8]; body=data[off+8:off+8+n]
            if k==b"IHDR": w,h,*_=struct.unpack(">IIBBBBB",body)
            elif k==b"IDAT": idat+=body
            off+=12+n
        raw=zlib.decompress(idat); stride=1+3*w; pos=180*stride+1+3*(6*32+8)
        self.assertEqual(tuple(raw[pos:pos+3]),(141,141,141))
    def test_generator_refuses_overwrite(self):
        script=Path(__file__).with_name("generate_color_value_calibration.py")
        r=subprocess.run([sys.executable,str(script),"--source",str(SOURCE),"--output",str(self.root)],capture_output=True)
        self.assertNotEqual(r.returncode,0)
    def test_exact_check(self):
        script=Path(__file__).with_name("generate_color_value_calibration.py")
        subprocess.run([sys.executable,str(script),"--source",str(SOURCE),"--output",str(self.root),"--check"],check=True,capture_output=True)
if __name__=="__main__": unittest.main()
