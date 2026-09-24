#!/usr/bin/env python3
"""Verify ART-006D Blender 5.2.2 glTF round-trip evidence without claiming Astral import."""
from __future__ import annotations
import argparse, base64, hashlib, json, math, struct
from pathlib import Path
from typing import Any

NAMES=("Lawn","Path_Longitudinal_PosX","Path_Longitudinal_NegX","Path_End_PosZ","Path_End_NegZ","Grove_Envelope_PosX","Grove_Envelope_NegX")
MATS={"Lawn_Blockout","Gravel_Path_Blockout","Tree_Grove_Envelope"}
ATTR={"POSITION","NORMAL","TANGENT","TEXCOORD_0"}
TOLERANCE=1e-4
EXPECTED_SOURCE_SHA="6c51463332199c65bcfbde04ee8e5883e03a94aba710980eebfaa6945f2759b7"
EXPECTED_MANIFEST={"schema_version":1,"asset_id":"national-mall-core-panel-module-v1","status":"source_validated_not_imported","gltf_file":"mall_core_panel_blockout.gltf","unique_meshes":3,"mesh_instances":7,"materials":3,"unit_box_vertices":24,"unit_box_indices":36}
EXPORT={"export_format":"GLTF_EMBEDDED","export_texcoords":True,"export_normals":True,"export_tangents":True,"export_materials":"EXPORT","export_image_format":"NONE","export_cameras":False,"export_lights":False,"export_extras":True,"export_yup":True,"export_apply":False,"export_animations":False,"export_gpu_instances":False}
CF={5120:("b",1),5121:("B",1),5122:("h",2),5123:("H",2),5125:("I",4),5126:("f",4)}
NC={"SCALAR":1,"VEC2":2,"VEC3":3,"VEC4":4}
FORBIDDEN_EXTENSIONS={"EXT_mesh_gpu_instancing","KHR_lights_punctual"}

class VerificationError(ValueError): pass

def req(c:bool,m:str)->None:
    if not c: raise VerificationError(m)

def jint(v:Any)->bool:return type(v) is int

def num(v:Any)->bool:return type(v) in (int,float) and math.isfinite(float(v))

def load(p:Path)->dict[str,Any]:
    try:v=json.loads(p.read_text(encoding="utf-8"))
    except Exception as e:raise VerificationError(f"cannot read JSON {p}: {e}") from e
    req(type(v) is dict,f"JSON root must be object: {p}")
    return v

def sha(p:Path)->str:
    h=hashlib.sha256()
    try:
        with p.open("rb") as f:
            for b in iter(lambda:f.read(1<<20),b""):h.update(b)
    except OSError as e:raise VerificationError(f"cannot hash {p}: {e}") from e
    return h.hexdigest()

def resource(p:Path)->dict[str,Any]:return {"sha256":sha(p),"bytes":p.stat().st_size}

def buffers(g:dict[str,Any])->list[bytes]:
    out=[]; bs=g.get("buffers"); req(type(bs) is list and bs,"glTF requires buffers"); pre="data:application/octet-stream;base64,"
    for i,b in enumerate(bs):
        req(type(b) is dict and type(b.get("uri")) is str and b["uri"].startswith(pre),f"buffer {i} must be embedded")
        try:x=base64.b64decode(b["uri"][len(pre):],validate=True)
        except Exception as e:raise VerificationError(f"buffer {i} base64 invalid: {e}") from e
        req(jint(b.get("byteLength")) and b["byteLength"]==len(x),f"buffer {i} length mismatch");out.append(x)
    return out

def accessor(g:dict[str,Any],bs:list[bytes],ai:int)->list[tuple[float|int,...]]:
    ac=g.get("accessors");bv=g.get("bufferViews");req(type(ac) is list and type(bv) is list and jint(ai) and 0<=ai<len(ac),"accessor index invalid")
    a=ac[ai];req(type(a) is dict and "sparse" not in a,f"accessor {ai} unsupported");vi=a.get("bufferView");ct=a.get("componentType");n=a.get("count");ty=a.get("type")
    req(jint(vi) and 0<=vi<len(bv) and jint(ct) and ct in CF and jint(n) and n>=0 and ty in NC,f"accessor {ai} contract invalid")
    v=bv[vi];req(type(v) is dict,"bufferView invalid");bi=v.get("buffer");vo=v.get("byteOffset",0);vl=v.get("byteLength");ao=a.get("byteOffset",0)
    req(jint(bi) and 0<=bi<len(bs) and jint(vo) and vo>=0 and jint(vl) and vl>=0 and jint(ao) and ao>=0,f"accessor {ai} offsets invalid")
    code,cs=CF[ct];k=NC[ty];packed=cs*k;stride=v.get("byteStride",packed);req(jint(stride) and stride>=packed and stride%cs==0,f"accessor {ai} stride invalid")
    start=vo+ao;end=start+(stride*(n-1)+packed if n else 0);req(end<=vo+vl and end<=len(bs[bi]),f"accessor {ai} escapes buffer");fmt="<"+code*k;out=[]
    for j in range(n):
        qv=struct.unpack_from(fmt,bs[bi],start+j*stride);req(ct!=5126 or all(math.isfinite(float(x)) for x in qv),f"accessor {ai} nonfinite");out.append(qv)
    return out

def acontract(g,ai,ct,ty,label):
    ac=g.get("accessors");req(type(ac) is list and jint(ai) and 0<=ai<len(ac),f"{label} accessor invalid");a=ac[ai];req(type(a) is dict and a.get("componentType")==ct and a.get("type")==ty,f"{label} accessor format invalid")

def ident():return [[1.,0,0,0],[0,1.,0,0],[0,0,1.,0],[0,0,0,1.]]

def mul(a,b):return [[sum(a[r][k]*b[k][c] for k in range(4)) for c in range(4)] for r in range(4)]

def local(n):
    if "matrix" in n:
        m=n["matrix"];req(type(m) is list and len(m)==16 and all(num(x) for x in m),"node matrix invalid");return [[float(m[c*4+r]) for c in range(4)] for r in range(4)]
    t=n.get("translation",[0,0,0]);s=n.get("scale",[1,1,1]);q=n.get("rotation",[0,0,0,1]);req(all(type(x) is list for x in (t,s,q)) and len(t)==3 and len(s)==3 and len(q)==4 and all(num(x) for x in t+s+q),"node TRS invalid")
    x,y,z,w=map(float,q);l=math.sqrt(x*x+y*y+z*z+w*w);req(l>0,"zero quaternion");x,y,z,w=[v/l for v in (x,y,z,w)]
    R=[[1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w),0],[2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w),0],[2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y),0],[0,0,0,1]]
    S=ident();T=ident();S[0][0],S[1][1],S[2][2]=map(float,s);T[0][3],T[1][3],T[2][3]=map(float,t)
    return mul(T,mul(R,S))

def det3(m):
    return m[0][0]*(m[1][1]*m[2][2]-m[1][2]*m[2][1])-m[0][1]*(m[1][0]*m[2][2]-m[1][2]*m[2][0])+m[0][2]*(m[1][0]*m[2][1]-m[1][1]*m[2][0])

def parity(m):
    d=det3(m);req(math.isfinite(d) and abs(d)>1e-12,"mesh world transform singular");return 1 if d>0 else -1

def world_signature(m):
    return tuple(float(m[r][c]) for r in range(3) for c in range(4))

def pt(m,p):
    v=[float(p[0]),float(p[1]),float(p[2]),1.]
    return tuple(sum(m[r][k]*v[k] for k in range(4)) for r in range(3))

def materials(g):
    ms=g.get("materials");req(type(ms) is list,"materials missing");out={}
    for m in ms:
        req(type(m) is dict and type(m.get("name")) is str,"material invalid");name=m["name"];req(name not in out,"duplicate material name");p=m.get("pbrMetallicRoughness",{});req(type(p) is dict,"material pbr invalid")
        base=p.get("baseColorFactor",[1,1,1,1]);metal=p.get("metallicFactor",1.);rough=p.get("roughnessFactor",1.);em=m.get("emissiveFactor",[0,0,0]);alpha=m.get("alphaMode","OPAQUE");cut=m.get("alphaCutoff",.5);ds=m.get("doubleSided",False)
        req(type(base) is list and len(base)==4 and all(num(x) for x in base),"material baseColorFactor invalid");req(num(metal) and num(rough),"material metallic/roughness invalid");req(type(em) is list and len(em)==3 and all(num(x) for x in em),"material emissiveFactor invalid");req(alpha in ("OPAQUE","MASK","BLEND") and num(cut),"material alpha invalid");req(type(ds) is bool,"material doubleSided invalid");req("extensions" not in m,"material extensions unsupported in ART-006D")
        out[name]=(tuple(map(float,base)),float(metal),float(rough),tuple(map(float,em)),alpha,float(cut) if alpha=="MASK" else None,ds)
    return out

def q(v):return tuple(int(round(float(x)/TOLERANCE)) for x in v)

def corner(pos,nor,tan,uv,i):return (q(pos[i][:3]),q(nor[i][:3]),q(tan[i][:4]),q(uv[i][:2]))

def tri(c):
    req(len(c)==3,"triangle corner count invalid")
    return min(tuple(c),tuple(c[1:]+c[:1]),tuple(c[2:]+c[:2]))

def semantics(g):
    used=g.get("extensionsUsed",[]);required=g.get("extensionsRequired",[])
    req(type(used) is list and all(type(x) is str for x in used) and type(required) is list and all(type(x) is str for x in required),"glTF extensions declarations invalid")
    req("EXT_mesh_gpu_instancing" not in used and "EXT_mesh_gpu_instancing" not in required,"GPU instancing unsupported in ART-006D")
    req("KHR_lights_punctual" not in used and "KHR_lights_punctual" not in required,"lights unsupported in ART-006D")
    root_ext=g.get("extensions",{})
    req(type(root_ext) is dict,"glTF root extensions invalid")
    req("EXT_mesh_gpu_instancing" not in root_ext,"GPU instancing unsupported in ART-006D")
    req("KHR_lights_punctual" not in root_ext,"lights unsupported in ART-006D")
    ns=g.get("nodes");meshes=g.get("meshes");ms=g.get("materials",[]);sc=g.get("scenes");si=g.get("scene",0)
    req(type(ns) is list and type(meshes) is list and type(sc) is list and jint(si) and 0<=si<len(sc),"scene graph invalid")
    for n in ns:
        req(type(n) is dict,"node invalid")
        ext=n.get("extensions",{})
        req(type(ext) is dict,"node extensions invalid")
        req("EXT_mesh_gpu_instancing" not in ext,"GPU instancing unsupported in ART-006D")
        req("KHR_lights_punctual" not in ext,"lights unsupported in ART-006D")
        req("camera" not in n,"cameras unsupported in ART-006D")
        req("weights" not in n,"morph targets unsupported in ART-006D")
    for mesh in meshes:
        req(type(mesh) is dict,"mesh invalid")
        req("weights" not in mesh,"morph targets unsupported in ART-006D")
        primitives=mesh.get("primitives")
        req(type(primitives) is list and primitives,"mesh primitives missing")
        for primitive in primitives:
            req(type(primitive) is dict,"primitive invalid")
            req("targets" not in primitive,"morph targets unsupported in ART-006D")
    scene=sc[si];req(type(scene) is dict,"scene invalid");roots=scene.get("nodes");req(type(roots) is list,"scene roots invalid");bs=buffers(g);seen=set();out={}
    def walk(i,parent):
        req(jint(i) and 0<=i<len(ns) and i not in seen,"node graph invalid/cyclic");seen.add(i);n=ns[i];world=mul(parent,local(n));name=n.get("name");mi=n.get("mesh")
        if mi is not None:
            req(type(name) is str and name in NAMES,f"unexpected mesh instance {name!r}");req(name not in out,f"duplicate mesh instance {name}");req(jint(mi) and 0<=mi<len(meshes),f"{name} mesh invalid");mesh=meshes[mi];ps=mesh.get("primitives");req(type(ps) is list and ps,f"{name} primitives missing");mins=[math.inf]*3;maxs=[-math.inf]*3;bound=set();top=[]
            for p in ps:
                req(type(p) is dict and p.get("mode",4)==4 and jint(p.get("indices")),f"{name} requires indexed TRIANGLES");at=p.get("attributes");req(type(at) is dict and ATTR<=set(at),f"{name} required attributes missing");req(set(at)==ATTR,f"{name} unexpected rendering attributes")
                for key,ty in (("POSITION","VEC3"),("NORMAL","VEC3"),("TANGENT","VEC4"),("TEXCOORD_0","VEC2")):acontract(g,at[key],5126,ty,f"{name} {key}")
                iai=p["indices"];ia=g["accessors"][iai];req(type(ia) is dict and ia.get("componentType") in (5121,5123,5125) and ia.get("type")=="SCALAR",f"{name} index accessor format invalid")
                pos=accessor(g,bs,at["POSITION"]);nor=accessor(g,bs,at["NORMAL"]);tan=accessor(g,bs,at["TANGENT"]);uv=accessor(g,bs,at["TEXCOORD_0"]);req(len(pos)>0 and len(nor)==len(pos)==len(tan)==len(uv),f"{name} attribute counts invalid")
                idx=[int(x[0]) for x in accessor(g,bs,iai)];req(idx and len(idx)%3==0 and min(idx)>=0 and max(idx)<len(pos),f"{name} indices invalid")
                mat=p.get("material");req(jint(mat) and 0<=mat<len(ms) and type(ms[mat]) is dict and type(ms[mat].get("name")) is str,f"{name} material invalid");mn=ms[mat]["name"];bound.add(mn)
                for ii in idx:
                    wv=pt(world,pos[ii])
                    for a in range(3):mins[a]=min(mins[a],wv[a]);maxs[a]=max(maxs[a],wv[a])
                for j in range(0,len(idx),3):top.append((mn,tri([corner(pos,nor,tan,uv,idx[j+k]) for k in range(3)])))
            out[name]={"center":tuple((mins[a]+maxs[a])/2 for a in range(3)),"dimensions":tuple(maxs[a]-mins[a] for a in range(3)),"materials":bound,"topology":tuple(sorted(top)),"parity":parity(world),"world":world_signature(world)}
        elif name in NAMES:raise VerificationError(f"{name} expected mesh instance missing mesh")
        ch=n.get("children",[]);req(type(ch) is list,"children invalid")
        for c in ch:walk(c,world)
    for r in roots:walk(r,ident())
    req(set(out)==set(NAMES),f"expected seven named mesh instances; got {sorted(out)}")
    return out

def strict_resource(rec,p,label):
    req(type(rec) is dict and set(rec)=={"path","sha256","bytes"},f"receipt {label} fields invalid");a=resource(p);req(type(rec["path"]) is str and Path(rec["path"]).name==p.name,f"receipt {label} path invalid");req(type(rec["sha256"]) is str and rec["sha256"]==a["sha256"],f"receipt {label} hash mismatch");req(jint(rec["bytes"]) and rec["bytes"]==a["bytes"],f"receipt {label} bytes mismatch")

def close(a,b):return len(a)==len(b) and all(abs(float(x)-float(y))<=TOLERANCE for x,y in zip(a,b))

def matclose(a,b):
    return a[4]==b[4] and a[6] is b[6] and ((a[5] is None and b[5] is None) or (a[5] is not None and b[5] is not None and abs(a[5]-b[5])<=TOLERANCE)) and close(a[0],b[0]) and abs(a[1]-b[1])<=TOLERANCE and abs(a[2]-b[2])<=TOLERANCE and close(a[3],b[3])

def exact_names(v,expected):req(type(v) is list and all(type(x) is str for x in v) and len(v)==len(expected) and set(v)==set(expected),"import inventory invalid")

def exact_export(v):
    req(type(v) is dict and set(v)==set(EXPORT),"export settings drift")
    for k,e in EXPORT.items():req(type(v[k]) is type(e) and v[k]==e,"export settings drift")

def empty_optional_arrays(g:dict[str,Any])->None:
    for key in ("animations","images","textures","cameras"):
        if key in g:
            req(type(g[key]) is list and not g[key],"round-trip glTF scope invalid")

def verify(source:Path,manifest:Path,roundtrip:Path,blend:Path,receipt_path:Path)->dict[str,Any]:
    src=load(source);man=load(manifest);out=load(roundtrip);rec=load(receipt_path)
    for k,v in EXPECTED_MANIFEST.items():req(type(man.get(k)) is type(v) and man.get(k)==v,f"ART-006B manifest identity drift: {k}")
    req(type(man.get("gltf_sha256")) is str and man["gltf_sha256"]==EXPECTED_SOURCE_SHA and sha(source)==EXPECTED_SOURCE_SHA,"source no longer matches pinned ART-006B input");req(jint(man.get("gltf_bytes")) and man["gltf_bytes"]==source.stat().st_size,"ART-006B manifest byte count drift")
    root={"schema_version","task_id","loop_id","status","run","blender","input","output","imported_scene","export_settings"};req(set(rec)==root,"receipt root fields invalid");req(jint(rec.get("schema_version")) and rec["schema_version"]==1 and rec.get("task_id")=="ART-006D" and rec.get("loop_id")=="astral-art-hourly-20260922" and rec.get("status")=="dcc_roundtrip_executed_not_astral_imported","receipt identity/status invalid")
    run=rec.get("run");req(type(run) is dict and set(run)=={"kind","background_mode","script_completed","working_directory"} and run["kind"]=="native_blender_background" and run["background_mode"] is True and run["script_completed"] is True and type(run["working_directory"]) is str and run["working_directory"],"receipt run invalid")
    b=rec.get("blender");req(type(b) is dict and set(b)=={"version","version_tuple","executable"} and b["version"]=="5.2.2" and type(b["version_tuple"]) is list and b["version_tuple"]==[5,2,2] and all(jint(x) for x in b["version_tuple"]) and type(b["executable"]) is str and b["executable"],"Blender evidence invalid")
    strict_resource(rec.get("input"),source,"input");o=rec.get("output");req(type(o) is dict and set(o)=={"gltf","blend"},"receipt output invalid");strict_resource(o["gltf"],roundtrip,"output.gltf");strict_resource(o["blend"],blend,"output.blend")
    im=rec.get("imported_scene");req(type(im) is dict and set(im)=={"mesh_object_count","object_names","material_names"} and jint(im.get("mesh_object_count")) and im["mesh_object_count"]==7,"import inventory invalid");exact_names(im.get("object_names"),NAMES);exact_names(im.get("material_names"),MATS);exact_export(rec.get("export_settings"))
    req(type(out.get("asset")) is dict and out["asset"].get("version")=="2.0","round-trip glTF scope invalid")
    empty_optional_arrays(out)
    sm=materials(src);om=materials(out);req(set(sm)==MATS and set(om)==MATS,"material inventory drift")
    for n in MATS:req(matclose(sm[n],om[n]),f"{n} material property drift")
    ss=semantics(src);os=semantics(out)
    for n in NAMES:
        req(ss[n]["parity"]==os[n]["parity"],f"{n} transform parity drift");req(close(ss[n]["world"],os[n]["world"]),f"{n} world transform drift");req(close(ss[n]["center"],os[n]["center"]) and close(ss[n]["dimensions"],os[n]["dimensions"]) and ss[n]["materials"]==os[n]["materials"],f"{n} transform/material binding drift");req(ss[n]["topology"]==os[n]["topology"],f"{n} topology/attribute drift")
    return {"nodes":7,"source_sha256":sha(source),"roundtrip_sha256":sha(roundtrip),"blend_sha256":sha(blend),"blender":"5.2.2","tolerance_m":TOLERANCE,"status":"dcc_roundtrip_verified_not_astral_imported"}

def main():
    p=argparse.ArgumentParser();p.add_argument("--source",required=True,type=Path);p.add_argument("--manifest",required=True,type=Path);p.add_argument("--roundtrip",required=True,type=Path);p.add_argument("--blend",required=True,type=Path);p.add_argument("--receipt",required=True,type=Path);a=p.parse_args()
    try:r=verify(a.source,a.manifest,a.roundtrip,a.blend,a.receipt)
    except VerificationError as e:raise SystemExit(f"FAIL: {e}") from e
    print(f"PASS: Blender round-trip source verification {r}")

if __name__=="__main__":main()
