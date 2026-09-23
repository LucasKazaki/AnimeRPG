from __future__ import annotations
import argparse, base64, hashlib, json, math, struct
from pathlib import Path

VERSION="astral-material-gallery-gltf-3"
SOURCE_STATUS="proposed_art_reference_not_runtime"
RUNTIME_STATUS="source_validated_not_imported"
MANIFEST_INTENT="Source-only neutral material-response gallery for future Astral import/render and art review."
CAPTURE_INTENT="Neutral material-response comparison. Absolute exposure and tonemapping are runtime-owned and are not encoded by glTF."
CONTRACT_KEYS={"units","up","forward","right","status","capture_intent","station_order","forbid_baked_lighting","source_sha256"}
MANIFEST_KEYS={"schema_version","generator","runtime_status","source_sha256","intent","counts","files"}
TOP_LEVEL_KEYS={"accessors","asset","buffers","bufferViews","cameras","extensions","extensionsUsed","extras","materials","meshes","nodes","scene","scenes"}
NONFLOOR_TANGENT_ALIGNMENT_MIN=0.95
FLOOR_TANGENT_ALIGNMENT_MIN=0.9999
CANONICAL_FLOAT_TOL=1e-6

def req(cond,msg):
    if not cond: raise ValueError(msg)

def exact_index(value,count,msg):
    req(type(value) is int and 0<=value<count,msg)
    return value

def canonical_text_bytes(path):
    return Path(path).read_bytes().replace(b"\r\n",b"\n").replace(b"\r",b"\n")

def load_source(path):
    raw=canonical_text_bytes(path); source=json.loads(raw)
    req(source.get("schema_version")==1,"source schema")
    req(source.get("status")==SOURCE_STATUS,"source status")
    req(source.get("units")=="metres","source units")
    req(source.get("axes")=={"forward":"+Z","right":"-X per glTF convention","up":"+Y"},"source axes")
    req(source.get("forbid_baked_lighting") is True,"baked-lighting rule")
    req(source.get("capture_intent")==CAPTURE_INTENT,"source capture intent")
    req(len(source.get("stations",[]))==4 and len(source.get("lights",[]))==2,"source counts")
    return source,raw

def data_uri(uri,prefix):
    req(isinstance(uri,str) and uri.startswith(prefix),"wrong data URI")
    return base64.b64decode(uri[len(prefix):],validate=True)

def sub(a,b): return tuple(a[i]-b[i] for i in range(3))
def dot(a,b): return sum(a[i]*b[i] for i in range(3))
def cross(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def scale(v,s): return tuple(c*s for c in v)
def length(v): return math.sqrt(dot(v,v))
def normalize(v,msg):
    n=length(v); req(n>1e-12,msg); return tuple(c/n for c in v)

def tri_normal(a,b,c): return cross(sub(b,a),sub(c,a))

def sphere_geometry(radius, lat_segments, lon_segments):
    pos,norm,tan,uv,idx=[],[],[],[],[]
    for j in range(lat_segments+1):
        theta=math.pi*j/lat_segments; st,ct=math.sin(theta),math.cos(theta)
        for i in range(lon_segments+1):
            phi=2.0*math.pi*i/lon_segments; cp,sp=math.cos(phi),math.sin(phi)
            n=(st*cp,ct,st*sp)
            pos.append(tuple(radius*c for c in n)); norm.append(n); tan.append((-sp,0.0,cp,1.0)); uv.append((i/lon_segments,j/lat_segments))
    row=lon_segments+1
    for j in range(lat_segments):
        for i in range(lon_segments):
            a=j*row+i; b=a+row
            if j>0: idx += [a,a+1,b]
            if j<lat_segments-1: idx += [a+1,b+1,b]
    return pos,norm,tan,uv,idx

def quad_face(center,tangent,bitangent,half_u,half_v):
    normal=cross(tangent,bitangent)
    def p(su,sv):
        return tuple(center[k]+tangent[k]*half_u*su+bitangent[k]*half_v*sv for k in range(3))
    return [p(-1,-1),p(1,-1),p(1,1),p(-1,1)],[normal]*4,[(*tangent,1.0)]*4,[(0,0),(1,0),(1,1),(0,1)],[0,1,2,0,2,3]

def cube_geometry(half):
    faces=[
        quad_face((0,0,half),(1,0,0),(0,1,0),half,half),
        quad_face((0,0,-half),(-1,0,0),(0,1,0),half,half),
        quad_face((half,0,0),(0,0,-1),(0,1,0),half,half),
        quad_face((-half,0,0),(0,0,1),(0,1,0),half,half),
        quad_face((0,half,0),(1,0,0),(0,0,-1),half,half),
        quad_face((0,-half,0),(1,0,0),(0,0,1),half,half),
    ]
    pos=[]; norm=[]; tan=[]; uv=[]; idx=[]
    for p,n,t,u,i in faces:
        base=len(pos); pos+=p; norm+=n; tan+=t; uv+=u; idx += [base+x for x in i]
    return pos,norm,tan,uv,idx

def plane_geometry(half_x,half_z):
    return (
        [(-half_x,0,-half_z),(half_x,0,-half_z),(half_x,0,half_z),(-half_x,0,half_z)],
        [(0,1,0)]*4,
        [(1,0,0,-1)]*4,
        [(0,0),(1,0),(1,1),(0,1)],
        [0,2,1,0,3,2],
    )

def read_accessor(g,buf,index):
    index=exact_index(index,len(g["accessors"]),"accessor index")
    a=g["accessors"][index]; req("sparse" not in a,"sparse accessor unsupported")
    vi=exact_index(a["bufferView"],len(g["bufferViews"]),"accessor bufferView")
    view=g["bufferViews"][vi]
    req(type(view.get("buffer",0)) is int and view.get("buffer",0)==0,"bufferView buffer"); req("byteStride" not in view,"strided source fixture unsupported")
    vo=view.get("byteOffset",0); vl=view["byteLength"]; ao=a.get("byteOffset",0)
    req(all(type(v) is int and v>=0 for v in (vo,vl,ao)),"accessor offsets"); req(vo+vl<=len(buf),"bufferView bounds")
    ctype=a["componentType"]; atype=a["type"]; count=a["count"]
    req(type(count) is int and count>0,"accessor count")
    comps={"SCALAR":1,"VEC2":2,"VEC3":3,"VEC4":4}; req(atype in comps and type(ctype) is int and ctype in (5126,5123),"accessor format")
    component_size=4 if ctype==5126 else 2
    req((vo+ao)%component_size==0,"accessor alignment")
    if view.get("target")==34962: req((vo+ao)%4==0,"accessor alignment")
    ncomp=comps[atype]; code="f" if ctype==5126 else "H"; size=struct.calcsize("<"+code*ncomp); span=count*size
    req(ao+span<=vl,"accessor within bufferView")
    return [struct.unpack_from("<"+code*ncomp,buf,vo+ao+i*size) for i in range(count)]

def verify_accessor_contract(g, accessor_index, component_type, accessor_type, msg, require_bounds=False):
    accessor_index=exact_index(accessor_index,len(g["accessors"]),"accessor index")
    accessor=g["accessors"][accessor_index]
    expected_fields={"bufferView","componentType","count","type"} | ({"min","max"} if require_bounds else set())
    req(set(accessor)==expected_fields,msg+" fields")
    req(type(accessor.get("componentType")) is int and accessor.get("componentType")==component_type and accessor.get("type")==accessor_type,msg)
    req(type(accessor.get("count")) is int and accessor["count"]>0,msg+" count")
    vi=exact_index(accessor["bufferView"],len(g["bufferViews"]),"accessor bufferView")
    view=g["bufferViews"][vi]
    req(set(view)=={"buffer","byteOffset","byteLength","target"},"bufferView fields")
    req(type(view["buffer"]) is int and view["buffer"]==0,"bufferView buffer")
    req(type(view["byteOffset"]) is int and view["byteOffset"]>=0 and type(view["byteLength"]) is int and view["byteLength"]>0,"bufferView bounds")
    expected_target=34963 if component_type==5123 else 34962
    req(type(view["target"]) is int and view["target"]==expected_target,"bufferView target")
    component_size=4 if component_type==5126 else 2
    req(view["byteOffset"]%component_size==0,"accessor alignment")
    if expected_target==34962: req(view["byteOffset"]%4==0,"accessor alignment")

def verify_position_accessor_contract(g, accessor_index):
    verify_accessor_contract(g,accessor_index,5126,"VEC3","position accessor format",True)

def verify_position_accessor_bounds(g, accessor_index, positions):
    accessor=g["accessors"][accessor_index]
    declared_min=accessor.get("min"); declared_max=accessor.get("max")
    req(
        isinstance(declared_min,list) and len(declared_min)==3
        and isinstance(declared_max,list) and len(declared_max)==3
        and all(type(v) in (int,float) and math.isfinite(v) for v in declared_min+declared_max),
        "position bounds",
    )
    actual_min=[min(p[i] for p in positions) for i in range(3)]
    actual_max=[max(p[i] for p in positions) for i in range(3)]
    req(
        all(abs(declared_min[i]-actual_min[i])<=1e-6 for i in range(3))
        and all(abs(declared_max[i]-actual_max[i])<=1e-6 for i in range(3)),
        "position bounds",
    )

def verify_index_accessor_bounds(g, accessor_index, indices):
    accessor=g["accessors"][accessor_index]
    declared_min=accessor.get("min"); declared_max=accessor.get("max")
    req(
        isinstance(declared_min,list) and len(declared_min)==1
        and isinstance(declared_max,list) and len(declared_max)==1
        and type(declared_min[0]) is int and type(declared_max[0]) is int,
        "index bounds",
    )
    actual=[i[0] for i in indices]
    req(declared_min==[min(actual)] and declared_max==[max(actual)],"index bounds")

def geometry_binding(mesh):
    req(set(mesh)=={"name","primitives"},"mesh properties")
    req(len(mesh["primitives"])==1,"primitive count")
    prim=mesh["primitives"][0]
    req(set(prim)=={"attributes","indices","material","mode"},"primitive properties")
    return prim["attributes"],prim["indices"]

def verify_tangent_frame(pos,normal,tangent,uv,indices,msg,strict=False):
    threshold=FLOOR_TANGENT_ALIGNMENT_MIN if strict else NONFLOOR_TANGENT_ALIGNMENT_MIN
    for k in range(0,len(indices),3):
        ia,ib,ic=(indices[k][0],indices[k+1][0],indices[k+2][0])
        e1=sub(pos[ib],pos[ia]); e2=sub(pos[ic],pos[ia])
        du1=uv[ib][0]-uv[ia][0]; dv1=uv[ib][1]-uv[ia][1]
        du2=uv[ic][0]-uv[ia][0]; dv2=uv[ic][1]-uv[ia][1]
        det=du1*dv2-dv1*du2; req(abs(det)>1e-12,msg)
        tref=normalize(tuple((e1[j]*dv2-e2[j]*dv1)/det for j in range(3)),msg)
        bref=normalize(tuple((e2[j]*du1-e1[j]*du2)/det for j in range(3)),msg)
        for vi in (ia,ib,ic):
            t=normalize(tangent[vi][:3],msg); b=normalize(scale(cross(normal[vi],t),tangent[vi][3]),msg)
            req(dot(t,tref)>threshold and dot(b,bref)>threshold,msg)

def verify_mesh(g,buf,mesh_index,expected_name,expected_material,expected_counts,expected_indices,frame_msg="mesh tangent frame",strict_frame=False):
    mesh=g["meshes"][mesh_index]; attrs,index_accessor=geometry_binding(mesh)
    req(mesh["name"]==expected_name,"mesh name")
    prim=mesh["primitives"][0]; req(type(prim["mode"]) is int and prim["mode"]==4,"triangle mode"); req(type(prim["material"]) is int and prim["material"]==expected_material,"material binding")
    req(set(attrs)=={"POSITION","NORMAL","TANGENT","TEXCOORD_0"},"attributes")
    verify_position_accessor_contract(g,attrs["POSITION"])
    verify_accessor_contract(g,attrs["NORMAL"],5126,"VEC3","normal accessor format")
    verify_accessor_contract(g,attrs["TANGENT"],5126,"VEC4","tangent accessor format")
    verify_accessor_contract(g,attrs["TEXCOORD_0"],5126,"VEC2","texcoord accessor format")
    verify_accessor_contract(g,index_accessor,5123,"SCALAR","index accessor format",True)
    pos=read_accessor(g,buf,attrs["POSITION"]); verify_position_accessor_bounds(g,attrs["POSITION"],pos)
    normal=read_accessor(g,buf,attrs["NORMAL"]); tangent=read_accessor(g,buf,attrs["TANGENT"]); uv=read_accessor(g,buf,attrs["TEXCOORD_0"]); indices=read_accessor(g,buf,index_accessor); verify_index_accessor_bounds(g,index_accessor,indices)
    req((len(pos),len(indices))==expected_counts,"geometry counts"); req(len(normal)==len(pos) and len(tangent)==len(pos) and len(uv)==len(pos),"attribute counts")
    req(all(math.isfinite(c) for seq in (pos,normal,tangent,uv) for v in seq for c in v),"finite geometry")
    req(all(abs(sum(c*c for c in n)-1.0)<1e-4 for n in normal),"unit normals")
    req(all(abs(sum(c*c for c in t[:3])-1.0)<1e-4 and t[3] in (-1.0,1.0) for t in tangent),"unit tangents")
    req(all(abs(dot(n,t[:3]))<1e-4 for n,t in zip(normal,tangent)),"tangent orthogonality")
    req(all(0.0<=u<=1.0 and 0.0<=v<=1.0 for u,v in uv),"uv range"); req(all(0<=i[0]<len(pos) for i in indices),"index range"); req(len(indices)%3==0,"triangle index count")
    req([i[0] for i in indices]==expected_indices,"canonical indices")
    for k in range(0,len(indices),3):
        ia,ib,ic=(indices[k][0],indices[k+1][0],indices[k+2][0]); cr=tri_normal(pos[ia],pos[ib],pos[ic]); req(length(cr)>1e-12,"triangle area")
        for vi in (ia,ib,ic): req(dot(cr,normal[vi])>1e-8,"triangle vertex normal")
    verify_tangent_frame(pos,normal,tangent,uv,indices,frame_msg,strict_frame)
    return {"POSITION":pos,"NORMAL":normal,"TANGENT":tangent,"TEXCOORD_0":uv,"indices":[i[0] for i in indices],"binding":(attrs,index_accessor)}

def rows_close(actual,expected,tol=CANONICAL_FLOAT_TOL):
    if len(actual)!=len(expected): return False
    for a,e in zip(actual,expected):
        if len(a)!=len(e): return False
        if any(abs(float(x)-float(y))>tol for x,y in zip(a,e)): return False
    return True

def verify_canonical_payload(actual,expected,msg):
    pos,norm,tan,uv,idx=expected
    req(rows_close(actual["POSITION"],pos),msg)
    req(rows_close(actual["NORMAL"],norm),msg)
    req(rows_close(actual["TANGENT"],tan),msg)
    req(rows_close(actual["TEXCOORD_0"],uv),msg)
    req(actual["indices"]==idx,msg)

def reject_nested_extras(value):
    if isinstance(value,dict):
        req("extras" not in value,"nested extras unsupported")
        for child in value.values(): reject_nested_extras(child)
    elif isinstance(value,list):
        for child in value: reject_nested_extras(child)

def quat_x(degrees):
    a=math.radians(degrees)/2.0; return [math.sin(a),0.0,0.0,math.cos(a)]

def quat_xy(x_deg,y_deg):
    x=math.radians(x_deg)/2.0; y=math.radians(y_deg)/2.0
    qx=(math.sin(x),0.0,0.0,math.cos(x)); qy=(0.0,math.sin(y),0.0,math.cos(y)); ax,ay,az,aw=qy; bx,by,bz,bw=qx
    return [aw*bx+ax*bw+ay*bz-az*by,aw*by-ax*bz+ay*bw+az*bx,aw*bz+ax*by-ay*bx+az*bw,aw*bw-ax*bx-ay*by-az*bz]

def quat_close(actual,expected,tol=1e-12):
    if not isinstance(actual,list) or len(actual)!=4 or not all(isinstance(v,(int,float)) and math.isfinite(v) for v in actual): return False
    return min(max(abs(a-b) for a,b in zip(actual,expected)),max(abs(a+b) for a,b in zip(actual,expected)))<=tol

def verify(path,source_path,manifest_path=None,expected_manifest_path=None):
    source,source_raw=load_source(source_path); path=Path(path); raw=path.read_bytes(); g=json.loads(raw)
    req("animations" not in g,"gallery animations unsupported")
    req("images" not in g and "textures" not in g and "samplers" not in g,"gallery must not embed texture lighting")
    for key,value in g.items():
        if key!="extras": reject_nested_extras(value)
    req(set(g)==TOP_LEVEL_KEYS,"top-level fields")
    req(g["asset"]=={"generator":VERSION,"version":"2.0"},"asset header"); req(g["extensionsUsed"]==["KHR_lights_punctual"],"extensions used")
    req(type(g["scene"]) is int and g["scene"]==0,"scene")
    req(len(g["scenes"])==1 and set(g["scenes"][0])=={"name","nodes"} and g["scenes"][0]["name"]=="AstralNeutralMaterialGallery","scene")
    scene_nodes=g["scenes"][0]["nodes"]; req(scene_nodes==list(range(12)) and all(type(i) is int for i in scene_nodes),"scene"); req(len(g["nodes"])==12,"node count")
    req(set(g["extensions"])=={"KHR_lights_punctual"} and set(g["extensions"]["KHR_lights_punctual"])=={"lights"},"extensions fields")
    req(set(g.get("extras",{}))=={"astral_contract"},"runtime extras")
    contract=g["extras"]["astral_contract"]; req(set(contract)==CONTRACT_KEYS,"runtime contract fields"); req(contract["status"]==RUNTIME_STATUS,"runtime status")
    req(contract["units"]==source["units"] and contract["up"]==source["axes"]["up"] and contract["forward"]==source["axes"]["forward"] and contract["right"]==source["axes"]["right"],"axis contract")
    req(contract["forbid_baked_lighting"] is True,"baked-lighting rule")
    labels=tuple(s["label"] for s in source["stations"]); req(tuple(contract["station_order"])==labels,"station order"); req(contract["capture_intent"]==CAPTURE_INTENT,"capture intent"); req(contract["source_sha256"]==hashlib.sha256(source_raw).hexdigest(),"source hash")
    req(len(g["buffers"])==1 and set(g["buffers"][0])=={"byteLength","uri"},"buffer fields")
    req(type(g["buffers"][0]["byteLength"]) is int and g["buffers"][0]["byteLength"]>=0,"buffer length")
    buf=data_uri(g["buffers"][0]["uri"],"data:application/octet-stream;base64,"); req(len(buf)==g["buffers"][0]["byteLength"],"buffer length")
    material_specs=source["stations"]+[source["floor_material"]]; req(len(g["materials"])==len(material_specs),"material count")
    for material,s in zip(g["materials"],material_specs):
        req(set(material)=={"name","pbrMetallicRoughness"},"material properties"); req(material["name"]==s["material_name"],"material name"); expected={"baseColorFactor":s["base_color_factor_linear"],"metallicFactor":s["metallic"],"roughnessFactor":s["roughness"]}; req(material["pbrMetallicRoughness"]==expected,"material values")
    geometry=source["geometry"]; r=geometry["sphere_radius"]; h=geometry["cube_half_extent"]; hx,hz=source["layout"]["floor_half_extents_xz"]
    sphere_expected_geometry=sphere_geometry(r,geometry["sphere_lat_segments"],geometry["sphere_lon_segments"])
    cube_expected_geometry=cube_geometry(h); floor_expected_geometry=plane_geometry(hx,hz)
    sphere_expected=sphere_expected_geometry[4]; cube_expected=cube_expected_geometry[4]; floor_expected=floor_expected_geometry[4]
    sphere_counts=(len(sphere_expected_geometry[0]),len(sphere_expected)); cube_counts=(len(cube_expected_geometry[0]),len(cube_expected)); floor_counts=(len(floor_expected_geometry[0]),len(floor_expected))
    req(len(g["meshes"])==9,"mesh count")
    sphere_binding=geometry_binding(g["meshes"][0]); cube_binding=geometry_binding(g["meshes"][4])
    for i in range(1,4): req(geometry_binding(g["meshes"][i])==sphere_binding,"matched sphere geometry")
    for i in range(5,8): req(geometry_binding(g["meshes"][i])==cube_binding,"matched cube geometry")
    sphere_payload=None
    for i,label in enumerate(labels):
        payload=verify_mesh(g,buf,i,f"{label}_SphereMesh",i,sphere_counts,sphere_expected)
        if sphere_payload is None: sphere_payload=payload
    cube_payload=None
    for i,label in enumerate(labels):
        payload=verify_mesh(g,buf,4+i,f"{label}_CubeMesh",i,cube_counts,cube_expected)
        if cube_payload is None: cube_payload=payload
    req(all(abs(length(p)-r)<1e-5 for p in sphere_payload["POSITION"]),"sphere radius")
    req(all(all(abs(abs(c)-h)<1e-6 for c in p) for p in cube_payload["POSITION"]),"cube extent")
    floor_payload=verify_mesh(g,buf,8,"FloorMesh",4,floor_counts,floor_expected,"floor tangent frame",True); floor_pos=floor_payload["POSITION"]
    req(all(abs(y)<1e-7 for x,y,z in floor_pos),"floor plane")
    req(all(abs(abs(x)-hx)<1e-6 and abs(abs(z)-hz)<1e-5 for x,y,z in floor_pos),"floor extents")
    verify_canonical_payload(sphere_payload,sphere_expected_geometry,"canonical sphere payload")
    verify_canonical_payload(cube_payload,cube_expected_geometry,"canonical cube payload")
    verify_canonical_payload(floor_payload,floor_expected_geometry,"canonical floor payload")
    referenced_accessors=set()
    for mesh in g["meshes"]:
        attrs,idx=geometry_binding(mesh); referenced_accessors.update(attrs.values()); referenced_accessors.add(idx)
    req(referenced_accessors==set(range(len(g["accessors"]))),"accessor coverage")
    referenced_views={g["accessors"][i]["bufferView"] for i in referenced_accessors}
    req(referenced_views==set(range(len(g["bufferViews"]))),"bufferView coverage")
    xs=source["layout"]["x_positions"]; sy,sz=source["layout"]["sphere_yz"]; cy,cz=source["layout"]["cube_yz"]; req(len(xs)==4,"layout count")
    for i,(label,x) in enumerate(zip(labels,xs)):
        sphere_node=g["nodes"][i]; cube_node=g["nodes"][4+i]
        req(type(sphere_node.get("mesh")) is int and sphere_node=={"name":f"{label}_Sphere","mesh":i,"translation":[x,sy,sz]},"sphere station node")
        req(type(cube_node.get("mesh")) is int and cube_node=={"name":f"{label}_Cube","mesh":4+i,"translation":[x,cy,cz]},"cube station node")
    req(type(g["nodes"][8].get("mesh")) is int and g["nodes"][8]=={"name":"NeutralFloor","mesh":8},"floor node")
    camera=source["camera"]; req(len(g["cameras"])==1,"camera count"); cam=g["cameras"][0]
    req(set(cam)=={"name","type","perspective"},"camera fields"); req(cam["name"]=="NeutralReviewCamera" and cam["type"]=="perspective","camera")
    p=cam["perspective"]; req(set(p)=={"aspectRatio","yfov","znear","zfar"},"camera perspective fields")
    aspect=camera["aspect_ratio"][0]/camera["aspect_ratio"][1]; req(abs(p["aspectRatio"]-aspect)<1e-12 and abs(p["yfov"]-math.radians(camera["vertical_fov_degrees"]))<1e-12,"camera framing"); req(p["znear"]==camera["znear"] and p["zfar"]==camera["zfar"],"camera clip")
    cam_node=g["nodes"][9]; req(set(cam_node)=={"name","camera","translation","rotation"},"camera transform"); req(type(cam_node["camera"]) is int and cam_node["name"]=="ReviewCamera" and cam_node["camera"]==0 and cam_node["translation"]==camera["translation"],"camera node"); req(quat_close(cam_node["rotation"],quat_x(camera["pitch_degrees"])),"camera rotation")
    lights=g["extensions"]["KHR_lights_punctual"]["lights"]; expected_lights=[{"color":s["color_linear"],"intensity":s["intensity_lux"],"name":s["name"],"type":s["type"]} for s in source["lights"]]; req(lights==expected_lights,"lights")
    for j,s in enumerate(source["lights"]):
        node=g["nodes"][10+j]; req(set(node)=={"name","rotation","extensions"},"light transform")
        light_ref=node.get("extensions",{}).get("KHR_lights_punctual",{}).get("light")
        req(type(light_ref) is int and node["name"]==s["name"] and node["extensions"]=={"KHR_lights_punctual":{"light":j}},"light node")
        rot=s["rotation_degrees"]; req(quat_close(node["rotation"],quat_xy(rot["x"],rot["y"])),"light rotation")
    if manifest_path:
        manifest_raw=Path(manifest_path).read_bytes(); m=json.loads(manifest_raw); req(set(m)==MANIFEST_KEYS,"manifest fields"); req(type(m["schema_version"]) is int and m["schema_version"]==1,"manifest schema"); req(m["generator"]==VERSION and m["runtime_status"]==RUNTIME_STATUS,"manifest status"); req(m["intent"]==MANIFEST_INTENT,"manifest intent"); req(m["source_sha256"]==hashlib.sha256(source_raw).hexdigest(),"manifest source hash")
        req(m["counts"]=={"cameras":1,"cube_nodes":4,"directional_lights":2,"materials":5,"sphere_nodes":4,"stations":4},"manifest counts"); req(len(m["files"])==1,"manifest files")
        rec=m["files"][0]; req(rec=={"path":path.name,"bytes":len(raw),"sha256":hashlib.sha256(raw).hexdigest()},"manifest file record")
        if expected_manifest_path: req(manifest_raw.replace(b"\r\n",b"\n").replace(b"\r",b"\n")==canonical_text_bytes(expected_manifest_path),"expected manifest pin")
    elif expected_manifest_path: raise ValueError("expected manifest requires generated manifest")
    return {"materials":5,"stations":4,"sphere_vertices":sphere_counts[0],"sphere_indices":sphere_counts[1],"lights":2,"camera_fov_degrees":camera["vertical_fov_degrees"]}

def main():
    p=argparse.ArgumentParser(); p.add_argument("gltf",type=Path); p.add_argument("--source",type=Path,required=True); p.add_argument("--manifest",type=Path); p.add_argument("--expected-manifest",type=Path); a=p.parse_args()
    try: print("PASS:",verify(a.gltf,a.source,a.manifest,a.expected_manifest))
    except (ValueError,KeyError,IndexError,TypeError,OSError,json.JSONDecodeError,struct.error,base64.binascii.Error) as e: p.exit(1,f"FAIL: {e}\n")
if __name__=="__main__": main()
