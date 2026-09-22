from __future__ import annotations
import argparse,base64,hashlib,json,math,struct,zlib
from pathlib import Path

def req(c,m):
    if not c: raise ValueError(m)
def data_uri(uri,prefix):
    req(uri.startswith(prefix),"wrong data URI"); return base64.b64decode(uri[len(prefix):],validate=True)
def png_dims(data):
    req(data[:8]==b"\x89PNG\r\n\x1a\n","png sig"); req(len(data)>33,"png short")
    n=struct.unpack(">I",data[8:12])[0]; req(n==13 and data[12:16]==b"IHDR","png ihdr")
    w,h,depth,color,comp,filt,inter=struct.unpack(">IIBBBBB",data[16:29]); req((w,h,depth,color,comp,filt,inter)==(16,16,8,2,0,0,0),"png format")
    return w,h
def read_accessor(g,buf,i):
    a=g["accessors"][i]; v=g["bufferViews"][a["bufferView"]]; off=v.get("byteOffset",0)+a.get("byteOffset",0)
    c=a["componentType"]; t=a["type"]; count=a["count"]; ncomp={"SCALAR":1,"VEC2":2,"VEC3":3,"VEC4":4}[t]
    fmt={5126:"f",5123:"H"}[c]; size=struct.calcsize("<"+fmt*ncomp)
    req(off+count*size<=len(buf),"accessor bounds")
    return [struct.unpack_from("<"+fmt*ncomp,buf,off+j*size) for j in range(count)]
def verify(path,manifest_path=None):
    path=Path(path); raw=path.read_bytes(); g=json.loads(raw)
    req(g["asset"]["version"]=="2.0","version"); req(g["scene"]==0 and len(g["scenes"])==1,"scene")
    req([n["name"] for n in g["nodes"]]==["AsymmetricBlock","ForwardRightMarker"],"nodes")
    contract=g["extras"]["astral_contract"]; req(contract=={"units":"metres","up":"+Y","forward":"+Z","right":"-X per glTF convention","status":"source_validated_not_imported"},"contract")
    buf=data_uri(g["buffers"][0]["uri"],"data:application/octet-stream;base64,"); req(len(buf)==g["buffers"][0]["byteLength"],"buffer length")
    image=data_uri(g["images"][0]["uri"],"data:image/png;base64,"); req(png_dims(image)==(16,16),"image")
    req(len(g["meshes"])==2 and len(g["materials"])==2,"mesh/material count")
    p0=g["meshes"][0]["primitives"][0]; req(p0["mode"]==4 and p0["material"]==0,"block primitive")
    attrs=p0["attributes"]; req(set(attrs)=={"POSITION","NORMAL","TANGENT","TEXCOORD_0"},"block attrs")
    pos=read_accessor(g,buf,attrs["POSITION"]); normal=read_accessor(g,buf,attrs["NORMAL"]); tangent=read_accessor(g,buf,attrs["TANGENT"]); uv=read_accessor(g,buf,attrs["TEXCOORD_0"]); idx=read_accessor(g,buf,p0["indices"])
    req(len(pos)==24 and len(idx)==36,"block counts")
    req(all(math.isfinite(c) for v in pos+normal+tangent+uv for c in v),"finite")
    req(all(abs(sum(c*c for c in n)-1)<1e-5 for n in normal),"unit normals")
    req(all(abs(sum(c*c for c in t[:3])-1)<1e-5 and t[3] in (-1.0,1.0) for t in tangent),"tangents")
    req(all(0<=u<=1 and 0<=v<=1 for u,v in uv),"uv range")
    req(all(0<=i[0]<24 for i in idx),"indices")
    for k in range(0,len(idx),3):
        ia,ib,ic=(idx[k][0],idx[k+1][0],idx[k+2][0]); a,b,c=pos[ia],pos[ib],pos[ic]
        ab=tuple(b[j]-a[j] for j in range(3)); ac=tuple(c[j]-a[j] for j in range(3))
        cr=(ab[1]*ac[2]-ab[2]*ac[1],ab[2]*ac[0]-ab[0]*ac[2],ab[0]*ac[1]-ab[1]*ac[0])
        req(sum(cr[j]*normal[ia][j] for j in range(3))>0,"winding")
    marker=g["meshes"][1]["primitives"][0]; req(marker["material"]==1 and marker["mode"]==4,"marker primitive")
    mpos=read_accessor(g,buf,marker["attributes"]["POSITION"]); req(len(mpos)==3,"marker pos")
    req(max(x for x,y,z in mpos)>0.3 and all(z>1.0 for x,y,z in mpos),"marker handedness/front")
    req(g["materials"][0]["pbrMetallicRoughness"]["baseColorTexture"]=={"index":0,"texCoord":0},"texture binding")
    req(g["samplers"][0]["wrapS"]==33071 and g["samplers"][0]["wrapT"]==33071,"sampler clamp")
    if manifest_path:
        m=json.loads(Path(manifest_path).read_text()); rec=m["files"][0]
        req(rec["path"]==path.name and rec["bytes"]==len(raw) and rec["sha256"]==hashlib.sha256(raw).hexdigest(),"manifest")
        req(m["runtime_status"]=="source_validated_not_imported","manifest status")
    return {"block_vertices":24,"block_indices":36,"marker_vertices":3}
def main():
    p=argparse.ArgumentParser();p.add_argument("gltf",type=Path);p.add_argument("--manifest",type=Path);a=p.parse_args()
    try: print("PASS:",verify(a.gltf,a.manifest))
    except (ValueError,KeyError,TypeError,OSError,json.JSONDecodeError,struct.error,zlib.error) as e: p.exit(1,f"FAIL: {e}\n")
if __name__=="__main__": main()
