"""Original Astral starter fixtures; Python 3.10+, standard library only.

Write to a NEW directory, or use --check on an existing generated pack. No
network, editor changes, asset downloads, or replacement of existing files.
Meshes are ASTRAL_MESH 1 wireframes, NOT textured triangle/skeletal meshes.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import zlib

VERSION = "astral-starter-1"


def box(w=1.0, h=1.0, d=1.0, x=0.0, y=0.0, z=0.0):
    v = [(x+a*w/2, y+b*h, z+c*d/2)
         for a, b, c in [(-1,0,-1),(1,0,-1),(1,0,1),(-1,0,1),
                         (-1,1,-1),(1,1,-1),(1,1,1),(-1,1,1)]]
    return v, [(0,1),(1,2),(2,3),(3,0),(4,5),(5,6),(6,7),(7,4),
               (0,4),(1,5),(2,6),(3,7)]


def join(*parts):
    vertices, edges = [], []
    for v, e in parts:
        offset = len(vertices)
        vertices.extend(v)
        edges.extend((a+offset, b+offset) for a, b in e)
    return vertices, edges


def lathe(profile, n=12):
    vertices, edges, rings = [], [], []
    for radius, height in profile:
        if radius == 0:
            ring = [len(vertices)]
            vertices.append((0.0, height, 0.0))
        else:
            ring = list(range(len(vertices), len(vertices)+n))
            vertices.extend((radius*math.cos(2*math.pi*i/n), height,
                             radius*math.sin(2*math.pi*i/n)) for i in range(n))
            edges.extend((ring[i], ring[(i+1)%n]) for i in range(n))
        if rings:
            prev = rings[-1]
            if len(prev) == 1:
                edges.extend((prev[0], b) for b in ring)
            elif len(ring) == 1:
                edges.extend((a, ring[0]) for a in prev)
            else:
                edges.extend(zip(prev, ring))
        rings.append(ring)
    return vertices, edges


def meshes():
    sphere = lathe([(0,0)] + [(0.5*math.sin(math.pi*j/6),
                              0.5-0.5*math.cos(math.pi*j/6))
                             for j in range(1,6)] + [(0,1)])
    ramp = ([(-.5,0,-.5),(.5,0,-.5),(.5,0,.5),(-.5,0,.5),
             (.5,1,.5),(-.5,1,.5)],
            [(0,1),(1,2),(2,3),(3,0),(2,4),(3,5),(4,5),(0,5),(1,4)])
    plane = ([(-.5,0,-.5),(.5,0,-.5),(.5,0,.5),(-.5,0,.5)],
             [(0,1),(1,2),(2,3),(3,0)])
    arch_v = [(r*math.cos(math.pi*i/12), 1+ r*math.sin(math.pi*i/12), z)
              for z in (-.15,.15) for r in (1.0,1.25) for i in range(13)]
    arch_e = [(k+i,k+i+1) for k in (0,13,26,39) for i in range(12)]
    arch_e += [(i,i+13) for i in range(13)] + [(i+26,i+39) for i in range(13)]
    arch_e += [(i,i+26) for i in range(26)]
    legs = lambda h: [box(.1,h,.1,x=x,z=z) for x in (-.4,.4) for z in (-.3,.3)]
    result = {
        "cube": box(), "plane": plane, "sphere": sphere,
        "cylinder": lathe([(.5,0),(.5,1)]),
        "cone": lathe([(.5,0),(0,1)]),
        "capsule": lathe([(0,0),(.18,.08),(.25,.25),(.25,1.25),(.18,1.42),(0,1.5)]),
        "ramp": ramp, "pyramid": lathe([(.707107,0),(0,1)],4),
        "octahedron": lathe([(0,0),(.5,.5),(0,1)],4),
        "disc": lathe([(.5,0)]),
        "floor_4m": box(4,.2,4,y=-.2),
        "wall_4m": box(4,3,.2),
        "doorway_4m": join(box(1.4,3,.2,x=-1.3),box(1.4,3,.2,x=1.3),
                            box(1.2,.8,.2,y=2.2)),
        "window_wall": join(box(1,3,.2,x=-1.5),box(1,3,.2,x=1.5),
                             box(2,1,.2),box(2,.5,.2,y=2.5)),
        "square_pillar": box(.5,3,.5),
        "round_pillar": lathe([(.3,0),(.3,3)]),
        "beam_4m": box(4,.25,.25),
        "stairs_4": join(*(box(2,.25*(i+1),.4,z=i*.4) for i in range(4))),
        "stairs_8": join(*(box(2,.2*(i+1),.3,z=i*.3) for i in range(8))),
        "arch": join((arch_v,arch_e),box(.25,1,.3,x=-1.125),box(.25,1,.3,x=1.125)),
        "round_platform": lathe([(1,0),(1,.25)]),
        "fence_4m": join(box(.15,1.2,.15,x=-2),box(.15,1.2,.15,x=2),
                          box(4,.12,.1,y=.35),box(4,.12,.1,y=.9)),
        "crate": join(box(),box(1.08,.1,1.08,y=.15),box(1.08,.1,1.08,y=.75)),
        "barrel": lathe([(.35,0),(.43,.2),(.45,.5),(.43,.8),(.35,1)]),
        "table": join(box(1,.1,.8,y=.7),*legs(.7)),
        "chair": join(box(.5,.08,.5,y=.45),box(.5,.55,.08,y=.45,z=.21),
                       *(box(.06,.45,.06,x=x,z=z) for x in (-.2,.2) for z in (-.2,.2))),
        "bench": join(box(1.8,.1,.5,y=.45),box(.12,.45,.5,x=-.6),box(.12,.45,.5,x=.6)),
        "pedestal": join(box(1,.12,1),box(.5,.8,.5,y=.12),box(.8,.12,.8,y=.92)),
        "bollard": lathe([(.12,0),(.12,.9),(0,1)]),
        "training_dummy": join(box(.45,.65,.25,y=.75),box(.28,.3,.28,y=1.4),
                                box(.12,.75,.12),box(1,.12,.12,y=1.15)),
        "axis_marker": ([(0,0,0),(1,0,0),(0,1,0),(0,0,1)],[(0,1),(0,2),(0,3)]),
        "grid_10m": ([(float(i),0.0,z) for i in range(-5,6) for z in (-5.0,5.0)] +
                       [(x,0.0,float(i)) for i in range(-5,6) for x in (-5.0,5.0)],
                       [(i,i+1) for i in range(0,44,2)]),
    }
    return result


def png(width, height, pixel):
    def chunk(kind, data):
        return struct.pack(">I",len(data))+kind+data+struct.pack(">I",zlib.crc32(kind+data)&0xffffffff)
    raw = b"".join(b"\0" + bytes(c for x in range(width) for c in pixel(x,y))
                   for y in range(height))
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR",struct.pack(">IIBBBBB",width,height,8,2,0,0,0)) +
            chunk(b"IDAT",zlib.compress(raw,9)) + chunk(b"IEND",b""))


# Name, unlit sRGB base color, linear roughness byte, linear metallic byte, pattern.
MATERIALS = [
    ("plaster",(205,198,181),210,0,"plain"),
    ("concrete",(128,132,132),220,0,"grid"),
    ("brick",(145,76,54),220,0,"brick"),
    ("limestone",(210,197,161),200,0,"brick"),
    ("ceramic_tile",(220,224,218),70,0,"grid"),
    ("wood_planks",(144,99,53),170,0,"plank"),
    ("painted_wood",(56,110,128),180,0,"plank"),
    ("steel",(190,195,200),90,255,"plain"),
    ("brushed_metal",(175,178,180),140,255,"stripe"),
    ("rubber",(30,32,35),235,0,"plain"),
    ("fabric",(102,72,115),235,0,"weave"),
    ("soil",(96,72,45),245,0,"weave"),
    ("grass_proxy",(76,115,52),240,0,"stripe"),
    ("sand",(199,177,125),230,0,"plain"),
    ("calibration_gray",(118,118,118),128,0,"plain"),
    ("missing_material",(220,0,200),255,0,"checker"),
]


def build_pack():
    files, records = {}, []
    def add(path, data, **metadata):
        files[path] = data
        records.append(dict(path=path, bytes=len(data), sha256=hashlib.sha256(data).hexdigest(), **metadata))
    for name, (vertices, edges) in meshes().items():
        text = "ASTRAL_MESH 1\n"
        text += "".join("vertex " + " ".join(f"{v:.6f}" for v in p) + "\n" for p in vertices)
        text += "".join(f"edge {a} {b}\n" for a,b in edges)
        add(f"Meshes/{name}.mesh",text.encode(), kind="wireframe_mesh", vertices=len(vertices),
            edges=len(edges), bounds_min=[round(min(p[i] for p in vertices),6) for i in range(3)],
            bounds_max=[round(max(p[i] for p in vertices),6) for i in range(3)],
            runtime_status="loader_compatible_not_editor_installed")
    def surface_pixel(x,y):
        _, color, _, _, pattern = MATERIALS[(y//64)*4+x//64]
        u,v=x%64,y%64
        seam = {"plain":False,"grid":u%32<2 or v%32<2,
                "brick":v%16<2 or (u+(16 if (v//16)%2 else 0))%32<2,
                "plank":u%16<2,"stripe":u%8<2,"weave":(u%4<1) != (v%4<1),
                "checker":((u//8)+(v//8))%2==0}[pattern]
        return tuple(max(0,c-35) if seam else c for c in color)
    textures = {
        "surface_basecolor_atlas.png": (256,256,surface_pixel,"srgb"),
        "flat_normal.png": (16,16,lambda x,y:(128,128,255),"linear"),
        "checker_grid.png": (64,64,lambda x,y:((210,210,210) if (x//8+y//8)%2 else (65,65,65)),"srgb"),
        "uv_orientation.png": (64,64,lambda x,y:(255 if x>=32 else 40,255 if y>=32 else 40,255 if (x<2 or y<2) else 40),"srgb"),
    }
    # ORM = occlusion, roughness, metallic (not an engine material implementation).
    textures["surface_orm_atlas.png"] = (256,256,lambda x,y:(255,MATERIALS[(y//64)*4+x//64][2],
                                                         MATERIALS[(y//64)*4+x//64][3]),"linear")
    for name,(w,h,fn,space) in textures.items():
        add(f"Textures/{name}",png(w,h,fn),kind="texture_source",width=w,height=h,color_space=space,
            runtime_status="blocked_no_texture_renderer")
    materials = [dict(id=name,atlas_pixel_rect=[(i%4)*64,(i//4)*64,64,64],
                      base_color="Textures/surface_basecolor_atlas.png",
                      orm="Textures/surface_orm_atlas.png",normal="Textures/flat_normal.png",
                      runtime_status="source_recipe_only",texture_origin="top_left",
                      normal_convention="tangent_space_positive_y",alpha_mode="opaque",
                      note="Crop each tile before repeat/mipmap use; atlas has no gutters.")
                 for i,(name,*_) in enumerate(MATERIALS)]
    add("materials.json",(json.dumps(materials,indent=2)+"\n").encode(),kind="material_recipes")
    manifest = dict(schema_version=1,generator=VERSION,
        provenance="Original procedural fixtures; no Epic or third-party assets copied.",
        license="No new public redistribution license granted; retain repository ownership policy.",
        coordinates=dict(up="+Y",units="1 unit = 1 metre for this pack only; confirm import transform",
                         pivot="See bounds; mostly ground-centred; floor top at zero; stairs start at z=-depth/2"),
        scope="Engine test/tutorial fixtures. Not production art or UE5 parity.",
        counts=dict(meshes=32,texture_files=5,material_recipes=16),files=records)
    files["manifest.json"] = (json.dumps(manifest,indent=2)+"\n").encode()
    return files


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output",type=Path,required=True)
    parser.add_argument("--check",action="store_true",help="Check exact regenerated bytes; never write")
    args=parser.parse_args()
    root=args.output
    if any(p.is_symlink() for p in (root,*root.parents)):
        parser.error("Symlink output roots/ancestors are not allowed")
    files=build_pack()
    if args.check:
        if not root.is_dir():
            parser.error("Check requires an existing pack directory")
        actual={p.relative_to(root).as_posix() for p in root.rglob("*") if p.is_file()}
        if actual != set(files) or any(p.is_symlink() for p in root.rglob("*")):
            parser.error("Unexpected, missing, or symlink files in pack")
        if any((root/p).read_bytes()!=data for p,data in files.items()):
            parser.error("Pack differs from generator (including possible zlib encoder differences)")
    else:
        if root.exists():
            parser.error("Refusing to replace existing output; use a new directory or --check")
        root.mkdir(parents=True)
        for path,data in files.items():
            dest=root/path
            dest.parent.mkdir(parents=True,exist_ok=True)
            with dest.open("xb") as f:
                f.write(data)
    print(f"PASS: {len(files)} files; 32 meshes, 5 PNGs, 16 material recipes; {VERSION}")


if __name__ == "__main__":
    main()
