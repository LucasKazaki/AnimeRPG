#!/usr/bin/env python3
from __future__ import annotations

import argparse
import base64
import hashlib
import json
import math
import struct
from pathlib import Path
from typing import Iterable

SCHEMA_VERSION = 1
STATUS = "source_validated_not_imported"
GENERATOR_ID = "astral-modular-architecture-v1-1"


def _reject_duplicates(pairs):
    out = {}
    for key, value in pairs:
        if key in out:
            raise ValueError(f"duplicate JSON key: {key}")
        out[key] = value
    return out


def _reject_constant(value: str):
    raise ValueError(f"non-standard JSON numeric constant: {value}")


def load_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=_reject_duplicates, parse_constant=_reject_constant)


def canonical_bytes(obj) -> bytes:
    return (json.dumps(obj, indent=2, sort_keys=True, ensure_ascii=False) + "\n").encode("utf-8")


def compact_json_bytes(obj) -> bytes:
    return (json.dumps(obj, sort_keys=True, separators=(",", ":"), ensure_ascii=False) + "\n").encode("utf-8")


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_path(path: Path) -> str:
    return sha256_bytes(path.read_bytes())


def _strict_int(value, name: str) -> int:
    if type(value) is not int:
        raise ValueError(f"{name} must be an integer")
    return value


def _strict_number(value, name: str) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(float(value)):
        raise ValueError(f"{name} must be a finite number")
    return float(value)


def _strict_vec(value, length: int, name: str) -> list[float]:
    if not isinstance(value, list) or len(value) != length:
        raise ValueError(f"{name} must be a {length}-element array")
    return [_strict_number(v, f"{name}[{i}]") for i, v in enumerate(value)]


def validate_source(source: dict) -> None:
    expected_root = {
        "schema_version", "status", "units", "coordinate_system", "snap_grid_m",
        "uv_units_per_meter", "materials", "modules", "provenance",
    }
    if set(source) != expected_root:
        raise ValueError(f"source root fields must be exactly {sorted(expected_root)}")
    if _strict_int(source["schema_version"], "schema_version") != SCHEMA_VERSION:
        raise ValueError("unsupported schema_version")
    if source["status"] != STATUS:
        raise ValueError("source status must remain source_validated_not_imported")
    if source["units"] != "metres":
        raise ValueError("units must be metres")
    if source["coordinate_system"] != {
        "handedness": "right_handed", "up": "+Y", "forward": "+Z", "right": "-X"
    }:
        raise ValueError("unexpected coordinate_system")
    snap = _strict_number(source["snap_grid_m"], "snap_grid_m")
    if snap <= 0.0:
        raise ValueError("snap_grid_m must be positive")
    if _strict_number(source["uv_units_per_meter"], "uv_units_per_meter") != 1.0:
        raise ValueError("uv_units_per_meter must be 1.0 for this fixture")
    if not isinstance(source["materials"], list) or len(source["materials"]) != 2:
        raise ValueError("exactly two materials are required")
    material_ids = []
    for i, material in enumerate(source["materials"]):
        expected = {"id", "base_color_factor", "metallic_factor", "roughness_factor"}
        if not isinstance(material, dict) or set(material) != expected:
            raise ValueError(f"materials[{i}] fields invalid")
        mid = material["id"]
        if not isinstance(mid, str) or not mid:
            raise ValueError(f"materials[{i}].id invalid")
        material_ids.append(mid)
        _strict_vec(material["base_color_factor"], 4, f"materials[{i}].base_color_factor")
        _strict_number(material["metallic_factor"], f"materials[{i}].metallic_factor")
        _strict_number(material["roughness_factor"], f"materials[{i}].roughness_factor")
    if material_ids != ["starter_neutral", "collision_debug"]:
        raise ValueError("material ids/order are part of the fixture contract")
    if not isinstance(source["modules"], list) or len(source["modules"]) != 7:
        raise ValueError("exactly seven modules are required")
    seen = set()
    supported = {"floor", "wall", "corner", "doorway", "stairs", "pillar"}
    for i, module in enumerate(source["modules"]):
        if not isinstance(module, dict):
            raise ValueError(f"modules[{i}] must be an object")
        module_id = module.get("id")
        if not isinstance(module_id, str) or not module_id or module_id in seen:
            raise ValueError(f"modules[{i}].id invalid or duplicate")
        seen.add(module_id)
        if module.get("kind") not in supported:
            raise ValueError(f"unsupported module kind: {module.get('kind')}")
        _strict_vec(module.get("gallery_translation_m"), 3, f"{module_id}.gallery_translation_m")
        points = module.get("snap_points_m")
        if not isinstance(points, list) or not points:
            raise ValueError(f"{module_id}.snap_points_m must be non-empty")
        for j, point in enumerate(points):
            vals = _strict_vec(point, 3, f"{module_id}.snap_points_m[{j}]")
            for v in vals:
                q = v / snap
                if abs(q - round(q)) > 1e-9:
                    raise ValueError(f"{module_id} snap point is off the {snap} m grid")
        if not isinstance(module.get("pivot_rule"), str) or not isinstance(module.get("collision_type"), str):
            raise ValueError(f"{module_id} pivot/collision metadata missing")
        for key, value in module.items():
            if key.endswith("_m") and key not in {"gallery_translation_m", "snap_points_m"}:
                if isinstance(value, list):
                    _strict_vec(value, len(value), f"{module_id}.{key}")
                else:
                    _strict_number(value, f"{module_id}.{key}")
        if "steps" in module:
            if _strict_int(module["steps"], f"{module_id}.steps") <= 0:
                raise ValueError("stairs steps must be positive")
    provenance = source["provenance"]
    if provenance != {
        "origin": "original_project_procedural",
        "third_party_content": False,
        "generator_family": "python_standard_library",
        "distribution_scope": "repository_source_only",
    }:
        raise ValueError("provenance contract changed")


def _vsub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def _cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def _dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def _norm(v):
    length = math.sqrt(_dot(v, v))
    if length <= 1e-12:
        raise ValueError("degenerate vector")
    return (v[0] / length, v[1] / length, v[2] / length)


def _basis(p0, p1, p2, uv0, uv1, uv2):
    e1 = _vsub(p1, p0)
    e2 = _vsub(p2, p0)
    du1, dv1 = uv1[0] - uv0[0], uv1[1] - uv0[1]
    du2, dv2 = uv2[0] - uv0[0], uv2[1] - uv0[1]
    det = du1 * dv2 - du2 * dv1
    if abs(det) <= 1e-12:
        raise ValueError("degenerate UV basis")
    inv = 1.0 / det
    tangent = _norm(((e1[0] * dv2 - e2[0] * dv1) * inv,
                     (e1[1] * dv2 - e2[1] * dv1) * inv,
                     (e1[2] * dv2 - e2[2] * dv1) * inv))
    bitangent = _norm(((-e1[0] * du2 + e2[0] * du1) * inv,
                       (-e1[1] * du2 + e2[1] * du1) * inv,
                       (-e1[2] * du2 + e2[2] * du1) * inv))
    normal = _norm(_cross(e1, e2))
    w = 1.0 if _dot(_cross(normal, tangent), bitangent) >= 0.0 else -1.0
    return normal, tangent, w


def new_mesh_data():
    return {"positions": [], "normals": [], "tangents": [], "uvs": [], "indices": []}


def append_quad(data, positions, uv_width: float, uv_height: float):
    uvs = [(0.0, 0.0), (uv_width, 0.0), (uv_width, uv_height), (0.0, uv_height)]
    normal, tangent, w = _basis(positions[0], positions[1], positions[2], uvs[0], uvs[1], uvs[2])
    base = len(data["positions"])
    data["positions"].extend(positions)
    data["normals"].extend([normal] * 4)
    data["tangents"].extend([(tangent[0], tangent[1], tangent[2], w)] * 4)
    data["uvs"].extend(uvs)
    data["indices"].extend([base, base + 1, base + 2, base, base + 2, base + 3])


def append_triangle(data, positions, uvs):
    normal, tangent, w = _basis(positions[0], positions[1], positions[2], uvs[0], uvs[1], uvs[2])
    base = len(data["positions"])
    data["positions"].extend(positions)
    data["normals"].extend([normal] * 3)
    data["tangents"].extend([(tangent[0], tangent[1], tangent[2], w)] * 3)
    data["uvs"].extend(uvs)
    data["indices"].extend([base, base + 1, base + 2])


def append_box(data, minimum, maximum):
    x0, y0, z0 = minimum
    x1, y1, z1 = maximum
    dx, dy, dz = x1 - x0, y1 - y0, z1 - z0
    if min(dx, dy, dz) <= 0:
        raise ValueError("box extents must be positive")
    append_quad(data, [(x1,y0,z0),(x1,y1,z0),(x1,y1,z1),(x1,y0,z1)], dy, dz)
    append_quad(data, [(x0,y0,z1),(x0,y1,z1),(x0,y1,z0),(x0,y0,z0)], dy, dz)
    append_quad(data, [(x0,y1,z1),(x1,y1,z1),(x1,y1,z0),(x0,y1,z0)], dx, dz)
    append_quad(data, [(x0,y0,z0),(x1,y0,z0),(x1,y0,z1),(x0,y0,z1)], dx, dz)
    append_quad(data, [(x0,y0,z1),(x1,y0,z1),(x1,y1,z1),(x0,y1,z1)], dx, dy)
    append_quad(data, [(x1,y0,z0),(x0,y0,z0),(x0,y1,z0),(x1,y1,z0)], dx, dy)


def _polygon_area(points):
    total = 0.0
    for i, (x0, y0) in enumerate(points):
        x1, y1 = points[(i + 1) % len(points)]
        total += x0 * y1 - x1 * y0
    return total * 0.5


def _point_in_tri(p, a, b, c):
    def sign(p1, p2, p3):
        return (p1[0] - p3[0]) * (p2[1] - p3[1]) - (p2[0] - p3[0]) * (p1[1] - p3[1])
    d1, d2, d3 = sign(p, a, b), sign(p, b, c), sign(p, c, a)
    has_neg = (d1 < -1e-12) or (d2 < -1e-12) or (d3 < -1e-12)
    has_pos = (d1 > 1e-12) or (d2 > 1e-12) or (d3 > 1e-12)
    return not (has_neg and has_pos)


def triangulate_ccw(points):
    if len(points) < 3 or _polygon_area(points) <= 1e-12:
        raise ValueError("profile must be CCW and nondegenerate")
    remaining = list(range(len(points)))
    triangles = []
    guard = 0
    while len(remaining) > 3:
        ear = False
        for pos, idx in enumerate(remaining):
            prev_idx = remaining[(pos - 1) % len(remaining)]
            next_idx = remaining[(pos + 1) % len(remaining)]
            a, b, c = points[prev_idx], points[idx], points[next_idx]
            cross2 = (b[0]-a[0])*(c[1]-b[1]) - (b[1]-a[1])*(c[0]-b[0])
            if cross2 <= 1e-12:
                continue
            if any(_point_in_tri(points[q], a, b, c) for q in remaining if q not in {prev_idx, idx, next_idx}):
                continue
            triangles.append((prev_idx, idx, next_idx))
            del remaining[pos]
            ear = True
            break
        guard += 1
        if not ear or guard > 1000:
            raise ValueError("failed to triangulate profile")
    triangles.append(tuple(remaining))
    return triangles


def extrude_profile_x(width: float, profile_zy):
    data = new_mesh_data()
    tris = triangulate_ccw(profile_zy)
    for a, b, c in tris:
        pts = [profile_zy[a], profile_zy[b], profile_zy[c]]
        p3 = [(0.0, p[1], p[0]) for p in pts]
        uvs = [(p[0], p[1]) for p in pts]
        append_triangle(data, p3, uvs)  # CCW zy -> -X cap
        p3r = [(width, p[1], p[0]) for p in (pts[0], pts[2], pts[1])]
        uvsr = [(p[0], p[1]) for p in (pts[0], pts[2], pts[1])]
        append_triangle(data, p3r, uvsr)
    for i, a in enumerate(profile_zy):
        b = profile_zy[(i + 1) % len(profile_zy)]
        edge_len = math.hypot(b[0] - a[0], b[1] - a[1])
        positions = [
            (0.0, a[1], a[0]), (width, a[1], a[0]),
            (width, b[1], b[0]), (0.0, b[1], b[0]),
        ]
        append_quad(data, positions, width, edge_len)
    return data


def build_render(module):
    kind = module["kind"]
    data = new_mesh_data()
    if kind == "floor":
        width, thickness, depth = [float(v) for v in module["size_m"]]
        append_box(data, (0.0, -thickness, 0.0), (width, 0.0, depth))
    elif kind == "wall":
        append_box(data, (0.0, 0.0, 0.0), (float(module["width_m"]), float(module["height_m"]), float(module["thickness_m"])))
    elif kind == "corner":
        leg, height, thickness = float(module["leg_m"]), float(module["height_m"]), float(module["thickness_m"])
        append_box(data, (0.0, 0.0, 0.0), (leg, height, thickness))
        append_box(data, (0.0, 0.0, thickness), (thickness, height, leg))
    elif kind == "doorway":
        width, height, thickness = float(module["width_m"]), float(module["height_m"]), float(module["thickness_m"])
        opening_width, opening_height = float(module["opening_width_m"]), float(module["opening_height_m"])
        side = (width - opening_width) * 0.5
        append_box(data, (0.0, 0.0, 0.0), (side, height, thickness))
        append_box(data, (width - side, 0.0, 0.0), (width, height, thickness))
        append_box(data, (side, opening_height, 0.0), (width - side, height, thickness))
    elif kind == "stairs":
        width, run, rise = float(module["width_m"]), float(module["run_m"]), float(module["rise_m"])
        steps = int(module["steps"])
        tread = run / steps
        step_rise = rise / steps
        profile = [(0.0, 0.0), (run, 0.0), (run, rise)]
        for i in range(steps - 1, -1, -1):
            z = i * tread
            y_top = (i + 1) * step_rise
            profile.append((z, y_top))
            if i > 0:
                profile.append((z, i * step_rise))
        data = extrude_profile_x(width, profile)
    elif kind == "pillar":
        append_box(data, (0.0, 0.0, 0.0), (float(module["width_m"]), float(module["height_m"]), float(module["depth_m"])))
    else:
        raise ValueError(f"unsupported kind {kind}")
    return data


def build_collision(module):
    kind = module["kind"]
    if kind != "stairs":
        return build_render(module)
    width, run, rise = float(module["width_m"]), float(module["run_m"]), float(module["rise_m"])
    return extrude_profile_x(width, [(0.0, 0.0), (run, 0.0), (run, rise)])


def _pack_floats(values: Iterable[Iterable[float]]) -> bytes:
    flat = [float(component) for row in values for component in row]
    return struct.pack("<" + "f" * len(flat), *flat)


def _pack_u16(values: Iterable[int]) -> bytes:
    vals = list(values)
    if vals and max(vals) > 65535:
        raise ValueError("uint16 index overflow")
    return struct.pack("<" + "H" * len(vals), *vals)


def _bounds(positions):
    mins = [min(p[i] for p in positions) for i in range(3)]
    maxs = [max(p[i] for p in positions) for i in range(3)]
    return mins, maxs


def build_gltf(source: dict):
    validate_source(source)
    blob = bytearray()
    buffer_views = []
    accessors = []
    meshes = []
    nodes = []
    render_nodes = []
    collision_nodes = []
    module_manifest = []

    def align4():
        while len(blob) % 4:
            blob.append(0)

    def add_view(data: bytes, target: int):
        align4()
        offset = len(blob)
        blob.extend(data)
        index = len(buffer_views)
        buffer_views.append({"buffer": 0, "byteOffset": offset, "byteLength": len(data), "target": target})
        return index

    def add_accessor(view, component_type, count, type_name, minimum=None, maximum=None):
        acc = {"bufferView": view, "byteOffset": 0, "componentType": component_type, "count": count, "type": type_name}
        if minimum is not None:
            acc["min"] = minimum
        if maximum is not None:
            acc["max"] = maximum
        index = len(accessors)
        accessors.append(acc)
        return index

    def add_mesh(name: str, data: dict, material_index: int):
        mins, maxs = _bounds(data["positions"])
        pos = add_accessor(add_view(_pack_floats(data["positions"]), 34962), 5126, len(data["positions"]), "VEC3", mins, maxs)
        nor = add_accessor(add_view(_pack_floats(data["normals"]), 34962), 5126, len(data["normals"]), "VEC3")
        tan = add_accessor(add_view(_pack_floats(data["tangents"]), 34962), 5126, len(data["tangents"]), "VEC4")
        uv = add_accessor(add_view(_pack_floats(data["uvs"]), 34962), 5126, len(data["uvs"]), "VEC2")
        idx_values = data["indices"]
        idx = add_accessor(add_view(_pack_u16(idx_values), 34963), 5123, len(idx_values), "SCALAR", [min(idx_values)], [max(idx_values)])
        mesh_index = len(meshes)
        meshes.append({
            "name": name,
            "primitives": [{
                "attributes": {"POSITION": pos, "NORMAL": nor, "TANGENT": tan, "TEXCOORD_0": uv},
                "indices": idx,
                "material": material_index,
                "mode": 4,
            }],
        })
        return mesh_index, mins, maxs

    materials = []
    for material in source["materials"]:
        materials.append({
            "name": material["id"],
            "pbrMetallicRoughness": {
                "baseColorFactor": [float(v) for v in material["base_color_factor"]],
                "metallicFactor": float(material["metallic_factor"]),
                "roughnessFactor": float(material["roughness_factor"]),
            },
        })

    total_render_vertices = total_render_indices = total_collision_vertices = total_collision_indices = 0
    for module in source["modules"]:
        render_data = build_render(module)
        collision_data = build_collision(module)
        render_mesh, rmin, rmax = add_mesh(module["id"] + "__render", render_data, 0)
        collision_mesh, cmin, cmax = add_mesh(module["id"] + "__collision", collision_data, 1)
        translation = [float(v) for v in module["gallery_translation_m"]]
        common_extras = {
            "asset_id": module["id"],
            "status": STATUS,
            "units": "metres",
            "snap_grid_m": float(source["snap_grid_m"]),
            "snap_points_m": [[float(v) for v in p] for p in module["snap_points_m"]],
            "pivot_rule": module["pivot_rule"],
        }
        render_index = len(nodes)
        nodes.append({
            "name": module["id"] + "__render_node",
            "mesh": render_mesh,
            "translation": translation,
            "extras": {**common_extras, "purpose": "render_source", "collision_type": module["collision_type"]},
        })
        render_nodes.append(render_index)
        collision_index = len(nodes)
        nodes.append({
            "name": module["id"] + "__collision_node",
            "mesh": collision_mesh,
            "translation": translation,
            "extras": {**common_extras, "purpose": "collision_proxy", "render_asset_id": module["id"], "collision_type": module["collision_type"]},
        })
        collision_nodes.append(collision_index)
        rv, ri = len(render_data["positions"]), len(render_data["indices"])
        cv, ci = len(collision_data["positions"]), len(collision_data["indices"])
        total_render_vertices += rv
        total_render_indices += ri
        total_collision_vertices += cv
        total_collision_indices += ci
        module_manifest.append({
            "id": module["id"],
            "render_vertices": rv,
            "render_indices": ri,
            "collision_vertices": cv,
            "collision_indices": ci,
            "render_bounds_min_m": rmin,
            "render_bounds_max_m": rmax,
            "collision_bounds_min_m": cmin,
            "collision_bounds_max_m": cmax,
            "collision_type": module["collision_type"],
            "snap_points_m": [[float(v) for v in p] for p in module["snap_points_m"]],
        })

    gltf = {
        "asset": {"version": "2.0", "generator": GENERATOR_ID},
        "scene": 0,
        "scenes": [
            {"name": "RenderGallery", "nodes": render_nodes},
            {"name": "CollisionGallery", "nodes": collision_nodes},
        ],
        "nodes": nodes,
        "meshes": meshes,
        "materials": materials,
        "buffers": [{"byteLength": len(blob), "uri": "data:application/octet-stream;base64," + base64.b64encode(bytes(blob)).decode("ascii")}],
        "bufferViews": buffer_views,
        "accessors": accessors,
    }
    manifest_template = {
        "schema_version": 1,
        "status": STATUS,
        "generator_id": GENERATOR_ID,
        "coordinate_system": source["coordinate_system"],
        "units": "metres",
        "snap_grid_m": float(source["snap_grid_m"]),
        "uv_units_per_meter": float(source["uv_units_per_meter"]),
        "counts": {
            "render_modules": len(source["modules"]),
            "collision_proxies": len(source["modules"]),
            "meshes": len(meshes),
            "nodes": len(nodes),
            "scenes": 2,
            "materials": len(materials),
            "render_vertices": total_render_vertices,
            "render_indices": total_render_indices,
            "collision_vertices": total_collision_vertices,
            "collision_indices": total_collision_indices,
        },
        "modules": module_manifest,
        "provenance": source["provenance"],
    }
    return gltf, manifest_template


def render_outputs(source_path: Path):
    source = load_json(source_path)
    gltf, manifest = build_gltf(source)
    gltf_bytes = compact_json_bytes(gltf)
    source_bytes = canonical_bytes(source)
    manifest = dict(manifest)
    manifest["source_sha256"] = sha256_bytes(source_bytes)
    manifest["gltf_sha256"] = sha256_bytes(gltf_bytes)
    manifest["gltf_bytes"] = len(gltf_bytes)
    return gltf_bytes, canonical_bytes(manifest)


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--gltf", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)
    gltf_bytes, manifest_bytes = render_outputs(args.source)
    if args.check:
        if not args.gltf.exists() or not args.manifest.exists():
            raise SystemExit("FAIL: pinned glTF/manifest missing")
        if args.gltf.read_bytes() != gltf_bytes:
            raise SystemExit("FAIL: pinned glTF is stale")
        if args.manifest.read_bytes() != manifest_bytes:
            raise SystemExit("FAIL: expected manifest is stale")
        print("PASS: modular architecture v1 matches pinned source outputs")
        return 0
    for path in (args.gltf, args.manifest):
        if path.exists():
            raise SystemExit(f"FAIL: refusing to overwrite existing output: {path}")
        path.parent.mkdir(parents=True, exist_ok=True)
    args.gltf.write_bytes(gltf_bytes)
    args.manifest.write_bytes(manifest_bytes)
    print("PASS: generated modular architecture v1 source fixture")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
