#!/usr/bin/env python3
from __future__ import annotations

import base64
import copy
import hashlib
import json
import struct
import tempfile
from pathlib import Path

import verify_blender_roundtrip_national_mall_panel as verifier

NODES = [
    ("Lawn", 0, [0, -0.05, 0], [51.816, 0.1, 137.16]),
    ("Path_Longitudinal_PosX", 1, [31.242, -0.04, 0], [10.668, 0.08, 137.16]),
    ("Path_Longitudinal_NegX", 1, [-31.242, -0.04, 0], [10.668, 0.08, 137.16]),
    ("Path_End_PosZ", 1, [0, -0.04, 73.914], [73.152, 0.08, 10.668]),
    ("Path_End_NegZ", 1, [0, -0.04, -73.914], [73.152, 0.08, 10.668]),
    ("Grove_Envelope_PosX", 2, [56.388, -0.03, 0], [39.624, 0.06, 158.496]),
    ("Grove_Envelope_NegX", 2, [-56.388, -0.03, 0], [39.624, 0.06, 158.496]),
]
MATERIALS = ["Lawn_Blockout", "Gravel_Path_Blockout", "Tree_Grove_Envelope"]
REAL_ART006B_SHA = "6c51463332199c65bcfbde04ee8e5883e03a94aba710980eebfaa6945f2759b7"


def pack_fixture_buffer() -> tuple[bytes, list[dict[str, object]], list[dict[str, object]]]:
    positions = [
        (-0.5,-0.5,-0.5),(0.5,-0.5,-0.5),(0.5,0.5,-0.5),(-0.5,0.5,-0.5),
        (-0.5,-0.5,0.5),(0.5,-0.5,0.5),(0.5,0.5,0.5),(-0.5,0.5,0.5),
    ]
    normals = [(0.0,1.0,0.0)] * 8
    tangents = [(1.0,0.0,0.0,1.0)] * 8
    uvs = [(0.0,0.0),(1.0,0.0),(1.0,1.0),(0.0,1.0),(0.0,0.0),(1.0,0.0),(1.0,1.0),(0.0,1.0)]
    indices = [0,1,2,0,2,3,4,6,5,4,7,6,0,4,5,0,5,1,1,5,6,1,6,2,2,6,7,2,7,3,3,7,4,3,4,0]
    sections: list[bytes] = []
    for values, fmt in [(positions,"<3f"),(normals,"<3f"),(tangents,"<4f"),(uvs,"<2f")]:
        sections.append(b"".join(struct.pack(fmt, *v) for v in values))
    sections.append(struct.pack("<" + "H" * len(indices), *indices))
    views = []
    offset = 0
    for section in sections:
        views.append({"buffer":0,"byteOffset":offset,"byteLength":len(section)})
        offset += len(section)
    accessors = [
        {"bufferView":0,"componentType":5126,"count":8,"type":"VEC3","min":[-0.5,-0.5,-0.5],"max":[0.5,0.5,0.5]},
        {"bufferView":1,"componentType":5126,"count":8,"type":"VEC3"},
        {"bufferView":2,"componentType":5126,"count":8,"type":"VEC4"},
        {"bufferView":3,"componentType":5126,"count":8,"type":"VEC2"},
        {"bufferView":4,"componentType":5123,"count":36,"type":"SCALAR"},
    ]
    return b"".join(sections), views, accessors


def make_gltf() -> dict[str, object]:
    payload, views, accessors = pack_fixture_buffer()
    uri = "data:application/octet-stream;base64," + base64.b64encode(payload).decode("ascii")
    meshes = []
    for i, mat in enumerate(MATERIALS):
        meshes.append({"name":f"FixtureMesh{i}","primitives":[{"attributes":{"POSITION":0,"NORMAL":1,"TANGENT":2,"TEXCOORD_0":3},"indices":4,"mode":4,"material":i}]})
    nodes = [{"name":name,"mesh":mesh,"translation":translation,"scale":scale} for name,mesh,translation,scale in NODES]
    colors = [(0.18,0.36,0.12,1.0),(0.5,0.45,0.35,1.0),(0.08,0.2,0.08,1.0)]
    roughness = [0.95,1.0,1.0]
    return {
        "asset":{"version":"2.0","generator":"ART-006D test fixture"},
        "scene":0,
        "scenes":[{"nodes":list(range(7))}],
        "nodes":nodes,
        "buffers":[{"byteLength":len(payload),"uri":uri}],
        "bufferViews":views,
        "accessors":accessors,
        "materials":[
            {"name":name,"pbrMetallicRoughness":{"baseColorFactor":list(colors[i]),"metallicFactor":0.0,"roughnessFactor":roughness[i]},"doubleSided":False}
            for i,name in enumerate(MATERIALS)
        ],
        "meshes":meshes,
    }


def write_json(path: Path, value: object) -> None:
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def prepare(root: Path) -> dict[str, Path]:
    paths = {name: root / filename for name, filename in {
        "source":"mall_core_panel_blockout.gltf", "manifest":"expected-manifest.json", "roundtrip":"roundtrip.gltf", "blend":"roundtrip.blend", "receipt":"receipt.json"
    }.items()}
    gltf = make_gltf()
    write_json(paths["source"], gltf)
    write_json(paths["roundtrip"], copy.deepcopy(gltf))
    paths["blend"].write_bytes(b"BLENDER-v5.2.2-fixture\x00")
    verifier.EXPECTED_SOURCE_SHA = sha(paths["source"])
    manifest = {
        "schema_version":1,
        "asset_id":"national-mall-core-panel-module-v1",
        "status":"source_validated_not_imported",
        "ledger_ref":"Content/Reference/NationalMall/reference-ledger.json",
        "ledger_blob_sha1":"ba8ec205d2aecfa4b2ace15c13c71fb9932cb7f4",
        "source_sha256":"5ec46c554172b0f83859ed6185df07bbd2d35cb89f13fe66d9ac10246d9005b1",
        "gltf_file":"mall_core_panel_blockout.gltf",
        "gltf_sha256":sha(paths["source"]),
        "gltf_bytes":paths["source"].stat().st_size,
        "unique_meshes":3,
        "mesh_instances":7,
        "materials":3,
        "unit_box_vertices":24,
        "unit_box_indices":36,
        "dimensions_m":{"lawn_longitudinal":137.16},
    }
    write_json(paths["manifest"], manifest)
    receipt = {
        "schema_version":1,
        "task_id":"ART-006D",
        "loop_id":"astral-art-hourly-20260922",
        "status":"dcc_roundtrip_executed_not_astral_imported",
        "run":{"kind":"native_blender_background","background_mode":True,"script_completed":True,"working_directory":root.as_posix()},
        "blender":{"version":"5.2.2","version_tuple":[5,2,2],"executable":"C:/Program Files/Blender Foundation/Blender 5.2/blender.exe"},
        "input":{"path":paths["source"].name,"sha256":sha(paths["source"]),"bytes":paths["source"].stat().st_size},
        "output":{
            "gltf":{"path":"roundtrip.gltf","sha256":sha(paths["roundtrip"]),"bytes":paths["roundtrip"].stat().st_size},
            "blend":{"path":"roundtrip.blend","sha256":sha(paths["blend"]),"bytes":paths["blend"].stat().st_size},
        },
        "imported_scene":{"mesh_object_count":7,"object_names":sorted(n[0] for n in NODES),"material_names":sorted(MATERIALS)},
        "export_settings":{
            "export_format":"GLTF_EMBEDDED","export_texcoords":True,"export_normals":True,"export_tangents":True,
            "export_materials":"EXPORT","export_image_format":"NONE","export_cameras":False,"export_lights":False,
            "export_extras":True,"export_yup":True,"export_apply":False,"export_animations":False,"export_gpu_instances":False,
        },
    }
    write_json(paths["receipt"], receipt)
    return paths


def repin_input(paths: dict[str, Path], receipt: dict[str, object]) -> None:
    inp = receipt["input"]  # type: ignore[index]
    inp["sha256"] = sha(paths["source"])  # type: ignore[index]
    inp["bytes"] = paths["source"].stat().st_size  # type: ignore[index]


def repin_output(paths: dict[str, Path], receipt: dict[str, object]) -> None:
    out = receipt["output"]["gltf"]  # type: ignore[index]
    out["sha256"] = sha(paths["roundtrip"])  # type: ignore[index]
    out["bytes"] = paths["roundtrip"].stat().st_size  # type: ignore[index]


def expect_fail(fn, contains: str) -> None:
    try:
        fn()
    except verifier.VerificationError as exc:
        assert contains in str(exc), f"expected {contains!r}, got {exc!r}"
    else:
        raise AssertionError(f"expected VerificationError containing {contains!r}")


def run_verify(paths: dict[str, Path]):
    return verifier.verify(paths["source"], paths["manifest"], paths["roundtrip"], paths["blend"], paths["receipt"], 1e-4)


def test_valid(paths):
    result = run_verify(paths)
    assert result["nodes"] == 7 and result["blender"] == "5.2.2"


def mutate_receipt(paths, mutator):
    receipt = json.loads(paths["receipt"].read_text())
    mutator(receipt)
    write_json(paths["receipt"], receipt)


def mutate_output(paths, mutator):
    gltf = json.loads(paths["roundtrip"].read_text())
    mutator(gltf)
    write_json(paths["roundtrip"], gltf)
    receipt = json.loads(paths["receipt"].read_text())
    repin_output(paths, receipt)
    write_json(paths["receipt"], receipt)


def mutate_self_consistent_source(paths):
    src = json.loads(paths["source"].read_text())
    src["asset"]["generator"] = "substituted-source"
    write_json(paths["source"], src)
    write_json(paths["roundtrip"], copy.deepcopy(src))
    manifest = json.loads(paths["manifest"].read_text())
    manifest["gltf_sha256"] = sha(paths["source"])
    manifest["gltf_bytes"] = paths["source"].stat().st_size
    write_json(paths["manifest"], manifest)
    receipt = json.loads(paths["receipt"].read_text())
    repin_input(paths, receipt)
    repin_output(paths, receipt)
    write_json(paths["receipt"], receipt)


def mutate_index_subset(paths):
    gltf = json.loads(paths["roundtrip"].read_text())
    prefix = "data:application/octet-stream;base64,"
    payload = bytearray(base64.b64decode(gltf["buffers"][0]["uri"][len(prefix):]))
    acc = gltf["accessors"][4]
    view = gltf["bufferViews"][acc["bufferView"]]
    start = view.get("byteOffset", 0) + acc.get("byteOffset", 0)
    subset = [0,1,2] * 12
    payload[start:start + 72] = struct.pack("<" + "H" * 36, *subset)
    gltf["buffers"][0]["uri"] = prefix + base64.b64encode(payload).decode("ascii")
    write_json(paths["roundtrip"], gltf)
    receipt = json.loads(paths["receipt"].read_text())
    repin_output(paths, receipt)
    write_json(paths["receipt"], receipt)


def assert_pin():
    assert REAL_ART006B_SHA == "6c51463332199c65bcfbde04ee8e5883e03a94aba710980eebfaa6945f2759b7"


def cases():
    return [
        ("valid", test_valid, None),
        ("production pin constant", lambda p: assert_pin(), None),
        ("manifest source hash", lambda p: expect_fail(lambda: run_verify(p), "source no longer matches pinned ART-006B input"), lambda p: write_json(p["manifest"], {**json.loads(p["manifest"].read_text()), "gltf_sha256":"0"*64})),
        ("self-consistent source substitution", lambda p: expect_fail(lambda: run_verify(p), "source no longer matches pinned ART-006B input"), mutate_self_consistent_source),
        ("manifest identity", lambda p: expect_fail(lambda: run_verify(p), "ART-006B manifest identity drift: asset_id"), lambda p: write_json(p["manifest"], {**json.loads(p["manifest"].read_text()), "asset_id":"other-panel"})),
        ("blender version", lambda p: expect_fail(lambda: run_verify(p), "Blender evidence invalid"), lambda p: mutate_receipt(p, lambda r: r["blender"].__setitem__("version", "5.2.1"))),
        ("version tuple bool", lambda p: expect_fail(lambda: run_verify(p), "Blender evidence invalid"), lambda p: mutate_receipt(p, lambda r: r["blender"].__setitem__("version_tuple", [5,2,True]))),
        ("input hash", lambda p: expect_fail(lambda: run_verify(p), "receipt input hash mismatch"), lambda p: mutate_receipt(p, lambda r: r["input"].__setitem__("sha256", "0"*64))),
        ("output hash", lambda p: expect_fail(lambda: run_verify(p), "receipt output.gltf hash mismatch"), lambda p: mutate_receipt(p, lambda r: r["output"]["gltf"].__setitem__("sha256", "0"*64))),
        ("blend hash", lambda p: expect_fail(lambda: run_verify(p), "receipt output.blend hash mismatch"), lambda p: mutate_receipt(p, lambda r: r["output"]["blend"].__setitem__("sha256", "0"*64))),
        ("mesh count bool", lambda p: expect_fail(lambda: run_verify(p), "import inventory invalid"), lambda p: mutate_receipt(p, lambda r: r["imported_scene"].__setitem__("mesh_object_count", True))),
        ("object inventory", lambda p: expect_fail(lambda: run_verify(p), "import inventory invalid"), lambda p: mutate_receipt(p, lambda r: r["imported_scene"]["object_names"].pop())),
        ("settings drift", lambda p: expect_fail(lambda: run_verify(p), "export settings drift"), lambda p: mutate_receipt(p, lambda r: r["export_settings"].__setitem__("export_tangents", False))),
        ("unknown receipt field", lambda p: expect_fail(lambda: run_verify(p), "receipt root fields invalid"), lambda p: mutate_receipt(p, lambda r: r.__setitem__("unexpected", 1))),
        ("missing node", lambda p: expect_fail(lambda: run_verify(p), "expected seven named mesh instances"), lambda p: mutate_output(p, lambda g: (g["scenes"][0]["nodes"].pop(), g["nodes"].pop()))),
        ("dimension drift", lambda p: expect_fail(lambda: run_verify(p), "transform/material binding drift"), lambda p: mutate_output(p, lambda g: g["nodes"][0]["scale"].__setitem__(0, 60.0))),
        ("center drift", lambda p: expect_fail(lambda: run_verify(p), "transform/material binding drift"), lambda p: mutate_output(p, lambda g: g["nodes"][0]["translation"].__setitem__(0, 1.0))),
        ("indexed subset bounds", lambda p: expect_fail(lambda: run_verify(p), "transform/material binding drift"), mutate_index_subset),
        ("missing tangent", lambda p: expect_fail(lambda: run_verify(p), "required attributes missing"), lambda p: mutate_output(p, lambda g: g["meshes"][0]["primitives"][0]["attributes"].pop("TANGENT"))),
        ("material binding drift", lambda p: expect_fail(lambda: run_verify(p), "transform/material binding drift"), lambda p: mutate_output(p, lambda g: g["meshes"][0]["primitives"][0].__setitem__("material", 1))),
        ("base color drift", lambda p: expect_fail(lambda: run_verify(p), "Lawn_Blockout material property drift"), lambda p: mutate_output(p, lambda g: g["materials"][0]["pbrMetallicRoughness"].__setitem__("baseColorFactor", [0,0,0,1]))),
        ("roughness drift", lambda p: expect_fail(lambda: run_verify(p), "Lawn_Blockout material property drift"), lambda p: mutate_output(p, lambda g: g["materials"][0]["pbrMetallicRoughness"].__setitem__("roughnessFactor", 0.0))),
        ("sidedness drift", lambda p: expect_fail(lambda: run_verify(p), "Lawn_Blockout material property drift"), lambda p: mutate_output(p, lambda g: g["materials"][0].__setitem__("doubleSided", True))),
        ("external buffer", lambda p: expect_fail(lambda: run_verify(p), "must be embedded"), lambda p: mutate_output(p, lambda g: g["buffers"][0].__setitem__("uri", "mesh.bin"))),
        ("nontriangles", lambda p: expect_fail(lambda: run_verify(p), "requires indexed TRIANGLES"), lambda p: mutate_output(p, lambda g: g["meshes"][0]["primitives"][0].__setitem__("mode", 1))),
    ]


def main() -> None:
    assert verifier.EXPECTED_SOURCE_SHA == REAL_ART006B_SHA
    passed = 0
    for name, check, mutate in cases():
        with tempfile.TemporaryDirectory() as td:
            paths = prepare(Path(td))
            if mutate is not None:
                mutate(paths)
            check(paths)
            passed += 1
    print(f"PASS: {passed}/{len(cases())} Blender round-trip verifier tests")


if __name__ == "__main__":
    main()
