# ART-009 Starter Solid Primitives v1 QA

Date: 2026-09-24  
Scope: source-only sandbox verification for a new generic starter-content asset packet.

## Delivered asset

A freshly generated `starter_solid_primitives_v1.gltf` contains four original indexed solid meshes with a single neutral material. The generated glTF is intentionally not checked in; the checked-in expected manifest pins its exact bytes:

| Primitive | Vertices | Indices | Nominal size |
|---|---:|---:|---|
| Cube | 24 | 36 | 1 x 1 x 1 m |
| UV sphere | 151 | 672 | 1 m diameter |
| Cylinder | 70 | 192 | 1 m diameter x 2 m high |
| Plane | 4 | 6 | 1 x 1 m |

Total: 249 vertices, 906 indices. These are functional source meshes, not render screenshots or engine-installed assets.

## Exact source hashes from the sandbox-authored packet

- source contract SHA-256: `12473281a43b46c8a41e04d9515c4d2b692387ba90ca175dcdd6e690c727b4cf`
- glTF SHA-256: `e79789761f295c2ae73094cc6643d1747e5fb6852eaca627a972d7e475c36b17`
- expected manifest SHA-256: `653d120860b9fcf248d2c50ab3b880d98d26a69b4c681ec212a3b39d8f90f71e`
- generator SHA-256: `e5e76d3fafec92d058f7f611cc401903b19ce010c0d1ca9dea7c25ae91e182c6`
- verifier SHA-256: `5dd671ab3c7f6b682dd4b9b5e3edcabe31594b76644ee013911f09d11b5d99ce`
- tests SHA-256: `5737fdf8aafc6ae071152896d836aab1a644142700317b89132aab4687ef2790`

## Sandbox execution

```text
python Scripts/generate_starter_solid_primitives.py --source Content/Starter/SolidPrimitivesV1/source-contract.json --gltf <fresh-dir>/starter_solid_primitives_v1.gltf --manifest <fresh-dir>/manifest.json
PASS: generated starter solid primitives v1

python Scripts/generate_starter_solid_primitives.py --source Content/Starter/SolidPrimitivesV1/source-contract.json --gltf <fresh-dir>/starter_solid_primitives_v1.gltf --manifest <fresh-dir>/manifest.json --check
PASS: deterministic starter solid primitives match pinned files

python Scripts/verify_starter_solid_primitives.py --source Content/Starter/SolidPrimitivesV1/source-contract.json --gltf <fresh-dir>/starter_solid_primitives_v1.gltf --manifest <fresh-dir>/manifest.json --expected-manifest Content/Starter/SolidPrimitivesV1/expected-manifest.json
PASS: source-only starter solid primitives {'meshes': 4, 'materials': 1, 'vertices': 249, 'indices': 906}

python Scripts/test_starter_solid_primitives.py
PASS: 27/27 starter solid primitive tests

python -m py_compile Scripts/generate_starter_solid_primitives.py Scripts/verify_starter_solid_primitives.py Scripts/test_starter_solid_primitives.py
exit 0
```

The fresh generated manifest was also byte-for-byte equal to the checked-in expected manifest.

The 27 focused tests include deterministic/stale-output behavior, overwrite refusal, source-schema boolean confusion, false runtime status, manifest hash and JSON-integer checks, checked-in manifest pin enforcement, external-buffer rejection, unexpected rendering attributes, material drift including bool-as-number substitution, scene bool-as-index substitution, UV/normal/tangent corruption, index bounds, reversed winding, sphere/cylinder/plane geometry drift, bufferView target/range changes, stale accessor bounds, unknown glTF root fields, UV-policy drift and official-reference drift.

## Evidence boundary

This pass did not execute Blender, glTF-Validator, Astral Engine, a native Windows GPU runtime, collision, physics, LOD generation, editor registration or independent visual review. The asset remains `source_validated_not_imported`.

A green hosted build or source regression suite cannot promote it to `imported`, `runtime_verified`, or `art_approved`.
