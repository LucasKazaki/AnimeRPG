"""Validate Astral starter material set v2 without external dependencies."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import struct
import zlib

EXPECTED_MATERIALS = {"limestone", "concrete", "brushed_steel", "asphalt"}
EXPECTED_SIZE = 512
MAX_FILE_BYTES = 8 * 1024 * 1024


def require(cond, msg):
    if not cond:
        raise ValueError(msg)


def safe_path(root, rel):
    require(isinstance(rel, str) and rel and "\\" not in rel and ":" not in rel, "unsafe path")
    p = PurePosixPath(rel)
    require(not p.is_absolute() and all(part not in ("", ".", "..") for part in p.parts), "unsafe path")
    f = root.joinpath(*p.parts)
    require(f.is_file() and not f.is_symlink(), "missing/symlink file")
    require(f.stat().st_size <= MAX_FILE_BYTES, "oversized file")
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
            require(width is None and n == 13 and len(kinds) == 1, "IHDR")
            width, height, depth, color, comp, filt, interlace = struct.unpack(">IIBBBBB", body)
            require((depth,color,comp,filt,interlace) == (8,2,0,0,0), "PNG format")
            # Bound the decompressed allocation before any IDAT bytes are inflated.
            require(0 < width <= EXPECTED_SIZE and 0 < height <= EXPECTED_SIZE, "PNG dimensions exceed pack budget")
        elif kind == b"IDAT":
            require(width is not None, "IDAT before IHDR")
            idat.extend(body)
        elif kind == b"IEND":
            require(n == 0 and end == len(data), "IEND")
        else:
            raise ValueError("unexpected PNG chunk")
        off = end
    require(kinds == [b"IHDR", b"IDAT", b"IEND"], "chunk order")
    require(width and height, "dimensions")
    expected = height * (1 + width*3)
    decoder = zlib.decompressobj()
    raw = decoder.decompress(bytes(idat), expected + 1)
    require(len(raw) == expected, "inflated size")
    require(decoder.eof and not decoder.unconsumed_tail and not decoder.unused_data, "compressed stream exceeds expected image")
    require(decoder.flush() == b"", "unexpected compressed tail")
    pixels = []
    stride = 1 + width*3
    for y in range(height):
        row = raw[y*stride:(y+1)*stride]
        require(row[0] == 0, "unsupported filter")
        pixels.append(row[1:])
    return width, height, pixels


def edge_wrap_is_reasonable(rows):
    # Periodic textures should not duplicate endpoints. Instead, the last-to-first
    # wrap step should be in-family with ordinary adjacent steps.
    max_internal = 0
    max_wrap = 0
    height = len(rows)
    width = len(rows[0]) // 3
    for y, row in enumerate(rows):
        for x in range(width - 1):
            a = row[x*3:(x+1)*3]
            b = row[(x+1)*3:(x+2)*3]
            max_internal = max(max_internal, *(abs(a[c]-b[c]) for c in range(3)))
        left = row[:3]
        right = row[-3:]
        max_wrap = max(max_wrap, *(abs(left[c]-right[c]) for c in range(3)))
    for x in range(width):
        for y in range(height - 1):
            a = rows[y][x*3:(x+1)*3]
            b = rows[y+1][x*3:(x+1)*3]
            max_internal = max(max_internal, *(abs(a[c]-b[c]) for c in range(3)))
        top = rows[0][x*3:(x+1)*3]
        bottom = rows[-1][x*3:(x+1)*3]
        max_wrap = max(max_wrap, *(abs(top[c]-bottom[c]) for c in range(3)))
    return max_wrap <= max_internal + 2


def verify(root, expected_manifest=None):
    root = Path(root).resolve()
    require(root.is_dir(), "pack root")
    manifest = json.loads((root/"manifest.json").read_text())
    if expected_manifest is not None:
        expected = json.loads(Path(expected_manifest).read_text())
        require(manifest == expected, "manifest differs from pinned expected manifest")
    require(manifest["schema_version"] == 1 and manifest["generator"] == "astral-materials-v2-2", "manifest version")
    require(manifest.get("periodic_sampling") == "unique_texels_[0,1)_repeat_wrap", "periodic sampling contract")
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
        require((w,h) == (EXPECTED_SIZE,EXPECTED_SIZE) == tuple(manifest["resolution"]), "resolution")
        require(r["kind"] in {"basecolor","normal","orm","height"}, "map kind")
        require(r["color_space"] == ("srgb" if r["kind"] == "basecolor" else "linear"), "color space")
        require(r["tileable"] is True and r["runtime_status"] == "source_validated_not_imported", "status")
        material = r["path"].split("/")[0]
        require(material in EXPECTED_MATERIALS, "material folder")
        by_material[material].add(r["kind"])
        require(edge_wrap_is_reasonable(rows), "repeat-boundary discontinuity")
        if r["kind"] == "height":
            require(all(row[i] == row[i+1] == row[i+2] for row in rows for i in range(0, len(row), 3)), "height must be replicated grayscale")
        if r["kind"] == "normal":
            zvals = [row[i+2] for row in rows for i in range(0, len(row), 3)]
            require(min(zvals) >= 128, "normal Z")
        if r["kind"] == "orm":
            metals = {row[i+2] for row in rows for i in range(0, len(row), 3)}
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
