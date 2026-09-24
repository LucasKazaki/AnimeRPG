#!/usr/bin/env python3
"""Verify ART-006D Blender 5.2.2 glTF round-trip evidence without claiming Astral import."""
from __future__ import annotations

import argparse
import base64
import hashlib
import json
import math
import struct
from pathlib import Path
from typing import Any

NAMES = (
    "Lawn",
    "Path_Longitudinal_PosX",
    "Path_Longitudinal_NegX",
    "Path_End_PosZ",
    "Path_End_NegZ",
    "Grove_Envelope_PosX",
    "Grove_Envelope_NegX",
)
MATS = {"Lawn_Blockout", "Gravel_Path_Blockout", "Tree_Grove_Envelope"}
ATTR = {"POSITION", "NORMAL", "TANGENT", "TEXCOORD_0"}
TOLERANCE = 1e-4
EXPECTED_SOURCE_SHA = "6c51463332199c65bcfbde04ee8e5883e03a94aba710980eebfaa6945f2759b7"
EXPECTED_MANIFEST = {
    "schema_version": 1,
    "asset_id": "national-mall-core-panel-module-v1",
    "status": "source_validated_not_imported",
    "gltf_file": "mall_core_panel_blockout.gltf",
    "unique_meshes": 3,
    "mesh_instances": 7,
    "materials": 3,
    "unit_box_vertices": 24,
    "unit_box_indices": 36,
}
EXPORT = {
    "export_format": "GLTF_EMBEDDED",
    "export_texcoords": True,
    "export_normals": True,
    "export_tangents": True,
    "export_materials": "EXPORT",
    "export_image_format": "NONE",
    "export_cameras": False,
    "export_lights": False,
    "export_extras": True,
    "export_yup": True,
    "export_apply": False,
    "export_animations": False,
    "export_gpu_instances": False,
}
CF = {
    5120: ("b", 1),
    5121: ("B", 1),
    5122: ("h", 2),
    5123: ("H", 2),
    5125: ("I", 4),
    5126: ("f", 4),
}
NC = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4}


class VerificationError(ValueError):
    pass


def req(condition: bool, message: str) -> None:
    if not condition:
        raise VerificationError(message)


def jint(value: Any) -> bool:
    return type(value) is int


def num(value: Any) -> bool:
    return type(value) in (int, float) and math.isfinite(float(value))


def load(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:
        raise VerificationError(f"cannot read JSON {path}: {exc}") from exc
    req(type(value) is dict, f"JSON root must be object: {path}")
    return value


def sha(path: Path) -> str:
    digest = hashlib.sha256()
    try:
        with path.open("rb") as handle:
            for block in iter(lambda: handle.read(1 << 20), b""):
                digest.update(block)
    except OSError as exc:
        raise VerificationError(f"cannot hash {path}: {exc}") from exc
    return digest.hexdigest()


def resource(path: Path) -> dict[str, Any]:
    return {"sha256": sha(path), "bytes": path.stat().st_size}


def buffers(gltf: dict[str, Any]) -> list[bytes]:
    result: list[bytes] = []
    source_buffers = gltf.get("buffers")
    req(type(source_buffers) is list and bool(source_buffers), "glTF requires buffers")
    prefix = "data:application/octet-stream;base64,"
    for index, item in enumerate(source_buffers):
        req(
            type(item) is dict
            and type(item.get("uri")) is str
            and item["uri"].startswith(prefix),
            f"buffer {index} must be embedded",
        )
        try:
            payload = base64.b64decode(item["uri"][len(prefix) :], validate=True)
        except Exception as exc:
            raise VerificationError(f"buffer {index} base64 invalid: {exc}") from exc
        req(
            jint(item.get("byteLength")) and item["byteLength"] == len(payload),
            f"buffer {index} length mismatch",
        )
        result.append(payload)
    return result


def accessor(
    gltf: dict[str, Any], source_buffers: list[bytes], accessor_index: int
) -> list[tuple[float | int, ...]]:
    accessors = gltf.get("accessors")
    buffer_views = gltf.get("bufferViews")
    req(
        type(accessors) is list
        and type(buffer_views) is list
        and jint(accessor_index)
        and 0 <= accessor_index < len(accessors),
        "accessor index invalid",
    )
    item = accessors[accessor_index]
    req(type(item) is dict and "sparse" not in item, f"accessor {accessor_index} unsupported")
    view_index = item.get("bufferView")
    component_type = item.get("componentType")
    count = item.get("count")
    value_type = item.get("type")
    req(
        jint(view_index)
        and 0 <= view_index < len(buffer_views)
        and jint(component_type)
        and component_type in CF
        and jint(count)
        and count >= 0
        and value_type in NC,
        f"accessor {accessor_index} contract invalid",
    )
    view = buffer_views[view_index]
    req(type(view) is dict, "bufferView invalid")
    buffer_index = view.get("buffer")
    view_offset = view.get("byteOffset", 0)
    view_length = view.get("byteLength")
    accessor_offset = item.get("byteOffset", 0)
    req(
        jint(buffer_index)
        and 0 <= buffer_index < len(source_buffers)
        and jint(view_offset)
        and view_offset >= 0
        and jint(view_length)
        and view_length >= 0
        and jint(accessor_offset)
        and accessor_offset >= 0,
        f"accessor {accessor_index} offsets invalid",
    )
    code, component_size = CF[component_type]
    component_count = NC[value_type]
    packed_size = component_size * component_count
    stride = view.get("byteStride", packed_size)
    req(
        jint(stride) and stride >= packed_size and stride % component_size == 0,
        f"accessor {accessor_index} stride invalid",
    )
    start = view_offset + accessor_offset
    end = start + (stride * (count - 1) + packed_size if count else 0)
    req(
        end <= view_offset + view_length and end <= len(source_buffers[buffer_index]),
        f"accessor {accessor_index} escapes buffer",
    )
    fmt = "<" + code * component_count
    values: list[tuple[float | int, ...]] = []
    for item_index in range(count):
        unpacked = struct.unpack_from(fmt, source_buffers[buffer_index], start + item_index * stride)
        req(
            component_type != 5126 or all(math.isfinite(float(x)) for x in unpacked),
            f"accessor {accessor_index} nonfinite",
        )
        values.append(unpacked)
    return values


def acontract(gltf: dict[str, Any], accessor_index: Any, component_type: int, value_type: str, label: str) -> None:
    accessors = gltf.get("accessors")
    req(
        type(accessors) is list
        and jint(accessor_index)
        and 0 <= accessor_index < len(accessors),
        f"{label} accessor invalid",
    )
    item = accessors[accessor_index]
    req(
        type(item) is dict
        and jint(item.get("componentType"))
        and item.get("componentType") == component_type
        and item.get("type") == value_type,
        f"{label} accessor format invalid",
    )


def ident() -> list[list[float]]:
    return [
        [1.0, 0.0, 0.0, 0.0],
        [0.0, 1.0, 0.0, 0.0],
        [0.0, 0.0, 1.0, 0.0],
        [0.0, 0.0, 0.0, 1.0],
    ]


def mul(a: list[list[float]], b: list[list[float]]) -> list[list[float]]:
    return [[sum(a[row][k] * b[k][column] for k in range(4)) for column in range(4)] for row in range(4)]


def local(node: dict[str, Any]) -> list[list[float]]:
    if "matrix" in node:
        matrix = node["matrix"]
        req(
            type(matrix) is list
            and len(matrix) == 16
            and all(num(value) for value in matrix),
            "node matrix invalid",
        )
        return [[float(matrix[column * 4 + row]) for column in range(4)] for row in range(4)]

    translation = node.get("translation", [0, 0, 0])
    scale = node.get("scale", [1, 1, 1])
    rotation = node.get("rotation", [0, 0, 0, 1])
    req(
        all(type(value) is list for value in (translation, scale, rotation))
        and len(translation) == 3
        and len(scale) == 3
        and len(rotation) == 4
        and all(num(value) for value in translation + scale + rotation),
        "node TRS invalid",
    )
    x, y, z, w = map(float, rotation)
    length = math.sqrt(x * x + y * y + z * z + w * w)
    req(length > 0, "zero quaternion")
    x, y, z, w = [value / length for value in (x, y, z, w)]
    rotation_matrix = [
        [1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w), 0],
        [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w), 0],
        [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y), 0],
        [0, 0, 0, 1],
    ]
    scale_matrix = ident()
    translation_matrix = ident()
    scale_matrix[0][0], scale_matrix[1][1], scale_matrix[2][2] = map(float, scale)
    translation_matrix[0][3], translation_matrix[1][3], translation_matrix[2][3] = map(float, translation)
    return mul(translation_matrix, mul(rotation_matrix, scale_matrix))


def det3(matrix: list[list[float]]) -> float:
    return (
        matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1])
        - matrix[0][1] * (matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0])
        + matrix[0][2] * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0])
    )


def parity(matrix: list[list[float]]) -> int:
    determinant = det3(matrix)
    req(
        math.isfinite(determinant) and abs(determinant) > 1e-12,
        "mesh world transform singular",
    )
    return 1 if determinant > 0 else -1


def world_signature(matrix: list[list[float]]) -> tuple[float, ...]:
    return tuple(float(matrix[row][column]) for row in range(3) for column in range(4))


def pt(matrix: list[list[float]], position: tuple[float | int, ...]) -> tuple[float, float, float]:
    vector = [float(position[0]), float(position[1]), float(position[2]), 1.0]
    return tuple(sum(matrix[row][k] * vector[k] for k in range(4)) for row in range(3))  # type: ignore[return-value]


def materials(gltf: dict[str, Any]) -> dict[str, Any]:
    source_materials = gltf.get("materials")
    req(type(source_materials) is list, "materials missing")
    result: dict[str, Any] = {}
    for material in source_materials:
        req(type(material) is dict and type(material.get("name")) is str, "material invalid")
        name = material["name"]
        req(name not in result, "duplicate material name")
        pbr = material.get("pbrMetallicRoughness", {})
        req(type(pbr) is dict, "material pbr invalid")
        base = pbr.get("baseColorFactor", [1, 1, 1, 1])
        metallic = pbr.get("metallicFactor", 1.0)
        roughness = pbr.get("roughnessFactor", 1.0)
        emissive = material.get("emissiveFactor", [0, 0, 0])
        alpha = material.get("alphaMode", "OPAQUE")
        cutoff = material.get("alphaCutoff", 0.5)
        double_sided = material.get("doubleSided", False)
        req(type(base) is list and len(base) == 4 and all(num(value) for value in base), "material baseColorFactor invalid")
        req(num(metallic) and num(roughness), "material metallic/roughness invalid")
        req(type(emissive) is list and len(emissive) == 3 and all(num(value) for value in emissive), "material emissiveFactor invalid")
        req(alpha in ("OPAQUE", "MASK", "BLEND") and num(cutoff), "material alpha invalid")
        req(type(double_sided) is bool, "material doubleSided invalid")
        req("extensions" not in material, "material extensions unsupported in ART-006D")
        result[name] = (
            tuple(map(float, base)),
            float(metallic),
            float(roughness),
            tuple(map(float, emissive)),
            alpha,
            float(cutoff) if alpha == "MASK" else None,
            double_sided,
        )
    return result


def close(a: Any, b: Any) -> bool:
    return len(a) == len(b) and all(abs(float(x) - float(y)) <= TOLERANCE for x, y in zip(a, b))


def corner(
    positions: list[tuple[float | int, ...]],
    normals: list[tuple[float | int, ...]],
    tangents: list[tuple[float | int, ...]],
    uvs: list[tuple[float | int, ...]],
    index: int,
) -> tuple[tuple[float, ...], tuple[float, ...], tuple[float, ...], tuple[float, ...]]:
    return (
        tuple(map(float, positions[index][:3])),
        tuple(map(float, normals[index][:3])),
        tuple(map(float, tangents[index][:4])),
        tuple(map(float, uvs[index][:2])),
    )


def corner_close(a: Any, b: Any) -> bool:
    return len(a) == len(b) and all(close(left, right) for left, right in zip(a, b))


def triangle_close(a: Any, b: Any) -> bool:
    if len(a) != 3 or len(b) != 3:
        return False
    for shift in range(3):
        if all(corner_close(a[index], b[(index + shift) % 3]) for index in range(3)):
            return True
    return False


def topology_close(source: Any, output: Any) -> bool:
    if len(source) != len(output):
        return False
    source_by_material: dict[str, list[Any]] = {}
    output_by_material: dict[str, list[Any]] = {}
    for material, triangle in source:
        source_by_material.setdefault(material, []).append(triangle)
    for material, triangle in output:
        output_by_material.setdefault(material, []).append(triangle)
    if set(source_by_material) != set(output_by_material):
        return False
    for material, source_triangles in source_by_material.items():
        output_triangles = output_by_material[material]
        if len(source_triangles) != len(output_triangles):
            return False
        adjacency = [
            [index for index, candidate in enumerate(output_triangles) if triangle_close(triangle, candidate)]
            for triangle in source_triangles
        ]
        if any(not candidates for candidates in adjacency):
            return False
        matched_source: list[int | None] = [None] * len(output_triangles)

        def augment(source_index: int, seen: set[int]) -> bool:
            for output_index in adjacency[source_index]:
                if output_index in seen:
                    continue
                seen.add(output_index)
                previous = matched_source[output_index]
                if previous is None or augment(previous, seen):
                    matched_source[output_index] = source_index
                    return True
            return False

        if not all(augment(index, set()) for index in range(len(source_triangles))):
            return False
    return True


def validate_primitive(
    gltf: dict[str, Any],
    source_buffers: list[bytes],
    source_materials: list[Any],
    primitive: Any,
    label: str,
) -> tuple[
    str,
    list[tuple[float | int, ...]],
    list[tuple[float | int, ...]],
    list[tuple[float | int, ...]],
    list[tuple[float | int, ...]],
    list[int],
]:
    req(type(primitive) is dict, f"{label} invalid")
    req("targets" not in primitive, "morph targets unsupported in ART-006D")
    mode = primitive.get("mode", 4)
    index_accessor = primitive.get("indices")
    req(jint(mode) and mode == 4 and jint(index_accessor), f"{label} requires indexed TRIANGLES")
    attributes = primitive.get("attributes")
    req(type(attributes) is dict and ATTR <= set(attributes), f"{label} required attributes missing")
    req(set(attributes) == ATTR, f"{label} unexpected rendering attributes")
    for key, value_type in (
        ("POSITION", "VEC3"),
        ("NORMAL", "VEC3"),
        ("TANGENT", "VEC4"),
        ("TEXCOORD_0", "VEC2"),
    ):
        acontract(gltf, attributes[key], 5126, value_type, f"{label} {key}")
    accessors = gltf.get("accessors")
    req(
        type(accessors) is list and 0 <= index_accessor < len(accessors),
        f"{label} index accessor invalid",
    )
    index_item = accessors[index_accessor]
    req(
        type(index_item) is dict
        and jint(index_item.get("componentType"))
        and index_item.get("componentType") in (5121, 5123, 5125)
        and index_item.get("type") == "SCALAR",
        f"{label} index accessor format invalid",
    )
    positions = accessor(gltf, source_buffers, attributes["POSITION"])
    normals = accessor(gltf, source_buffers, attributes["NORMAL"])
    tangents = accessor(gltf, source_buffers, attributes["TANGENT"])
    uvs = accessor(gltf, source_buffers, attributes["TEXCOORD_0"])
    req(
        len(positions) > 0
        and len(normals) == len(positions) == len(tangents) == len(uvs),
        f"{label} attribute counts invalid",
    )
    indices = [int(value[0]) for value in accessor(gltf, source_buffers, index_accessor)]
    req(
        bool(indices)
        and len(indices) % 3 == 0
        and min(indices) >= 0
        and max(indices) < len(positions),
        f"{label} indices invalid",
    )
    material_index = primitive.get("material")
    req(
        jint(material_index)
        and 0 <= material_index < len(source_materials)
        and type(source_materials[material_index]) is dict
        and type(source_materials[material_index].get("name")) is str,
        f"{label} material invalid",
    )
    return (
        source_materials[material_index]["name"],
        positions,
        normals,
        tangents,
        uvs,
        indices,
    )


def semantics(gltf: dict[str, Any]) -> dict[str, Any]:
    used = gltf.get("extensionsUsed", [])
    required = gltf.get("extensionsRequired", [])
    req(
        type(used) is list
        and all(type(item) is str for item in used)
        and type(required) is list
        and all(type(item) is str for item in required),
        "glTF extensions declarations invalid",
    )
    req(
        "EXT_mesh_gpu_instancing" not in used and "EXT_mesh_gpu_instancing" not in required,
        "GPU instancing unsupported in ART-006D",
    )
    req(
        "KHR_lights_punctual" not in used and "KHR_lights_punctual" not in required,
        "lights unsupported in ART-006D",
    )
    root_extensions = gltf.get("extensions", {})
    req(type(root_extensions) is dict, "glTF root extensions invalid")
    req("EXT_mesh_gpu_instancing" not in root_extensions, "GPU instancing unsupported in ART-006D")
    req("KHR_lights_punctual" not in root_extensions, "lights unsupported in ART-006D")

    nodes = gltf.get("nodes")
    meshes = gltf.get("meshes")
    source_materials = gltf.get("materials", [])
    scenes = gltf.get("scenes")
    scene_index = gltf.get("scene", 0)
    req(
        type(nodes) is list
        and type(meshes) is list
        and type(source_materials) is list
        and type(scenes) is list
        and jint(scene_index)
        and 0 <= scene_index < len(scenes),
        "scene graph invalid",
    )
    source_buffers = buffers(gltf)

    for node in nodes:
        req(type(node) is dict, "node invalid")
        extensions = node.get("extensions", {})
        req(type(extensions) is dict, "node extensions invalid")
        req("EXT_mesh_gpu_instancing" not in extensions, "GPU instancing unsupported in ART-006D")
        req("KHR_lights_punctual" not in extensions, "lights unsupported in ART-006D")
        req("camera" not in node, "cameras unsupported in ART-006D")
        req("weights" not in node, "morph targets unsupported in ART-006D")

    for mesh_index, mesh in enumerate(meshes):
        req(type(mesh) is dict, "mesh invalid")
        req("weights" not in mesh, "morph targets unsupported in ART-006D")
        primitives = mesh.get("primitives")
        req(type(primitives) is list and bool(primitives), "mesh primitives missing")
        for primitive_index, primitive in enumerate(primitives):
            validate_primitive(
                gltf,
                source_buffers,
                source_materials,
                primitive,
                f"mesh {mesh_index} primitive {primitive_index}",
            )

    scene = scenes[scene_index]
    req(type(scene) is dict, "scene invalid")
    roots = scene.get("nodes")
    req(type(roots) is list, "scene roots invalid")
    seen: set[int] = set()
    result: dict[str, Any] = {}

    def walk(node_index: Any, parent: list[list[float]]) -> None:
        req(
            jint(node_index) and 0 <= node_index < len(nodes) and node_index not in seen,
            "node graph invalid/cyclic",
        )
        seen.add(node_index)
        node = nodes[node_index]
        world = mul(parent, local(node))
        name = node.get("name")
        mesh_index = node.get("mesh")
        if mesh_index is not None:
            req(type(name) is str and name in NAMES, f"unexpected mesh instance {name!r}")
            req(name not in result, f"duplicate mesh instance {name}")
            req(jint(mesh_index) and 0 <= mesh_index < len(meshes), f"{name} mesh invalid")
            mesh = meshes[mesh_index]
            primitives = mesh.get("primitives")
            req(type(primitives) is list and bool(primitives), f"{name} primitives missing")
            minimums = [math.inf] * 3
            maximums = [-math.inf] * 3
            bound_materials: set[str] = set()
            topology: list[Any] = []
            for primitive_index, primitive in enumerate(primitives):
                material_name, positions, normals, tangents, uvs, indices = validate_primitive(
                    gltf,
                    source_buffers,
                    source_materials,
                    primitive,
                    f"{name} primitive {primitive_index}",
                )
                bound_materials.add(material_name)
                for index in indices:
                    world_position = pt(world, positions[index])
                    for axis in range(3):
                        minimums[axis] = min(minimums[axis], world_position[axis])
                        maximums[axis] = max(maximums[axis], world_position[axis])
                for offset in range(0, len(indices), 3):
                    triangle = tuple(
                        corner(positions, normals, tangents, uvs, indices[offset + corner_index])
                        for corner_index in range(3)
                    )
                    topology.append((material_name, triangle))
            result[name] = {
                "center": tuple((minimums[axis] + maximums[axis]) / 2 for axis in range(3)),
                "dimensions": tuple(maximums[axis] - minimums[axis] for axis in range(3)),
                "materials": bound_materials,
                "topology": topology,
                "parity": parity(world),
                "world": world_signature(world),
            }
        elif name in NAMES:
            raise VerificationError(f"{name} expected mesh instance missing mesh")

        children = node.get("children", [])
        req(type(children) is list, "children invalid")
        for child in children:
            walk(child, world)

    for root in roots:
        walk(root, ident())
    req(set(result) == set(NAMES), f"expected seven named mesh instances; got {sorted(result)}")
    return result


def strict_resource(receipt: Any, path: Path, label: str) -> None:
    req(
        type(receipt) is dict and set(receipt) == {"path", "sha256", "bytes"},
        f"receipt {label} fields invalid",
    )
    actual = resource(path)
    req(
        type(receipt["path"]) is str and Path(receipt["path"]).name == path.name,
        f"receipt {label} path invalid",
    )
    req(
        type(receipt["sha256"]) is str and receipt["sha256"] == actual["sha256"],
        f"receipt {label} hash mismatch",
    )
    req(
        jint(receipt["bytes"]) and receipt["bytes"] == actual["bytes"],
        f"receipt {label} bytes mismatch",
    )


def matclose(a: Any, b: Any) -> bool:
    return (
        a[4] == b[4]
        and a[6] is b[6]
        and (
            (a[5] is None and b[5] is None)
            or (
                a[5] is not None
                and b[5] is not None
                and abs(a[5] - b[5]) <= TOLERANCE
            )
        )
        and close(a[0], b[0])
        and abs(a[1] - b[1]) <= TOLERANCE
        and abs(a[2] - b[2]) <= TOLERANCE
        and close(a[3], b[3])
    )


def exact_names(value: Any, expected: Any) -> None:
    req(
        type(value) is list
        and all(type(item) is str for item in value)
        and len(value) == len(expected)
        and set(value) == set(expected),
        "import inventory invalid",
    )


def exact_export(value: Any) -> None:
    req(type(value) is dict and set(value) == set(EXPORT), "export settings drift")
    for key, expected in EXPORT.items():
        req(type(value[key]) is type(expected) and value[key] == expected, "export settings drift")


def empty_optional_arrays(gltf: dict[str, Any]) -> None:
    for key in ("animations", "images", "textures", "cameras"):
        if key in gltf:
            req(type(gltf[key]) is list and not gltf[key], "round-trip glTF scope invalid")


def verify(
    source: Path,
    manifest: Path,
    roundtrip: Path,
    blend: Path,
    receipt_path: Path,
) -> dict[str, Any]:
    source_gltf = load(source)
    expected_manifest = load(manifest)
    output_gltf = load(roundtrip)
    receipt = load(receipt_path)

    for key, expected in EXPECTED_MANIFEST.items():
        req(
            type(expected_manifest.get(key)) is type(expected)
            and expected_manifest.get(key) == expected,
            f"ART-006B manifest identity drift: {key}",
        )
    req(
        type(expected_manifest.get("gltf_sha256")) is str
        and expected_manifest["gltf_sha256"] == EXPECTED_SOURCE_SHA
        and sha(source) == EXPECTED_SOURCE_SHA,
        "source no longer matches pinned ART-006B input",
    )
    req(
        jint(expected_manifest.get("gltf_bytes"))
        and expected_manifest["gltf_bytes"] == source.stat().st_size,
        "ART-006B manifest byte count drift",
    )

    root_fields = {
        "schema_version",
        "task_id",
        "loop_id",
        "status",
        "run",
        "blender",
        "input",
        "output",
        "imported_scene",
        "export_settings",
    }
    req(set(receipt) == root_fields, "receipt root fields invalid")
    req(
        jint(receipt.get("schema_version"))
        and receipt["schema_version"] == 1
        and receipt.get("task_id") == "ART-006D"
        and receipt.get("loop_id") == "astral-art-hourly-20260922"
        and receipt.get("status") == "dcc_roundtrip_executed_not_astral_imported",
        "receipt identity/status invalid",
    )
    run = receipt.get("run")
    req(
        type(run) is dict
        and set(run) == {"kind", "background_mode", "script_completed", "working_directory"}
        and run["kind"] == "native_blender_background"
        and run["background_mode"] is True
        and run["script_completed"] is True
        and type(run["working_directory"]) is str
        and bool(run["working_directory"]),
        "receipt run invalid",
    )
    blender_info = receipt.get("blender")
    req(
        type(blender_info) is dict
        and set(blender_info) == {"version", "version_tuple", "executable"}
        and blender_info["version"] == "5.2.2"
        and type(blender_info["version_tuple"]) is list
        and blender_info["version_tuple"] == [5, 2, 2]
        and all(jint(value) for value in blender_info["version_tuple"])
        and type(blender_info["executable"]) is str
        and bool(blender_info["executable"]),
        "Blender evidence invalid",
    )
    strict_resource(receipt.get("input"), source, "input")
    output_receipt = receipt.get("output")
    req(type(output_receipt) is dict and set(output_receipt) == {"gltf", "blend"}, "receipt output invalid")
    strict_resource(output_receipt["gltf"], roundtrip, "output.gltf")
    strict_resource(output_receipt["blend"], blend, "output.blend")
    imported_scene = receipt.get("imported_scene")
    req(
        type(imported_scene) is dict
        and set(imported_scene) == {"mesh_object_count", "object_names", "material_names"}
        and jint(imported_scene.get("mesh_object_count"))
        and imported_scene["mesh_object_count"] == 7,
        "import inventory invalid",
    )
    exact_names(imported_scene.get("object_names"), NAMES)
    exact_names(imported_scene.get("material_names"), MATS)
    exact_export(receipt.get("export_settings"))

    req(
        type(output_gltf.get("asset")) is dict and output_gltf["asset"].get("version") == "2.0",
        "round-trip glTF scope invalid",
    )
    empty_optional_arrays(output_gltf)
    source_materials = materials(source_gltf)
    output_materials = materials(output_gltf)
    req(set(source_materials) == MATS and set(output_materials) == MATS, "material inventory drift")
    for name in MATS:
        req(matclose(source_materials[name], output_materials[name]), f"{name} material property drift")

    source_semantics = semantics(source_gltf)
    output_semantics = semantics(output_gltf)
    for name in NAMES:
        req(
            close(source_semantics[name]["center"], output_semantics[name]["center"])
            and close(source_semantics[name]["dimensions"], output_semantics[name]["dimensions"])
            and source_semantics[name]["materials"] == output_semantics[name]["materials"],
            f"{name} transform/material binding drift",
        )
        req(
            source_semantics[name]["parity"] == output_semantics[name]["parity"],
            f"{name} transform parity drift",
        )
        req(
            close(source_semantics[name]["world"], output_semantics[name]["world"]),
            f"{name} world transform drift",
        )
        req(
            topology_close(source_semantics[name]["topology"], output_semantics[name]["topology"]),
            f"{name} topology/attribute drift",
        )

    return {
        "nodes": 7,
        "source_sha256": sha(source),
        "roundtrip_sha256": sha(roundtrip),
        "blend_sha256": sha(blend),
        "blender": "5.2.2",
        "tolerance_m": TOLERANCE,
        "status": "dcc_roundtrip_verified_not_astral_imported",
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--roundtrip", required=True, type=Path)
    parser.add_argument("--blend", required=True, type=Path)
    parser.add_argument("--receipt", required=True, type=Path)
    args = parser.parse_args()
    try:
        result = verify(args.source, args.manifest, args.roundtrip, args.blend, args.receipt)
    except VerificationError as exc:
        raise SystemExit(f"FAIL: {exc}") from exc
    print(f"PASS: Blender round-trip source verification {result}")


if __name__ == "__main__":
    main()
