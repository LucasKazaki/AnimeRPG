#!/usr/bin/env python3
"""Generate ART-009 starter solid primitives v1 as a self-contained glTF 2.0 source asset."""
from __future__ import annotations

import argparse
import base64
import hashlib
import json
import math
import struct
from pathlib import Path

GENERATOR_ID = "Astral ART-009 Starter Solid Primitives 1"
STATUS = "source_validated_not_imported"
EXPECTED_ROOT_KEYS = {
    "schema_version", "asset_id", "loop_id", "status", "purpose", "coordinate_system",
    "material", "primitives", "scene_layout", "reference_scope",
}
EXPECTED_PRIMITIVES = {
    "cube-1m": ("Starter_Cube_1m", "cube", 24, 36),
    "sphere-1m": ("Starter_Sphere_1m", "uv_sphere", 151, 672),
    "cylinder-1x2m": ("Starter_Cylinder_1x2m", "cylinder", 70, 192),
    "plane-1m": ("Starter_Plane_1m", "plane", 4, 6),
}
EXPECTED_UV_POLICIES = {
    "cube-1m": "each_face_maps_0_to_1",
    "sphere-1m": "longitude_latitude_with_seam_and_segment_poles",
    "cylinder-1x2m": "side_wrap_once_caps_each_map_to_unit_disc",
    "plane-1m": "single_0_to_1_square",
}
EXPECTED_REFERENCES = [
    {
        "engine": "Unity",
        "official_url": "https://docs.unity3d.com/Manual/PrimitiveObjects.html",
        "note": "current manual exposes Cube, Sphere, Capsule, Cylinder, Plane and Quad as editor primitives/placeholders",
    },
    {
        "engine": "Unreal Engine 5.8",
        "official_url": "https://dev.epicgames.com/documentation/unreal-engine/predefined-shapes-in-unreal-engine",
        "note": "Modeling Mode documents Box, Sphere, Cylinder, Cone, Torus, Arrow, Rectangle, Disc and Stairs predefined shapes",
    },
]
EXPECTED_COVERAGE = "This v1 packet deliberately implements only cube, sphere, cylinder and plane. It is not parity with Unity or Unreal starter/modeling libraries."


def _finite_number(value, label):
    if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(float(value)):
        raise ValueError(f"{label} must be a finite JSON number")
    return float(value)


def _vector(values, count, label):
    if type(values) is not list or len(values) != count:
        raise ValueError(f"{label} must contain {count} numbers")
    return [_finite_number(v, f"{label}[{i}]") for i, v in enumerate(values)]


def load_source(path: Path) -> dict:
    source = json.loads(path.read_text(encoding="utf-8"))
    if type(source) is not dict or set(source) != EXPECTED_ROOT_KEYS:
        raise ValueError("source root keys do not match ART-009 contract")
    if type(source["schema_version"]) is not int or source["schema_version"] != 1:
        raise ValueError("schema_version must be JSON integer 1")
    if source["asset_id"] != "astral-starter-solid-primitives-v1":
        raise ValueError("unexpected asset_id")
    if source["loop_id"] != "astral-art-hourly-20260922":
        raise ValueError("unexpected loop_id")
    if source["status"] != STATUS:
        raise ValueError("source status must remain source-only")
    if source["purpose"] != "generic_starter_content_and_scale_uv_tangent_calibration":
        raise ValueError("unexpected purpose")

    coord = source["coordinate_system"]
    if type(coord) is not dict or set(coord) != {"handedness", "up", "forward", "right", "linear_unit"}:
        raise ValueError("coordinate_system contract changed")
    if (coord["handedness"], coord["up"], coord["forward"], coord["right"], coord["linear_unit"]) != (
        "right", "+Y", "+Z", "-X", "metre"
    ):
        raise ValueError("coordinate convention changed")

    material = source["material"]
    if type(material) is not dict or set(material) != {
        "name", "base_color_srgb", "metallic", "roughness", "alpha_mode", "double_sided"
    }:
        raise ValueError("material contract changed")
    if material["name"] != "StarterNeutral" or material["alpha_mode"] != "OPAQUE" or type(material["double_sided"]) is not bool:
        raise ValueError("material identity changed")
    base = _vector(material["base_color_srgb"], 4, "base_color_srgb")
    if any(v < 0.0 or v > 1.0 for v in base):
        raise ValueError("base color must be normalized")
    metallic = _finite_number(material["metallic"], "metallic")
    roughness = _finite_number(material["roughness"], "roughness")
    if metallic != 0.0 or not (0.0 <= roughness <= 1.0) or material["double_sided"] is not False:
        raise ValueError("material semantics changed")

    primitives = source["primitives"]
    if type(primitives) is not list or len(primitives) != 4:
        raise ValueError("exactly four starter primitives are required")
    seen = set()
    for item in primitives:
        if type(item) is not dict or "id" not in item:
            raise ValueError("primitive entry invalid")
        pid = item["id"]
        if pid in seen or pid not in EXPECTED_PRIMITIVES:
            raise ValueError("primitive identity invalid")
        seen.add(pid)
        mesh_name, shape, vertices, indices = EXPECTED_PRIMITIVES[pid]
        common = {"id", "mesh_name", "shape", "vertex_count", "index_count", "uv_policy"}
        if shape in ("cube", "plane"):
            expected_keys = common | {"dimensions_m"}
        else:
            expected_keys = common | {"diameter_m", "segments"} | ({"height_m"} if shape == "cylinder" else {"latitude_bands"})
        if set(item) != expected_keys:
            raise ValueError(f"{pid} keys changed")
        if item["mesh_name"] != mesh_name or item["shape"] != shape or item["uv_policy"] != EXPECTED_UV_POLICIES[pid]:
            raise ValueError(f"{pid} mesh/UV identity changed")
        if type(item["vertex_count"]) is not int or item["vertex_count"] != vertices:
            raise ValueError(f"{pid} vertex_count changed")
        if type(item["index_count"]) is not int or item["index_count"] != indices:
            raise ValueError(f"{pid} index_count changed")
        if shape == "cube":
            if _vector(item["dimensions_m"], 3, "cube dimensions") != [1.0, 1.0, 1.0]:
                raise ValueError("cube dimensions changed")
        elif shape == "plane":
            if _vector(item["dimensions_m"], 2, "plane dimensions") != [1.0, 1.0]:
                raise ValueError("plane dimensions changed")
        if shape == "uv_sphere":
            if _finite_number(item["diameter_m"], "sphere diameter") != 1.0:
                raise ValueError("sphere diameter changed")
            if type(item["segments"]) is not int or item["segments"] != 16:
                raise ValueError("sphere segments changed")
            if type(item["latitude_bands"]) is not int or item["latitude_bands"] != 8:
                raise ValueError("sphere latitude bands changed")
        elif shape == "cylinder":
            if _finite_number(item["diameter_m"], "cylinder diameter") != 1.0 or _finite_number(item["height_m"], "cylinder height") != 2.0:
                raise ValueError("cylinder dimensions changed")
            if type(item["segments"]) is not int or item["segments"] != 16:
                raise ValueError("cylinder segments changed")
    if seen != set(EXPECTED_PRIMITIVES):
        raise ValueError("starter primitive set incomplete")

    layout = source["scene_layout"]
    expected_layout = [
        ("cube-1m", [-3.0, 0.5, 0.0]),
        ("sphere-1m", [-1.0, 0.5, 0.0]),
        ("cylinder-1x2m", [1.0, 1.0, 0.0]),
        ("plane-1m", [3.0, 0.0, 0.0]),
    ]
    if type(layout) is not list or len(layout) != 4:
        raise ValueError("scene_layout invalid")
    for entry, (pid, translation) in zip(layout, expected_layout):
        if type(entry) is not dict or set(entry) != {"primitive_id", "translation_m"}:
            raise ValueError("scene_layout entry invalid")
        if entry["primitive_id"] != pid or _vector(entry["translation_m"], 3, "translation") != translation:
            raise ValueError("scene_layout changed")

    refs = source["reference_scope"]
    if type(refs) is not dict or set(refs) != {"observed_2026_09_24", "coverage_statement"}:
        raise ValueError("reference_scope invalid")
    if refs["observed_2026_09_24"] != EXPECTED_REFERENCES:
        raise ValueError("reference inventory changed")
    if refs["coverage_statement"] != EXPECTED_COVERAGE:
        raise ValueError("coverage statement changed")
    return source


def _cube():
    positions, normals, tangents, uvs, indices = [], [], [], [], []
    faces = [
        ((0, 0, 1), (1, 0, 0), (0, 1, 0)),
        ((0, 0, -1), (-1, 0, 0), (0, 1, 0)),
        ((1, 0, 0), (0, 0, -1), (0, 1, 0)),
        ((-1, 0, 0), (0, 0, 1), (0, 1, 0)),
        ((0, 1, 0), (1, 0, 0), (0, 0, -1)),
        ((0, -1, 0), (1, 0, 0), (0, 0, 1)),
    ]
    uv4 = [(0, 0), (1, 0), (1, 1), (0, 1)]
    for face_index, (normal, tangent3, bitangent) in enumerate(faces):
        center = tuple(0.5 * c for c in normal)
        base = face_index * 4
        for (u, v) in uv4:
            p = tuple(center[i] + (u - 0.5) * tangent3[i] + (v - 0.5) * bitangent[i] for i in range(3))
            positions.extend(p)
            normals.extend(normal)
            tangents.extend((*tangent3, 1.0))
            uvs.extend((u, v))
        indices.extend((base, base + 1, base + 2, base, base + 2, base + 3))
    return positions, normals, tangents, uvs, indices


def _plane():
    positions = [-0.5, 0.0, -0.5,  0.5, 0.0, -0.5,  0.5, 0.0, 0.5,  -0.5, 0.0, 0.5]
    normals = [0.0, 1.0, 0.0] * 4
    tangents = [1.0, 0.0, 0.0, -1.0] * 4
    uvs = [0.0, 0.0,  1.0, 0.0,  1.0, 1.0,  0.0, 1.0]
    indices = [0, 2, 1, 0, 3, 2]
    return positions, normals, tangents, uvs, indices


def _sphere(segments=16, latitude_bands=8):
    positions, normals, tangents, uvs, indices = [], [], [], [], []

    # One pole vertex per segment avoids a single UV seam pole without producing degenerate quads.
    top_base = 0
    for j in range(segments):
        theta = 2.0 * math.pi * (j + 0.5) / segments
        positions.extend((0.0, 0.5, 0.0))
        normals.extend((0.0, 1.0, 0.0))
        tangents.extend((-math.sin(theta), 0.0, math.cos(theta), 1.0))
        uvs.extend(((j + 0.5) / segments, 0.0))

    ring_bases = []
    for band in range(1, latitude_bands):
        phi = math.pi * band / latitude_bands
        ring_bases.append(len(positions) // 3)
        for j in range(segments + 1):
            theta = 2.0 * math.pi * j / segments
            x = 0.5 * math.sin(phi) * math.cos(theta)
            y = 0.5 * math.cos(phi)
            z = 0.5 * math.sin(phi) * math.sin(theta)
            positions.extend((x, y, z))
            normals.extend((2.0 * x, 2.0 * y, 2.0 * z))
            tangents.extend((-math.sin(theta), 0.0, math.cos(theta), 1.0))
            uvs.extend((j / segments, band / latitude_bands))

    bottom_base = len(positions) // 3
    for j in range(segments):
        theta = 2.0 * math.pi * (j + 0.5) / segments
        positions.extend((0.0, -0.5, 0.0))
        normals.extend((0.0, -1.0, 0.0))
        tangents.extend((-math.sin(theta), 0.0, math.cos(theta), 1.0))
        uvs.extend(((j + 0.5) / segments, 1.0))

    first = ring_bases[0]
    for j in range(segments):
        indices.extend((top_base + j, first + j + 1, first + j))

    for r in range(len(ring_bases) - 1):
        a = ring_bases[r]
        b = ring_bases[r + 1]
        for j in range(segments):
            indices.extend((a + j, b + j + 1, b + j, a + j, a + j + 1, b + j + 1))

    last = ring_bases[-1]
    for j in range(segments):
        indices.extend((last + j, last + j + 1, bottom_base + j))

    return positions, normals, tangents, uvs, indices


def _cylinder(segments=16):
    positions, normals, tangents, uvs, indices = [], [], [], [], []

    # Side seam is duplicated. U wraps around, V runs bottom->top. Tangent W=-1 makes +V point +Y.
    side_base = 0
    for j in range(segments + 1):
        theta = 2.0 * math.pi * j / segments
        c, s = math.cos(theta), math.sin(theta)
        t = (-s, 0.0, c, -1.0)
        for y, v in ((-1.0, 0.0), (1.0, 1.0)):
            positions.extend((0.5 * c, y, 0.5 * s))
            normals.extend((c, 0.0, s))
            tangents.extend(t)
            uvs.extend((j / segments, v))
    for j in range(segments):
        a = side_base + 2 * j
        b = a + 2
        indices.extend((a, a + 1, b + 1, a, b + 1, b))

    # Top cap, +Y normal, UV unit disc with +U=+X and +V=+Z => tangent W=-1.
    top_base = len(positions) // 3
    positions.extend((0.0, 1.0, 0.0)); normals.extend((0.0, 1.0, 0.0)); tangents.extend((1.0, 0.0, 0.0, -1.0)); uvs.extend((0.5, 0.5))
    for j in range(segments + 1):
        theta = 2.0 * math.pi * j / segments
        c, s = math.cos(theta), math.sin(theta)
        positions.extend((0.5 * c, 1.0, 0.5 * s)); normals.extend((0.0, 1.0, 0.0)); tangents.extend((1.0, 0.0, 0.0, -1.0)); uvs.extend((0.5 + 0.5 * c, 0.5 + 0.5 * s))
    for j in range(segments):
        indices.extend((top_base, top_base + 2 + j, top_base + 1 + j))

    # Bottom cap, -Y normal, same planar UV orientation => tangent W=+1.
    bottom_base = len(positions) // 3
    positions.extend((0.0, -1.0, 0.0)); normals.extend((0.0, -1.0, 0.0)); tangents.extend((1.0, 0.0, 0.0, 1.0)); uvs.extend((0.5, 0.5))
    for j in range(segments + 1):
        theta = 2.0 * math.pi * j / segments
        c, s = math.cos(theta), math.sin(theta)
        positions.extend((0.5 * c, -1.0, 0.5 * s)); normals.extend((0.0, -1.0, 0.0)); tangents.extend((1.0, 0.0, 0.0, 1.0)); uvs.extend((0.5 + 0.5 * c, 0.5 + 0.5 * s))
    for j in range(segments):
        indices.extend((bottom_base, bottom_base + 1 + j, bottom_base + 2 + j))

    return positions, normals, tangents, uvs, indices


def _align4(blob: bytearray):
    while len(blob) % 4:
        blob.append(0)


def _pack_f32(blob: bytearray, values):
    offset = len(blob)
    blob.extend(struct.pack("<" + "f" * len(values), *values))
    length = len(values) * 4
    _align4(blob)
    return offset, length


def _pack_u16(blob: bytearray, values):
    offset = len(blob)
    blob.extend(struct.pack("<" + "H" * len(values), *values))
    length = len(values) * 2
    _align4(blob)
    return offset, length


def _minmax(values, width):
    rows = [values[i:i + width] for i in range(0, len(values), width)]
    return [min(r[i] for r in rows) for i in range(width)], [max(r[i] for r in rows) for i in range(width)]


def build_gltf(source: dict) -> dict:
    payloads = [
        ("cube-1m", _cube()),
        ("sphere-1m", _sphere()),
        ("cylinder-1x2m", _cylinder()),
        ("plane-1m", _plane()),
    ]
    primitive_specs = {p["id"]: p for p in source["primitives"]}
    blob = bytearray()
    buffer_views = []
    accessors = []
    meshes = []

    def add_view(offset, length, target):
        idx = len(buffer_views)
        buffer_views.append({"buffer": 0, "byteOffset": offset, "byteLength": length, "target": target})
        return idx

    def add_accessor(view, component_type, count, type_name, minv=None, maxv=None):
        idx = len(accessors)
        item = {"bufferView": view, "componentType": component_type, "count": count, "type": type_name}
        if minv is not None:
            item["min"] = minv
        if maxv is not None:
            item["max"] = maxv
        accessors.append(item)
        return idx

    for pid, (positions, normals, tangents, uvs, indices) in payloads:
        spec = primitive_specs[pid]
        vertex_count = len(positions) // 3
        if vertex_count != spec["vertex_count"] or len(indices) != spec["index_count"]:
            raise AssertionError(f"internal {pid} budget mismatch")
        p_off, p_len = _pack_f32(blob, positions)
        n_off, n_len = _pack_f32(blob, normals)
        t_off, t_len = _pack_f32(blob, tangents)
        u_off, u_len = _pack_f32(blob, uvs)
        i_off, i_len = _pack_u16(blob, indices)
        pmin, pmax = _minmax(positions, 3)
        p_acc = add_accessor(add_view(p_off, p_len, 34962), 5126, vertex_count, "VEC3", pmin, pmax)
        n_acc = add_accessor(add_view(n_off, n_len, 34962), 5126, vertex_count, "VEC3")
        t_acc = add_accessor(add_view(t_off, t_len, 34962), 5126, vertex_count, "VEC4")
        uv_acc = add_accessor(add_view(u_off, u_len, 34962), 5126, vertex_count, "VEC2")
        i_acc = add_accessor(add_view(i_off, i_len, 34963), 5123, len(indices), "SCALAR", [min(indices)], [max(indices)])
        meshes.append({
            "name": spec["mesh_name"],
            "primitives": [{
                "attributes": {"POSITION": p_acc, "NORMAL": n_acc, "TANGENT": t_acc, "TEXCOORD_0": uv_acc},
                "indices": i_acc,
                "material": 0,
                "mode": 4,
            }],
        })

    material = source["material"]
    nodes = []
    pid_to_mesh = {pid: i for i, (pid, _) in enumerate(payloads)}
    pid_to_name = {p["id"]: p["mesh_name"] for p in source["primitives"]}
    for entry in source["scene_layout"]:
        nodes.append({
            "name": pid_to_name[entry["primitive_id"]],
            "mesh": pid_to_mesh[entry["primitive_id"]],
            "translation": entry["translation_m"],
        })

    return {
        "asset": {"version": "2.0", "generator": GENERATOR_ID},
        "scene": 0,
        "scenes": [{"name": "Astral_Starter_Solid_Primitives_v1", "nodes": list(range(4))}],
        "nodes": nodes,
        "buffers": [{"byteLength": len(blob), "uri": "data:application/octet-stream;base64," + base64.b64encode(bytes(blob)).decode("ascii")}],
        "bufferViews": buffer_views,
        "accessors": accessors,
        "materials": [{
            "name": material["name"],
            "pbrMetallicRoughness": {
                "baseColorFactor": material["base_color_srgb"],
                "metallicFactor": material["metallic"],
                "roughnessFactor": material["roughness"],
            },
            "alphaMode": material["alpha_mode"],
            "doubleSided": material["double_sided"],
        }],
        "meshes": meshes,
        "extras": {
            "astral_asset_id": source["asset_id"],
            "astral_status": STATUS,
            "linear_unit": "metre",
            "coordinate_note": "right-handed; +Y up; +Z forward; -X right",
        },
    }


def _canonical_bytes(obj):
    return (json.dumps(obj, indent=2, separators=(",", ": "), ensure_ascii=False) + "\n").encode("utf-8")


def build_outputs(source_path: Path):
    source = load_source(source_path)
    gltf = build_gltf(source)
    gltf_bytes = _canonical_bytes(gltf)
    # Hash the parsed canonical JSON, not checkout bytes, so LF/CRLF working trees share one source identity.
    source_bytes = _canonical_bytes(source)
    manifest = {
        "schema_version": 1,
        "asset_id": source["asset_id"],
        "status": STATUS,
        "generator_id": GENERATOR_ID,
        "source_file": "source-contract.json",
        "source_sha256": hashlib.sha256(source_bytes).hexdigest(),
        "gltf_file": "starter_solid_primitives_v1.gltf",
        "gltf_sha256": hashlib.sha256(gltf_bytes).hexdigest(),
        "gltf_bytes": len(gltf_bytes),
        "mesh_count": 4,
        "material_count": 1,
        "vertex_count_total": sum(p["vertex_count"] for p in source["primitives"]),
        "index_count_total": sum(p["index_count"] for p in source["primitives"]),
        "primitive_ids": [p["id"] for p in source["primitives"]],
        "runtime_state": STATUS,
    }
    return gltf_bytes, _canonical_bytes(manifest)


def write_or_check(source_path: Path, gltf_path: Path, manifest_path: Path, check: bool):
    gltf_bytes, manifest_bytes = build_outputs(source_path)
    expected = [(gltf_path, gltf_bytes), (manifest_path, manifest_bytes)]
    if check:
        for path, data in expected:
            if not path.is_file() or path.read_bytes() != data:
                raise SystemExit(f"FAIL: stale or missing generated file: {path}")
        print("PASS: deterministic starter solid primitives match pinned files")
        return
    for path, data in expected:
        if path.exists():
            raise SystemExit(f"REFUSE: output already exists: {path}")
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    print("PASS: generated starter solid primitives v1")


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--gltf", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)
    write_or_check(args.source, args.gltf, args.manifest, args.check)


if __name__ == "__main__":
    main()
