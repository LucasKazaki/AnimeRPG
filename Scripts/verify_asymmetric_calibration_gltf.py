from __future__ import annotations
import argparse,base64,hashlib,json,math,struct,zlib
from pathlib import Path

PNG_SIG=b"\x89PNG\r\n\x1a\n"

def req(c,m):
    if not c: raise ValueError(m)
def data_uri(uri,prefix):
    req(uri.startswith(prefix),"wrong data URI"); return base64.b64decode(uri[len(prefix):],validate=True)
def png_dims(data):
    req(data[:8]==PNG_SIG,"png sig")
    off=8; chunks=[]; idat=bytearray(); w=h=None
    while off<len(data):
        req(off+12<=len(data),"png truncated chunk")
        n=struct.unpack(">I",data[off:off+4])[0]; end=off+12+n
        req(end<=len(data),"png chunk bounds")
        kind=data[off+4:off+8]; body=data[off+8:off+8+n]
        crc=struct.unpack(">I",data[off+8+n:end])[0]
        req(crc==(zlib.crc32(kind+body)&0xffffffff),"png crc")
        chunks.append(kind)
        if kind==b"IHDR":
            req(len(chunks)==1 and n==13,"png ihdr")
            w,h,depth,color,comp,filt,inter=struct.unpack(">IIBBBBB",body)
            req((w,h,depth,color,comp,filt,inter)==(16,16,8,2,0,0,0),"png format")
        elif kind==b"IDAT": idat.extend(body)
        elif kind==b"IEND":
            req(n==0 and end==len(data),"png end")
            off=end; break
        else:
            req(kind in (b"IHDR",b"IDAT",b"IEND"),"unexpected png chunk")
        off=end
    req(chunks and chunks[0]==b"IHDR" and b"IDAT" in chunks and chunks[-1]==b"IEND","png chunk order")
    expected=h*(1+3*w)
    dec=zlib.decompressobj(); raw=dec.decompress(bytes(idat),expected+1)
    req(len(raw)==expected and dec.eof and not dec.unused_data and not dec.unconsumed_tail,"png inflated size")
    req(all(raw[y*(1+3*w)]==0 for y in range(h)),"png filter")
    return w,h
def read_accessor(g,buf,i):
    a=g["accessors"][i]; req(not a.get("sparse"),"sparse accessor unsupported")
    vi=a["bufferView"]; req(0<=vi<len(g["bufferViews"]),"accessor bufferView")
    v=g["bufferViews"][vi]; req(v.get("buffer",0)==0,"bufferView buffer"); req("byteStride" not in v,"strided fixture unsupported")
    view_off=v.get("byteOffset",0); view_len=v["byteLength"]; acc_off=a.get("byteOffset",0)
    req(isinstance(view_off,int) and isinstance(view_len,int) and isinstance(acc_off,int) and view_off>=0 and view_len>=0 and acc_off>=0,"accessor offsets")
    req(view_off+view_len<=len(buf),"bufferView bounds")
    c=a["componentType"]; t=a["type"]; count=a["count"]; req(isinstance(count,int) and count>0,"accessor count")
    ncomp={"SCALAR":1,"VEC2":2,"VEC3":3,"VEC4":4}[t]
    fmt={5126:"f",5123:"H"}[c]; size=struct.calcsize("<"+fmt*ncomp); span=count*size
    req(acc_off+span<=view_len,"accessor within bufferView")
    off=view_off+acc_off
    return [struct.unpack_from("<"+fmt*ncomp,buf,off+j*size) for j in range(count)]
def tri_normal(a,b,c):
    ab=tuple(b[j]-a[j] for j in range(3)); ac=tuple(c[j]-a[j] for j in range(3))
    return (ab[1]*ac[2]-ab[2]*ac[1],ab[2]*ac[0]-ab[0]*ac[2],ab[0]*ac[1]-ab[1]*ac[0])
def verify(path,manifest_path=None,expected_manifest_path=None):
    path=Path(path); raw=path.read_bytes(); g=json.loads(raw)
    req(g["asset"]["version"]=="2.0","version"); req(g["scene"]==0 and len(g["scenes"])==1,"scene")
    req(g["scenes"][0]=={"name":"AstralCalibrationScene","nodes":[0,1]},"scene nodes")
    req(g["nodes"]==[{"name":"AsymmetricBlock","mesh":0},{"name":"ForwardRightMarker","mesh":1}],"node mesh ownership")
    contract=g["extras"]["astral_contract"]; req(contract=={"units":"metres","up":"+Y","forward":"+Z","right":"-X per glTF convention","status":"source_validated_not_imported"},"contract")
    req(len(g["buffers"])==1,"buffer count"); buf=data_uri(g["buffers"][0]["uri"],"data:application/octet-stream;base64,"); req(len(buf)==g["buffers"][0]["byteLength"],"buffer length")
    req(len(g["images"])==1,"image count"); image=data_uri(g["images"][0]["uri"],"data:image/png;base64,"); req(png_dims(image)==(16,16),"image")
    req(len(g["meshes"])==2 and len(g["materials"])==2,"mesh/material count")
    req([m["name"] for m in g["meshes"]]==["AsymmetricBlockMesh","ForwardRightMarkerMesh"],"mesh names")
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
        ia,ib,ic=(idx[k][0],idx[k+1][0],idx[k+2][0]); cr=tri_normal(pos[ia],pos[ib],pos[ic])
        req(sum(cr[j]*normal[ia][j] for j in range(3))>0,"winding")
    marker=g["meshes"][1]["primitives"][0]; req(marker["material"]==1 and marker["mode"]==4,"marker primitive")
    mattrs=marker["attributes"]; req(set(mattrs)=={"POSITION","NORMAL","TANGENT","TEXCOORD_0"},"marker attrs")
    mpos=read_accessor(g,buf,mattrs["POSITION"]); mnormal=read_accessor(g,buf,mattrs["NORMAL"]); mtangent=read_accessor(g,buf,mattrs["TANGENT"]); muv=read_accessor(g,buf,mattrs["TEXCOORD_0"]); midx=read_accessor(g,buf,marker["indices"])
    req(len(mpos)==3 and len(midx)==3,"marker counts"); req([i[0] for i in midx]==[0,2,1],"marker indices")
    req(all(n==(0.0,0.0,1.0) for n in mnormal),"marker normals")
    req(all(t==(1.0,0.0,0.0,-1.0) for t in mtangent),"marker tangent handedness")
    req(muv==[(0.0,1.0),(0.0,0.0),(1.0,0.5)],"marker uv")
    cr=tri_normal(mpos[midx[0][0]],mpos[midx[1][0]],mpos[midx[2][0]])
    req(cr[2]>0 and sum(cr[j]*mnormal[midx[0][0]][j] for j in range(3))>0,"marker winding")
    req(max(x for x,y,z in mpos)>0.3 and all(z>1.0 for x,y,z in mpos),"marker handedness/front")
    req(g["materials"][0]["pbrMetallicRoughness"]["baseColorTexture"]=={"index":0,"texCoord":0},"texture binding")
    req(g["samplers"]==[{"magFilter":9728,"minFilter":9728,"wrapS":33071,"wrapT":33071}],"sampler clamp")
    if manifest_path:
        manifest_raw=Path(manifest_path).read_bytes(); m=json.loads(manifest_raw); rec=m["files"][0]
        req(rec["path"]==path.name and rec["bytes"]==len(raw) and rec["sha256"]==hashlib.sha256(raw).hexdigest(),"manifest")
        req(m["runtime_status"]=="source_validated_not_imported","manifest status")
        if expected_manifest_path:
            req(manifest_raw==Path(expected_manifest_path).read_bytes(),"expected manifest pin")
    elif expected_manifest_path:
        raise ValueError("expected manifest requires generated manifest")
    return {"block_vertices":24,"block_indices":36,"marker_vertices":3}
def main():
    p=argparse.ArgumentParser();p.add_argument("gltf",type=Path);p.add_argument("--manifest",type=Path);p.add_argument("--expected-manifest",type=Path);a=p.parse_args()
    try: print("PASS:",verify(a.gltf,a.manifest,a.expected_manifest))
    except (ValueError,KeyError,IndexError,TypeError,OSError,json.JSONDecodeError,struct.error,zlib.error,base64.binascii.Error) as e: p.exit(1,f"FAIL: {e}\n")
if __name__=="__main__": main()
