from __future__ import annotations
import argparse, base64, hashlib, json, math, struct
from pathlib import Path

VERSION = "astral-material-gallery-gltf-3"
SOURCE_STATUS = "proposed_art_reference_not_runtime"
RUNTIME_STATUS = "source_validated_not_imported"

def canonical_text_bytes(path):
    return Path(path).read_bytes().replace(b"\r\n",b"\n").replace(b"\r",b"\n")

def load_source(path):
    raw=canonical_text_bytes(path)
    source=json.loads(raw)
    if source.get("schema_version")!=1:
        raise ValueError("source schema")
    if source.get("status")!=SOURCE_STATUS:
        raise ValueError("source status")
    if source.get("units")!="metres" or source.get("axes")!={"forward":"+Z","right":"-X per glTF convention","up":"+Y"}:
        raise ValueError("source axes")
    if source.get("forbid_baked_lighting") is not True:
        raise ValueError("baked-lighting rule")
    if len(source.get("stations",[]))!=4 or len(source.get("lights",[]))!=2:
        raise ValueError("source counts")
    return source, raw

def add_aligned(buf, data, alignment=4):
    while len(buf) % alignment:
        buf.append(0)
    offset = len(buf)
    buf.extend(data)
    return offset, len(data)

def pack_floats(values):
    return struct.pack("<" + "f" * len(values), *values)

def pack_u16(values):
    return struct.pack("<" + "H" * len(values), *values)

def sphere_geometry(radius, lat_segments, lon_segments):
    pos, norm, tan, uv, idx = [], [], [], [], []
    for j in range(lat_segments + 1):
        theta = math.pi * j / lat_segments
        st, ct = math.sin(theta), math.cos(theta)
        for i in range(lon_segments + 1):
            phi = 2.0 * math.pi * i / lon_segments
            cp, sp = math.cos(phi), math.sin(phi)
            n = (st * cp, ct, st * sp)
            pos.append(tuple(radius * c for c in n))
            norm.append(n)
            tan.append((-sp, 0.0, cp, 1.0))
            uv.append((i / lon_segments, j / lat_segments))
    row = lon_segments + 1
    for j in range(lat_segments):
        for i in range(lon_segments):
            a = j * row + i
            b = a + row
            if j > 0:
                idx += [a, a + 1, b]
            if j < lat_segments - 1:
                idx += [a + 1, b + 1, b]
    return pos, norm, tan, uv, idx

def quad_face(center, tangent, bitangent, half_u, half_v):
    normal = (
        tangent[1] * bitangent[2] - tangent[2] * bitangent[1],
        tangent[2] * bitangent[0] - tangent[0] * bitangent[2],
        tangent[0] * bitangent[1] - tangent[1] * bitangent[0],
    )
    def p(su, sv):
        return tuple(center[k] + tangent[k] * half_u * su + bitangent[k] * half_v * sv for k in range(3))
    pos = [p(-1, -1), p(1, -1), p(1, 1), p(-1, 1)]
    return pos, [normal] * 4, [(*tangent, 1.0)] * 4, [(0,0),(1,0),(1,1),(0,1)], [0,1,2,0,2,3]

def cube_geometry(half):
    faces = [
        quad_face((0,0,half),(1,0,0),(0,1,0),half,half),
        quad_face((0,0,-half),(-1,0,0),(0,1,0),half,half),
        quad_face((half,0,0),(0,0,-1),(0,1,0),half,half),
        quad_face((-half,0,0),(0,0,1),(0,1,0),half,half),
        quad_face((0,half,0),(1,0,0),(0,0,-1),half,half),
        quad_face((0,-half,0),(1,0,0),(0,0,1),half,half),
    ]
    pos=[]; norm=[]; tan=[]; uv=[]; idx=[]
    for p,n,t,u,i in faces:
        base=len(pos)
        pos += p; norm += n; tan += t; uv += u
        idx += [base+x for x in i]
    return pos,norm,tan,uv,idx

def plane_geometry(half_x, half_z):
    return (
        [(-half_x,0,-half_z),(half_x,0,-half_z),(half_x,0,half_z),(-half_x,0,half_z)],
        [(0,1,0)]*4,
        [(1,0,0,-1)]*4,
        [(0,0),(1,0),(1,1),(0,1)],
        [0,2,1,0,3,2],
    )

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

def validate_source_numbers(source):
    geometry=source["geometry"]
    if not (0.1<=geometry["sphere_radius"]<=2.0 and 4<=geometry["sphere_lat_segments"]<=64 and 8<=geometry["sphere_lon_segments"]<=128):
        raise ValueError("sphere budget")
    if not (0.1<=geometry["cube_half_extent"]<=2.0):
        raise ValueError("cube budget")
    camera=source["camera"]
    if not (20.0<=camera["vertical_fov_degrees"]<=90.0 and 0.01<=camera["znear"]<camera["zfar"]<=1000.0):
        raise ValueError("camera budget")
    if camera["aspect_ratio"]!=[16,9]:
        raise ValueError("camera aspect")
    for light in source["lights"]:
        if light["type"]!="directional" or not (0.0<light["intensity_lux"]<=200000.0):
            raise ValueError("light budget")
        if light["color_linear"]!=[1.0,1.0,1.0]:
            raise ValueError("neutral light color")
    for material in source["stations"]+[source["floor_material"]]:
        c=material["base_color_factor_linear"]
        if len(c)!=4 or any((not isinstance(v,(int,float)) or not 0.0<=v<=1.0) for v in c):
            raise ValueError("material color")
        if not 0.0<=material["metallic"]<=1.0 or not 0.0<=material["roughness"]<=1.0:
            raise ValueError("material factors")

def build(source_path):
    source, source_raw=load_source(source_path)
    validate_source_numbers(source)
    buf=bytearray(); views=[]; accessors=[]
    def add_accessor(values, ctype, atype, target, minv=None, maxv=None):
        flat=[c for v in values for c in (v if isinstance(v,(tuple,list)) else (v,))]
        data=pack_floats(flat) if ctype==5126 else pack_u16(flat)
        off,n=add_aligned(buf,data,4)
        vi=len(views)
        views.append({"buffer":0,"byteOffset":off,"byteLength":n,"target":target})
        acc={"bufferView":vi,"componentType":ctype,"count":len(values),"type":atype}
        if minv is not None: acc["min"]=list(minv)
        if maxv is not None: acc["max"]=list(maxv)
        ai=len(accessors); accessors.append(acc); return ai

    def add_geometry(geom, bounds):
        pos,norm,tan,uv,idx=geom
        return {
            "POSITION": add_accessor(pos,5126,"VEC3",34962,bounds[0],bounds[1]),
            "NORMAL": add_accessor(norm,5126,"VEC3",34962),
            "TANGENT": add_accessor(tan,5126,"VEC4",34962),
            "TEXCOORD_0": add_accessor(uv,5126,"VEC2",34962),
            "indices": add_accessor(idx,5123,"SCALAR",34963,(min(idx),),(max(idx),)),
        }

    geometry=source["geometry"]
    r=geometry["sphere_radius"]; h=geometry["cube_half_extent"]
    sphere=add_geometry(
        sphere_geometry(r,geometry["sphere_lat_segments"],geometry["sphere_lon_segments"]),
        ((-r,-r,-r),(r,r,r)),
    )
    cube=add_geometry(cube_geometry(h),((-h,-h,-h),(h,h,h)))
    hx,hz=source["layout"]["floor_half_extents_xz"]
    floor=add_geometry(plane_geometry(hx,hz),((-hx,0,-hz),(hx,0,hz)))

    material_specs=source["stations"]+[source["floor_material"]]
    materials=[{
        "name":m["material_name"],
        "pbrMetallicRoughness":{
            "baseColorFactor":m["base_color_factor_linear"],
            "metallicFactor":m["metallic"],
            "roughnessFactor":m["roughness"],
        }
    } for m in material_specs]

    meshes=[]
    labels=[s["label"] for s in source["stations"]]
    for i,label in enumerate(labels):
        meshes.append({"name":f"{label}_SphereMesh","primitives":[{"attributes":{k:v for k,v in sphere.items() if k!="indices"},"indices":sphere["indices"],"material":i,"mode":4}]})
    for i,label in enumerate(labels):
        meshes.append({"name":f"{label}_CubeMesh","primitives":[{"attributes":{k:v for k,v in cube.items() if k!="indices"},"indices":cube["indices"],"material":i,"mode":4}]})
    meshes.append({"name":"FloorMesh","primitives":[{"attributes":{k:v for k,v in floor.items() if k!="indices"},"indices":floor["indices"],"material":len(materials)-1,"mode":4}]})

    xs=source["layout"]["x_positions"]; sy,sz=source["layout"]["sphere_yz"]; cy,cz=source["layout"]["cube_yz"]
    nodes=[]
    for i,(x,label) in enumerate(zip(xs,labels)):
        nodes.append({"name":f"{label}_Sphere","mesh":i,"translation":[x,sy,sz]})
    for i,(x,label) in enumerate(zip(xs,labels)):
        nodes.append({"name":f"{label}_Cube","mesh":4+i,"translation":[x,cy,cz]})
    nodes.append({"name":"NeutralFloor","mesh":8})
    camera=source["camera"]
    nodes.append({"name":"ReviewCamera","camera":0,"translation":camera["translation"],"rotation":quat_x(camera["pitch_degrees"])})
    for light_index,light in enumerate(source["lights"]):
        rot=light["rotation_degrees"]
        nodes.append({"name":light["name"],"rotation":quat_xy(rot["x"],rot["y"]),"extensions":{"KHR_lights_punctual":{"light":light_index}}})

    aspect=camera["aspect_ratio"][0]/camera["aspect_ratio"][1]
    gltf={
        "asset":{"version":"2.0","generator":VERSION},
        "extensionsUsed":["KHR_lights_punctual"],
        "scene":0,
        "scenes":[{"name":"AstralNeutralMaterialGallery","nodes":list(range(len(nodes)))}],
        "nodes":nodes,
        "meshes":meshes,
        "materials":materials,
        "cameras":[{"name":"NeutralReviewCamera","type":"perspective","perspective":{"aspectRatio":aspect,"yfov":math.radians(camera["vertical_fov_degrees"]),"znear":camera["znear"],"zfar":camera["zfar"]}}],
        "extensions":{"KHR_lights_punctual":{"lights":[{
            "name":light["name"],"type":light["type"],"color":light["color_linear"],"intensity":light["intensity_lux"]
        } for light in source["lights"]]}},
        "buffers":[{"byteLength":len(buf),"uri":"data:application/octet-stream;base64,"+base64.b64encode(bytes(buf)).decode()}],
        "bufferViews":views,
        "accessors":accessors,
        "extras":{"astral_contract":{
            "units":source["units"],**source["axes"],
            "status":RUNTIME_STATUS,
            "capture_intent":source["capture_intent"],
            "station_order":labels,
            "forbid_baked_lighting":source["forbid_baked_lighting"],
            "source_sha256":hashlib.sha256(source_raw).hexdigest(),
        }},
    }
    text=(json.dumps(gltf,indent=2,sort_keys=True)+"\n").encode()
    manifest={
        "schema_version":1,
        "generator":VERSION,
        "source_sha256":hashlib.sha256(source_raw).hexdigest(),
        "files":[{"path":"material_gallery.gltf","bytes":len(text),"sha256":hashlib.sha256(text).hexdigest()}],
        "counts":{"materials":len(materials),"stations":len(source["stations"]),"sphere_nodes":4,"cube_nodes":4,"directional_lights":2,"cameras":1},
        "intent":"Source-only neutral material-response gallery for future Astral import/render and art review.",
        "runtime_status":RUNTIME_STATUS,
    }
    return text,(json.dumps(manifest,indent=2,sort_keys=True)+"\n").encode()

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--source",type=Path,required=True)
    p.add_argument("--output",type=Path,required=True)
    p.add_argument("--check",action="store_true")
    a=p.parse_args()
    try:
        files=dict(zip(("material_gallery.gltf","manifest.json"),build(a.source)))
    except (ValueError,KeyError,TypeError,json.JSONDecodeError,OSError) as e:
        p.error(str(e))
    if a.check:
        if not a.output.is_dir(): p.error("check requires existing directory")
        actual={x.name for x in a.output.iterdir() if x.is_file()}
        if actual!=set(files): p.error("unexpected/missing files")
        for name,data in files.items():
            if (a.output/name).read_bytes()!=data: p.error(f"{name} differs")
    else:
        if a.output.exists(): p.error("refusing existing output")
        a.output.mkdir(parents=True)
        for name,data in files.items(): (a.output/name).write_bytes(data)
    print("PASS: neutral material gallery source")
if __name__=="__main__":
    main()
