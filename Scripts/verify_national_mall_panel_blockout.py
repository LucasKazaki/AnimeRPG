#!/usr/bin/env python3
"""Independent structural verifier for ART-006B National Mall panel glTF."""
from __future__ import annotations
import argparse, base64, hashlib, json, math, struct
from pathlib import Path
import generate_national_mall_panel_blockout as gen

ROOT_KEYS={"asset","scene","scenes","nodes","buffers","bufferViews","accessors","materials","meshes"}
MANIFEST_KEYS={
    "schema_version","asset_id","status","ledger_ref","ledger_blob_sha1","source_sha256",
    "gltf_file","gltf_sha256","gltf_bytes","unique_meshes","mesh_instances","materials",
    "unit_box_vertices","unit_box_indices","dimensions_m"
}
DIM_KEYS={"lawn_longitudinal","lawn_cross","path_width","grove_band_width","outer_path_longitudinal","outer_path_cross","total_module_cross"}

def _load_json(path):
    return json.loads(Path(path).read_text(encoding="utf-8"))

def _finite_list(v,n,name):
    if type(v) is not list or len(v)!=n:
        raise ValueError(f"{name} shape invalid")
    out=[]
    for x in v:
        if isinstance(x,bool) or not isinstance(x,(int,float)) or not math.isfinite(float(x)):
            raise ValueError(f"{name} contains non-finite number")
        out.append(float(x))
    return out

def _eq_list(a,b,tol=1e-6):
    return len(a)==len(b) and all(math.isclose(float(x),float(y),rel_tol=0.0,abs_tol=tol) for x,y in zip(a,b))

def verify(source_path: Path, gltf_path: Path, manifest_path: Path):
    src=gen.load_source(source_path)
    g=_load_json(gltf_path); m=_load_json(manifest_path)
    if type(g) is not dict or set(g)!=ROOT_KEYS:
        raise ValueError("glTF root keys must match closed ART-006B profile")
    if g["asset"]!={"version":"2.0","generator":gen.GENERATOR_ID}:
        raise ValueError("glTF asset metadata changed")
    if type(g["scene"]) is not int or g["scene"]!=0:
        raise ValueError("scene must be integer 0")
    if g["scenes"]!=[{"name":"National_Mall_Core_Panel_Module","nodes":[0,1,2,3,4,5,6]}]:
        raise ValueError("scene ownership changed")
    if type(g["buffers"]) is not list or len(g["buffers"])!=1 or set(g["buffers"][0])!={"byteLength","uri"}:
        raise ValueError("buffer profile invalid")
    uri=g["buffers"][0]["uri"]
    prefix="data:application/octet-stream;base64,"
    if type(uri) is not str or not uri.startswith(prefix):
        raise ValueError("buffer must be embedded application/octet-stream data URI")
    try:
        blob=base64.b64decode(uri[len(prefix):], validate=True)
    except Exception as e:
        raise ValueError("invalid base64 buffer") from e
    if type(g["buffers"][0]["byteLength"]) is not int or g["buffers"][0]["byteLength"]!=len(blob):
        raise ValueError("buffer byteLength mismatch")
    if len(blob)>4096:
        raise ValueError("unexpectedly large bounded fixture buffer")
    exp_gltf,_=gen.build_outputs(source_path)
    exp=json.loads(exp_gltf)
    if g["bufferViews"]!=exp["bufferViews"] or g["accessors"]!=exp["accessors"]:
        raise ValueError("bufferView/accessor contract changed")
    if g["materials"]!=exp["materials"]:
        raise ValueError("material blockout contract changed")
    if g["meshes"]!=exp["meshes"]:
        raise ValueError("mesh/material ownership changed")
    if type(g["nodes"]) is not list or len(g["nodes"])!=7:
        raise ValueError("node count invalid")
    for got,want in zip(g["nodes"],exp["nodes"]):
        if set(got)!={"name","mesh","translation","scale"}:
            raise ValueError("node keys invalid")
        if got["name"]!=want["name"] or type(got["mesh"]) is not int or got["mesh"]!=want["mesh"]:
            raise ValueError("node identity/mesh ownership changed")
        if not _eq_list(_finite_list(got["translation"],3,got["name"]+".translation"), want["translation"]):
            raise ValueError("node translation changed")
        if not _eq_list(_finite_list(got["scale"],3,got["name"]+".scale"), want["scale"]):
            raise ValueError("node scale changed")
    exp_uri=exp["buffers"][0]["uri"]
    exp_blob=base64.b64decode(exp_uri.split(",",1)[1], validate=True)
    if blob!=exp_blob:
        raise ValueError("decoded canonical unit-box geometry changed")
    if len(blob)<72:
        raise ValueError("buffer too small")
    idx_view=g["bufferViews"][4]
    idx=struct.unpack_from("<36H",blob,idx_view["byteOffset"])
    if len(idx)!=36 or min(idx)!=0 or max(idx)!=23:
        raise ValueError("index range invalid")
    for tri in range(0,36,3):
        if len({idx[tri],idx[tri+1],idx[tri+2]})<3:
            raise ValueError("degenerate triangle")
    if type(m) is not dict or set(m)!=MANIFEST_KEYS:
        raise ValueError("manifest root keys invalid")
    if type(m["schema_version"]) is not int or m["schema_version"]!=1:
        raise ValueError("manifest schema_version must be integer 1")
    if m["status"]!=gen.STATUS or m["asset_id"]!=src["asset_id"]:
        raise ValueError("manifest source-only identity changed")
    if m["ledger_ref"]!=src["ledger_ref"] or m["ledger_blob_sha1"]!=src["ledger_blob_sha1"]:
        raise ValueError("manifest ledger provenance changed")
    if m["gltf_file"]!=gltf_path.name:
        raise ValueError("manifest glTF filename mismatch")
    sb=source_path.read_bytes(); gb=gltf_path.read_bytes()
    if m["source_sha256"]!=hashlib.sha256(sb).hexdigest():
        raise ValueError("source hash mismatch")
    if m["gltf_sha256"]!=hashlib.sha256(gb).hexdigest() or m["gltf_bytes"]!=len(gb):
        raise ValueError("glTF hash/size mismatch")
    for k,v in {"unique_meshes":3,"mesh_instances":7,"materials":3,"unit_box_vertices":24,"unit_box_indices":36}.items():
        if type(m[k]) is not int or m[k]!=v:
            raise ValueError(f"manifest {k} invalid")
    if type(m["dimensions_m"]) is not dict or set(m["dimensions_m"])!=DIM_KEYS:
        raise ValueError("manifest dimensions keys invalid")
    _,exp_manifest=gen.build_outputs(source_path)
    em=json.loads(exp_manifest)
    for k,w in em["dimensions_m"].items():
        got=m["dimensions_m"][k]
        if isinstance(got,bool) or not isinstance(got,(int,float)) or not math.isclose(float(got),float(w),rel_tol=0,abs_tol=1e-9):
            raise ValueError(f"manifest dimension {k} invalid")
    return {"nodes":7,"materials":3,"vertices":24,"indices":36}

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--source",type=Path,required=True)
    ap.add_argument("--gltf",type=Path,required=True)
    ap.add_argument("--manifest",type=Path,required=True)
    a=ap.parse_args()
    r=verify(a.source,a.gltf,a.manifest)
    print(f"PASS: source-only National Mall panel blockout {r}")

if __name__=="__main__":
    main()
