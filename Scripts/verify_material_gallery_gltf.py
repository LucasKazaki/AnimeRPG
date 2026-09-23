from __future__ import annotations
import argparse, base64, hashlib, json, math, struct
from pathlib import Path

VERSION="astral-material-gallery-gltf-3"
SOURCE_STATUS="proposed_art_reference_not_runtime"
RUNTIME_STATUS="source_validated_not_imported"

def req(cond,msg):
    if not cond: raise ValueError(msg)

def canonical_text_bytes(path):
    return Path(path).read_bytes().replace(b"\r\n",b"\n").replace(b"\r",b"\n")

def load_source(path):
    raw=canonical_text_bytes(path)
    source=json.loads(raw)
    req(source.get("schema_version")==1,"source schema")
    req(source.get("status")==SOURCE_STATUS,"source status")
    req(source.get("units")=="metres","source units")
    req(source.get("axes")=={"forward":"+Z","right":"-X per glTF convention","up":"+Y"},"source axes")
    req(source.get("forbid_baked_lighting") is True,"baked-lighting rule")
    req(len(source.get("stations",[]))==4 and len(source.get("lights",[]))==2,"source counts")
    return source,raw

def data_uri(uri,prefix):
    req(isinstance(uri,str) and uri.startswith(prefix),"wrong data URI")
    return base64.b64decode(uri[len(prefix):],validate=True)

def read_accessor(g,buf,index):
    req(isinstance(index,int) and 0<=index<len(g["accessors"]),"accessor index")
    a=g["accessors"][index]
    req("sparse" not in a,"sparse accessor unsupported")
    vi=a["bufferView"]; req(isinstance(vi,int) and 0<=vi<len(g["bufferViews"]),"accessor bufferView")
    view=g["bufferViews"][vi]
    req(view.get("buffer",0)==0,"bufferView buffer")
    req("byteStride" not in view,"strided source fixture unsupported")
    vo=view.get("byteOffset",0); vl=view["byteLength"]; ao=a.get("byteOffset",0)
    req(all(isinstance(v,int) and v>=0 for v in (vo,vl,ao)),"accessor offsets")
    req(vo+vl<=len(buf),"bufferView bounds")
    ctype=a["componentType"]; atype=a["type"]; count=a["count"]
    req(isinstance(count,int) and count>0,"accessor count")
    comps={"SCALAR":1,"VEC2":2,"VEC3":3,"VEC4":4}
    req(atype in comps and ctype in (5126,5123),"accessor format")
    ncomp=comps[atype]; code="f" if ctype==5126 else "H"
    size=struct.calcsize("<"+code*ncomp); span=count*size
    req(ao+span<=vl,"accessor within bufferView")
    return [struct.unpack_from("<"+code*ncomp,buf,vo+ao+i*size) for i in range(count)]

def tri_normal(a,b,c):
    ab=tuple(b[i]-a[i] for i in range(3)); ac=tuple(c[i]-a[i] for i in range(3))
    return (ab[1]*ac[2]-ab[2]*ac[1],ab[2]*ac[0]-ab[0]*ac[2],ab[0]*ac[1]-ab[1]*ac[0])

def verify_mesh(g,buf,mesh_index,expected_material,expected_counts,expected_tangent_w=None):
    mesh=g["meshes"][mesh_index]
    req(len(mesh["primitives"])==1,"primitive count")
    prim=mesh["primitives"][0]
    req(prim.get("mode",4)==4,"triangle mode")
    req(prim["material"]==expected_material,"material binding")
    attrs=prim["attributes"]
    req(set(attrs)=={"POSITION","NORMAL","TANGENT","TEXCOORD_0"},"attributes")
    pos=read_accessor(g,buf,attrs["POSITION"])
    normal=read_accessor(g,buf,attrs["NORMAL"])
    tangent=read_accessor(g,buf,attrs["TANGENT"])
    uv=read_accessor(g,buf,attrs["TEXCOORD_0"])
    indices=read_accessor(g,buf,prim["indices"])
    req((len(pos),len(indices))==expected_counts,"geometry counts")
    req(len(normal)==len(pos) and len(tangent)==len(pos) and len(uv)==len(pos),"attribute counts")
    req(all(math.isfinite(c) for seq in (pos,normal,tangent,uv) for v in seq for c in v),"finite geometry")
    req(all(abs(sum(c*c for c in n)-1.0)<1e-4 for n in normal),"unit normals")
    req(all(abs(sum(c*c for c in t[:3])-1.0)<1e-4 and t[3] in (-1.0,1.0) for t in tangent),"unit tangents")
    if expected_tangent_w is not None:
        req(all(t[3]==expected_tangent_w for t in tangent),"tangent handedness")
    req(all(0.0<=u<=1.0 and 0.0<=v<=1.0 for u,v in uv),"uv range")
    req(all(0<=i[0]<len(pos) for i in indices),"index range")
    req(len(indices)%3==0,"triangle index count")
    for k in range(0,len(indices),3):
        ia,ib,ic=(indices[k][0],indices[k+1][0],indices[k+2][0])
        cr=tri_normal(pos[ia],pos[ib],pos[ic])
        req(sum(cr[j]*normal[ia][j] for j in range(3))>1e-8,"triangle winding")
    return pos

def quat_x(degrees):
    a=math.radians(degrees)/2.0
    return [math.sin(a),0.0,0.0,math.cos(a)]

def quat_xy(x_deg,y_deg):
    x=math.radians(x_deg)/2.0
    y=math.radians(y_deg)/2.0
    qx=(math.sin(x),0.0,0.0,math.cos(x))
    qy=(0.0,math.sin(y),0.0,math.cos(y))
    ax,ay,az,aw=qy; bx,by,bz,bw=qx
    return [
        aw*bx + ax*bw + ay*bz - az*by,
        aw*by - ax*bz + ay*bw + az*bx,
        aw*bz + ax*by - ay*bx + az*bw,
        aw*bw - ax*bx - ay*by - az*bz,
    ]

def quat_close(actual, expected, tol=1e-12):
    if not isinstance(actual,list) or len(actual)!=4 or not all(isinstance(v,(int,float)) and math.isfinite(v) for v in actual):
        return False
    direct=max(abs(a-b) for a,b in zip(actual,expected))
    negated=max(abs(a+b) for a,b in zip(actual,expected))
    return min(direct,negated)<=tol

def verify(path,source_path,manifest_path=None,expected_manifest_path=None):
    source,source_raw=load_source(source_path)
    path=Path(path); raw=path.read_bytes(); g=json.loads(raw)
    req(g["asset"]=={"generator":VERSION,"version":"2.0"},"asset header")
    req(g["extensionsUsed"]==["KHR_lights_punctual"],"extensions used")
    req(g["scene"]==0 and g["scenes"]==[{"name":"AstralNeutralMaterialGallery","nodes":list(range(12))}],"scene")
    req(len(g["nodes"])==12,"node count")
    contract=g["extras"]["astral_contract"]
    req(contract["status"]==RUNTIME_STATUS,"runtime status")
    req(contract["units"]==source["units"] and contract["up"]==source["axes"]["up"] and contract["forward"]==source["axes"]["forward"] and contract["right"]==source["axes"]["right"],"axis contract")
    req(contract["forbid_baked_lighting"] is True,"baked-lighting rule")
    labels=tuple(s["label"] for s in source["stations"])
    req(tuple(contract["station_order"])==labels,"station order")
    req(contract["capture_intent"]==source["capture_intent"],"capture intent")
    req(contract["source_sha256"]==hashlib.sha256(source_raw).hexdigest(),"source hash")
    req("images" not in g and "textures" not in g and "samplers" not in g,"gallery must not embed texture lighting")

    req(len(g["buffers"])==1,"buffer count")
    buf=data_uri(g["buffers"][0]["uri"],"data:application/octet-stream;base64,")
    req(len(buf)==g["buffers"][0]["byteLength"],"buffer length")

    material_specs=source["stations"]+[source["floor_material"]]
    req(len(g["materials"])==len(material_specs),"material count")
    for material,s in zip(g["materials"],material_specs):
        req(material["name"]==s["material_name"],"material name")
        expected={"baseColorFactor":s["base_color_factor_linear"],"metallicFactor":s["metallic"],"roughnessFactor":s["roughness"]}
        req(material["pbrMetallicRoughness"]==expected,"material values")

    geometry=source["geometry"]
    sphere_counts=((geometry["sphere_lat_segments"]+1)*(geometry["sphere_lon_segments"]+1), 6*geometry["sphere_lon_segments"]*(geometry["sphere_lat_segments"]-1))
    cube_counts=(24,36); floor_counts=(4,6)
    req(len(g["meshes"])==9,"mesh count")
    for i in range(4): verify_mesh(g,buf,i,i,sphere_counts)
    for i in range(4): verify_mesh(g,buf,4+i,i,cube_counts)
    floor_pos=verify_mesh(g,buf,8,4,floor_counts,expected_tangent_w=-1.0)
    req(all(abs(y)<1e-7 for x,y,z in floor_pos),"floor plane")

    xs=source["layout"]["x_positions"]; sy,sz=source["layout"]["sphere_yz"]; cy,cz=source["layout"]["cube_yz"]
    req(len(xs)==4,"layout count")
    for i,(label,x) in enumerate(zip(labels,xs)):
        req(g["nodes"][i]=={"name":f"{label}_Sphere","mesh":i,"translation":[x,sy,sz]},"sphere station node")
        req(g["nodes"][4+i]=={"name":f"{label}_Cube","mesh":4+i,"translation":[x,cy,cz]},"cube station node")
    req(g["nodes"][8]=={"name":"NeutralFloor","mesh":8},"floor node")

    camera=source["camera"]
    req(len(g["cameras"])==1,"camera count")
    cam=g["cameras"][0]
    req(cam["name"]=="NeutralReviewCamera" and cam["type"]=="perspective","camera")
    p=cam["perspective"]; aspect=camera["aspect_ratio"][0]/camera["aspect_ratio"][1]
    req(abs(p["aspectRatio"]-aspect)<1e-12 and abs(p["yfov"]-math.radians(camera["vertical_fov_degrees"]))<1e-12,"camera framing")
    req(p["znear"]==camera["znear"] and p["zfar"]==camera["zfar"],"camera clip")
    req(g["nodes"][9]["name"]=="ReviewCamera" and g["nodes"][9]["camera"]==0 and g["nodes"][9]["translation"]==camera["translation"],"camera node")
    req(quat_close(g["nodes"][9].get("rotation"),quat_x(camera["pitch_degrees"])),"camera rotation")

    lights=g["extensions"]["KHR_lights_punctual"]["lights"]
    expected_lights=[{"color":s["color_linear"],"intensity":s["intensity_lux"],"name":s["name"],"type":s["type"]} for s in source["lights"]]
    req(lights==expected_lights,"lights")
    for j,s in enumerate(source["lights"]):
        node=g["nodes"][10+j]
        req(node["name"]==s["name"] and node["extensions"]=={"KHR_lights_punctual":{"light":j}},"light node")
        rot=s["rotation_degrees"]
        req(quat_close(node.get("rotation"),quat_xy(rot["x"],rot["y"])),"light rotation")

    if manifest_path:
        manifest_raw=Path(manifest_path).read_bytes(); m=json.loads(manifest_raw)
        req(m["generator"]==VERSION and m["runtime_status"]==RUNTIME_STATUS,"manifest status")
        req(m["source_sha256"]==hashlib.sha256(source_raw).hexdigest(),"manifest source hash")
        req(m["counts"]=={"cameras":1,"cube_nodes":4,"directional_lights":2,"materials":5,"sphere_nodes":4,"stations":4},"manifest counts")
        req(len(m["files"])==1,"manifest files")
        rec=m["files"][0]
        req(rec=={"path":path.name,"bytes":len(raw),"sha256":hashlib.sha256(raw).hexdigest()},"manifest file record")
        if expected_manifest_path:
            req(manifest_raw.replace(b"\r\n",b"\n").replace(b"\r",b"\n")==canonical_text_bytes(expected_manifest_path),"expected manifest pin")
    elif expected_manifest_path:
        raise ValueError("expected manifest requires generated manifest")
    return {"materials":5,"stations":4,"sphere_vertices":sphere_counts[0],"sphere_indices":sphere_counts[1],"lights":2,"camera_fov_degrees":camera["vertical_fov_degrees"]}

def main():
    p=argparse.ArgumentParser()
    p.add_argument("gltf",type=Path)
    p.add_argument("--source",type=Path,required=True)
    p.add_argument("--manifest",type=Path)
    p.add_argument("--expected-manifest",type=Path)
    a=p.parse_args()
    try: print("PASS:",verify(a.gltf,a.source,a.manifest,a.expected_manifest))
    except (ValueError,KeyError,IndexError,TypeError,OSError,json.JSONDecodeError,struct.error,base64.binascii.Error) as e:
        p.exit(1,f"FAIL: {e}\n")
if __name__=="__main__":
    main()
