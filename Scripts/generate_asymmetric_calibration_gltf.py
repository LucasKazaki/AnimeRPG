from __future__ import annotations
import argparse, base64, hashlib, json, struct, zlib
from pathlib import Path

VERSION="astral-calibration-gltf-1"
SIZE=16

def png_rgb8(width,height,pixel):
    def chunk(kind,data):
        return struct.pack(">I",len(data))+kind+data+struct.pack(">I",zlib.crc32(kind+data)&0xffffffff)
    raw=b"".join(b"\x00"+bytes(c for x in range(width) for c in pixel(x,y)) for y in range(height))
    return b"\x89PNG\r\n\x1a\n"+chunk(b"IHDR",struct.pack(">IIBBBBB",width,height,8,2,0,0,0))+chunk(b"IDAT",zlib.compress(raw,9))+chunk(b"IEND",b"")

def orient_pixel(x,y):
    if x==0 or y==0: return (0,0,0)
    left=x < SIZE//2; top=y < SIZE//2
    if top and left: return (255,48,48)
    if top and not left: return (48,255,48)
    if not top and left: return (48,96,255)
    return (255,220,48)

def add_aligned(buf,data,alignment=4):
    while len(buf)%alignment: buf.append(0)
    offset=len(buf); buf.extend(data); return offset,len(data)

def pack_floats(values): return struct.pack("<"+"f"*len(values),*values)
def pack_u16(values): return struct.pack("<"+"H"*len(values),*values)

def face(center,t,b,hu,hv):
    n=(t[1]*b[2]-t[2]*b[1],t[2]*b[0]-t[0]*b[2],t[0]*b[1]-t[1]*b[0])
    def p(su,sv): return tuple(center[i]+t[i]*hu*su+b[i]*hv*sv for i in range(3))
    pos=[p(-1,-1),p(1,-1),p(1,1),p(-1,1)]
    return pos,[n]*4,[(*t,1.0)]*4,[(0,0),(1,0),(1,1),(0,1)],[0,1,2,0,2,3]

def build():
    faces=[]
    faces += [face((0,0.75,1.0),(1,0,0),(0,1,0),0.5,0.75)]
    faces += [face((0,0.75,-1.0),(-1,0,0),(0,1,0),0.5,0.75)]
    faces += [face((0.5,0.75,0),(0,0,-1),(0,1,0),1.0,0.75)]
    faces += [face((-0.5,0.75,0),(0,0,1),(0,1,0),1.0,0.75)]
    faces += [face((0,1.5,0),(1,0,0),(0,0,-1),0.5,1.0)]
    faces += [face((0,0,0),(1,0,0),(0,0,1),0.5,1.0)]
    pos=[];norm=[];tan=[];uv=[];idx=[]
    for f in faces:
        base=len(pos); p,n,t,u,i=f
        pos+=p;norm+=n;tan+=t;uv+=u;idx += [base+x for x in i]
    mpos=[(-0.15,1.15,1.01),(-0.15,1.35,1.01),(0.32,1.25,1.01)]
    mnorm=[(0,0,1)]*3; mtan=[(1,0,0,1)]*3; muv=[(0,1),(0,0),(1,0.5)]; midx=[0,1,2]
    buf=bytearray(); views=[]; accessors=[]
    def add_accessor(vals,ctype,atype,target=None,minv=None,maxv=None):
        flat=[c for v in vals for c in (v if isinstance(v,(tuple,list)) else (v,))]
        data=pack_floats(flat) if ctype==5126 else pack_u16(flat)
        off,n=add_aligned(buf,data,4)
        vi=len(views); view={"buffer":0,"byteOffset":off,"byteLength":n}
        if target: view["target"]=target
        views.append(view)
        acc={"bufferView":vi,"componentType":ctype,"count":len(vals),"type":atype}
        if minv is not None: acc["min"]=list(minv)
        if maxv is not None: acc["max"]=list(maxv)
        ai=len(accessors); accessors.append(acc); return ai
    bp=add_accessor(pos,5126,"VEC3",34962,(-0.5,0,-1.0),(0.5,1.5,1.0))
    bn=add_accessor(norm,5126,"VEC3",34962); bt=add_accessor(tan,5126,"VEC4",34962); bu=add_accessor(uv,5126,"VEC2",34962)
    bi=add_accessor(idx,5123,"SCALAR",34963,minv=(0,),maxv=(23,))
    mp=add_accessor(mpos,5126,"VEC3",34962,(-0.15,1.15,1.01),(0.32,1.35,1.01))
    mn=add_accessor(mnorm,5126,"VEC3",34962); mt=add_accessor(mtan,5126,"VEC4",34962); mu=add_accessor(muv,5126,"VEC2",34962)
    mi=add_accessor(midx,5123,"SCALAR",34963,minv=(0,),maxv=(2,))
    image=png_rgb8(SIZE,SIZE,orient_pixel)
    gltf={
      "asset":{"version":"2.0","generator":VERSION},
      "scene":0,
      "scenes":[{"name":"AstralCalibrationScene","nodes":[0,1]}],
      "nodes":[{"name":"AsymmetricBlock","mesh":0},{"name":"ForwardRightMarker","mesh":1}],
      "meshes":[
        {"name":"AsymmetricBlockMesh","primitives":[{"attributes":{"POSITION":bp,"NORMAL":bn,"TANGENT":bt,"TEXCOORD_0":bu},"indices":bi,"material":0,"mode":4}]},
        {"name":"ForwardRightMarkerMesh","primitives":[{"attributes":{"POSITION":mp,"NORMAL":mn,"TANGENT":mt,"TEXCOORD_0":mu},"indices":mi,"material":1,"mode":4}]}
      ],
      "materials":[
        {"name":"UVOrientation","pbrMetallicRoughness":{"baseColorTexture":{"index":0,"texCoord":0},"metallicFactor":0.0,"roughnessFactor":1.0}},
        {"name":"MarkerMagenta","pbrMetallicRoughness":{"baseColorFactor":[1.0,0.0,1.0,1.0],"metallicFactor":0.0,"roughnessFactor":1.0},"emissiveFactor":[0.2,0.0,0.2]}
      ],
      "samplers":[{"magFilter":9728,"minFilter":9728,"wrapS":33071,"wrapT":33071}],
      "images":[{"name":"UVOrientation16","uri":"data:image/png;base64,"+base64.b64encode(image).decode()}],
      "textures":[{"sampler":0,"source":0}],
      "buffers":[{"byteLength":len(buf),"uri":"data:application/octet-stream;base64,"+base64.b64encode(bytes(buf)).decode()}],
      "bufferViews":views,"accessors":accessors,
      "extras":{"astral_contract":{"units":"metres","up":"+Y","forward":"+Z","right":"-X per glTF convention","status":"source_validated_not_imported"}}
    }
    text=json.dumps(gltf,indent=2,sort_keys=True)+"\n"
    manifest={"schema_version":1,"generator":VERSION,"files":[{"path":"asymmetric_surface.gltf","bytes":len(text.encode()),"sha256":hashlib.sha256(text.encode()).hexdigest()}],
              "intent":"Source-only glTF 2.0 calibration fixture for future Astral solid-mesh/texture import validation.",
              "runtime_status":"source_validated_not_imported"}
    return text.encode(),(json.dumps(manifest,indent=2,sort_keys=True)+"\n").encode()

def main():
    ap=argparse.ArgumentParser();ap.add_argument("--output",type=Path,required=True);ap.add_argument("--check",action="store_true");a=ap.parse_args()
    files=dict(zip(("asymmetric_surface.gltf","manifest.json"),build()))
    if a.check:
        if not a.output.is_dir(): ap.error("check requires existing directory")
        actual={p.name for p in a.output.iterdir() if p.is_file()}
        if actual!=set(files): ap.error("unexpected/missing files")
        for name,data in files.items():
            if (a.output/name).read_bytes()!=data: ap.error(f"{name} differs")
    else:
        if a.output.exists(): ap.error("refusing existing output")
        a.output.mkdir(parents=True)
        for name,data in files.items(): (a.output/name).write_bytes(data)
    print("PASS: asymmetric glTF calibration fixture")
if __name__=="__main__": main()
