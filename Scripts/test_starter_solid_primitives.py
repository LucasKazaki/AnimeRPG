#!/usr/bin/env python3
"""Focused regression suite for ART-009 starter solid primitives."""
from __future__ import annotations

import base64
import hashlib
import importlib.util
import json
import struct
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
SOURCE = REPO / "Content/Starter/SolidPrimitivesV1/source-contract.json"
EXPECTED_MANIFEST = REPO / "Content/Starter/SolidPrimitivesV1/expected-manifest.json"
GEN_PATH = HERE / "generate_starter_solid_primitives.py"
VER_PATH = HERE / "verify_starter_solid_primitives.py"

def load_module(name,path):
    spec=importlib.util.spec_from_file_location(name,path)
    mod=importlib.util.module_from_spec(spec); spec.loader.exec_module(mod); return mod

GEN=load_module("art009_gen",GEN_PATH)
VER=load_module("art009_ver",VER_PATH)

def canon(obj): return json.dumps(obj,indent=2,separators=(",",": "),ensure_ascii=False)+"\n"

def workspace():
    td=tempfile.TemporaryDirectory()
    root=Path(td.name)
    src=root/"source-contract.json"; src.write_bytes(SOURCE.read_bytes())
    gltf=root/"starter_solid_primitives_v1.gltf"; manifest=root/"expected-manifest.json"
    GEN.write_or_check(src,gltf,manifest,False)
    return td,src,gltf,manifest

def expect_fail(fn,needle):
    try: fn()
    except (ValueError,SystemExit) as exc:
        if needle.lower() not in str(exc).lower():
            raise AssertionError(f"expected {needle!r}, got {exc!r}")
        return
    raise AssertionError(f"expected failure containing {needle!r}")

def repin(manifest,gltf):
    m=json.loads(manifest.read_text())
    b=gltf.read_bytes()
    m["gltf_sha256"]=hashlib.sha256(b).hexdigest()
    m["gltf_bytes"]=len(b)
    manifest.write_text(canon(m),encoding="utf-8")

def save_gltf(gltf,obj,manifest):
    gltf.write_text(canon(obj),encoding="utf-8")
    repin(manifest,gltf)

def refresh_accessor_bounds(g,accessor_index,data):
    a=g["accessors"][accessor_index]; v=g["bufferViews"][a["bufferView"]]
    widths={"SCALAR":1,"VEC2":2,"VEC3":3,"VEC4":4}; width=widths[a["type"]]
    if a["componentType"]==5126: fmt="f"; size=4
    elif a["componentType"]==5123: fmt="H"; size=2
    else: raise AssertionError("unsupported test accessor")
    count=a["count"]; vals=list(struct.unpack_from("<"+fmt*(count*width),data,v["byteOffset"]))
    rows=[vals[i:i+width] for i in range(0,len(vals),width)]
    a["min"]=[min(row[i] for row in rows) for i in range(width)]
    a["max"]=[max(row[i] for row in rows) for i in range(width)]

def mutate_buffer(gltf,manifest,accessor_index,component_index,value,fmt,refresh_bounds=False):
    g=json.loads(gltf.read_text())
    prefix="data:application/octet-stream;base64,"
    data=bytearray(base64.b64decode(g["buffers"][0]["uri"][len(prefix):]))
    a=g["accessors"][accessor_index]; v=g["bufferViews"][a["bufferView"]]
    if fmt=="f":
        offset=v["byteOffset"]+component_index*4
        struct.pack_into("<f",data,offset,float(value))
    elif fmt=="H":
        offset=v["byteOffset"]+component_index*2
        struct.pack_into("<H",data,offset,int(value))
    else: raise AssertionError(fmt)
    g["buffers"][0]["uri"]=prefix+base64.b64encode(bytes(data)).decode("ascii")
    if refresh_bounds:
        refresh_accessor_bounds(g,accessor_index,data)
    save_gltf(gltf,g,manifest)

def mutate_float_accessor(gltf,manifest,accessor_index,mutator):
    g=json.loads(gltf.read_text())
    prefix="data:application/octet-stream;base64,"
    data=bytearray(base64.b64decode(g["buffers"][0]["uri"][len(prefix):]))
    a=g["accessors"][accessor_index]; v=g["bufferViews"][a["bufferView"]]
    widths={"VEC2":2,"VEC3":3,"VEC4":4}; width=widths[a["type"]]
    if a["componentType"]!=5126: raise AssertionError("float accessor required")
    count=a["count"]
    values=list(struct.unpack_from("<"+"f"*(count*width),data,v["byteOffset"]))
    mutator(values,width,count)
    struct.pack_into("<"+"f"*len(values),data,v["byteOffset"],*values)
    g["buffers"][0]["uri"]=prefix+base64.b64encode(bytes(data)).decode("ascii")
    save_gltf(gltf,g,manifest)

def verify(src,gltf,manifest,expected_manifest=EXPECTED_MANIFEST):
    VER.verify_source(src)
    VER.verify_manifest(manifest,src,gltf)
    VER.verify_expected_manifest(manifest,expected_manifest)
    return VER.verify_gltf(gltf)

def verify_semantic(src,gltf,manifest):
    with tempfile.TemporaryDirectory() as td:
        expected=Path(td)/"expected-manifest.json"
        expected.write_bytes(manifest.read_bytes())
        return verify(src,gltf,manifest,expected)

def test_valid_packet():
    td,src,gltf,manifest=workspace()
    try:
        result=verify(src,gltf,manifest)
        assert result=={"meshes":4,"materials":1,"vertices":249,"indices":906}
        GEN.write_or_check(src,gltf,manifest,True)
    finally: td.cleanup()

def test_generator_check_detects_stale():
    td,src,gltf,manifest=workspace()
    try:
        gltf.write_text(gltf.read_text()+" ",encoding="utf-8")
        expect_fail(lambda: GEN.write_or_check(src,gltf,manifest,True),"stale")
    finally: td.cleanup()

def test_generator_refuses_overwrite():
    td,src,gltf,manifest=workspace()
    try: expect_fail(lambda: GEN.write_or_check(src,gltf,manifest,False),"REFUSE")
    finally: td.cleanup()

def test_source_schema_bool_rejected_both():
    td,src,gltf,manifest=workspace()
    try:
        s=json.loads(src.read_text()); s["schema_version"]=True; src.write_text(canon(s))
        expect_fail(lambda: GEN.load_source(src),"integer 1")
        expect_fail(lambda: VER.verify_source(src),"JSON integer")
    finally: td.cleanup()

def test_source_runtime_claim_rejected():
    td,src,gltf,manifest=workspace()
    try:
        s=json.loads(src.read_text()); s["status"]="runtime_verified"; src.write_text(canon(s))
        expect_fail(lambda: GEN.load_source(src),"source-only")
        expect_fail(lambda: VER.verify_source(src),"identity/status")
    finally: td.cleanup()

def test_manifest_source_hash_required():
    td,src,gltf,manifest=workspace()
    try:
        m=json.loads(manifest.read_text()); m["source_sha256"]="0"*64; manifest.write_text(canon(m))
        expect_fail(lambda: VER.verify_manifest(manifest,src,gltf),"source hash")
    finally: td.cleanup()

def test_manifest_bool_count_rejected():
    td,src,gltf,manifest=workspace()
    try:
        m=json.loads(manifest.read_text()); m["mesh_count"]=True; manifest.write_text(canon(m))
        expect_fail(lambda: VER.verify_manifest(manifest,src,gltf),"JSON integer")
    finally: td.cleanup()

def test_checked_in_expected_manifest_pin_required():
    td,src,gltf,manifest=workspace()
    try:
        expected=Path(td.name)/"stale-expected.json"; expected.write_text("{}\n",encoding="utf-8")
        expect_fail(lambda: verify(src,gltf,manifest,expected),"expected manifest")
    finally: td.cleanup()

def test_external_buffer_rejected_after_repin():
    td,src,gltf,manifest=workspace()
    try:
        g=json.loads(gltf.read_text()); g["buffers"][0]["uri"]="external.bin"; save_gltf(gltf,g,manifest)
        expect_fail(lambda: verify_semantic(src,gltf,manifest),"embedded")
    finally: td.cleanup()

def test_extra_attribute_rejected_after_repin():
    td,src,gltf,manifest=workspace()
    try:
        g=json.loads(gltf.read_text()); g["meshes"][0]["primitives"][0]["attributes"]["COLOR_0"]=0; save_gltf(gltf,g,manifest)
        expect_fail(lambda: verify_semantic(src,gltf,manifest),"attribute binding")
    finally: td.cleanup()

def test_material_drift_rejected_after_repin():
    td,src,gltf,manifest=workspace()
    try:
        g=json.loads(gltf.read_text()); g["materials"][0]["pbrMetallicRoughness"]["roughnessFactor"]=0.2; save_gltf(gltf,g,manifest)
        expect_fail(lambda: verify_semantic(src,gltf,manifest),"material")
    finally: td.cleanup()

def test_uv_out_of_range_rejected_after_repin():
    td,src,gltf,manifest=workspace()
    try:
        mutate_buffer(gltf,manifest,3,0,1.5,"f")
        expect_fail(lambda: verify_semantic(src,gltf,manifest),"UV")
    finally: td.cleanup()

def test_nonunit_normal_rejected_after_repin():
    td,src,gltf,manifest=workspace()
    try:
        mutate_buffer(gltf,manifest,1,0,0.5,"f")
        expect_fail(lambda: verify_semantic(src,gltf,manifest),"normal")
    finally: td.cleanup()

def test_bad_tangent_rejected_after_repin():
    td,src,gltf,manifest=workspace()
    try:
        mutate_buffer(gltf,manifest,2,0,0.25,"f")
        expect_fail(lambda: verify_semantic(src,gltf,manifest),"tangent")
    finally: td.cleanup()

def test_index_out_of_range_rejected_after_repin():
    td,src,gltf,manifest=workspace()
    try:
        mutate_buffer(gltf,manifest,4,0,999,"H",refresh_bounds=True)
        expect_fail(lambda: verify_semantic(src,gltf,manifest),"index out of range")
    finally: td.cleanup()

def test_reversed_winding_rejected_after_repin():
    td,src,gltf,manifest=workspace()
    try:
        g=json.loads(gltf.read_text()); prefix="data:application/octet-stream;base64,"
        data=bytearray(base64.b64decode(g["buffers"][0]["uri"][len(prefix):]))
        a=g["accessors"][4]; v=g["bufferViews"][a["bufferView"]]; off=v["byteOffset"]
        i0,i1=struct.unpack_from("<HH",data,off); struct.pack_into("<HH",data,off,i1,i0)
        g["buffers"][0]["uri"]=prefix+base64.b64encode(bytes(data)).decode("ascii"); save_gltf(gltf,g,manifest)
        expect_fail(lambda: verify_semantic(src,gltf,manifest),"winding")
    finally: td.cleanup()

def test_sphere_radius_drift_rejected_after_repin():
    td,src,gltf,manifest=workspace()
    try:
        mutate_buffer(gltf,manifest,5,1,0.45,"f")
        expect_fail(lambda: verify_semantic(src,gltf,manifest),"sphere vertex")
    finally: td.cleanup()

def test_cylinder_cap_drift_rejected_after_repin():
    td,src,gltf,manifest=workspace()
    try:
        mutate_buffer(gltf,manifest,10,34*3+1,0.9,"f")
        expect_fail(lambda: verify_semantic(src,gltf,manifest),"cylinder cap")
    finally: td.cleanup()

def test_plane_height_drift_rejected_after_repin():
    td,src,gltf,manifest=workspace()
    try:
        mutate_buffer(gltf,manifest,15,1,0.1,"f",refresh_bounds=True)
        expect_fail(lambda: verify_semantic(src,gltf,manifest),"AABB")
    finally: td.cleanup()

def test_bufferview_overflow_rejected_after_repin():
    td,src,gltf,manifest=workspace()
    try:
        g=json.loads(gltf.read_text()); g["bufferViews"][0]["byteLength"]+=4; save_gltf(gltf,g,manifest)
        expect_fail(lambda: verify_semantic(src,gltf,manifest),"exactly consume")
    finally: td.cleanup()

def test_unknown_root_field_rejected_after_repin():
    td,src,gltf,manifest=workspace()
    try:
        g=json.loads(gltf.read_text()); g["extensionsUsed"]=[]; save_gltf(gltf,g,manifest)
        expect_fail(lambda: verify_semantic(src,gltf,manifest),"root keys")
    finally: td.cleanup()

def test_material_bool_numeric_rejected_after_repin():
    td,src,gltf,manifest=workspace()
    try:
        g=json.loads(gltf.read_text()); g["materials"][0]["pbrMetallicRoughness"]["metallicFactor"]=False; save_gltf(gltf,g,manifest)
        expect_fail(lambda: verify_semantic(src,gltf,manifest),"finite JSON number")
    finally: td.cleanup()

def test_scene_bool_node_index_rejected_after_repin():
    td,src,gltf,manifest=workspace()
    try:
        g=json.loads(gltf.read_text()); g["scenes"][0]["nodes"][0]=False; save_gltf(gltf,g,manifest)
        expect_fail(lambda: verify_semantic(src,gltf,manifest),"JSON integer")
    finally: td.cleanup()

def test_bufferview_target_rejected_after_repin():
    td,src,gltf,manifest=workspace()
    try:
        g=json.loads(gltf.read_text()); g["bufferViews"][0]["target"]=34963; save_gltf(gltf,g,manifest)
        expect_fail(lambda: verify_semantic(src,gltf,manifest),"target")
    finally: td.cleanup()

def test_accessor_bounds_stale_rejected_after_repin():
    td,src,gltf,manifest=workspace()
    try:
        g=json.loads(gltf.read_text()); g["accessors"][0]["max"][0]=0.6; save_gltf(gltf,g,manifest)
        expect_fail(lambda: verify_semantic(src,gltf,manifest),"min/max")
    finally: td.cleanup()

def test_source_uv_policy_drift_rejected_both():
    td,src,gltf,manifest=workspace()
    try:
        source=json.loads(src.read_text()); source["primitives"][0]["uv_policy"]="anything"; src.write_text(canon(source))
        expect_fail(lambda: GEN.load_source(src),"mesh/UV")
        expect_fail(lambda: VER.verify_source(src),"mesh/shape/UV")
    finally: td.cleanup()

def test_source_reference_drift_rejected_both():
    td,src,gltf,manifest=workspace()
    try:
        source=json.loads(src.read_text()); source["reference_scope"]["observed_2026_09_24"][0]["official_url"]="https://example.invalid"; src.write_text(canon(source))
        expect_fail(lambda: GEN.load_source(src),"reference inventory")
        expect_fail(lambda: VER.verify_source(src),"reference inventory")
    finally: td.cleanup()

def test_semantically_wrong_cube_tangent_rejected_after_repin():
    td,src,gltf,manifest=workspace()
    try:
        def change(values,width,count):
            assert width==4 and count>=1
            values[0:4]=[0.0,1.0,0.0,1.0]
        mutate_float_accessor(gltf,manifest,2,change)
        expect_fail(lambda: verify_semantic(src,gltf,manifest),"tangent/UV")
    finally: td.cleanup()

def test_cube_uv_policy_collapse_rejected_after_repin():
    td,src,gltf,manifest=workspace()
    try:
        def change(values,width,count):
            assert width==2 and count>=4
            for i in range(4): values[i*2:i*2+2]=[0.5,0.5]
        mutate_float_accessor(gltf,manifest,3,change)
        expect_fail(lambda: verify_semantic(src,gltf,manifest),"UV policy")
    finally: td.cleanup()

def test_attribute_bool_binding_rejected_after_repin():
    td,src,gltf,manifest=workspace()
    try:
        g=json.loads(gltf.read_text())
        g["meshes"][0]["primitives"][0]["attributes"]["POSITION"]=False
        save_gltf(gltf,g,manifest)
        expect_fail(lambda: verify_semantic(src,gltf,manifest),"JSON integer")
    finally: td.cleanup()


def test_opposing_cube_tangent_cancellation_rejected_after_repin():
    td,src,gltf,manifest=workspace()
    try:
        def change(values,width,count):
            assert width==4 and count>=3
            c=0.17364817766693041
            s=0.984807753012208
            values[0:4]=[c,s,0.0,1.0]
            values[8:12]=[c,-s,0.0,1.0]
        mutate_float_accessor(gltf,manifest,2,change)
        expect_fail(lambda: verify_semantic(src,gltf,manifest),"tangent/UV")
    finally: td.cleanup()

def test_crlf_source_line_endings_preserve_manifest_pin():
    td=tempfile.TemporaryDirectory()
    try:
        root=Path(td.name)
        src=root/"source-contract.json"
        normalized=SOURCE.read_text(encoding="utf-8").replace("\r\n","\n").replace("\r","\n")
        src.write_bytes(normalized.replace("\n","\r\n").encode("utf-8"))
        gltf=root/"starter_solid_primitives_v1.gltf"; manifest=root/"expected-manifest.json"
        GEN.write_or_check(src,gltf,manifest,False)
        result=verify(src,gltf,manifest)
        assert result=={"meshes":4,"materials":1,"vertices":249,"indices":906}
    finally: td.cleanup()

def test_crlf_expected_manifest_line_endings_preserve_pin():
    td,src,gltf,manifest=workspace()
    try:
        expected=Path(td.name)/"expected-manifest-crlf.json"
        normalized=EXPECTED_MANIFEST.read_bytes().replace(b"\r\n",b"\n").replace(b"\r",b"\n")
        expected.write_bytes(normalized.replace(b"\n",b"\r\n"))
        result=verify(src,gltf,manifest,expected)
        assert result=={"meshes":4,"materials":1,"vertices":249,"indices":906}
    finally: td.cleanup()


def test_source_material_color_space_key_rejected_both():
    td,src,gltf,manifest=workspace()
    try:
        source=json.loads(src.read_text())
        source["material"]["base_color_srgb"]=source["material"].pop("base_color_linear_factor")
        src.write_text(canon(source))
        expect_fail(lambda: GEN.load_source(src),"material")
        expect_fail(lambda: VER.verify_source(src),"material")
    finally: td.cleanup()

TESTS=[v for k,v in sorted(globals().items()) if k.startswith("test_") and callable(v)]

if __name__=="__main__":
    failures=[]
    for test in TESTS:
        try: test()
        except Exception as exc: failures.append((test.__name__,exc))
    if failures:
        for name,exc in failures: print(f"FAIL: {name}: {exc}")
        raise SystemExit(1)
    print(f"PASS: {len(TESTS)}/{len(TESTS)} starter solid primitive tests")
