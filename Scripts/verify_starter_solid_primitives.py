#!/usr/bin/env python3
"""Independently verify ART-009 starter solid primitives v1."""
from __future__ import annotations

import argparse
import base64
import binascii
import hashlib
import json
import math
import struct
from pathlib import Path

STATUS = "source_validated_not_imported"
ASSET_ID = "astral-starter-solid-primitives-v1"
GENERATOR_ID = "Astral ART-009 Starter Solid Primitives 1"
ROOT_KEYS = {"asset","scene","scenes","nodes","buffers","bufferViews","accessors","materials","meshes","extras"}
EXPECTED = {
    "cube-1m": {"mesh_name":"Starter_Cube_1m","vertices":24,"indices":36,"aabb":([-0.5,-0.5,-0.5],[0.5,0.5,0.5])},
    "sphere-1m": {"mesh_name":"Starter_Sphere_1m","vertices":151,"indices":672,"aabb":([-0.5,-0.5,-0.5],[0.5,0.5,0.5])},
    "cylinder-1x2m": {"mesh_name":"Starter_Cylinder_1x2m","vertices":70,"indices":192,"aabb":([-0.5,-1.0,-0.5],[0.5,1.0,0.5])},
    "plane-1m": {"mesh_name":"Starter_Plane_1m","vertices":4,"indices":6,"aabb":([-0.5,0.0,-0.5],[0.5,0.0,0.5])},
}
ORDER = ["cube-1m","sphere-1m","cylinder-1x2m","plane-1m"]
EXPECTED_UV_POLICIES = {
    "cube-1m":"each_face_maps_0_to_1",
    "sphere-1m":"longitude_latitude_with_seam_and_segment_poles",
    "cylinder-1x2m":"side_wrap_once_caps_each_map_to_unit_disc",
    "plane-1m":"single_0_to_1_square",
}
EXPECTED_REFERENCES = [
    {"engine":"Unity","official_url":"https://docs.unity3d.com/Manual/PrimitiveObjects.html","note":"current manual exposes Cube, Sphere, Capsule, Cylinder, Plane and Quad as editor primitives/placeholders"},
    {"engine":"Unreal Engine 5.8","official_url":"https://dev.epicgames.com/documentation/unreal-engine/predefined-shapes-in-unreal-engine","note":"Modeling Mode documents Box, Sphere, Cylinder, Cone, Torus, Arrow, Rectangle, Disc and Stairs predefined shapes"},
]
EXPECTED_COVERAGE = "This v1 packet deliberately implements only cube, sphere, cylinder and plane. It is not parity with Unity or Unreal starter/modeling libraries."
EXPECTED_TRANSLATIONS = [[-3.0,0.5,0.0],[-1.0,0.5,0.0],[1.0,1.0,0.0],[3.0,0.0,0.0]]
MANIFEST_KEYS = {
    "schema_version","asset_id","status","generator_id","source_file","source_sha256",
    "gltf_file","gltf_sha256","gltf_bytes","mesh_count","material_count",
    "vertex_count_total","index_count_total","primitive_ids","runtime_state",
}

def fail(message):
    raise ValueError(message)

def strict_int(v,label,minimum=None):
    if type(v) is not int:
        fail(f"{label} must be a JSON integer")
    if minimum is not None and v < minimum:
        fail(f"{label} is below minimum")
    return v

def number(v,label):
    if isinstance(v,bool) or not isinstance(v,(int,float)) or not math.isfinite(float(v)):
        fail(f"{label} must be finite JSON number")
    return float(v)

def vector(v,n,label):
    if type(v) is not list or len(v)!=n:
        fail(f"{label} must contain {n} numbers")
    return [number(x,f"{label}[{i}]") for i,x in enumerate(v)]

def read_json(path):
    data=json.loads(Path(path).read_text(encoding="utf-8"))
    if type(data) is not dict:
        fail(f"{path} root must be object")
    return data

def verify_source(source_path):
    s=read_json(source_path)
    required={"schema_version","asset_id","loop_id","status","purpose","coordinate_system","material","primitives","scene_layout","reference_scope"}
    if set(s)!=required: fail("source root keys invalid")
    if strict_int(s["schema_version"],"schema_version")!=1: fail("source schema version invalid")
    if s["asset_id"]!=ASSET_ID or s["loop_id"]!="astral-art-hourly-20260922" or s["status"]!=STATUS:
        fail("source identity/status changed")
    if s["purpose"]!="generic_starter_content_and_scale_uv_tangent_calibration": fail("source purpose changed")
    c=s["coordinate_system"]
    if type(c) is not dict or set(c)!={"handedness","up","forward","right","linear_unit"}: fail("coordinate contract invalid")
    if (c["handedness"],c["up"],c["forward"],c["right"],c["linear_unit"])!=("right","+Y","+Z","-X","metre"):
        fail("coordinate values changed")
    m=s["material"]
    if type(m) is not dict or set(m)!={"name","base_color_srgb","metallic","roughness","alpha_mode","double_sided"}: fail("material source contract invalid")
    if m["name"]!="StarterNeutral" or m["alpha_mode"]!="OPAQUE" or type(m["double_sided"]) is not bool or m["double_sided"]:
        fail("material source identity changed")
    base=vector(m["base_color_srgb"],4,"base_color")
    if base!=[0.62,0.64,0.68,1.0] or number(m["metallic"],"metallic")!=0.0 or number(m["roughness"],"roughness")!=0.72:
        fail("material source values changed")
    plist=s["primitives"]
    if type(plist) is not list or len(plist)!=4: fail("primitive source list invalid")
    if [p.get("id") if type(p) is dict else None for p in plist]!=ORDER: fail("primitive source order/ids changed")
    for p,pid in zip(plist,ORDER):
        if type(p) is not dict: fail("primitive source entry invalid")
        exp=EXPECTED[pid]
        shape = {"cube-1m":"cube","sphere-1m":"uv_sphere","cylinder-1x2m":"cylinder","plane-1m":"plane"}[pid]
        common={"id","mesh_name","shape","vertex_count","index_count","uv_policy"}
        if shape in ("cube","plane"):
            expected_keys=common|{"dimensions_m"}
        elif shape=="uv_sphere":
            expected_keys=common|{"diameter_m","segments","latitude_bands"}
        else:
            expected_keys=common|{"diameter_m","height_m","segments"}
        if set(p)!=expected_keys: fail(f"{pid} source keys changed")
        if p["mesh_name"]!=exp["mesh_name"] or p["shape"]!=shape or p["uv_policy"]!=EXPECTED_UV_POLICIES[pid]:
            fail(f"{pid} mesh/shape/UV identity changed")
        if strict_int(p["vertex_count"],f"{pid}.vertex_count")!=exp["vertices"]: fail(f"{pid} vertex budget changed")
        if strict_int(p["index_count"],f"{pid}.index_count")!=exp["indices"]: fail(f"{pid} index budget changed")
        if pid=="cube-1m" and vector(p["dimensions_m"],3,"cube dimensions")!=[1.0,1.0,1.0]: fail("cube dimensions changed")
        if pid=="plane-1m" and vector(p["dimensions_m"],2,"plane dimensions")!=[1.0,1.0]: fail("plane dimensions changed")
        if pid=="sphere-1m":
            if number(p["diameter_m"],"sphere diameter")!=1.0 or strict_int(p["segments"],"sphere segments")!=16 or strict_int(p["latitude_bands"],"sphere latitude bands")!=8:
                fail("sphere dimensions/tessellation changed")
        if pid=="cylinder-1x2m":
            if number(p["diameter_m"],"cylinder diameter")!=1.0 or number(p["height_m"],"cylinder height")!=2.0 or strict_int(p["segments"],"cylinder segments")!=16:
                fail("cylinder dimensions/tessellation changed")
    layout=s["scene_layout"]
    if type(layout) is not list or len(layout)!=4: fail("scene layout invalid")
    for i,(entry,pid,t) in enumerate(zip(layout,ORDER,EXPECTED_TRANSLATIONS)):
        if type(entry) is not dict or set(entry)!={"primitive_id","translation_m"}: fail("scene layout entry invalid")
        if entry["primitive_id"]!=pid or vector(entry["translation_m"],3,"translation")!=t: fail("scene translation changed")
    refs=s["reference_scope"]
    if type(refs) is not dict or set(refs)!={"observed_2026_09_24","coverage_statement"}: fail("reference scope invalid")
    if refs["observed_2026_09_24"]!=EXPECTED_REFERENCES: fail("reference inventory changed")
    if refs["coverage_statement"]!=EXPECTED_COVERAGE: fail("coverage statement changed")
    return s

def verify_manifest(manifest_path,source_path,gltf_path):
    m=read_json(manifest_path)
    if set(m)!=MANIFEST_KEYS: fail("manifest keys invalid")
    if strict_int(m["schema_version"],"manifest.schema_version")!=1: fail("manifest schema invalid")
    if m["asset_id"]!=ASSET_ID or m["status"]!=STATUS or m["runtime_state"]!=STATUS or m["generator_id"]!=GENERATOR_ID:
        fail("manifest identity/status invalid")
    if m["source_file"]!="source-contract.json" or m["gltf_file"]!="starter_solid_primitives_v1.gltf": fail("manifest filenames invalid")
    sb=Path(source_path).read_bytes(); gb=Path(gltf_path).read_bytes()
    if m["source_sha256"]!=hashlib.sha256(sb).hexdigest(): fail("manifest source hash mismatch")
    if m["gltf_sha256"]!=hashlib.sha256(gb).hexdigest(): fail("manifest glTF hash mismatch")
    if strict_int(m["gltf_bytes"],"manifest.gltf_bytes",1)!=len(gb): fail("manifest glTF size mismatch")
    if strict_int(m["mesh_count"],"manifest.mesh_count")!=4 or strict_int(m["material_count"],"manifest.material_count")!=1:
        fail("manifest object counts invalid")
    if strict_int(m["vertex_count_total"],"manifest.vertex_count_total")!=249: fail("manifest vertex total invalid")
    if strict_int(m["index_count_total"],"manifest.index_count_total")!=906: fail("manifest index total invalid")
    if m["primitive_ids"]!=ORDER: fail("manifest primitive ids invalid")
    return m

def verify_expected_manifest(manifest_path, expected_manifest_path):
    actual=Path(manifest_path).read_bytes()
    expected=Path(expected_manifest_path).read_bytes()
    if actual!=expected:
        fail("generated manifest does not match checked-in expected manifest")

def decode_embedded_buffer(g):
    buffers=g["buffers"]
    if type(buffers) is not list or len(buffers)!=1: fail("exactly one embedded buffer required")
    b=buffers[0]
    if type(b) is not dict or set(b)!={"byteLength","uri"}: fail("buffer contract invalid")
    length=strict_int(b["byteLength"],"buffer.byteLength",1)
    uri=b["uri"]
    prefix="data:application/octet-stream;base64,"
    if type(uri) is not str or not uri.startswith(prefix): fail("buffer must be embedded base64")
    try:
        data=base64.b64decode(uri[len(prefix):],validate=True)
    except (binascii.Error,ValueError) as e:
        fail(f"invalid buffer base64: {e}")
    if len(data)!=length: fail("buffer byteLength mismatch")
    return data

def accessor_values(g,data,index):
    accessors=g["accessors"]; views=g["bufferViews"]
    if type(accessors) is not list or index<0 or index>=len(accessors): fail("accessor index invalid")
    a=accessors[index]
    allowed={"bufferView","componentType","count","type","min","max"}
    if type(a) is not dict or not set(a).issubset(allowed) or not {"bufferView","componentType","count","type"}.issubset(a):
        fail("accessor contract invalid")
    vi=strict_int(a["bufferView"],"accessor.bufferView",0)
    if type(views) is not list or vi>=len(views): fail("bufferView index invalid")
    v=views[vi]
    if type(v) is not dict or set(v)!={"buffer","byteOffset","byteLength","target"}: fail("bufferView contract invalid")
    if strict_int(v["buffer"],"bufferView.buffer")!=0: fail("bufferView buffer must be 0")
    expected_target=34963 if index%5==4 else 34962
    if strict_int(v["target"],"bufferView.target")!=expected_target: fail("bufferView target changed")
    off=strict_int(v["byteOffset"],"bufferView.byteOffset",0); blen=strict_int(v["byteLength"],"bufferView.byteLength",1)
    if off%4!=0 or off+blen>len(data): fail("bufferView alignment/range invalid")
    ct=strict_int(a["componentType"],"accessor.componentType")
    count=strict_int(a["count"],"accessor.count",1); typ=a["type"]
    widths={"SCALAR":1,"VEC2":2,"VEC3":3,"VEC4":4}
    if typ not in widths: fail("unsupported accessor type")
    width=widths[typ]
    if ct==5126:
        fmt="f"; size=4
    elif ct==5123 and typ=="SCALAR":
        fmt="H"; size=2
    else:
        fail("unsupported accessor component type")
    used=count*width*size
    if used!=blen: fail("accessor must exactly consume its bufferView")
    raw=data[off:off+used]
    vals=list(struct.unpack("<"+fmt*(count*width),raw))
    rows=[vals[i:i+width] for i in range(0,len(vals),width)]
    if any(any((not math.isfinite(float(x))) for x in row) for row in rows): fail("non-finite accessor value")
    should_have_bounds = index%5 in (0,4)
    if should_have_bounds:
        if "min" not in a or "max" not in a: fail("bounded accessor missing min/max")
        amin=vector(a["min"],width,"accessor.min"); amax=vector(a["max"],width,"accessor.max")
        actual_min=[min(row[i] for row in rows) for i in range(width)]
        actual_max=[max(row[i] for row in rows) for i in range(width)]
        if any(abs(float(x)-float(y))>2e-5 for x,y in zip(amin,actual_min)) or any(abs(float(x)-float(y))>2e-5 for x,y in zip(amax,actual_max)):
            fail("accessor min/max does not match decoded data")
    elif "min" in a or "max" in a:
        fail("unexpected accessor min/max")
    return rows

def close(a,b,tol=1e-5): return abs(a-b)<=tol

def vec3_sub(a,b): return [a[i]-b[i] for i in range(3)]
def cross(a,b): return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
def dot(a,b): return sum(a[i]*b[i] for i in range(3))
def norm(a): return math.sqrt(dot(a,a))

def verify_mesh_geometry(pid,positions,normals,tangents,uvs,indices):
    exp=EXPECTED[pid]
    if len(positions)!=exp["vertices"] or len(normals)!=len(positions) or len(tangents)!=len(positions) or len(uvs)!=len(positions):
        fail(f"{pid} vertex/attribute count invalid")
    if len(indices)!=exp["indices"]: fail(f"{pid} index count invalid")
    flat_indices=[]
    for row in indices:
        if len(row)!=1: fail("index accessor must be scalar")
        value=row[0]
        if type(value) is not int or value<0 or value>=len(positions): fail(f"{pid} index out of range")
        flat_indices.append(value)
    if len(flat_indices)%3: fail(f"{pid} indices not triangles")
    expected_min,expected_max=exp["aabb"]
    actual_min=[min(p[i] for p in positions) for i in range(3)]
    actual_max=[max(p[i] for p in positions) for i in range(3)]
    for got,want in zip(actual_min+actual_max,expected_min+expected_max):
        if not close(float(got),float(want),2e-5): fail(f"{pid} AABB changed")
    for i,(p,n,t,uv) in enumerate(zip(positions,normals,tangents,uvs)):
        if len(p)!=3 or len(n)!=3 or len(t)!=4 or len(uv)!=2: fail(f"{pid} attribute width invalid")
        if not close(norm(n),1.0,2e-4): fail(f"{pid} normal {i} not unit")
        tv=t[:3]
        if not close(norm(tv),1.0,2e-4) or abs(dot(n,tv))>2e-4 or t[3] not in (-1.0,1.0):
            fail(f"{pid} tangent {i} invalid")
        if not (-1e-6<=uv[0]<=1.0+1e-6 and -1e-6<=uv[1]<=1.0+1e-6): fail(f"{pid} UV {i} out of range")
        if pid=="cube-1m":
            if not all(-0.50001<=c<=0.50001 for c in p): fail("cube position out of bounds")
            if sum(1 for c in p if close(abs(c),0.5,2e-5))<1: fail("cube vertex not on surface")
            if sum(1 for c in n if close(abs(c),1.0,2e-5))!=1: fail("cube normal not axis aligned")
        elif pid=="sphere-1m":
            if not close(norm(p),0.5,2e-4): fail("sphere vertex not on radius")
            if norm(p)>1e-8 and dot([2*x for x in p],n)<0.999: fail("sphere normal not radial")
        elif pid=="cylinder-1x2m":
            if not (-1.00001<=p[1]<=1.00001): fail("cylinder y out of range")
            radial=math.hypot(p[0],p[2])
            if abs(n[1])<0.5:
                if not close(radial,0.5,2e-4) or abs(n[1])>2e-4: fail("cylinder side vertex invalid")
            else:
                if not close(abs(p[1]),1.0,2e-5) or radial>0.50001 or abs(abs(n[1])-1.0)>2e-5: fail("cylinder cap vertex invalid")
        elif pid=="plane-1m":
            if abs(p[1])>2e-5 or n!=[0.0,1.0,0.0]: fail("plane surface contract invalid")
    for tri in range(0,len(flat_indices),3):
        ia,ib,ic=flat_indices[tri:tri+3]
        a,b,c=positions[ia],positions[ib],positions[ic]
        geometric=cross(vec3_sub(b,a),vec3_sub(c,a))
        area2=norm(geometric)
        if area2<=1e-8: fail(f"{pid} degenerate triangle")
        avg=[(normals[ia][j]+normals[ib][j]+normals[ic][j])/3.0 for j in range(3)]
        if dot(geometric,avg)<=1e-8: fail(f"{pid} triangle winding disagrees with normals")

def verify_gltf(gltf_path):
    g=read_json(gltf_path)
    if set(g)!=ROOT_KEYS: fail("glTF root keys invalid")
    if g["asset"]!={"version":"2.0","generator":GENERATOR_ID}: fail("glTF asset identity invalid")
    if strict_int(g["scene"],"scene")!=0: fail("default scene changed")
    if g["extras"]!={"astral_asset_id":ASSET_ID,"astral_status":STATUS,"linear_unit":"metre","coordinate_note":"right-handed; +Y up; +Z forward; -X right"}:
        fail("glTF extras contract changed")
    scenes=g["scenes"]
    if type(scenes) is not list or len(scenes)!=1 or type(scenes[0]) is not dict or set(scenes[0])!={"name","nodes"}:
        fail("scene contract changed")
    if scenes[0]["name"]!="Astral_Starter_Solid_Primitives_v1" or type(scenes[0]["nodes"]) is not list or len(scenes[0]["nodes"])!=4:
        fail("scene contract changed")
    for i,node_index in enumerate(scenes[0]["nodes"]):
        if strict_int(node_index,"scene node index")!=i: fail("scene membership changed")
    nodes=g["nodes"]
    if type(nodes) is not list or len(nodes)!=4: fail("node count invalid")
    for i,(node,pid,t) in enumerate(zip(nodes,ORDER,EXPECTED_TRANSLATIONS)):
        if type(node) is not dict or set(node)!={"name","mesh","translation"}: fail("node contract invalid")
        if node["name"]!=EXPECTED[pid]["mesh_name"] or strict_int(node["mesh"],"node.mesh")!=i or vector(node["translation"],3,"node.translation")!=t:
            fail("node identity/transform changed")
    mats=g["materials"]
    if type(mats) is not list or len(mats)!=1 or type(mats[0]) is not dict or set(mats[0])!={"name","pbrMetallicRoughness","alphaMode","doubleSided"}:
        fail("material contract changed")
    mat=mats[0]
    if mat["name"]!="StarterNeutral" or mat["alphaMode"]!="OPAQUE" or type(mat["doubleSided"]) is not bool or mat["doubleSided"]:
        fail("material contract changed")
    pbr=mat["pbrMetallicRoughness"]
    if type(pbr) is not dict or set(pbr)!={"baseColorFactor","metallicFactor","roughnessFactor"}:
        fail("material PBR contract changed")
    if vector(pbr["baseColorFactor"],4,"material base color")!=[0.62,0.64,0.68,1.0] or number(pbr["metallicFactor"],"material metallic")!=0.0 or number(pbr["roughnessFactor"],"material roughness")!=0.72:
        fail("material values changed")
    data=decode_embedded_buffer(g)
    if type(g["bufferViews"]) is not list or len(g["bufferViews"])!=20: fail("bufferView count invalid")
    if type(g["accessors"]) is not list or len(g["accessors"])!=20: fail("accessor count invalid")
    meshes=g["meshes"]
    if type(meshes) is not list or len(meshes)!=4: fail("mesh count invalid")
    totals=[0,0]
    for i,(mesh,pid) in enumerate(zip(meshes,ORDER)):
        if type(mesh) is not dict or set(mesh)!={"name","primitives"} or mesh["name"]!=EXPECTED[pid]["mesh_name"]: fail("mesh identity invalid")
        prims=mesh["primitives"]
        if type(prims) is not list or len(prims)!=1: fail("mesh must have one primitive")
        prim=prims[0]
        if type(prim) is not dict or set(prim)!={"attributes","indices","material","mode"}: fail("primitive contract invalid")
        if prim["attributes"]!={"POSITION":i*5,"NORMAL":i*5+1,"TANGENT":i*5+2,"TEXCOORD_0":i*5+3}: fail("attribute binding changed")
        if strict_int(prim["indices"],"primitive.indices")!=i*5+4 or strict_int(prim["material"],"primitive.material")!=0 or strict_int(prim["mode"],"primitive.mode")!=4:
            fail("primitive binding/mode changed")
        positions=accessor_values(g,data,i*5)
        normals=accessor_values(g,data,i*5+1)
        tangents=accessor_values(g,data,i*5+2)
        uvs=accessor_values(g,data,i*5+3)
        indices=accessor_values(g,data,i*5+4)
        verify_mesh_geometry(pid,positions,normals,tangents,uvs,indices)
        totals[0]+=len(positions); totals[1]+=len(indices)
    if totals!=[249,906]: fail("decoded totals changed")
    return {"meshes":4,"materials":1,"vertices":249,"indices":906}

def main(argv=None):
    ap=argparse.ArgumentParser()
    ap.add_argument("--source",type=Path,required=True)
    ap.add_argument("--gltf",type=Path,required=True)
    ap.add_argument("--manifest",type=Path,required=True)
    ap.add_argument("--expected-manifest",type=Path,required=True)
    args=ap.parse_args(argv)
    verify_source(args.source)
    verify_manifest(args.manifest,args.source,args.gltf)
    verify_expected_manifest(args.manifest,args.expected_manifest)
    result=verify_gltf(args.gltf)
    print(f"PASS: source-only starter solid primitives {result}")

if __name__=="__main__":
    main()
