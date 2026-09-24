# ART-009: Starter Solid Primitives v1

**Loop:** `astral-art-hourly-20260922`  
**Owner:** art worker  
**Base:** ART-006D exact head `5019f44e3c9b0cf8f97c6567685f52b6e14799ac`  
**Status:** bounded source-asset candidate, not runtime acceptance

## Goal

Close one concrete starter-library gap with original solid, UV-mapped, tangent-bearing geometry that can later exercise Astral's engine-owned mesh/material import path. Do not change the renderer, importer, editor, gameplay, build, workflows, dependencies, or local scheduler.

## Allowed paths

- `Content/Starter/SolidPrimitivesV1/**`
- `Scripts/generate_starter_solid_primitives.py`
- `Scripts/verify_starter_solid_primitives.py`
- `Scripts/test_starter_solid_primitives.py`
- `Tasks/ART-009-STARTER-SOLID-PRIMITIVES-v1.md`
- `Docs/QA/ART-009-STARTER-SOLID-PRIMITIVES-v1.md`

## Reference scope

Observed from official documentation on 2026-09-24:

- Unity documents Cube, Sphere, Capsule, Cylinder, Plane and Quad primitive/placeholder objects:
  https://docs.unity3d.com/Manual/PrimitiveObjects.html
- Unreal Engine 5.8 Modeling Mode documents Box, Sphere, Cylinder, Cone, Torus, Arrow, Rectangle, Disc and Stairs predefined shapes:
  https://dev.epicgames.com/documentation/unreal-engine/predefined-shapes-in-unreal-engine

This packet adopts only cube, sphere, cylinder and plane as a bounded first slice. It does not copy engine content and does not claim count, quality, workflow or editor parity.

## Source contract

- generated glTF 2.0 JSON with one embedded binary buffer; generated output is pinned but not checked in
- right-handed, metres, `+Y` up, `+Z` forward, `-X` right
- exact scene inventory: four named mesh nodes
- one neutral opaque material
- every primitive is indexed `TRIANGLES`
- exact attributes: `POSITION`, `NORMAL`, `TANGENT`, `TEXCOORD_0`
- attribute and index bindings must be genuine JSON integers; booleans do not satisfy index semantics
- exact vertex/index budgets pinned by source and manifest
- finite positions, unit normals/tangents, tangent-normal orthogonality, normalized UV range
- each shape's declared UV mapping policy is checked against its exact seams/corners/parameterization, not only `[0,1]` bounds
- triangle position/UV derivatives must agree with the supplied tangent direction and reconstructed bitangent handedness (`TANGENT.w`)
- nondegenerate outward winding checked against vertex normals
- shape-specific bounds and surface equations checked independently
- source, manifest and glTF runtime state remain `source_validated_not_imported`

## Exact commands

```text
python Scripts/generate_starter_solid_primitives.py --source Content/Starter/SolidPrimitivesV1/source-contract.json --gltf <fresh-dir>/starter_solid_primitives_v1.gltf --manifest <fresh-dir>/manifest.json
python Scripts/generate_starter_solid_primitives.py --source Content/Starter/SolidPrimitivesV1/source-contract.json --gltf <fresh-dir>/starter_solid_primitives_v1.gltf --manifest <fresh-dir>/manifest.json --check
python Scripts/verify_starter_solid_primitives.py --source Content/Starter/SolidPrimitivesV1/source-contract.json --gltf <fresh-dir>/starter_solid_primitives_v1.gltf --manifest <fresh-dir>/manifest.json --expected-manifest Content/Starter/SolidPrimitivesV1/expected-manifest.json
python Scripts/test_starter_solid_primitives.py
python -m py_compile Scripts/generate_starter_solid_primitives.py Scripts/verify_starter_solid_primitives.py Scripts/test_starter_solid_primitives.py
```

## Stop condition

Stop this packet after the exact branch head has:

1. deterministic generator `--check` pass,
2. independent verifier pass,
3. focused regression suite pass,
4. Python compile pass,
5. hosted repository checks applicable to the branch,
6. independent review with blocking findings repaired or explicitly carried.

Do not merge under this art worker's authority.

## Not claimed

No Blender execution, Khronos validator run, Astral import/render, editor primitive browser registration, collision/physics, LODs, GPU frame-time/memory, native workstation execution, or independent visual-art approval. This packet is a source asset and handoff fixture only.
