"""Validate Astral starter material set v2 without external dependencies."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import struct
import zlib

EXPECTED_MATERIALS = {"limestone", "concrete", "brushed_steel", "asphalt"}

def require(cond, msg):
    if not cond:
        raise ValueError(msg)

def safe_path(root, rel):
    require(isinstance(rel, str) and rel and "\\" not in rel and ":" not in rel, "unsafe path")
    p = PurePosixPath(rel)
    require(not p.is_absolute() and all(part not in ("", ".", "..") for part in p.parts), "unsafe path")
    f = root.joinpath(*p.parts)
    require(f.is_file() and not f.is_symlink(), "missing/symlink file")
    require(f.stat().st_size <= 8*1024*1024, "oversized file")
    return f

def decode_png(data):
    require(data[:8] == b"\x89PNG\r\n\x1a\n", "PNG signature")
    off = 8
    width = height = None
    idat = bytearray()
    kinds = []
    while off < len(data):
        require(off + 12 <= len(data), "truncated chunk")
        n = struct.unpack(">I", data[off:off+4])[0]
        end = off + 12 + n
        require(end <= len(data), "chunk overflow")
        kind = data[off+4:off+8]
        body = data[off+8:off+8+n]
        crc = struct.unpack(">I", data[off+8+n:end])[0]
        require(crc == (zlib.crc32(kind+body) & 0xffffffff), "CRC")
        kinds.append(kind)
        if kind == b"IHDR":
            require(width is None and n == 13, "IHDR")
            width, height, depth, color, comp, filt, interlace = struct.unpack(">IIBBBBB", body)
            require((depth,color,comp,filt,interlace) == (8,2,0,0,0), "PNG format")
        elif kind == b"IDAT":
            idat.extend(body)
        elif kind == b"IEND":
            require(n == 0 and end == len(data), "IEND")
        else:
            raise ValueError("unexpected PNG chunk")
        off = end
    require(kinds == [b"IHDR", b"IDAT", b"IEND"], "chunk order")
    require(width and height, "dimensions")
    expected = height * (1 + width*3)
    raw = zlib.decompress(bytes(idat))
    require(len(raw) == expected, "inflated size")
    pixels = []
    stride = 1 + width*3
    for y in range(height):
        row = raw[y*stride:(y+1)*stride]
        require(row[0] == 0, "unsupported filter")
        pixels.append(row[1:])
    return width, height, pixels

def verify(root, expected_manifest=None):
    root = Path(root).resolve()
    require(root.is_dir(), "pack root")
    manifest = json.loads((root/"manifest.json").read_text())
    if expected_manifest is not None:
        expected = json.loads(Path(expected_manifest).read_text())
        require(manifest == expected, "manifest differs from pinned expected manifest")
    require(manifest["schema_version"] == 1 and manifest["generator"] == "astral-materials-v2-1", "manifest version")
    require(set(manifest["materials"]) == EXPECTED_MATERIALS, "materials")
    require(manifest["counts"] == {"materials": 4, "png_files": 16}, "counts")
    require(manifest["texture_origin"] == "top_left", "origin")
    require(manifest["normal_convention"] == "tangent_space_positive_y", "normal convention")
    records = manifest["files"]
    require(len(records) == 16, "record count")
    paths = [r["path"] for r in records]
    require(len(set(paths)) == 16, "duplicate paths")
    actual = {p.relative_to(root).as_posix() for p in root.rglob("*") if p.is_file()}
    require(actual == set(paths) | {"manifest.json"}, "unexpected/missing files")

    by_material = {m: set() for m in EXPECTED_MATERIALS}
    for r in records:
        f = safe_path(root, r["path"])
        data = f.read_bytes()
        require(len(data) == r["bytes"], "byte count")
        require(hashlib.sha256(data).hexdigest() == r["sha256"], "hash mismatch")
        w,h,rows = decode_png(data)
        require((w,h) == (512,512) == tuple(manifest["resolution"]), "resolution")
        require(r["kind"] in {"basecolor","normal","orm","height"}, "map kind")
        require(r["color_space"] == ("srgb" if r["kind"] == "basecolor" else "linear"), "color space")
        require(r["tileable"] is True and r["runtime_status"] == "source_validated_not_imported", "status")
        material = r["path"].split("/")[0]
        require(material in EXPECTED_MATERIALS, "material folder")
        by_material[material].add(r["kind"])
        require(rows[0] == rows[-1], "vertical tile seam")
        left = b"".join(row[:3] for row in rows)
        right = b"".join(row[-3:] for row in rows)
        require(left == right, "horizontal tile seam")
        if r["kind"] == "normal":
            zvals = [row[i+2] for row in rows[::32] for i in range(0, len(row), 96)]
            require(min(zvals) >= 128, "normal Z")
        if r["kind"] == "orm":
            metals = {row[i+2] for row in rows[::32] for i in range(0, len(row), 96)}
            expected_metal = 255 if material == "brushed_steel" else 0
            require(metals == {expected_metal}, "metallic channel must be binary and material-consistent")
    require(all(v == {"basecolor","normal","orm","height"} for v in by_material.values()), "incomplete material")
    return manifest["counts"]

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("pack", type=Path)
    p.add_argument("--expected-manifest", type=Path)
    args = p.parse_args()
    try:
        print("PASS:", verify(args.pack, args.expected_manifest))
    except (ValueError, KeyError, TypeError, OSError, UnicodeError, struct.error, zlib.error, json.JSONDecodeError) as exc:
        p.exit(1, f"FAIL: {exc}\n")

if __name__ == "__main__":
    main()
