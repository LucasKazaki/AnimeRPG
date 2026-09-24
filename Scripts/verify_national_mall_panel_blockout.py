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
CANON_BUFFER_VIEWS=[
    {"buffer":0,"byteOffset":0,"byteLength":288,"target":34962},
    {"buffer":0,"byteOffset":288,"byteLength":288,"target":34962},
    {"buffer":0,"byteOffset":576,"byteLength":384,"target":34962},
    {"buffer":0,"byteOffset":960,"byteLength":192,"target":34962},
    {"buffer":0,"byteOffset":1152,"byteLength":72,"target":34963},
]
CANON_ACCESSORS=[
    {"bufferView":0,"componentType":5126,"count":24,"type":"VEC3","min":[-0.5,-0.5,-0.5],"max":[0.5,0.5,0.5]},
    {"bufferView":1,"componentType":5126,"count":24,"type":"VEC3","min":[-1,-1,-1],"max":[1,1,1]},
    {"bufferView":2,"componentType":5126,"count":24,"type":"VEC4","min":[-1,0,-1,1],"max":[1,0,1,1]},
    {"bufferView":3,"componentType":5126,"count":24,"type":"VEC2","min":[0,0],"max":[1,1]},
    {"bufferView":4,"componentType":5123,"count":36,"type":"SCALAR","min":[0],"max":[23]},
]
CANON_MATERIALS=[
    {"name":"Lawn_Blockout","pbrMetallicRoughness":{"baseColorFactor":[0.18,0.36,0.12,1.0],"metallicFactor":0.0,"roughnessFactor":0.95},"doubleSided":False},
    {"name":"Gravel_Path_Blockout","pbrMetallicRoughness":{"baseColorFactor":[0.50,0.45,0.35,1.0],"metallicFactor":0.0,"roughnessFactor":1.0},"doubleSided":False},
    {"name":"Tree_Grove_Envelope","pbrMetallicRoughness":{"baseColorFactor":[0.08,0.20,0.08,1.0],"metallicFactor":0.0,"roughnessFactor":1.0},"doubleSided":False},
]
_PRIM={"attributes":{"POSITION":0,"NORMAL":1,"TANGENT":2,"TEXCOORD_0":3},"indices":4,"mode":4}
CANON_MESHES=[
    {"name":"UnitBox_Lawn","primitives":[dict(_PRIM,material=0)]},
    {"name":"UnitBox_Path","primitives":[dict(_PRIM,material=1)]},
    {"name":"UnitBox_GroveEnvelope","primitives":[dict(_PRIM,material=2)]},
]
CANON_FACES=[
    ([(-.5,-.5,.5),(.5,-.5,.5),(.5,.5,.5),(-.5,.5,.5)], (0,0,1), (1,0,0,1)),
    ([(.5,-.5,-.5),(-.5,-.5,-.5),(-.5,.5,-.5),(.5,.5,-.5)], (0,0,-1), (-1,0,0,1)),
    ([(.5,-.5,.5),(.5,-.5,-.5),(.5,.5,-.5),(.5,.5,.5)], (1,0,0), (0,0,-1,1)),
    ([(-.5,-.5,-.5),(-.5,-.5,.5),(-.5,.5,.5),(-.5,.5,-.5)], (-1,0,0), (0,0,1,1)),
    ([(-.5,.5,.5),(.5,.5,.5),(.5,.5,-.5),(-.5,.5,-.5)], (0,1,0), (1,0,0,1)),
    ([(-.5,-.5,-.5),(.5,-.5,-.5),(.5,-.5,.5),(-.5,-.5,.5)], (0,-1,0), (1,0,0,1)),
]
CANON_UVS=[(0,0),(1,0),(1,1),(0,1)]

def _canonical_payload():
    pos=[]; norm=[]; tan=[]; uv=[]; idx=[]
    for fi,(verts,n,t) in enumerate(CANON_FACES):
        base=fi*4
        for i,v in enumerate(verts):
            pos.extend(v); norm.extend(n); tan.extend(t); uv.extend(CANON_UVS[i])
        idx.extend([base,base+1,base+2,base,base+2,base+3])
    return tuple(pos),tuple(norm),tuple(tan),tuple(uv),tuple(idx)

CANON_POS,CANON_NORM,CANON_TAN,CANON_UV,CANON_IDX=_canonical_payload()

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

def _sub(a,b): return tuple(x-y for x,y in zip(a,b))
def _mul(a,s): return tuple(x*s for x in a)
def _cross(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def _dot(a,b): return sum(x*y for x,y in zip(a,b))
def _norm(a):
    l=math.sqrt(_dot(a,a))
    if l<=1e-12: raise ValueError("zero-length basis vector")
    return tuple(x/l for x in a)

def _expected_nodes(src):
    ms={m["label"]:m for m in src["source_measurements"]}
    lawn_z=float(ms["typical_lawn_panel_length"]["converted_m"])
    lawn_x=float(ms["typical_lawn_panel_width"]["converted_m"])
    path=float(ms["perimeter_gravel_path_width"]["converted_m"])
    grove=float(ms["tree_grove_width"]["converted_m"])
    outer_z=round(lawn_z+2*path,6); outer_x=round(lawn_x+2*path,6)
    a=src["authoring"]
    gy=float(a["ground_thickness_m"]); py=float(a["path_thickness_m"]); ty=float(a["grove_envelope_thickness_m"])
    return [
        {"name":"Lawn","mesh":0,"translation":[0,-gy/2,0],"scale":[lawn_x,gy,lawn_z]},
        {"name":"Path_Longitudinal_PosX","mesh":1,"translation":[round(lawn_x/2+path/2,6),-py/2,0],"scale":[path,py,lawn_z]},
        {"name":"Path_Longitudinal_NegX","mesh":1,"translation":[-round(lawn_x/2+path/2,6),-py/2,0],"scale":[path,py,lawn_z]},
        {"name":"Path_End_PosZ","mesh":1,"translation":[0,-py/2,round(lawn_z/2+path/2,6)],"scale":[outer_x,py,path]},
        {"name":"Path_End_NegZ","mesh":1,"translation":[0,-py/2,-round(lawn_z/2+path/2,6)],"scale":[outer_x,py,path]},
        {"name":"Grove_Envelope_PosX","mesh":2,"translation":[round(outer_x/2+grove/2,6),-ty/2,0],"scale":[grove,ty,outer_z]},
        {"name":"Grove_Envelope_NegX","mesh":2,"translation":[-round(outer_x/2+grove/2,6),-ty/2,0],"scale":[grove,ty,outer_z]},
    ]

def _decode_geometry(blob):
    pos=struct.unpack_from("<72f",blob,CANON_BUFFER_VIEWS[0]["byteOffset"])
    norm=struct.unpack_from("<72f",blob,CANON_BUFFER_VIEWS[1]["byteOffset"])
    tan=struct.unpack_from("<96f",blob,CANON_BUFFER_VIEWS[2]["byteOffset"])
    uv=struct.unpack_from("<48f",blob,CANON_BUFFER_VIEWS[3]["byteOffset"])
    idx=struct.unpack_from("<36H",blob,CANON_BUFFER_VIEWS[4]["byteOffset"])
    return pos,norm,tan,uv,idx

def _verify_basis(pos,norm,tan,uv,idx):
    for tri in range(0,len(idx),3):
        ids=idx[tri:tri+3]
        if len(set(ids))<3: raise ValueError("degenerate triangle")
        p=[pos[i*3:i*3+3] for i in ids]; t=[uv[i*2:i*2+2] for i in ids]
        e1=_sub(p[1],p[0]); e2=_sub(p[2],p[0])
        face_n=_norm(_cross(e1,e2))
        n0=tuple(norm[ids[0]*3:ids[0]*3+3])
        if _dot(face_n,n0)<0.999999: raise ValueError("triangle winding/normal mismatch")
        du1=t[1][0]-t[0][0]; dv1=t[1][1]-t[0][1]; du2=t[2][0]-t[0][0]; dv2=t[2][1]-t[0][1]
        det=du1*dv2-du2*dv1
        if abs(det)<=1e-12: raise ValueError("degenerate UV triangle")
        inv=1.0/det
        dpu=_norm(_mul(_sub(_mul(e1,dv2),_mul(e2,dv1)),inv))
        dpv=_norm(_mul(_sub(_mul(e2,du1),_mul(e1,du2)),inv))
        tangent=tuple(tan[ids[0]*4:ids[0]*4+3]); handed=float(tan[ids[0]*4+3])
        if _dot(_norm(tangent),dpu)<0.999999: raise ValueError("tangent does not follow +U")
        reconstructed=_norm(_mul(_cross(n0,tangent),handed))
        if _dot(reconstructed,dpv)<0.999999: raise ValueError("tangent handedness does not follow +V")

def verify(source_path: Path, gltf_path: Path, manifest_path: Path):
    src=gen.load_source(source_path)
    g=_load_json(gltf_path); m=_load_json(manifest_path)
    if type(g) is not dict or set(g)!=ROOT_KEYS: raise ValueError("glTF root keys must match closed ART-006B profile")
    if g["asset"]!={"version":"2.0","generator":gen.GENERATOR_ID}: raise ValueError("glTF asset metadata changed")
    if type(g["scene"]) is not int or g["scene"]!=0: raise ValueError("scene must be integer 0")
    if g["scenes"]!=[{"name":"National_Mall_Core_Panel_Module","nodes":[0,1,2,3,4,5,6]}]: raise ValueError("scene ownership changed")
    if type(g["buffers"]) is not list or len(g["buffers"])!=1 or set(g["buffers"][0])!={"byteLength","uri"}: raise ValueError("buffer profile invalid")
    uri=g["buffers"][0]["uri"]; prefix="data:application/octet-stream;base64,"
    if type(uri) is not str or not uri.startswith(prefix): raise ValueError("buffer must be embedded application/octet-stream data URI")
    try: blob=base64.b64decode(uri[len(prefix):],validate=True)
    except Exception as e: raise ValueError("invalid base64 buffer") from e
    if type(g["buffers"][0]["byteLength"]) is not int or g["buffers"][0]["byteLength"]!=len(blob): raise ValueError("buffer byteLength mismatch")
    if len(blob)!=1224: raise ValueError("canonical fixture buffer length changed")
    if g["bufferViews"]!=CANON_BUFFER_VIEWS or g["accessors"]!=CANON_ACCESSORS: raise ValueError("bufferView/accessor contract changed")
    if g["materials"]!=CANON_MATERIALS: raise ValueError("material blockout contract changed")
    if g["meshes"]!=CANON_MESHES: raise ValueError("mesh/material ownership changed")
    want_nodes=_expected_nodes(src)
    if type(g["nodes"]) is not list or len(g["nodes"])!=len(want_nodes): raise ValueError("node count invalid")
    for got,want in zip(g["nodes"],want_nodes):
        if set(got)!={"name","mesh","translation","scale"}: raise ValueError("node keys invalid")
        if got["name"]!=want["name"] or type(got["mesh"]) is not int or got["mesh"]!=want["mesh"]: raise ValueError("node identity/mesh ownership changed")
        if not _eq_list(_finite_list(got["translation"],3,got["name"]+".translation"),want["translation"]): raise ValueError("node translation changed")
        if not _eq_list(_finite_list(got["scale"],3,got["name"]+".scale"),want["scale"]): raise ValueError("node scale changed")
    pos,norm,tan,uv,idx=_decode_geometry(blob)
    for got,want,name in ((pos,CANON_POS,"positions"),(norm,CANON_NORM,"normals"),(tan,CANON_TAN,"tangents"),(uv,CANON_UV,"uvs")):
        if tuple(got)!=want: raise ValueError(f"canonical {name} changed")
    if tuple(idx)!=CANON_IDX: raise ValueError("canonical indices/topology changed")
    _verify_basis(pos,norm,tan,uv,idx)
    if type(m) is not dict or set(m)!=MANIFEST_KEYS: raise ValueError("manifest root keys invalid")
    if type(m["schema_version"]) is not int or m["schema_version"]!=1: raise ValueError("manifest schema_version must be integer 1")
    if m["status"]!=gen.STATUS or m["asset_id"]!=src["asset_id"]: raise ValueError("manifest source-only identity changed")
    if m["ledger_ref"]!=src["ledger_ref"] or m["ledger_blob_sha1"]!=src["ledger_blob_sha1"]: raise ValueError("manifest ledger provenance changed")
    if m["gltf_file"]!=gltf_path.name: raise ValueError("manifest glTF filename mismatch")
    sb=source_path.read_bytes(); gb=gltf_path.read_bytes()
    if m["source_sha256"]!=hashlib.sha256(sb).hexdigest(): raise ValueError("source hash mismatch")
    if m["gltf_sha256"]!=hashlib.sha256(gb).hexdigest() or m["gltf_bytes"]!=len(gb): raise ValueError("glTF hash/size mismatch")
    for k,v in {"unique_meshes":3,"mesh_instances":7,"materials":3,"unit_box_vertices":24,"unit_box_indices":36}.items():
        if type(m[k]) is not int or m[k]!=v: raise ValueError(f"manifest {k} invalid")
    if type(m["dimensions_m"]) is not dict or set(m["dimensions_m"])!=DIM_KEYS: raise ValueError("manifest dimensions keys invalid")
    ms={x["label"]:x for x in src["source_measurements"]}; lawn_z=float(ms["typical_lawn_panel_length"]["converted_m"]); lawn_x=float(ms["typical_lawn_panel_width"]["converted_m"]); path=float(ms["perimeter_gravel_path_width"]["converted_m"]); grove=float(ms["tree_grove_width"]["converted_m"]); outer_z=round(lawn_z+2*path,6); outer_x=round(lawn_x+2*path,6)
    dims={"lawn_longitudinal":lawn_z,"lawn_cross":lawn_x,"path_width":path,"grove_band_width":grove,"outer_path_longitudinal":outer_z,"outer_path_cross":outer_x,"total_module_cross":round(outer_x+2*grove,6)}
    for k,w in dims.items():
        got=m["dimensions_m"][k]
        if isinstance(got,bool) or not isinstance(got,(int,float)) or not math.isclose(float(got),float(w),rel_tol=0,abs_tol=1e-9): raise ValueError(f"manifest dimension {k} invalid")
    return {"nodes":7,"materials":3,"vertices":24,"indices":36}

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--source",type=Path,required=True); ap.add_argument("--gltf",type=Path,required=True); ap.add_argument("--manifest",type=Path,required=True)
    a=ap.parse_args(); r=verify(a.source,a.gltf,a.manifest); print(f"PASS: source-only National Mall panel blockout {r}")

if __name__=="__main__": main()
