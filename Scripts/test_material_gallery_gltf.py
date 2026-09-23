from __future__ import annotations
import base64, hashlib, json, struct, subprocess, sys, tempfile
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
GEN=ROOT/"Scripts/generate_material_gallery_gltf.py"
VER=ROOT/"Scripts/verify_material_gallery_gltf.py"
SOURCE=ROOT/"Content/Calibration/MaterialGallery/gallery-spec.json"
PIN=ROOT/"Content/Calibration/MaterialGallery/expected-manifest.json"

def run(args,ok=True):
    p=subprocess.run([sys.executable,*map(str,args)],capture_output=True,text=True)
    if ok and p.returncode!=0: raise AssertionError(p.stderr or p.stdout)
    if not ok and p.returncode==0: raise AssertionError("expected failure")
    return p

def generate(tmp,source=SOURCE):
    out=tmp/"out"; run([GEN,"--source",source,"--output",out]); return out

def repin(out):
    gltf=out/"material_gallery.gltf"; raw=gltf.read_bytes(); manifest=json.loads((out/"manifest.json").read_text())
    manifest["files"][0]={"path":"material_gallery.gltf","bytes":len(raw),"sha256":hashlib.sha256(raw).hexdigest()}
    (out/"manifest.json").write_text(json.dumps(manifest,indent=2,sort_keys=True)+"\n"); return out/"manifest.json"

def mutate_gltf(out,fn):
    path=out/"material_gallery.gltf"; g=json.loads(path.read_text()); fn(g); path.write_text(json.dumps(g,indent=2,sort_keys=True)+"\n"); repin(out)

def set_accessor_float(g, accessor_index, vertex_index, component_index, value):
    accessor=g["accessors"][accessor_index]; assert accessor["componentType"]==5126
    component_counts={"SCALAR":1,"VEC2":2,"VEC3":3,"VEC4":4}; components=component_counts[accessor["type"]]; view=g["bufferViews"][accessor["bufferView"]]
    prefix="data:application/octet-stream;base64,"; uri=g["buffers"][0]["uri"]; assert uri.startswith(prefix)
    buf=bytearray(base64.b64decode(uri[len(prefix):])); offset=view.get("byteOffset",0)+accessor.get("byteOffset",0)+(vertex_index*components+component_index)*4
    struct.pack_into("<f",buf,offset,value); g["buffers"][0]["uri"]=prefix+base64.b64encode(buf).decode()

def test_expected_pin_matches_generator(tmp):
    out=generate(tmp); assert (out/"manifest.json").read_bytes()==PIN.read_bytes(); run([VER,out/"material_gallery.gltf","--source",SOURCE,"--manifest",out/"manifest.json","--expected-manifest",PIN])

def test_exact_check(tmp):
    out=generate(tmp); run([GEN,"--source",SOURCE,"--output",out,"--check"])

def test_light_intensity_semantics(tmp):
    out=generate(tmp); mutate_gltf(out,lambda g:g["extensions"]["KHR_lights_punctual"]["lights"][0].__setitem__("intensity",900.0)); p=run([VER,out/"material_gallery.gltf","--source",SOURCE,"--manifest",out/"manifest.json"],ok=False); assert "lights" in p.stderr

def test_material_binding_semantics(tmp):
    out=generate(tmp); mutate_gltf(out,lambda g:g["meshes"][0]["primitives"][0].__setitem__("material",3)); p=run([VER,out/"material_gallery.gltf","--source",SOURCE,"--manifest",out/"manifest.json"],ok=False); assert "material binding" in p.stderr

def test_uncontracted_material_property_semantics(tmp):
    out=generate(tmp); mutate_gltf(out,lambda g:g["materials"][0].__setitem__("emissiveFactor",[1.0,1.0,1.0])); p=run([VER,out/"material_gallery.gltf","--source",SOURCE,"--manifest",out/"manifest.json"],ok=False); assert "material properties" in p.stderr

def test_camera_fov_semantics(tmp):
    out=generate(tmp); mutate_gltf(out,lambda g:g["cameras"][0]["perspective"].__setitem__("yfov",0.6)); p=run([VER,out/"material_gallery.gltf","--source",SOURCE,"--manifest",out/"manifest.json"],ok=False); assert "camera framing" in p.stderr

def test_camera_rotation_semantics(tmp):
    out=generate(tmp); mutate_gltf(out,lambda g:g["nodes"][9].__setitem__("rotation",[0.0,0.0,0.0,1.0])); p=run([VER,out/"material_gallery.gltf","--source",SOURCE,"--manifest",out/"manifest.json"],ok=False); assert "camera rotation" in p.stderr

def test_light_rotation_semantics(tmp):
    out=generate(tmp); mutate_gltf(out,lambda g:g["nodes"][10].__setitem__("rotation",[0.0,0.0,0.0,1.0])); p=run([VER,out/"material_gallery.gltf","--source",SOURCE,"--manifest",out/"manifest.json"],ok=False); assert "light rotation" in p.stderr

def test_all_triangle_vertex_normals_semantics(tmp):
    out=generate(tmp)
    def mutate(g):
        normal_accessor=g["meshes"][4]["primitives"][0]["attributes"]["NORMAL"]
        for vertex in (1,2,3): set_accessor_float(g,normal_accessor,vertex,2,-1.0)
    mutate_gltf(out,mutate); p=run([VER,out/"material_gallery.gltf","--source",SOURCE,"--manifest",out/"manifest.json"],ok=False); assert "triangle vertex normal" in p.stderr

def test_floor_tangent_handedness(tmp):
    out=generate(tmp)
    def mutate(g):
        tangent_accessor=g["meshes"][8]["primitives"][0]["attributes"]["TANGENT"]
        for vertex in range(g["accessors"][tangent_accessor]["count"]): set_accessor_float(g,tangent_accessor,vertex,3,1.0)
    mutate_gltf(out,mutate); p=run([VER,out/"material_gallery.gltf","--source",SOURCE,"--manifest",out/"manifest.json"],ok=False); assert "floor tangent frame" in p.stderr

def test_floor_tangent_direction_semantics(tmp):
    out=generate(tmp)
    def mutate(g):
        tangent_accessor=g["meshes"][8]["primitives"][0]["attributes"]["TANGENT"]
        for vertex in range(g["accessors"][tangent_accessor]["count"]):
            set_accessor_float(g,tangent_accessor,vertex,0,-1.0); set_accessor_float(g,tangent_accessor,vertex,1,0.0); set_accessor_float(g,tangent_accessor,vertex,2,0.0); set_accessor_float(g,tangent_accessor,vertex,3,-1.0)
    mutate_gltf(out,mutate); p=run([VER,out/"material_gallery.gltf","--source",SOURCE,"--manifest",out/"manifest.json"],ok=False); assert "floor tangent frame" in p.stderr

def test_camera_transform_override_semantics(tmp):
    out=generate(tmp); mutate_gltf(out,lambda g:g["nodes"][9].__setitem__("scale",[1.0,1.0,-1.0])); p=run([VER,out/"material_gallery.gltf","--source",SOURCE,"--manifest",out/"manifest.json"],ok=False); assert "camera transform" in p.stderr

def test_light_transform_override_semantics(tmp):
    out=generate(tmp); mutate_gltf(out,lambda g:g["nodes"][10].__setitem__("scale",[1.0,1.0,-1.0])); p=run([VER,out/"material_gallery.gltf","--source",SOURCE,"--manifest",out/"manifest.json"],ok=False); assert "light transform" in p.stderr

def test_source_status_rejected_by_generator(tmp):
    source=json.loads(SOURCE.read_text()); source["status"]="runtime_verified"; bad=tmp/"bad-source.json"; bad.write_text(json.dumps(source,indent=2,sort_keys=True)+"\n"); p=run([GEN,"--source",bad,"--output",tmp/"bad-out"],ok=False); assert "source status" in p.stderr

def test_runtime_status_semantics(tmp):
    out=generate(tmp); mutate_gltf(out,lambda g:g["extras"]["astral_contract"].__setitem__("status","runtime_verified")); p=run([VER,out/"material_gallery.gltf","--source",SOURCE,"--manifest",out/"manifest.json"],ok=False); assert "runtime status" in p.stderr

def test_reject_embedded_texture_or_baked_lighting_path(tmp):
    out=generate(tmp); mutate_gltf(out,lambda g:g.__setitem__("images",[{"uri":"data:image/png;base64,"}])); p=run([VER,out/"material_gallery.gltf","--source",SOURCE,"--manifest",out/"manifest.json"],ok=False); assert "must not embed texture lighting" in p.stderr

def test_accessor_bounds(tmp):
    out=generate(tmp); mutate_gltf(out,lambda g:g["bufferViews"][0].__setitem__("byteLength",4)); p=run([VER,out/"material_gallery.gltf","--source",SOURCE,"--manifest",out/"manifest.json"],ok=False); assert "accessor within bufferView" in p.stderr

def test_expected_pin_negative(tmp):
    out=generate(tmp); m=json.loads((out/"manifest.json").read_text()); m["intent"]+=" changed"; (out/"manifest.json").write_text(json.dumps(m,indent=2,sort_keys=True)+"\n"); p=run([VER,out/"material_gallery.gltf","--source",SOURCE,"--manifest",out/"manifest.json","--expected-manifest",PIN],ok=False); assert "expected manifest pin" in p.stderr

def test_crlf_source_and_pin_portability(tmp):
    source=tmp/"source-crlf.json"; source.write_bytes(SOURCE.read_bytes().replace(b"\n",b"\r\n")); pin=tmp/"pin-crlf.json"; pin.write_bytes(PIN.read_bytes().replace(b"\n",b"\r\n")); out=generate(tmp,source); assert (out/"manifest.json").read_bytes()==PIN.read_bytes(); run([VER,out/"material_gallery.gltf","--source",source,"--manifest",out/"manifest.json","--expected-manifest",pin])

TESTS=[test_expected_pin_matches_generator,test_exact_check,test_light_intensity_semantics,test_material_binding_semantics,test_uncontracted_material_property_semantics,test_camera_fov_semantics,test_camera_rotation_semantics,test_light_rotation_semantics,test_all_triangle_vertex_normals_semantics,test_floor_tangent_handedness,test_floor_tangent_direction_semantics,test_camera_transform_override_semantics,test_light_transform_override_semantics,test_source_status_rejected_by_generator,test_runtime_status_semantics,test_reject_embedded_texture_or_baked_lighting_path,test_accessor_bounds,test_expected_pin_negative,test_crlf_source_and_pin_portability]

def main():
    passed=0
    for test in TESTS:
        with tempfile.TemporaryDirectory() as td:
            test(Path(td)); passed+=1; print("PASS",test.__name__)
    print(f"PASS: {passed}/{len(TESTS)} material-gallery regressions")
if __name__=="__main__": main()
