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
        gltf["nodes"].append({"name": "InheritedRotationParent", "rotation": [0, 1, 0, 0], "children": [0]})
        roots.append(parent_index)
    base.mutate_output(paths, change)


def append_unreachable_mesh(paths, mutator):
    def change(gltf):
        mesh = copy.deepcopy(gltf["meshes"][0])
        mutator(mesh)
        gltf["meshes"].append(mesh)
    base.mutate_output(paths, change)


def append_unreachable_mesh_with_gltf(paths, mutator):
    def change(gltf):
        mesh = copy.deepcopy(gltf["meshes"][0])
        mutator(gltf, mesh)
        gltf["meshes"].append(mesh)
    base.mutate_output(paths, change)


def clone_accessor(gltf, accessor_index):
    cloned = copy.deepcopy(gltf["accessors"][accessor_index])
    gltf["accessors"].append(cloned)
    return len(gltf["accessors"]) - 1


def unreachable_normal_zero_count(paths):
    def mutate(gltf, mesh):
        accessor_index = clone_accessor(gltf, 1)
        gltf["accessors"][accessor_index]["count"] = 0
        mesh["primitives"][0]["attributes"]["NORMAL"] = accessor_index
    append_unreachable_mesh_with_gltf(paths, mutate)


def unreachable_normal_escapes_buffer(paths):
    def mutate(gltf, mesh):
        source_accessor = copy.deepcopy(gltf["accessors"][1])
        source_view = copy.deepcopy(gltf["bufferViews"][source_accessor["bufferView"]])
        source_view["byteOffset"] = gltf["buffers"][0]["byteLength"]
        source_view["byteLength"] = 4
        gltf["bufferViews"].append(source_view)
        source_accessor["bufferView"] = len(gltf["bufferViews"]) - 1
        gltf["accessors"].append(source_accessor)
        mesh["primitives"][0]["attributes"]["NORMAL"] = len(gltf["accessors"]) - 1
    append_unreachable_mesh_with_gltf(paths, mutate)


def unreachable_normal_oversized_view(paths):
    def mutate(gltf, mesh):
        source_accessor = copy.deepcopy(gltf["accessors"][1])
        source_view = copy.deepcopy(gltf["bufferViews"][source_accessor["bufferView"]])
        source_view["byteLength"] = gltf["buffers"][0]["byteLength"]
        gltf["bufferViews"].append(source_view)
        source_accessor["bufferView"] = len(gltf["bufferViews"]) - 1
        gltf["accessors"].append(source_accessor)
        mesh["primitives"][0]["attributes"]["NORMAL"] = len(gltf["accessors"]) - 1
    append_unreachable_mesh_with_gltf(paths, mutate)


def unreachable_index_zero_count(paths):
    def mutate(gltf, mesh):
        accessor_index = clone_accessor(gltf, 4)
        gltf["accessors"][accessor_index]["count"] = 0
        mesh["primitives"][0]["indices"] = accessor_index
    append_unreachable_mesh_with_gltf(paths, mutate)


def cases():
    fail = lambda text: (lambda paths: base.expect_fail(lambda: base.run_verify(paths), text))
    return [
        ("GPU instancing unreachable node", fail("GPU instancing unsupported"), lambda p: base.mutate_output(p, lambda g: g["nodes"].append({"name":"Unreachable","extensions":{"EXT_mesh_gpu_instancing":{"attributes":{"TRANSLATION":0}}}}))),
        ("punctual light payload", fail("lights unsupported"), lambda p: base.mutate_output(p, lambda g: (g.__setitem__("extensionsUsed",["KHR_lights_punctual"]),g.__setitem__("extensions",{"KHR_lights_punctual":{"lights":[{"type":"point"}]}}),g["nodes"].append({"name":"Light","extensions":{"KHR_lights_punctual":{"light":0}}})))),
        ("camera node payload", fail("cameras unsupported"), lambda p: base.mutate_output(p, lambda g: g["nodes"].append({"name":"Camera","camera":0}))),
        ("animations wrong type", fail("round-trip glTF scope invalid"), lambda p: base.mutate_output(p, lambda g: g.__setitem__("animations",{}))),
        ("images wrong type", fail("round-trip glTF scope invalid"), lambda p: base.mutate_output(p, lambda g: g.__setitem__("images",False))),
        ("textures wrong type", fail("round-trip glTF scope invalid"), lambda p: base.mutate_output(p, lambda g: g.__setitem__("textures",0))),
        ("cameras wrong type", fail("round-trip glTF scope invalid"), lambda p: base.mutate_output(p, lambda g: g.__setitem__("cameras",{}))),
        ("inherited parent world rotation drift", fail("world transform drift"), wrap_lawn_under_rotated_parent),
        ("primitive morph target reachable", fail("morph targets unsupported"), lambda p: base.mutate_output(p, lambda g: g["meshes"][0]["primitives"][0].__setitem__("targets",[{"POSITION":0}]))),
        ("mesh weights reachable", fail("morph targets unsupported"), lambda p: base.mutate_output(p, lambda g: g["meshes"][0].__setitem__("weights",[1.0]))),
        ("node weights reachable", fail("morph targets unsupported"), lambda p: base.mutate_output(p, lambda g: g["nodes"][0].__setitem__("weights",[1.0]))),
        ("primitive morph target unreachable", fail("morph targets unsupported"), lambda p: append_unreachable_mesh(p, lambda m: m["primitives"][0].__setitem__("targets",[{"POSITION":0}]))),
        ("mesh weights unreachable", fail("morph targets unsupported"), lambda p: append_unreachable_mesh(p, lambda m: m.__setitem__("weights",[1.0]))),
        ("node weights unreachable", fail("morph targets unsupported"), lambda p: base.mutate_output(p, lambda g: g["nodes"].append({"name":"UnreachableWeights","weights":[1.0]}))),
        ("vertex color attribute reachable", fail("unexpected rendering attributes"), lambda p: base.mutate_output(p, lambda g: g["meshes"][0]["primitives"][0]["attributes"].__setitem__("COLOR_0",0))),
        ("vertex color attribute unreachable mesh", fail("unexpected rendering attributes"), lambda p: append_unreachable_mesh(p, lambda m: m["primitives"][0]["attributes"].__setitem__("COLOR_0",0))),
        ("unreachable normal accessor index", fail("NORMAL accessor invalid"), lambda p: append_unreachable_mesh(p, lambda m: m["primitives"][0]["attributes"].__setitem__("NORMAL",999))),
        ("unreachable normal accessor format", fail("NORMAL accessor format invalid"), lambda p: append_unreachable_mesh(p, lambda m: m["primitives"][0]["attributes"].__setitem__("NORMAL",4))),
        ("unreachable normal accessor zero count", fail("attribute counts invalid"), unreachable_normal_zero_count),
        ("unreachable normal accessor buffer bounds", fail("bufferView escapes buffer"), unreachable_normal_escapes_buffer),
        ("unreachable normal accessor oversized view", fail("bufferView escapes buffer"), unreachable_normal_oversized_view),
        ("unreachable index accessor format", fail("index accessor format invalid"), lambda p: append_unreachable_mesh(p, lambda m: m["primitives"][0].__setitem__("indices",0))),
        ("unreachable index accessor zero count", fail("indices invalid"), unreachable_index_zero_count),
        ("attribute payload within tolerance", lambda p: base.run_verify(p), lambda p: base.mutate_attribute_payload(p,1,0,0.00006)),
    ]


def main() -> None:
    passed=0
    for name,check,mutate in cases():
        with tempfile.TemporaryDirectory() as temp_dir:
            paths=base.prepare(Path(temp_dir));mutate(paths)
            try:check(paths)
            except Exception as exc:raise AssertionError(f"case {name!r} failed: {exc}") from exc
            passed+=1
    print(f"PASS: {passed}/{len(cases())} Blender hidden-payload verifier tests")

if __name__=="__main__":main()
