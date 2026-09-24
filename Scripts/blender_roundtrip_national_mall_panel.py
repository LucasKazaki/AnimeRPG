#!/usr/bin/env python3
"""Blender 5.2.2 background driver for the ART-006B National Mall panel.

Run only through Blender's Python interpreter. The script refuses to overwrite
outputs, imports the source glTF into a clean scene, saves an editable .blend,
exports an embedded glTF with the pinned ART-006D profile, and writes a receipt.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import sys
from pathlib import Path

import bpy  # type: ignore

EXPECTED_VERSION = (5, 2, 2)
EXPECTED_OBJECTS = {
    "Lawn",
    "Path_Longitudinal_PosX",
    "Path_Longitudinal_NegX",
    "Path_End_PosZ",
    "Path_End_NegZ",
    "Grove_Envelope_PosX",
    "Grove_Envelope_NegX",
}
EXPECTED_MATERIALS = {"Lawn_Blockout", "Gravel_Path_Blockout", "Tree_Grove_Envelope"}
EXPORT_SETTINGS = {
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


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def resource(path: Path, recorded_path: str) -> dict[str, object]:
    return {"path": recorded_path.replace("\\", "/"), "sha256": sha256_file(path), "bytes": path.stat().st_size}


def parse_script_args() -> argparse.Namespace:
    argv = sys.argv
    args = argv[argv.index("--") + 1:] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--blend", required=True)
    parser.add_argument("--receipt", required=True)
    return parser.parse_args(args)


def require_fresh(path: Path) -> None:
    if path.exists():
        raise RuntimeError(f"refusing to overwrite existing output: {path}")
    path.parent.mkdir(parents=True, exist_ok=True)


def validate_import_inventory() -> tuple[list[str], list[str]]:
    mesh_objects = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    names = sorted(obj.name for obj in mesh_objects)
    if set(names) != EXPECTED_OBJECTS or len(names) != len(EXPECTED_OBJECTS):
        raise RuntimeError(f"imported mesh inventory mismatch: {names}")

    material_names = sorted({slot.material.name for obj in mesh_objects for slot in obj.material_slots if slot.material is not None})
    if set(material_names) != EXPECTED_MATERIALS:
        raise RuntimeError(f"imported material inventory mismatch: {material_names}")

    for obj in mesh_objects:
        dims = tuple(float(v) for v in obj.dimensions)
        if not all(math.isfinite(v) and v > 0.0 for v in dims):
            raise RuntimeError(f"invalid imported object dimensions for {obj.name}: {dims}")
    return names, material_names


def main() -> None:
    args = parse_script_args()
    if tuple(bpy.app.version[:3]) != EXPECTED_VERSION:
        raise RuntimeError(f"ART-006D requires Blender {EXPECTED_VERSION}, observed {tuple(bpy.app.version[:3])}")
    if not bool(bpy.app.background):
        raise RuntimeError("ART-006D must run in Blender background mode")

    input_path = Path(args.input).resolve(strict=True)
    output_path = Path(args.output).resolve()
    blend_path = Path(args.blend).resolve()
    receipt_path = Path(args.receipt).resolve()
    for path in (output_path, blend_path, receipt_path):
        require_fresh(path)

    input_sha = sha256_file(input_path)
    input_bytes = input_path.stat().st_size

    bpy.ops.wm.read_factory_settings(use_empty=True)
    imported = bpy.ops.import_scene.gltf(
        filepath=str(input_path),
        import_pack_images=False,
        merge_vertices=False,
        import_shading="NORMALS",
        bone_heuristic="BLENDER",
        import_scene_extras=True,
        import_scene_as_collection=False,
        import_merge_material_slots=False,
        import_select_created_objects=True,
    )
    if "FINISHED" not in imported:
        raise RuntimeError(f"glTF import did not finish: {sorted(imported)}")
    object_names, material_names = validate_import_inventory()

    saved = bpy.ops.wm.save_as_mainfile(filepath=str(blend_path), check_existing=False)
    if "FINISHED" not in saved:
        raise RuntimeError(f".blend save did not finish: {sorted(saved)}")

    exported = bpy.ops.export_scene.gltf(filepath=str(output_path), check_existing=False, **EXPORT_SETTINGS)
    if "FINISHED" not in exported:
        raise RuntimeError(f"glTF export did not finish: {sorted(exported)}")
    if not output_path.is_file() or not blend_path.is_file():
        raise RuntimeError("expected Blender outputs were not created")

    receipt = {
        "schema_version": 1,
        "task_id": "ART-006D",
        "loop_id": "astral-art-hourly-20260922",
        "status": "dcc_roundtrip_executed_not_astral_imported",
        "run": {
            "kind": "native_blender_background",
            "background_mode": True,
            "script_completed": True,
            "working_directory": Path.cwd().as_posix(),
        },
        "blender": {
            "version": ".".join(str(v) for v in EXPECTED_VERSION),
            "version_tuple": list(EXPECTED_VERSION),
            "executable": Path(bpy.app.binary_path).as_posix(),
        },
        "input": {"path": args.input.replace("\\", "/"), "sha256": input_sha, "bytes": input_bytes},
        "output": {
            "gltf": resource(output_path, args.output),
            "blend": resource(blend_path, args.blend),
        },
        "imported_scene": {
            "mesh_object_count": len(object_names),
            "object_names": object_names,
            "material_names": material_names,
        },
        "export_settings": EXPORT_SETTINGS,
    }
    receipt_path.write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n", encoding="utf-8", newline="\n")
    print(f"PASS: ART-006D Blender round-trip created {output_path.name}, {blend_path.name}, {receipt_path.name}")


if __name__ == "__main__":
    main()
