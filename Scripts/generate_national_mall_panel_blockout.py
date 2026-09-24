#!/usr/bin/env python3
"""Deterministically generate the ART-006B National Mall core-panel source glTF."""
from __future__ import annotations
import argparse, base64, hashlib, json, math, struct
from pathlib import Path

GENERATOR_ID = "Astral ART-006B National Mall panel blockout 1"
STATUS = "source_validated_not_imported"
SOURCE_KEYS = {
    "schema_version","asset_id","loop_id","status","ledger_ref","ledger_blob_sha1",
    "ledger_entry_id","source_units_policy","coordinate_system","source_measurements","authoring"
}
COORD_KEYS = {"handedness","up","forward","right","linear_unit","axis_note"}
AUTHOR_KEYS = {
    "module_scope","ground_thickness_m","path_thickness_m","grove_envelope_thickness_m",
    "grove_band_length_policy","tree_geometry","collision","textures","runtime_import"
}
EXPECTED_MEASUREMENTS = {
    "lawn_panel_count": (8, "count", "exact_count", None),
    "typical_lawn_panel_length": (450, "feet", "approximate", 137.16),
    "typical_lawn_panel_width": (170, "feet", "approximate", 51.816),
    "perimeter_gravel_path_width": (35, "feet", "approximate", 10.668),
    "tree_grove_width": (130, "feet", "approximate", 39.624),
}
MATERIALS = [
    ("Lawn_Blockout", [0.18,0.36,0.12,1.0], 0.95),
    ("Gravel_Path_Blockout", [0.50,0.45,0.35,1.0], 1.0),
    ("Tree_Grove_Envelope", [0.08,0.20,0.08,1.0], 1.0),
]

def _num(v, name):
    if isinstance(v, bool) or not isinstance(v, (int,float)) or not math.isfinite(float(v)):
        raise ValueError(f"{name} must be a finite JSON number")
    return float(v)

def load_source(path: Path) -> dict:
    src = json.loads(path.read_text(encoding="utf-8"))
    if type(src) is not dict or set(src) != SOURCE_KEYS:
        raise ValueError("source root keys do not match ART-006B contract")
    if type(src["schema_version"]) is not int or src["schema_version"] != 1:
        raise ValueError("schema_version must be JSON integer 1")
    if src["asset_id"] != "national-mall-core-panel-module-v1":
        raise ValueError("unexpected asset_id")
    if src["loop_id"] != "astral-art-hourly-20260922":
        raise ValueError("unexpected loop_id")
    if src["status"] != STATUS:
        raise ValueError("source status must remain source-only")
    if src["ledger_ref"] != "Content/Reference/NationalMall/reference-ledger.json":
        raise ValueError("unexpected ledger_ref")
    if src["ledger_blob_sha1"] != "ba8ec205d2aecfa4b2ace15c13c71fb9932cb7f4":
        raise ValueError("source must pin the reviewed ART-006A ledger blob")
    if src["ledger_entry_id"] != "mall-core-axis":
        raise ValueError("unexpected ledger_entry_id")
    if src["source_units_policy"] != "retain_source_values_and_convert_to_metres_at_authoring_boundary":
        raise ValueError("unexpected units policy")
    c = src["coordinate_system"]
    if type(c) is not dict or set(c) != COORD_KEYS:
        raise ValueError("coordinate_system keys invalid")
    if (c["handedness"],c["up"],c["forward"],c["right"],c["linear_unit"]) != ("right","+Y","+Z","-X","metre"):
        raise ValueError("coordinate contract changed")
    if c["axis_note"] != "asset-local +Z is the panel longitudinal axis; no survey east/west placement is claimed":
        raise ValueError("axis note changed")
    a = src["authoring"]
    if type(a) is not dict or set(a) != AUTHOR_KEYS:
        raise ValueError("authoring keys invalid")
    if a["module_scope"] != "one_typical_panel_of_eight" or a["grove_band_length_policy"] != "match_outer_path_length":
        raise ValueError("authoring scope changed")
    if (a["tree_geometry"],a["collision"],a["textures"],a["runtime_import"]) != ("not_authored","not_authored","not_authored","not_claimed"):
        raise ValueError("unsupported capability claim")
    for k in ("ground_thickness_m","path_thickness_m","grove_envelope_thickness_m"):
        if _num(a[k], k) <= 0:
            raise ValueError(f"{k} must be positive")
    ms = src["source_measurements"]
    if type(ms) is not list or len(ms) != len(EXPECTED_MEASUREMENTS):
        raise ValueError("source_measurements count invalid")
    seen = set()
    for m in ms:
        allowed = {"label","source_value","source_unit","source_relation"} | ({"converted_m"} if "converted_m" in m else set())
        if type(m) is not dict or set(m) != allowed:
            raise ValueError("measurement keys invalid")
        label = m["label"]
        if label in seen or label not in EXPECTED_MEASUREMENTS:
            raise ValueError("measurement identity invalid")
        seen.add(label)
        exp_value, exp_unit, exp_rel, exp_m = EXPECTED_MEASUREMENTS[label]
        value = _num(m["source_value"], f"{label}.source_value")
        if value != float(exp_value) or m["source_unit"] != exp_unit or m["source_relation"] != exp_rel:
            raise ValueError(f"{label} source provenance changed")
        if exp_m is None:
            if "converted_m" in m:
                raise ValueError("count must not have converted_m")
        else:
            if set(m) != {"label","source_value","source_unit","source_relation","converted_m"}:
                raise ValueError("linear measurement keys invalid")
            cm = _num(m["converted_m"], f"{label}.converted_m")
            if not math.isclose(cm, exp_m, rel_tol=0.0, abs_tol=1e-9):
                raise ValueError(f"{label} converted metres changed")
            if not math.isclose(cm, value * 0.3048, rel_tol=0.0, abs_tol=1e-9):
                raise ValueError(f"{label} conversion is not feet-to-metres")
    return src

def _measurements(src):
    return {m["label"]: m for m in src["source_measurements"]}

def _align4(blob: bytearray):
    while len(blob) % 4:
        blob.append(0)

def _pack_f32(blob, values):
    off = len(blob); blob.extend(struct.pack("<"+"f"*len(values), *values)); _align4(blob); return off, len(values)*4

def _pack_u16(blob, values):
    off = len(blob); blob.extend(struct.pack("<"+"H"*len(values), *values)); _align4(blob); return off, len(values)*2

def _cube_payload():
    # 24 vertices, four unique vertices per face, CCW from the outside.
    faces = [
        # +Z
        ([(-.5,-.5,.5),(.5,-.5,.5),(.5,.5,.5),(-.5,.5,.5)], (0,0,1), (1,0,0,1)),
        # -Z
        ([(.5,-.5,-.5),(-.5,-.5,-.5),(-.5,.5,-.5),(.5,.5,-.5)], (0,0,-1), (-1,0,0,1)),
        # +X
        ([(.5,-.5,.5),(.5,-.5,-.5),(.5,.5,-.5),(.5,.5,.5)], (1,0,0), (0,0,-1,1)),
        # -X
        ([(-.5,-.5,-.5),(-.5,-.5,.5),(-.5,.5,.5),(-.5,.5,-.5)], (-1,0,0), (0,0,1,1)),
        # +Y
        ([(-.5,.5,.5),(.5,.5,.5),(.5,.5,-.5),(-.5,.5,-.5)], (0,1,0), (1,0,0,1)),
        # -Y
        ([(-.5,-.5,-.5),(.5,-.5,-.5),(.5,-.5,.5),(-.5,-.5,.5)], (0,-1,0), (1,0,0,-1)),
    ]
    pos=[]; norm=[]; tan=[]; uv=[]; idx=[]
    uvs=[(0,0),(1,0),(1,1),(0,1)]
    for fi,(verts,n,t) in enumerate(faces):
        base=fi*4
        for i,v in enumerate(verts):
            pos.extend(v); norm.extend(n); tan.extend(t); uv.extend(uvs[i])
        idx.extend([base,base+1,base+2,base,base+2,base+3])
    return pos,norm,tan,uv,idx

def build_gltf(src: dict) -> dict:
    ms = _measurements(src)
    lawn_z = float(ms["typical_lawn_panel_length"]["converted_m"])
    lawn_x = float(ms["typical_lawn_panel_width"]["converted_m"])
    path = float(ms["perimeter_gravel_path_width"]["converted_m"])
    grove = float(ms["tree_grove_width"]["converted_m"])
    outer_z = round(lawn_z + 2*path, 6)
    outer_x = round(lawn_x + 2*path, 6)
    total_x = round(outer_x + 2*grove, 6)
    a=src["authoring"]
    gy=float(a["ground_thickness_m"]); py=float(a["path_thickness_m"]); ty=float(a["grove_envelope_thickness_m"])
    blob=bytearray()
    pos,norm,tan,uv,idx=_cube_payload()
    p_off,p_len=_pack_f32(blob,pos); n_off,n_len=_pack_f32(blob,norm)
    t_off,t_len=_pack_f32(blob,tan); u_off,u_len=_pack_f32(blob,uv); i_off,i_len=_pack_u16(blob,idx)
    b64=base64.b64encode(bytes(blob)).decode("ascii")
    bvs=[
        {"buffer":0,"byteOffset":p_off,"byteLength":p_len,"target":34962},
        {"buffer":0,"byteOffset":n_off,"byteLength":n_len,"target":34962},
        {"buffer":0,"byteOffset":t_off,"byteLength":t_len,"target":34962},
        {"buffer":0,"byteOffset":u_off,"byteLength":u_len,"target":34962},
        {"buffer":0,"byteOffset":i_off,"byteLength":i_len,"target":34963},
    ]
    acc=[
        {"bufferView":0,"componentType":5126,"count":24,"type":"VEC3","min":[-.5,-.5,-.5],"max":[.5,.5,.5]},
        {"bufferView":1,"componentType":5126,"count":24,"type":"VEC3","min":[-1,-1,-1],"max":[1,1,1]},
        {"bufferView":2,"componentType":5126,"count":24,"type":"VEC4","min":[-1,0,-1,-1],"max":[1,0,1,1]},
        {"bufferView":3,"componentType":5126,"count":24,"type":"VEC2","min":[0,0],"max":[1,1]},
        {"bufferView":4,"componentType":5123,"count":36,"type":"SCALAR","min":[0],"max":[23]},
    ]
    mats=[]
    for name,color,rough in MATERIALS:
        mats.append({"name":name,"pbrMetallicRoughness":{"baseColorFactor":color,"metallicFactor":0.0,"roughnessFactor":rough},"doubleSided":False})
    prim={"attributes":{"POSITION":0,"NORMAL":1,"TANGENT":2,"TEXCOORD_0":3},"indices":4,"mode":4}
    meshes=[
        {"name":"UnitBox_Lawn","primitives":[dict(prim,material=0)]},
        {"name":"UnitBox_Path","primitives":[dict(prim,material=1)]},
        {"name":"UnitBox_GroveEnvelope","primitives":[dict(prim,material=2)]},
    ]
    nodes=[
        {"name":"Lawn","mesh":0,"translation":[0,-gy/2,0],"scale":[lawn_x,gy,lawn_z]},
        {"name":"Path_Longitudinal_PosX","mesh":1,"translation":[round(lawn_x/2+path/2,6),-py/2,0],"scale":[path,py,lawn_z]},
        {"name":"Path_Longitudinal_NegX","mesh":1,"translation":[-round(lawn_x/2+path/2,6),-py/2,0],"scale":[path,py,lawn_z]},
        {"name":"Path_End_PosZ","mesh":1,"translation":[0,-py/2,round(lawn_z/2+path/2,6)],"scale":[outer_x,py,path]},
        {"name":"Path_End_NegZ","mesh":1,"translation":[0,-py/2,-round(lawn_z/2+path/2,6)],"scale":[outer_x,py,path]},
        {"name":"Grove_Envelope_PosX","mesh":2,"translation":[round(outer_x/2+grove/2,6),-ty/2,0],"scale":[grove,ty,outer_z]},
        {"name":"Grove_Envelope_NegX","mesh":2,"translation":[-round(outer_x/2+grove/2,6),-ty/2,0],"scale":[grove,ty,outer_z]},
    ]
    return {
        "asset":{"version":"2.0","generator":GENERATOR_ID},
        "scene":0,
        "scenes":[{"name":"National_Mall_Core_Panel_Module","nodes":list(range(7))}],
        "nodes":nodes,
        "buffers":[{"byteLength":len(blob),"uri":"data:application/octet-stream;base64,"+b64}],
        "bufferViews":bvs,
        "accessors":acc,
        "materials":mats,
        "meshes":meshes,
    }

def _canon(obj):
    return (json.dumps(obj, indent=2, separators=(",", ": "), ensure_ascii=False)+"\n").encode("utf-8")

def build_outputs(source_path: Path):
    src=load_source(source_path)
    gltf=build_gltf(src)
    gltf_bytes=_canon(gltf)
    source_bytes=source_path.read_bytes()
    ms=_measurements(src)
    lawn_z=float(ms["typical_lawn_panel_length"]["converted_m"])
    lawn_x=float(ms["typical_lawn_panel_width"]["converted_m"])
    path=float(ms["perimeter_gravel_path_width"]["converted_m"])
    grove=float(ms["tree_grove_width"]["converted_m"])
    outer_z=round(lawn_z+2*path,6); outer_x=round(lawn_x+2*path,6)
    manifest={
        "schema_version":1,
        "asset_id":src["asset_id"],
        "status":STATUS,
        "ledger_ref":src["ledger_ref"],
        "ledger_blob_sha1":src["ledger_blob_sha1"],
        "source_sha256":hashlib.sha256(source_bytes).hexdigest(),
        "gltf_file":"mall_core_panel_blockout.gltf",
        "gltf_sha256":hashlib.sha256(gltf_bytes).hexdigest(),
        "gltf_bytes":len(gltf_bytes),
        "unique_meshes":3,
        "mesh_instances":7,
        "materials":3,
        "unit_box_vertices":24,
        "unit_box_indices":36,
        "dimensions_m":{
            "lawn_longitudinal":lawn_z,
            "lawn_cross":lawn_x,
            "path_width":path,
            "grove_band_width":grove,
            "outer_path_longitudinal":outer_z,
            "outer_path_cross":outer_x,
            "total_module_cross":round(outer_x+2*grove,6),
        }
    }
    return gltf_bytes, _canon(manifest)

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--source",type=Path,required=True)
    ap.add_argument("--gltf",type=Path,required=True)
    ap.add_argument("--manifest",type=Path,required=True)
    ap.add_argument("--check",action="store_true")
    args=ap.parse_args()
    gltf_b,man_b=build_outputs(args.source)
    if args.check:
        if not args.gltf.is_file() or args.gltf.read_bytes()!=gltf_b:
            raise SystemExit("FAIL: glTF differs from deterministic output")
        if not args.manifest.is_file() or args.manifest.read_bytes()!=man_b:
            raise SystemExit("FAIL: manifest differs from deterministic output")
        print("PASS: deterministic National Mall panel blockout matches pinned files")
        return
    if args.gltf.exists() or args.manifest.exists():
        raise SystemExit("FAIL: refusing to overwrite existing output; use --check")
    args.gltf.parent.mkdir(parents=True, exist_ok=True)
    args.manifest.parent.mkdir(parents=True, exist_ok=True)
    args.gltf.write_bytes(gltf_b); args.manifest.write_bytes(man_b)
    print("PASS: wrote source-only National Mall panel blockout")

if __name__=="__main__":
    main()
