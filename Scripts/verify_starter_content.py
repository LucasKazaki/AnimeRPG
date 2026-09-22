"""Independent, bounded validation of the v1 original Astral starter pack."""
from __future__ import annotations
import argparse
import hashlib
import json
import math
from pathlib import Path, PurePosixPath
import struct
import subprocess
import zlib


def require(condition, message):
    if not condition:
        raise ValueError(message)


def safe_file(root, relative):
    require(isinstance(relative,str),"Non-string path")
    p=PurePosixPath(relative)
    require(relative and not p.is_absolute() and "\\" not in relative and ":" not in relative
            and all(x not in (".","..","") for x in relative.split("/")),"Unsafe path")
    target=root.joinpath(*p.parts)
    require(not any(x.is_symlink() for x in (target,*target.parents)),"Symlink path")
    require(target.is_file() and target.stat().st_size <= 16*1024*1024,"Missing/oversized file")
    return target


def read_png(data):
    require(data[:8]==b"\x89PNG\r\n\x1a\n","PNG signature")
    offset=8; chunks=[]; payload=b""
    while offset<len(data):
        require(offset+12<=len(data),"Truncated PNG chunk")
        n=struct.unpack(">I",data[offset:offset+4])[0]
        end=offset+12+n
        require(end<=len(data),"PNG length")
        kind=data[offset+4:offset+8]; body=data[offset+8:offset+8+n]
        crc=struct.unpack(">I",data[offset+8+n:end])[0]
        require(crc==zlib.crc32(kind+body)&0xffffffff,"PNG CRC")
        require(kind in (b"IHDR",b"IDAT",b"IEND"),"Unexpected PNG chunk")
        chunks.append(kind)
        if kind==b"IHDR":
            require(len(chunks)==1 and n==13,"PNG header")
            w,h,depth,color,compression,filtering,interlace=struct.unpack(">IIBBBBB",body)
            require(0<w<=4096 and 0<h<=4096 and (depth,color,compression,filtering,interlace)==(8,2,0,0,0),"PNG format")
        if kind==b"IDAT": payload+=body
        if kind==b"IEND": require(n==0 and end==len(data),"PNG end")
        offset=end
    require(chunks==[b"IHDR",b"IDAT",b"IEND"],"PNG chunk order")
    expected=h*(1+3*w)
    decoder=zlib.decompressobj()
    raw=decoder.decompress(payload,expected+1)
    require(len(raw)==expected and decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail,"PNG inflated size")
    require(all(raw[y*(1+3*w)]==0 for y in range(h)),"PNG fixture filter")
    return w,h


def verify(root, probe=None):
    root=Path(root).absolute()
    require(root.is_dir() and not any(p.is_symlink() for p in (root,*root.parents)),"Pack root")
    manifest=json.loads(safe_file(root,"manifest.json").read_text())
    require(manifest["schema_version"]==1 and manifest["generator"]=="astral-starter-1","Manifest version")
    records=manifest["files"]
    paths=[r["path"] for r in records]
    require(len(paths)==len(set(paths)) and "manifest.json" not in paths,"Duplicate/self-listed manifest paths")
    actual={p.relative_to(root).as_posix() for p in root.rglob("*") if p.is_file()}
    require(not any(p.is_symlink() for p in root.rglob("*")),"Pack symlink")
    require(actual==set(paths)|{"manifest.json"},"Unexpected or missing pack files")
    counts={"meshes":0,"texture_files":0,"material_recipes":0}
    mesh_paths=[]; mesh_counts=[]
    for r in records:
        file=safe_file(root,r["path"]); data=file.read_bytes()
        require(len(data)==r["bytes"] and hashlib.sha256(data).hexdigest()==r["sha256"],"Hash/size mismatch: "+r["path"])
        if r["kind"]=="wireframe_mesh":
            lines=data.decode("ascii").splitlines()
            require(lines[0]=="ASTRAL_MESH 1","Mesh header")
            vertices=[]; edges=[]
            for line in lines[1:]:
                words=line.split()
                require(bool(words),"Empty mesh record")
                if words[0]=="vertex":
                    require(len(words)==4,"Vertex arity")
                    v=tuple(float(x) for x in words[1:])
                    require(all(math.isfinite(x) and abs(x)<=10000 for x in v),"Vertex bounds")
                    vertices.append(v)
                else:
                    require(words[0]=="edge" and len(words)==3 and all(x.isascii() and x.isdigit() for x in words[1:]),"Edge record")
                    edges.append(tuple(int(x) for x in words[1:]))
            require(len(vertices)==r["vertices"] and len(edges)==r["edges"] and vertices and edges,"Mesh counts")
            require(len(vertices)<=262144 and len(edges)<=524288,"Loader budgets")
            seen=set()
            for a,b in edges:
                require(0<=a<len(vertices) and 0<=b<len(vertices) and a!=b,"Edge indices")
                require(vertices[a]!=vertices[b],"Zero-length geometric edge")
                pair=tuple(sorted((a,b))); require(pair not in seen,"Duplicate edge"); seen.add(pair)
            for i in range(3):
                require(abs(min(v[i] for v in vertices)-r["bounds_min"][i])<1e-5 and
                        abs(max(v[i] for v in vertices)-r["bounds_max"][i])<1e-5,"Mesh bounding box")
            require(r["runtime_status"]=="loader_compatible_not_editor_installed","False mesh integration claim")
            counts["meshes"]+=1;mesh_paths.append(str(file));mesh_counts.append((len(vertices),len(edges)))
        elif r["kind"]=="texture_source":
            require(read_png(data)==(r["width"],r["height"]),"PNG dimensions")
            require(r["color_space"] in ("srgb","linear") and r["runtime_status"]=="blocked_no_texture_renderer","Texture contract")
            counts["texture_files"]+=1
        elif r["kind"]=="material_recipes":
            materials=json.loads(data)
            require(len({m["id"] for m in materials})==len(materials),"Duplicate material ID")
            for m in materials:
                for key in ("base_color","orm","normal"):
                    require(m[key] in paths,"Untracked material texture");safe_file(root,m[key])
                rect=m["atlas_pixel_rect"]
                require(len(rect)==4 and all(type(x) is int for x in rect),"Atlas rectangle type")
                x,y,w,h=rect
                require(w==h==64 and x>=0 and y>=0 and x+w<=256 and y+h<=256,"Atlas rectangle")
                require(m["runtime_status"]=="source_recipe_only","False material integration claim")
            counts["material_recipes"]=len(materials)
        else:
            raise ValueError("Unknown record kind")
    require(counts==manifest["counts"]==dict(meshes=32,texture_files=5,material_recipes=16),"Pack counts")
    if probe is not None:
        result=subprocess.run([str(Path(probe).resolve()),*mesh_paths],capture_output=True,text=True,timeout=15,check=False)
        require(result.returncode==0,"Actual engine loader failed: "+result.stderr)
        observed=[tuple(map(int,line.split())) for line in result.stdout.splitlines()]
        require(observed==mesh_counts,"Actual loader count mismatch")
    return counts


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("pack",type=Path);p.add_argument("--probe",type=Path)
    args=p.parse_args()
    try:
        print("PASS:",verify(args.pack,args.probe))
    except (ValueError,KeyError,TypeError,OSError,UnicodeError,struct.error,zlib.error,subprocess.SubprocessError) as exc:
        p.exit(1,f"FAIL: {exc}\n")


if __name__=="__main__": main()
