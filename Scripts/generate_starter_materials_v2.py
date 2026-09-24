"""Generate Astral starter material set v2.

Standard-library only. Produces original, deterministic, standalone tileable
source textures for calibration and future import tests. It does not install or
render materials in Astral.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import zlib

SIZE = 512
VERSION = "astral-materials-v2-2"
TAU = math.tau

MATERIALS = {
    "limestone": {"base": (186, 178, 155), "rough": 195, "metal": 0, "height": "stone"},
    "concrete": {"base": (128, 132, 132), "rough": 218, "metal": 0, "height": "concrete"},
    "brushed_steel": {"base": (174, 179, 184), "rough": 92, "metal": 255, "height": "steel"},
    "asphalt": {"base": (43, 45, 47), "rough": 236, "metal": 0, "height": "asphalt"},
}


def clamp(v, lo=0.0, hi=1.0):
    return lo if v < lo else hi if v > hi else v


def wave(u, v, terms):
    total = 0.0
    for amp, fx, fy, phase in terms:
        total += amp * math.sin(TAU * (fx*u + phase)) * math.cos(TAU * (fy*v + phase*0.73))
    return total


def height_value(kind, u, v):
    if kind == "stone":
        n = wave(u, v, [(0.22,1,2,.11),(0.12,3,2,.37),(0.06,7,5,.23),(0.03,17,13,.47)])
        veins = 0.06 * math.sin(TAU*(3*u + 1.2*math.sin(TAU*2*v)))
        return clamp(0.52 + n + veins)
    if kind == "concrete":
        n = wave(u, v, [(0.20,2,3,.17),(0.11,5,7,.41),(0.07,13,11,.29),(0.035,29,31,.07)])
        pores = -0.08 * max(0.0, math.cos(TAU*19*u)*math.cos(TAU*23*v))**8
        return clamp(0.50 + n + pores)
    if kind == "steel":
        brushed = 0.035*math.sin(TAU*64*v) + 0.02*math.sin(TAU*113*v + 0.4*math.sin(TAU*3*u))
        broad = wave(u, v, [(0.025,1,2,.21),(0.015,3,1,.31)])
        return clamp(0.5 + brushed + broad)
    if kind == "asphalt":
        n = wave(u, v, [(0.17,3,5,.19),(0.12,7,9,.31),(0.08,17,13,.43),(0.05,37,29,.07)])
        aggregate = 0.035*math.sin(TAU*53*u)*math.sin(TAU*47*v)
        return clamp(0.48 + n + aggregate)
    raise KeyError(kind)


def png_rgb(width, height, pixel_fn):
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind+data) & 0xffffffff)
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        for x in range(width):
            raw.extend(pixel_fn(x, y))
    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(bytes(raw), 9))
        + chunk(b"IEND", b"")
    )


def build_material(name, spec):
    # Sample one complete periodic domain over [0, 1). Do not duplicate u/v=0
    # at the final texel, which would create a one-texel plateau at repeats.
    heights = [[0.0] * SIZE for _ in range(SIZE)]
    for y in range(SIZE):
        v = y / SIZE
        row = heights[y]
        for x in range(SIZE):
            row[x] = height_value(spec["height"], x / SIZE, v)

    base = spec["base"]
    rough0 = spec["rough"]
    metal = spec["metal"]

    def base_px(x, y):
        hh = heights[y][x]
        variation = int(round((hh - 0.5) * (26 if name != "brushed_steel" else 14)))
        if name == "asphalt":
            variation = int(round((hh - 0.5) * 34))
        return tuple(max(0, min(255, c + variation)) for c in base)

    def height_px(x, y):
        q = int(round(clamp(heights[y][x]) * 255))
        return (q, q, q)

    # Positive-Y tangent-space convention. Height changes are centered and wrapped
    # over all SIZE unique texels, including derivatives across the repeat boundary.
    def normal_px(x, y):
        xl = (x - 1) % SIZE
        xr = (x + 1) % SIZE
        yu = (y - 1) % SIZE
        yd = (y + 1) % SIZE
        dx = heights[y][xr] - heights[y][xl]
        dy = heights[yd][x] - heights[yu][x]
        strength = 7.0 if name in ("concrete", "asphalt") else 5.0
        nx, ny, nz = -dx * strength, -dy * strength, 1.0
        inv = 1.0 / math.sqrt(nx*nx + ny*ny + nz*nz)
        nx, ny, nz = nx*inv, ny*inv, nz*inv
        return (
            int(round((nx*0.5+0.5)*255)),
            int(round((ny*0.5+0.5)*255)),
            int(round((nz*0.5+0.5)*255)),
        )

    def orm_px(x, y):
        hh = heights[y][x]
        # AO is a mild micro-cavity cue only. No directional lighting is baked.
        ao = int(round(255 * clamp(0.90 + 0.10*hh)))
        rough = int(round(clamp((rough0 + (0.5-hh)*36) / 255.0) * 255))
        return (ao, rough, metal)

    return {
        "basecolor": png_rgb(SIZE, SIZE, base_px),
        "normal": png_rgb(SIZE, SIZE, normal_px),
        "orm": png_rgb(SIZE, SIZE, orm_px),
        "height": png_rgb(SIZE, SIZE, height_px),
    }


def build_pack():
    files = {}
    records = []
    for name, spec in MATERIALS.items():
        maps = build_material(name, spec)
        for map_name, data in maps.items():
            rel = f"{name}/{name}_{map_name}.png"
            color_space = "srgb" if map_name == "basecolor" else "linear"
            channels = {
                "basecolor": "RGB base color; no directional lighting",
                "normal": "RGB tangent-space normal; positive-Y convention",
                "orm": "R=occlusion, G=roughness, B=metallic",
                "height": "RGB replicated grayscale authoring height",
            }[map_name]
            files[rel] = data
            records.append({
                "path": rel,
                "bytes": len(data),
                "sha256": hashlib.sha256(data).hexdigest(),
                "kind": map_name,
                "width": SIZE,
                "height": SIZE,
                "color_space": color_space,
                "channels": channels,
                "tileable": True,
                "runtime_status": "source_validated_not_imported",
            })
    manifest = {
        "schema_version": 1,
        "generator": VERSION,
        "scope": "Original standalone material sources for calibration and future Astral import/render tests.",
        "provenance": "Procedural standard-library generation; no third-party texture pixels.",
        "license": "No new public redistribution license granted; preserve repository ownership policy.",
        "texture_origin": "top_left",
        "normal_convention": "tangent_space_positive_y",
        "packing": "ORM: R occlusion, G roughness, B metallic",
        "periodic_sampling": "unique_texels_[0,1)_repeat_wrap",
        "resolution": [SIZE, SIZE],
        "materials": list(MATERIALS),
        "counts": {"materials": len(MATERIALS), "png_files": len(records)},
        "files": records,
    }
    files["manifest.json"] = (json.dumps(manifest, indent=2) + "\n").encode("utf-8")
    return files


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--check", action="store_true")
    args = p.parse_args()
    files = build_pack()
    root = args.output

    if args.check:
        if not root.is_dir():
            p.error("--check requires an existing directory")
        actual = {x.relative_to(root).as_posix() for x in root.rglob("*") if x.is_file()}
        if actual != set(files):
            p.error("pack file set differs from generator")
        for rel, data in files.items():
            if (root / rel).read_bytes() != data:
                p.error(f"pack differs at {rel}")
    else:
        if root.exists():
            p.error("refusing to overwrite existing output")
        root.mkdir(parents=True)
        for rel, data in files.items():
            dest = root / rel
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(data)
    print(f"PASS: {len(MATERIALS)} materials, {len(MATERIALS)*4} PNGs, {VERSION}")


if __name__ == "__main__":
    main()
