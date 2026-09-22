"""Bounded independent verifier for Astral color/value calibration references."""
from __future__ import annotations
import argparse, hashlib, json, struct, zlib
from pathlib import Path

MAX_FILE=2*1024*1024

def req(x,msg):
    if not x: raise ValueError(msg)

def read_png(path:Path):
    data=path.read_bytes(); req(len(data)<=MAX_FILE,"oversized png"); req(data[:8]==b"\x89PNG\r\n\x1a\n","png signature")
    off=8; chunks=[]; idat=b""; w=h=None
    while off<len(data):
        req(off+12<=len(data),"png truncated"); n=struct.unpack(">I",data[off:off+4])[0]; end=off+12+n; req(end<=len(data),"png chunk bounds")
        k=data[off+4:off+8]; body=data[off+8:off+8+n]; crc=struct.unpack(">I",data[off+8+n:end])[0]; req(crc==(zlib.crc32(k+body)&0xffffffff),"png crc")
        chunks.append(k)
        if k==b"IHDR":
            req(n==13 and len(chunks)==1,"png ihdr"); w,h,depth,color,comp,filt,inter=struct.unpack(">IIBBBBB",body); req((depth,color,comp,filt,inter)==(8,2,0,0,0),"png format")
        elif k==b"IDAT": idat+=body
        elif k==b"IEND": req(n==0 and end==len(data),"png end")
        else: raise ValueError("unexpected png chunk")
        off=end
    req(chunks[0]==b"IHDR" and chunks[-1]==b"IEND" and b"IDAT" in chunks,"png chunk order")
    expected=h*(1+3*w); dec=zlib.decompressobj(); raw=dec.decompress(idat,expected+1)
    req(len(raw)==expected and dec.eof and not dec.unused_data and not dec.unconsumed_tail,"png inflate bounds")
    req(all(raw[y*(1+3*w)]==0 for y in range(h)),"png filter")
    pixels=[]
    for y in range(h):
        row=raw[y*(1+3*w)+1:(y+1)*(1+3*w)]
        pixels.append([tuple(row[x*3:x*3+3]) for x in range(w)])
    return w,h,pixels,data

def rgb(h): h=h.lstrip("#"); return tuple(int(h[i:i+2],16) for i in (0,2,4))
def lin(c):
    c=c/255.0; return c/12.92 if c<=0.04045 else ((c+0.055)/1.055)**2.4
def lum(c): return 0.2126*lin(c[0])+0.7152*lin(c[1])+0.0722*lin(c[2])

def verify(root:Path,source:Path,expected_manifest:Path|None=None):
    root=root.resolve(); source_data=source.read_bytes(); src=json.loads(source_data); roles=src["roles"]; req(len(roles)==8,"roles")
    manifest_bytes=(root/"manifest.json").read_bytes(); manifest=json.loads(manifest_bytes); req(manifest["generator"]=="astral-color-calibration-1","generator")
    req(manifest["source_sha256"]==hashlib.sha256(source_data).hexdigest(),"source hash")
    if expected_manifest is not None: req(manifest_bytes==expected_manifest.read_bytes(),"expected manifest pin")
    listed={r["path"] for r in manifest["files"]}; req(listed=={"palette_card.png","neutral_lut_16.png","value_ramp_16.png"},"file list")
    for r in manifest["files"]:
        p=root/r["path"]; req(p.is_file() and not p.is_symlink(),"file missing/link"); d=p.read_bytes(); req(len(d)==r["bytes"] and hashlib.sha256(d).hexdigest()==r["sha256"],"file hash")
    w,h,pal,_=read_png(root/"palette_card.png"); req((w,h)==(512,256),"palette dims")
    colors=[rgb(r["hex"]) for r in roles]
    for i,c in enumerate(colors):
        for sample in ((i*64+1,1),(i*64+63,127)): req(pal[sample[1]][sample[0]]==c,"palette swatch")
    # Exact 16-step sRGB value ladder in lower-right half.
    for s in range(16):
        v=round(s*255/15); req(pal[200][256+s*16+8]==(v,v,v),"palette value ladder")
    w,h,lut,_=read_png(root/"neutral_lut_16.png"); req((w,h)==(256,16),"lut dims")
    for b in range(16):
        for g in range(16):
            for r in range(16):
                q=lambda n: round(n*255/15); req(lut[g][r+16*b]==(q(r),q(g),q(b)),"neutral lut")
    w,h,ramp,_=read_png(root/"value_ramp_16.png"); req((w,h)==(256,64),"ramp dims")
    for s in range(16):
        v=round(s*255/15); req(ramp[17][s*16+3]==(v,v,v),"value ramp")
    # Recompute proposed UI screening ratios. This is a project screen, not a blanket game-accessibility claim.
    lums={r["id"]:lum(rgb(r["hex"])) for r in roles}
    for item in src["ui_screening_pairs"]:
        hi,lo=sorted((lums[item["foreground"]],lums[item["background"]]),reverse=True); ratio=(hi+0.05)/(lo+0.05)
        req(ratio+1e-9>=float(item["minimum_ratio"]),"ui screening contrast")
    return {"roles":8,"png_files":3,"ui_pairs":len(src["ui_screening_pairs"])}

def main():
    ap=argparse.ArgumentParser(); ap.add_argument("root",type=Path); ap.add_argument("--source",type=Path,required=True); ap.add_argument("--expected-manifest",type=Path)
    a=ap.parse_args()
    try: print("PASS:",verify(a.root,a.source,a.expected_manifest))
    except (OSError,ValueError,KeyError,TypeError,json.JSONDecodeError,zlib.error,struct.error) as e: ap.exit(1,"FAIL: "+str(e)+"\n")
if __name__=="__main__": main()
