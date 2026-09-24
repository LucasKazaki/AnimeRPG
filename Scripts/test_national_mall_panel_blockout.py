#!/usr/bin/env python3
"""Focused ART-006B regression suite."""
from __future__ import annotations
import copy, hashlib, json, tempfile
from pathlib import Path
import generate_national_mall_panel_blockout as gen
import verify_national_mall_panel_blockout as ver

HERE=Path(__file__).resolve().parent
REPO=HERE.parent
SOURCE=REPO/"Content/Reference/NationalMall/Blockout/panel-module-source.json"
GLTF=REPO/"Content/Reference/NationalMall/Blockout/mall_core_panel_blockout.gltf"
MANIFEST=REPO/"Content/Reference/NationalMall/Blockout/expected-manifest.json"

def _dump(p,obj): p.write_text(json.dumps(obj,indent=2,separators=(",",": "))+"\n",encoding="utf-8")
def _expect_fail(fn):
    try: fn()
    except (ValueError,SystemExit,json.JSONDecodeError): return
    raise AssertionError("expected failure")

def _fixture():
    td=tempfile.TemporaryDirectory()
    d=Path(td.name)
    s=d/"source.json"; g=d/"mall_core_panel_blockout.gltf"; m=d/"expected-manifest.json"
    s.write_bytes(SOURCE.read_bytes()); g.write_bytes(GLTF.read_bytes()); m.write_bytes(MANIFEST.read_bytes())
    return td,s,g,m

def _repin(s,g,m):
    obj=json.loads(m.read_text())
    obj["source_sha256"]=hashlib.sha256(s.read_bytes()).hexdigest()
    obj["gltf_sha256"]=hashlib.sha256(g.read_bytes()).hexdigest()
    obj["gltf_bytes"]=len(g.read_bytes())
    _dump(m,obj)

def t_valid():
    assert ver.verify(SOURCE,GLTF,MANIFEST)=={"nodes":7,"materials":3,"vertices":24,"indices":36}

def t_source_bool_schema():
    td,s,g,m=_fixture()
    with td:
        o=json.loads(s.read_text()); o["schema_version"]=True; _dump(s,o)
        _expect_fail(lambda: ver.verify(s,g,m))

def t_source_false_runtime_status():
    td,s,g,m=_fixture()
    with td:
        o=json.loads(s.read_text()); o["status"]="runtime_verified"; _dump(s,o); _repin(s,g,m)
        _expect_fail(lambda: ver.verify(s,g,m))

def t_source_conversion_drift():
    td,s,g,m=_fixture()
    with td:
        o=json.loads(s.read_text()); o["source_measurements"][1]["converted_m"]+=1; _dump(s,o); _repin(s,g,m)
        _expect_fail(lambda: ver.verify(s,g,m))

def t_ledger_blob_drift():
    td,s,g,m=_fixture()
    with td:
        o=json.loads(s.read_text()); o["ledger_blob_sha1"]="0"*40; _dump(s,o); _repin(s,g,m)
        _expect_fail(lambda: ver.verify(s,g,m))

def t_root_extra_field():
    td,s,g,m=_fixture()
    with td:
        o=json.loads(g.read_text()); o["images"]=[]; _dump(g,o); _repin(s,g,m)
        _expect_fail(lambda: ver.verify(s,g,m))

def t_external_buffer_uri():
    td,s,g,m=_fixture()
    with td:
        o=json.loads(g.read_text()); o["buffers"][0]["uri"]="buffer.bin"; _dump(g,o); _repin(s,g,m)
        _expect_fail(lambda: ver.verify(s,g,m))

def t_node_scale_drift():
    td,s,g,m=_fixture()
    with td:
        o=json.loads(g.read_text()); o["nodes"][0]["scale"][2]+=1; _dump(g,o); _repin(s,g,m)
        _expect_fail(lambda: ver.verify(s,g,m))

def t_node_mesh_swap():
    td,s,g,m=_fixture()
    with td:
        o=json.loads(g.read_text()); o["nodes"][0]["mesh"]=1; _dump(g,o); _repin(s,g,m)
        _expect_fail(lambda: ver.verify(s,g,m))

def t_material_drift():
    td,s,g,m=_fixture()
    with td:
        o=json.loads(g.read_text()); o["materials"][0]["pbrMetallicRoughness"]["roughnessFactor"]=0.2; _dump(g,o); _repin(s,g,m)
        _expect_fail(lambda: ver.verify(s,g,m))

def t_geometry_corruption():
    td,s,g,m=_fixture()
    with td:
        o=json.loads(g.read_text())
        uri=o["buffers"][0]["uri"]; prefix="data:application/octet-stream;base64,"
        import base64
        b=bytearray(base64.b64decode(uri[len(prefix):])); b[0]^=1
        o["buffers"][0]["uri"]=prefix+base64.b64encode(bytes(b)).decode()
        _dump(g,o); _repin(s,g,m)
        _expect_fail(lambda: ver.verify(s,g,m))

def t_tangent_handedness_drift_repin():
    td,s,g,m=_fixture()
    with td:
        o=json.loads(g.read_text())
        uri=o["buffers"][0]["uri"]; prefix="data:application/octet-stream;base64,"
        import base64, struct
        b=bytearray(base64.b64decode(uri[len(prefix):])); struct.pack_into("<f",b,576+(20*4+3)*4,-1.0)
        o["buffers"][0]["uri"]=prefix+base64.b64encode(bytes(b)).decode()
        _dump(g,o); _repin(s,g,m)
        _expect_fail(lambda: ver.verify(s,g,m))

def t_tangent_accessor_contract_drift():
    td,s,g,m=_fixture()
    with td:
        o=json.loads(g.read_text()); o["accessors"][2]["min"][3]=-1; _dump(g,o); _repin(s,g,m)
        _expect_fail(lambda: ver.verify(s,g,m))

def t_manifest_runtime_status():
    td,s,g,m=_fixture()
    with td:
        o=json.loads(m.read_text()); o["status"]="runtime_verified"; _dump(m,o)
        _expect_fail(lambda: ver.verify(s,g,m))

def t_manifest_unknown_field():
    td,s,g,m=_fixture()
    with td:
        o=json.loads(m.read_text()); o["runtime_screenshot"]="fake.png"; _dump(m,o)
        _expect_fail(lambda: ver.verify(s,g,m))

def t_manifest_dimension_drift():
    td,s,g,m=_fixture()
    with td:
        o=json.loads(m.read_text()); o["dimensions_m"]["total_module_cross"]+=1; _dump(m,o)
        _expect_fail(lambda: ver.verify(s,g,m))

def t_generator_check_detects_stale():
    td,s,g,m=_fixture()
    with td:
        g.write_text(g.read_text()+"\n",encoding="utf-8")
        good_g,good_m=gen.build_outputs(s)
        assert g.read_bytes()!=good_g and m.read_bytes()==good_m

TESTS=[
    t_valid,t_source_bool_schema,t_source_false_runtime_status,t_source_conversion_drift,
    t_ledger_blob_drift,t_root_extra_field,t_external_buffer_uri,t_node_scale_drift,
    t_node_mesh_swap,t_material_drift,t_geometry_corruption,t_tangent_handedness_drift_repin,
    t_tangent_accessor_contract_drift,t_manifest_runtime_status,t_manifest_unknown_field,
    t_manifest_dimension_drift,t_generator_check_detects_stale,
]
if __name__=="__main__":
    for i,t in enumerate(TESTS,1):
        t()
        print(f"PASS {i:02d}/{len(TESTS)} {t.__name__}")
    print(f"PASS: {len(TESTS)}/{len(TESTS)} National Mall panel blockout tests")
