#!/usr/bin/env python3
"""Verify an ART-006D Blender 5.2.2 glTF round trip without claiming Astral import."""
from __future__ import annotations
import argparse, base64, hashlib, json, math, struct
from pathlib import Path
from typing import Any

NAMES=("Lawn","Path_Longitudinal_PosX","Path_Longitudinal_NegX","Path_End_PosZ","Path_End_NegZ","Grove_Envelope_PosX","Grove_Envelope_NegX")
MATS={"Lawn_Blockout","Gravel_Path_Blockout","Tree_Grove_Envelope"}
ATTR={"POSITION","NORMAL","TANGENT","TEXCOORD_0"}
STATUS="source_validated_not_imported"
EXPECTED_SOURCE_SHA="6c51463332199c65bcfbde04ee8e5883e03a94aba710980eebfaa6945f2759b7"
EXPECTED_MANIFEST={
    "schema_version":1,
    "asset_id":"national-mall-core-panel-module-v1",
    "status":STATUS,
    "gltf_file":"mall_core_panel_blockout.gltf",
    "unique_meshes":3,
    "mesh_instances":7,
    "materials":3,
    "unit_box_vertices":24,
    "unit_box_indices":36,
}
EXPORT={"export_format":"GLTF_EMBEDDED","export_texcoords":True,"export_normals":True,"export_tangents":True,"export_materials":"EXPORT","export_image_format":"NONE","export_cameras":False,"export_lights":False,"export_extras":True,"export_yup":True,"export_apply":False,"export_animations":False,"export_gpu_instances":False}
class VerificationError(ValueError): pass

def req(c: bool,m: str)->None:
    if not c: raise VerificationError(m)
def jint(v:Any)->bool:return type(v) is int
def num(v:Any)->bool:return type(v) in (int,float) and math.isfinite(float(v))
def load(p:Path)->dict[str,Any]:
    try:v=json.loads(p.read_text(encoding="utf-8"))
    except Exception as e: raise VerificationError(f"cannot read JSON {p}: {e}")
    req(type(v) is dict,f"JSON root must be object: {p}");return v
def sha(p:Path)->str:
    h=hashlib.sha256()
    try:
        with p.open("rb") as f:
            for b in iter(lambda:f.read(1<<20),b""):h.update(b)
    except OSError as e: raise VerificationError(f"cannot hash {p}: {e}")
    return h.hexdigest()
def res(p:Path)->dict[str,Any]:return {"sha256":sha(p),"bytes":p.stat().st_size}

def buffers(g:dict[str,Any])->list[bytes]:
    out=[]
    bs=g.get("buffers");req(type(bs) is list and bs,"glTF requires buffers")
    pre="data:application/octet-stream;base64,"
    for i,b in enumerate(bs):
        req(type(b) is dict and type(b.get("uri")) is str and b["uri"].startswith(pre),f"buffer {i} must be embedded")
        try:x=base64.b64decode(b["uri"][len(pre):],validate=True)
        except Exception as e: raise VerificationError(f"buffer {i} base64 invalid: {e}")
        req(jint(b.get("byteLength")) and b["byteLength"]==len(x),f"buffer {i} length mismatch");out.append(x)
    return out
CF={5120:("b",1),5121:("B",1),5122:("h",2),5123:("H",2),5125:("I",4),5126:("f",4)}; NC={"SCALAR":1,"VEC2":2,"VEC3":3,"VEC4":4}
def accessor(g:dict[str,Any],bs:list[bytes],ai:int)->list[tuple[float|int,...]]:
    ac=g.get("accessors");bv=g.get("bufferViews");req(type(ac) is list and type(bv) is list and jint(ai) and 0<=ai<len(ac),"accessor index invalid")
    a=ac[ai];req(type(a) is dict and "sparse" not in a,f"accessor {ai} unsupported");vi=a.get("bufferView");ct=a.get("componentType");n=a.get("count");ty=a.get("type")
    req(jint(vi) and 0<=vi<len(bv) and jint(ct) and ct in CF and jint(n) and n>=0 and ty in NC,f"accessor {ai} contract invalid")
    v=bv[vi];req(type(v) is dict,"bufferView invalid");bi=v.get("buffer");vo=v.get("byteOffset",0);vl=v.get("byteLength");ao=a.get("byteOffset",0)
    req(jint(bi) and 0<=bi<len(bs) and jint(vo) and vo>=0 and jint(vl) and vl>=0 and jint(ao) and ao>=0,f"accessor {ai} offsets invalid")
    cfmt,cs=CF[ct];k=NC[ty];packed=cs*k;stride=v.get("byteStride",packed);req(jint(stride) and stride>=packed and stride%cs==0,f"accessor {ai} stride invalid")
    start=vo+ao;end=start+(stride*(n-1)+packed if n else 0);req(end<=vo+vl and end<=len(bs[bi]),f"accessor {ai} escapes buffer")
    fmt="<"+cfmt*k;ans=[]
    for j in range(n):
        q=struct.unpack_from(fmt,bs[bi],start+j*stride);req(ct!=5126 or all(math.isfinite(float(x)) for x in q),f"accessor {ai} nonfinite");ans.append(q)
    return ans

def accessor_contract(g:dict[str,Any], ai:int, component:int, kind:str, label:str)->None:
    ac=g.get("accessors");req(type(ac) is list and jint(ai) and 0<=ai<len(ac),f"{label} accessor invalid")
    a=ac[ai];req(type(a) is dict and a.get("componentType")==component and a.get("type")==kind,f"{label} accessor format invalid")

def ident():return [[1.,0,0,0],[0,1.,0,0],[0,0,1.,0],[0,0,0,1.]]
def mul(a,b):return [[sum(a[r][k]*b[k][c] for k in range(4)) for c in range(4)] for r in range(4)]
def local(n:dict[str,Any]):
    if "matrix" in n:
        m=n["matrix"];req(type(m) is list and len(m)==16 and all(num(x) for x in m),"node matrix invalid");return [[float(m[c*4+r]) for c in range(4)] for r in range(4)]
    t=n.get("translation",[0,0,0]);s=n.get("scale",[1,1,1]);q=n.get("rotation",[0,0,0,1]);req(all(type(x) is list for x in (t,s,q)) and len(t)==3 and len(s)==3 and len(q)==4 and all(num(x) for x in t+s+q),"node TRS invalid")
    x,y,z,w=map(float,q);l=math.sqrt(x*x+y*y+z*z+w*w);req(l>0,"zero quaternion");x,y,z,w=[v/l for v in (x,y,z,w)]
    R=[[1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w),0],[2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w),0],[2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y),0],[0,0,0,1]]
    S=ident();S[0][0]=float(s[0]);S[1][1]=float(s[1]);S[2][2]=float(s[2]);T=ident();T[0][3]=float(t[0]);T[1][3]=float(t[1]);T[2][3]=float(t[2]);return mul(T,mul(R,S))
def pt(m,p):
    x,y,z=map(float,p);return tuple(sum(m[r][k]*[x,y,z,1.][k] for k in range(4)) for r in range(3))

def material_semantics(g:dict[str,Any])->dict[str,dict[str,Any]]:
    mats=g.get("materials");req(type(mats) is list,"materials missing")
    out={}
    for m in mats:
        req(type(m) is dict and type(m.get("name")) is str,"material invalid")
        name=m["name"];req(name not in out,"duplicate material name")
        pbr=m.get("pbrMetallicRoughness",{});req(type(pbr) is dict,"material pbr invalid")
        base=pbr.get("baseColorFactor",[1,1,1,1]);req(type(base) is list and len(base)==4 and all(num(x) for x in base),"material baseColorFactor invalid")
        metallic=pbr.get("metallicFactor",1.0);rough=pbr.get("roughnessFactor",1.0);req(num(metallic) and num(rough),"material metallic/roughness invalid")
        emissive=m.get("emissiveFactor",[0,0,0]);req(type(emissive) is list and len(emissive)==3 and all(num(x) for x in emissive),"material emissiveFactor invalid")
        alpha=m.get("alphaMode","OPAQUE");req(alpha in ("OPAQUE","MASK","BLEND"),"material alphaMode invalid")
        cutoff=m.get("alphaCutoff",0.5);req(num(cutoff),"material alphaCutoff invalid")
        ds=m.get("doubleSided",False);req(type(ds) is bool,"material doubleSided invalid")
        req("extensions" not in m,"material extensions unsupported in ART-006D")
        out[name]={
            "baseColorFactor":tuple(map(float,base)),
            "metallicFactor":float(metallic),
            "roughnessFactor":float(rough),
            "emissiveFactor":tuple(map(float,emissive)),
            "alphaMode":alpha,
            "alphaCutoff":float(cutoff) if alpha=="MASK" else None,
            "doubleSided":ds,
        }
    return out

def semantics(g:dict[str,Any])->dict[str,dict[str,Any]]:
    nodes=g.get("nodes");meshes=g.get("meshes");mats=g.get("materials",[]);sc=g.get("scenes");si=g.get("scene",0);req(type(nodes) is list and type(meshes) is list and type(sc) is list and jint(si) and 0<=si<len(sc),"scene graph invalid")
    roots=sc[si].get("nodes");req(type(roots) is list,"scene roots invalid");bs=buffers(g);seen=set();out={}
    def walk(i:int,parent):
        req(jint(i) and 0<=i<len(nodes) and i not in seen,"node graph invalid/cyclic");seen.add(i);n=nodes[i];req(type(n) is dict,"node invalid");world=mul(parent,local(n));name=n.get("name")
        if name in NAMES:
            mi=n.get("mesh");req(jint(mi) and 0<=mi<len(meshes),f"{name} mesh invalid");pr=meshes[mi].get("primitives");req(type(pr) is list and pr,f"{name} primitives missing");mins=[math.inf]*3;maxs=[-math.inf]*3;bound=set()
            for p in pr:
                req(type(p) is dict and p.get("mode",4)==4 and jint(p.get("indices")),f"{name} requires indexed TRIANGLES");at=p.get("attributes");req(type(at) is dict and ATTR<=set(at),f"{name} required attributes missing")
                accessor_contract(g,at["POSITION"],5126,"VEC3",f"{name} POSITION");accessor_contract(g,at["NORMAL"],5126,"VEC3",f"{name} NORMAL");accessor_contract(g,at["TANGENT"],5126,"VEC4",f"{name} TANGENT");accessor_contract(g,at["TEXCOORD_0"],5126,"VEC2",f"{name} TEXCOORD_0")
                iai=p["indices"];ac=g["accessors"][iai];req(type(ac) is dict and ac.get("componentType") in (5121,5123,5125) and ac.get("type")=="SCALAR",f"{name} index accessor format invalid")
                pos=accessor(g,bs,at["POSITION"]);idx=accessor(g,bs,iai);flat=[int(v[0]) for v in idx];req(len(flat)%3==0 and flat and min(flat)>=0 and max(flat)<len(pos),f"{name} indices invalid")
                for ii in flat:
                    wv=pt(world,pos[ii][:3])
                    for a in range(3):mins[a]=min(mins[a],wv[a]);maxs[a]=max(maxs[a],wv[a])
                mat=p.get("material");req(jint(mat) and 0<=mat<len(mats) and type(mats[mat]) is dict and type(mats[mat].get("name")) is str,f"{name} material invalid");bound.add(mats[mat]["name"])
            out[name]={"center":tuple((mins[a]+maxs[a])/2 for a in range(3)),"dimensions":tuple(maxs[a]-mins[a] for a in range(3)),"materials":bound}
        ch=n.get("children",[]);req(type(ch) is list,"children invalid")
        for c in ch:walk(c,world)
    for r in roots:walk(r,ident())
    req(set(out)==set(NAMES),f"expected seven named mesh instances; got {sorted(out)}");return out

def strict_resource(rec:Any,p:Path,label:str)->None:
    req(type(rec) is dict and set(rec)=={"path","sha256","bytes"},f"receipt {label} fields invalid");a=res(p);req(type(rec["path"]) is str and Path(rec["path"]).name==p.name,f"receipt {label} path invalid");req(type(rec["sha256"]) is str and rec["sha256"]==a["sha256"],f"receipt {label} hash mismatch");req(jint(rec["bytes"]) and rec["bytes"]==a["bytes"],f"receipt {label} bytes mismatch")
def close(a,b,t):return all(abs(float(x)-float(y))<=t for x,y in zip(a,b))
def material_close(a:dict[str,Any],b:dict[str,Any],t:float)->bool:
    if a["alphaMode"]!=b["alphaMode"] or a["doubleSided"] is not b["doubleSided"]: return False
    if (a["alphaCutoff"] is None)!=(b["alphaCutoff"] is None): return False
    if a["alphaCutoff"] is not None and abs(a["alphaCutoff"]-b["alphaCutoff"])>t:return False
    return close(a["baseColorFactor"],b["baseColorFactor"],t) and abs(a["metallicFactor"]-b["metallicFactor"])<=t and abs(a["roughnessFactor"]-b["roughnessFactor"])<=t and close(a["emissiveFactor"],b["emissiveFactor"],t)

def verify(source:Path,manifest:Path,roundtrip:Path,blend:Path,receipt_path:Path,tol:float=1e-4)->dict[str,Any]:
    req(num(tol) and tol>0,"tolerance invalid");src=load(source);man=load(manifest);out=load(roundtrip);rec=load(receipt_path)
    for k,v in EXPECTED_MANIFEST.items():req(type(man.get(k)) is type(v) and man.get(k)==v,f"ART-006B manifest identity drift: {k}")
    req(type(man.get("gltf_sha256")) is str and man["gltf_sha256"]==EXPECTED_SOURCE_SHA and sha(source)==EXPECTED_SOURCE_SHA,"source no longer matches pinned ART-006B input")
    req(jint(man.get("gltf_bytes")) and man["gltf_bytes"]==source.stat().st_size,"ART-006B manifest byte count drift")
    root={"schema_version","task_id","loop_id","status","run","blender","input","output","imported_scene","export_settings"};req(set(rec)==root,"receipt root fields invalid");req(jint(rec.get("schema_version")) and rec["schema_version"]==1 and rec.get("task_id")=="ART-006D" and rec.get("loop_id")=="astral-art-hourly-20260922" and rec.get("status")=="dcc_roundtrip_executed_not_astral_imported","receipt identity/status invalid")
    run=rec.get("run");req(type(run) is dict and set(run)=={"kind","background_mode","script_completed","working_directory"} and run["kind"]=="native_blender_background" and run["background_mode"] is True and run["script_completed"] is True and type(run["working_directory"]) is str and run["working_directory"],"receipt run invalid")
    b=rec.get("blender");req(type(b) is dict and set(b)=={"version","version_tuple","executable"} and b["version"]=="5.2.2" and type(b["version_tuple"]) is list and b["version_tuple"]==[5,2,2] and all(jint(x) for x in b["version_tuple"]) and type(b["executable"]) is str and b["executable"],"Blender evidence invalid")
    strict_resource(rec.get("input"),source,"input");o=rec.get("output");req(type(o) is dict and set(o)=={"gltf","blend"},"receipt output invalid");strict_resource(o["gltf"],roundtrip,"output.gltf");strict_resource(o["blend"],blend,"output.blend")
    im=rec.get("imported_scene");req(type(im) is dict and set(im)=={"mesh_object_count","object_names","material_names"} and jint(im["mesh_object_count"]) and im["mesh_object_count"]==7 and sorted(im["object_names"])==sorted(NAMES) and set(im["material_names"])==MATS,"import inventory invalid");req(rec.get("export_settings")==EXPORT,"export settings drift")
    req(type(out.get("asset")) is dict and out["asset"].get("version")=="2.0" and not out.get("animations") and not out.get("images") and not out.get("textures"),"round-trip glTF scope invalid")
    smat=material_semantics(src);omat=material_semantics(out);req(set(smat)==MATS and set(omat)==MATS,"material inventory drift")
    for name in MATS:req(material_close(smat[name],omat[name],tol),f"{name} material property drift")
    ssem=semantics(src);osem=semantics(out)
    for n in NAMES:req(close(ssem[n]["center"],osem[n]["center"],tol) and close(ssem[n]["dimensions"],osem[n]["dimensions"],tol) and ssem[n]["materials"]==osem[n]["materials"],f"{n} transform/material binding drift")
    return {"nodes":7,"source_sha256":sha(source),"roundtrip_sha256":sha(roundtrip),"blend_sha256":sha(blend),"blender":"5.2.2","status":"dcc_roundtrip_verified_not_astral_imported"}
def main():
    p=argparse.ArgumentParser();p.add_argument("--source",required=True,type=Path);p.add_argument("--manifest",required=True,type=Path);p.add_argument("--roundtrip",required=True,type=Path);p.add_argument("--blend",required=True,type=Path);p.add_argument("--receipt",required=True,type=Path);p.add_argument("--tolerance",type=float,default=1e-4);a=p.parse_args()
    try:r=verify(a.source,a.manifest,a.roundtrip,a.blend,a.receipt,a.tolerance)
    except VerificationError as e:raise SystemExit(f"FAIL: {e}")
    print(f"PASS: Blender round-trip source verification {r}")
if __name__=="__main__":main()
