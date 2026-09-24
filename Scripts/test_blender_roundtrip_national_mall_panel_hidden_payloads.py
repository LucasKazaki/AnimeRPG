#!/usr/bin/env python3
from __future__ import annotations

import copy
import tempfile
from pathlib import Path

import test_blender_roundtrip_national_mall_panel as base


def wrap_lawn_under_rotated_parent(paths):
    def change(gltf):
        roots = gltf["scenes"][0]["nodes"]
        roots.remove(0)
        parent_index = len(gltf["nodes"])
        gltf["nodes"].append(
            {
                "name": "InheritedRotationParent",
                "rotation": [0, 1, 0, 0],
                "children": [0],
            }
        )
        roots.append(parent_index)
    base.mutate_output(paths, change)


def append_unreachable_mesh(paths, mutator):
    def change(gltf):
        mesh = copy.deepcopy(gltf["meshes"][0])
        mutator(mesh)
        gltf["meshes"].append(mesh)
    base.mutate_output(paths, change)


def cases():
    fail = lambda text: (lambda paths: base.expect_fail(lambda: base.run_verify(paths), text))
    return [
        (
            "GPU instancing unreachable node",
            fail("GPU instancing unsupported"),
            lambda paths: base.mutate_output(
                paths,
                lambda gltf: gltf["nodes"].append(
                    {
                        "name": "Unreachable",
                        "extensions": {
                            "EXT_mesh_gpu_instancing": {
                                "attributes": {"TRANSLATION": 0}
                            }
                        },
                    }
                ),
            ),
        ),
        (
            "punctual light payload",
            fail("lights unsupported"),
            lambda paths: base.mutate_output(
                paths,
                lambda gltf: (
                    gltf.__setitem__("extensionsUsed", ["KHR_lights_punctual"]),
                    gltf.__setitem__(
                        "extensions",
                        {"KHR_lights_punctual": {"lights": [{"type": "point"}]}},
                    ),
                    gltf["nodes"].append(
                        {
                            "name": "Light",
                            "extensions": {"KHR_lights_punctual": {"light": 0}},
                        }
                    ),
                ),
            ),
        ),
        (
            "camera node payload",
            fail("cameras unsupported"),
            lambda paths: base.mutate_output(
                paths, lambda gltf: gltf["nodes"].append({"name": "Camera", "camera": 0})
            ),
        ),
        (
            "animations wrong type",
            fail("round-trip glTF scope invalid"),
            lambda paths: base.mutate_output(
                paths, lambda gltf: gltf.__setitem__("animations", {})
            ),
        ),
        (
            "images wrong type",
            fail("round-trip glTF scope invalid"),
            lambda paths: base.mutate_output(
                paths, lambda gltf: gltf.__setitem__("images", False)
            ),
        ),
        (
            "textures wrong type",
            fail("round-trip glTF scope invalid"),
            lambda paths: base.mutate_output(
                paths, lambda gltf: gltf.__setitem__("textures", 0)
            ),
        ),
        (
            "cameras wrong type",
            fail("round-trip glTF scope invalid"),
            lambda paths: base.mutate_output(
                paths, lambda gltf: gltf.__setitem__("cameras", {})
            ),
        ),
        (
            "inherited parent world rotation drift",
            fail("world transform drift"),
            wrap_lawn_under_rotated_parent,
        ),
        (
            "primitive morph target reachable",
            fail("morph targets unsupported"),
            lambda paths: base.mutate_output(
                paths,
                lambda gltf: gltf["meshes"][0]["primitives"][0].__setitem__(
                    "targets", [{"POSITION": 0}]
                ),
            ),
        ),
        (
            "mesh weights reachable",
            fail("morph targets unsupported"),
            lambda paths: base.mutate_output(
                paths, lambda gltf: gltf["meshes"][0].__setitem__("weights", [1.0])
            ),
        ),
        (
            "node weights reachable",
            fail("morph targets unsupported"),
            lambda paths: base.mutate_output(
                paths, lambda gltf: gltf["nodes"][0].__setitem__("weights", [1.0])
            ),
        ),
        (
            "primitive morph target unreachable",
            fail("morph targets unsupported"),
            lambda paths: append_unreachable_mesh(
                paths,
                lambda mesh: mesh["primitives"][0].__setitem__(
                    "targets", [{"POSITION": 0}]
                ),
            ),
        ),
        (
            "mesh weights unreachable",
            fail("morph targets unsupported"),
            lambda paths: append_unreachable_mesh(
                paths, lambda mesh: mesh.__setitem__("weights", [1.0])
            ),
        ),
        (
            "node weights unreachable",
            fail("morph targets unsupported"),
            lambda paths: base.mutate_output(
                paths,
                lambda gltf: gltf["nodes"].append(
                    {"name": "UnreachableWeights", "weights": [1.0]}
                ),
            ),
        ),
        (
            "vertex color attribute reachable",
            fail("unexpected rendering attributes"),
            lambda paths: base.mutate_output(
                paths,
                lambda gltf: gltf["meshes"][0]["primitives"][0]["attributes"].__setitem__(
                    "COLOR_0", 0
                ),
            ),
        ),
        (
            "vertex color attribute unreachable mesh",
            fail("unexpected rendering attributes"),
            lambda paths: append_unreachable_mesh(
                paths,
                lambda mesh: mesh["primitives"][0]["attributes"].__setitem__(
                    "COLOR_0", 0
                ),
            ),
        ),
    ]


def main() -> None:
    passed = 0
    for name, check, mutate in cases():
        with tempfile.TemporaryDirectory() as temp_dir:
            paths = base.prepare(Path(temp_dir))
            mutate(paths)
            try:
                check(paths)
            except Exception as exc:
                raise AssertionError(f"case {name!r} failed: {exc}") from exc
            passed += 1
    print(f"PASS: {passed}/{len(cases())} Blender hidden-payload verifier tests")


if __name__ == "__main__":
    main()
