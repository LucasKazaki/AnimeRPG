"""Generate deterministic Astral art-reference color/value calibration PNGs.
Standard library only. Outputs are review references, not runtime color-grading assets.
"""
from __future__ import annotations
import argparse, hashlib, json, struct, zlib
from pathlib import Path

VERSION="astral-color-calibration-2"


def canonical_text_bytes(path:Path):
    """Normalize text newlines for cross-platform hashing/pinning."""
    return path.read_text(encoding="utf-8").replace("\r\n","\n").replace("\r","\n").encode("utf-8")


def png_rgb8(width,height,pixel):
    def chunk(kind,data):
        return struct.pack(">I",len(data))+kind+data+struct.pack(">I",zlib.crc32(kind+data)&0xffffffff)
    raw=b"".join(b"\0"+bytes(c for x in range(width) for c in pixel(x,y)) for y in range(height))
    return b"\x89PNG\r\n\x1a\n"+chunk(b"IHDR",struct.pack(">IIBBBBB",width,height,8,2,0,0,0))+chunk(b"IDAT",zlib.compress(raw,9))+chunk(b"IEND",b"")


def rgb(h):
    h=h.lstrip("#")
    if len(h)!=6: raise ValueError("hex color")
    return tuple(int(h[i:i+2],16) for i in (0,2,4))


def lin(c):
    c=c/255.0
    return c/12.92 if c<=0.04045 else ((c+0.055)/1.055)**2.4


def relative_luminance(c):
    return 0.2126*lin(c[0])+0.7152*lin(c[1])+0.0722*lin(c[2])


def srgb_gray_from_linear_luminance(y):
    encoded=12.92*y if y<=0.0031308 else 1.055*(y**(1/2.4))-0.055
    v=max(0,min(255,round(encoded*255)))
    return (v,v,v)


def build(source_path:Path):
    source_bytes=canonical_text_bytes(source_path)
    source=json.loads(source_bytes.decode("utf-8"))
    if source.get("schema_version")!=1: raise ValueError("source schema")
    roles=source.get("roles")
    if not isinstance(roles,list) or len(roles)!=8: raise ValueError("exactly eight roles required")
    colors=[rgb(r["hex"]) for r in roles]
    ids=[r["id"] for r in roles]
    if len(set(ids))!=len(ids): raise ValueError("duplicate role")
    luminance={r["id"]:relative_luminance(c) for r,c in zip(roles,colors)}

    grays=[srgb_gray_from_linear_luminance(luminance[r["id"]]) for r in roles]
    def palette_pixel(x,y):
        if y<128:
            return colors[min(7,x//64)]
        if x<256:
            return grays[min(7,x//32)]
        step=min(15,(x-256)//16)
        v=round(step*255/15)
        return (v,v,v)

    def lut_pixel(x,y):
        b=x//16; r=x%16; g=y
        q=lambda n: round(n*255/15)
        return (q(r),q(g),q(b))

    def ramp_pixel(x,y):
        step=min(15,x//16); v=round(step*255/15); return (v,v,v)

    files={
        "palette_card.png": png_rgb8(512,256,palette_pixel),
        "neutral_lut_16.png": png_rgb8(256,16,lut_pixel),
        "value_ramp_16.png": png_rgb8(256,64,ramp_pixel),
    }
    contrast=[]
    by_id={r["id"]:r for r in roles}
    for pair in source.get("ui_screening_pairs",[]):
        fg,bg=pair["foreground"],pair["background"]
        if fg not in by_id or bg not in by_id: raise ValueError("screening role")
        hi,lo=sorted((luminance[fg],luminance[bg]),reverse=True)
        contrast.append({"foreground":fg,"background":bg,"ratio":round((hi+0.05)/(lo+0.05),6),"minimum_ratio":pair["minimum_ratio"]})
    manifest={
        "schema_version":1,"generator":VERSION,
        "status":"art_reference_source_validated_not_runtime",
        "source_sha256":hashlib.sha256(source_bytes).hexdigest(),
        "files":[{"path":p,"bytes":len(d),"sha256":hashlib.sha256(d).hexdigest()} for p,d in sorted(files.items())],
        "roles":[{"id":r["id"],"hex":r["hex"].upper(),"relative_luminance":round(luminance[r["id"]],6),
                  "preview_gray_srgb8":grays[i][0]} for i,r in enumerate(roles)],
        "ui_screening":contrast,
        "lut_layout":"16x16x16 neutral cube; x=R+16*B, y=G; diagnostic only",
        "grayscale_preview":"linearize sRGB, compute relative luminance, then re-encode luminance as sRGB gray",
        "color_note":"role hex values and PNG RGB are sRGB display references; do not treat as scene-linear shader constants"
    }
    return files,(json.dumps(manifest,indent=2,sort_keys=True)+"\n").encode()


def main():
    ap=argparse.ArgumentParser(); ap.add_argument("--source",type=Path,required=True); ap.add_argument("--output",type=Path,required=True); ap.add_argument("--check",action="store_true")
    a=ap.parse_args(); files,manifest=build(a.source); root=a.output
    expected={**files,"manifest.json":manifest}
    if a.check:
        if not root.is_dir(): ap.error("check requires existing output")
        actual={p.relative_to(root).as_posix() for p in root.rglob("*") if p.is_file()}
        if actual!=set(expected): ap.error("unexpected/missing files")
        for p,d in expected.items():
            if (root/p).read_bytes()!=d: ap.error("output differs: "+p)
    else:
        if root.exists(): ap.error("refusing existing output")
        root.mkdir(parents=True)
        for p,d in expected.items(): (root/p).write_bytes(d)
    print("PASS: 3 color/value calibration PNGs; "+VERSION)
if __name__=="__main__": main()
