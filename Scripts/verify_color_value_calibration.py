"""Bounded independent verifier for Astral color/value calibration references."""
from __future__ import annotations
import argparse, hashlib, json, struct, zlib
from pathlib import Path

MAX_FILE=2*1024*1024
MAX_WIDTH=4096
MAX_HEIGHT=4096
MAX_INFLATED=2*1024*1024

def req(x,msg):
    if not x: raise ValueError(msg)

def read_png(path:Path, expected_dims:tuple[int,int]|None=None):
    data=path.read_bytes(); req(len(data)<=MAX_FILE,"oversized png"); req(data[:8]==b"\x89PNG\r\n\x1a\n","png signature")
    off=8; chunks=[]; idat=b""; w=h=None
    while off<len(data):
        req(off+12<=len(data),"png truncated"); n=struct.unpack(">I",data[off:off+4])[0]; end=off+12+n; req(end<=len(data),"png chunk bounds")
        k=data[off+4:off+8]; body=data[off+8:off+8+n]; crc=struct.unpack(">I",data[off+8+n:end])[0]; req(crc==(zlib.crc32(k+body)&0xffffffff),"png crc")
        chunks.append(k)
        if k==b"IHDR":
            req(n==13 and len(chunks)==1,"png ihdr"); w,h,depth,color,comp,filt,inter=struct.unpack(">IIBBBBB",body)
            req((depth,color,comp,filt,inter)==(8,2,0,0,0),"png format")
            req(0<w<=MAX_WIDTH and 0<h<=MAX_HEIGHT,"png dimensions")
            if expected_dims is not None: req((w,h)==expected_dims,"png dimensions")
            expected=h*(1+3*w); req(expected<=MAX_INFLATED,"png inflate limit")
        elif k==b"IDAT":
            req(w is not None,"png idat before ihdr"); idat+=body; req(len(idat)<=MAX_FILE,"png compressed data")
        elif k==b"IEND": req(n==0 and end==len(data),"png end")
        else: raise ValueError("unexpected png chunk")
        off=end
    req(chunks and chunks[0]==b"IHDR" and chunks[-1]==b"IEND" and b"IDAT" in chunks,"png chunk order")
    expected=h*(1+3*w); dec=zlib.decompressobj(); raw=dec.decompress(idat,expected+1)
    req(len(raw)==expected and dec.eof and not dec.unused_data and not dec.unconsumed_tail,"png inflate bounds")
    req(all(raw[y*(1+3*w)]==0 for y in range(h)),"png filter")
    pixels=[]
    for y in range(h):
        row=raw[y*(1+3*w)+1:(y+1)*(1+3*w)]
        pixels.append([tuple(row[x*3:x*3+3]) for x in range(w)])
    return w,h,pixels,data

def rgb(h):
    h=h.lstrip("#"); req(len(h)==6,"hex color")
    try: return tuple(int(h[i:i+2],16) for i in (0,2,4))
    except ValueError as e: raise ValueError("hex color") from e

def lin(c):
    c=c/255.0; return c/12.92 if c<=0.04045 else ((c+0.055)/1.055)**2.4

def lum(c): return 0.2126*lin(c[0])+0.7152*lin(c[1])+0.0722*lin(c[2])

def gray_from_lum(y):
    encoded=12.92*y if y<=0.0031308 else 1.055*(y**(1/2.4))-0.055
    v=max(0,min(255,round(encoded*255))); return (v,v,v)

def verify(root:Path,source:Path,expected_manifest:Path|None=None):
    root=root.resolve(); source_data=source.read_bytes(); src=json.loads(source_data); roles=src["roles"]; req(len(roles)==8,"roles")
    manifest_bytes=(root/"manifest.json").read_bytes(); manifest=json.loads(manifest_bytes); req(manifest["generator"]=="astral-color-calibration-2","generator")
    req(manifest["source_sha256"]==hashlib.sha256(source_data).hexdigest(),"source hash")
    if expected_manifest is not None: req(manifest_bytes==expected_manifest.read_bytes(),"expected manifest pin")
    listed={r["path"] for r in manifest["files"]}; req(listed=={"palette_card.png","neutral_lut_16.png","value_ramp_16.png"},"file list")
    for r in manifest["files"]:
        p=root/r["path"]; req(p.is_file() and not p.is_symlink(),"file missing/link"); d=p.read_bytes(); req(len(d)==r["bytes"] and hashlib.sha256(d).hexdigest()==r["sha256"],"file hash")
    _,_,pal,_=read_png(root/"palette_card.png",(512,256))
    colors=[rgb(r["hex"]) for r in roles]; grays=[gray_from_lum(lum(c)) for c in colors]
    for y in range(256):
        for x in range(512):
            if y<128: expected=colors[min(7,x//64)]
            elif x<256: expected=grays[min(7,x//32)]
            else:
                s=min(15,(x-256)//16); v=round(s*255/15); expected=(v,v,v)
            req(pal[y][x]==expected,"palette semantic pixel")
    _,_,lut,_=read_png(root/"neutral_lut_16.png",(256,16)); q=lambda n: round(n*255/15)
    for b in range(16):
        for g in range(16):
            for r in range(16): req(lut[g][r+16*b]==(q(r),q(g),q(b)),"neutral lut")
    _,_,ramp,_=read_png(root/"value_ramp_16.png",(256,64))
    for y in range(64):
        for x in range(256):
            s=min(15,x//16); v=round(s*255/15); req(ramp[y][x]==(v,v,v),"value ramp semantic pixel")
    lums={r["id"]:lum(rgb(r["hex"])) for r in roles}
    expected_roles=[{"id":r["id"],"hex":r["hex"].upper(),"relative_luminance":round(lums[r["id"]],6),"preview_gray_srgb8":grays[i][0]} for i,r in enumerate(roles)]
    req(manifest["roles"]==expected_roles,"role manifest")
    req(manifest.get("grayscale_preview")=="linearize sRGB, compute relative luminance, then re-encode luminance as sRGB gray","grayscale note")
    expected_screening=[]
    for item in src["ui_screening_pairs"]:
        hi,lo=sorted((lums[item["foreground"]],lums[item["background"]]),reverse=True); ratio=(hi+0.05)/(lo+0.05)
        req(ratio+1e-9>=float(item["minimum_ratio"]),"ui screening contrast")
        expected_screening.append({
            "foreground":item["foreground"],
            "background":item["background"],
            "ratio":round(ratio,6),
            "minimum_ratio":item["minimum_ratio"],
        })
    req(manifest.get("ui_screening")==expected_screening,"ui screening manifest")
    return {"roles":8,"png_files":3,"ui_pairs":len(src["ui_screening_pairs"])}

def main():
    ap=argparse.ArgumentParser(); ap.add_argument("root",type=Path); ap.add_argument("--source",type=Path,required=True); ap.add_argument("--expected-manifest",type=Path)
    a=ap.parse_args()
    try: print("PASS:",verify(a.root,a.source,a.expected_manifest))
    except (OSError,ValueError,KeyError,TypeError,json.JSONDecodeError,zlib.error,struct.error) as e: ap.exit(1,"FAIL: "+str(e)+"\n")
if __name__=="__main__": main()
