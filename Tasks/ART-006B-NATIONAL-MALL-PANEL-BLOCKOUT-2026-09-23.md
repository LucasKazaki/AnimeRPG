# ART-006B: National Mall core-panel source blockout

**Loop:** `astral-art-hourly-20260922`  
**Parent:** ART-006A / PR #45 exact head `00f30f6973f852bf1f53d64b25075a916bbb7f42`  
**Status:** bounded original source-asset packet; no runtime or finished-environment claim  
**Live `main` observed before branch creation:** `6d22da88402db71843ecd5a35766c0c77e62dca6`

## Goal

Advance ART-006 from reference planning into actual inspectable 3D source production without inventing landmark detail or waiting for Astral's engine-owned importer. Produce one reusable Mall-core lawn-panel module whose dimensions remain tied to the authoritative ART-006A ledger.

This is the first environment blockout asset in the reference-to-3D path:

`authoritative ledger -> explicit authoring conversion -> deterministic mesh source -> independent validation -> future DCC round trip -> future Astral import/render -> art/performance review`.

Only the first four stages are in scope here.

## Allowed paths

- `Content/Reference/NationalMall/Blockout/README.md`
- `Content/Reference/NationalMall/Blockout/panel-module-source.json`
- `Content/Reference/NationalMall/Blockout/mall_core_panel_blockout.gltf`
- `Content/Reference/NationalMall/Blockout/expected-manifest.json`
- `Scripts/generate_national_mall_panel_blockout.py`
- `Scripts/verify_national_mall_panel_blockout.py`
- `Scripts/test_national_mall_panel_blockout.py`
- `Tasks/ART-006B-NATIONAL-MALL-PANEL-BLOCKOUT-2026-09-23.md`
- `Docs/QA/ART-006B-NATIONAL-MALL-PANEL-BLOCKOUT-2026-09-23.md`

Do not edit `Engine/`, renderer/editor/importer/gameplay code, CMake, workflows, dependencies, another worker's branch, or `Docs/Agents/art-hourly/*`. PR #33 still owns the shared art-hourly continuation files.

## Source contract

The module uses the reviewed `mall-core-axis` measurements:

- typical lawn panel: 450 ft x 170 ft -> 137.16 m x 51.816 m;
- perimeter gravel path width: 35 ft -> 10.668 m;
- tree-grove width: 130 ft -> 39.624 m;
- context: one typical module from the source's eight-panel Mall rhythm.

The glTF itself uses a canonical unit cube with real triangles, normals, tangents, UVs and uint16 indices. Three semantic mesh/material bindings reuse that geometry across seven node instances. Materials are neutral non-textured blockout colors only, with no baked lighting.

The grove bands matching the outer path length are an explicit authoring simplification, not a sourced geographic measurement. The packet does not place trees or landmarks and does not claim survey orientation.

## Acceptance

1. Source contract fails closed on schema/type/status/provenance changes.
2. Every feet-to-metre conversion is exact to the declared authoring values.
3. glTF is self-contained and bounded, with no external URI, images, textures, samplers or unsupported metadata.
4. Canonical 24-vertex / 36-index cube payload, accessors, material bindings, scene ownership, node identity and transforms are exact.
5. Manifest pins source and glTF hashes, byte count, unique meshes, instances, materials, topology counts and derived dimensions.
6. Generator `--check`, independent verifier, focused regression suite and `py_compile` pass on exact candidate content.
7. Hosted checks and fresh independent review must pass before the PR can leave draft.
8. Blender/DCC round trip, Astral import/rendering, collision/LOD, native performance and visual approval remain separate downstream gates.

## Stop condition

Stop this packet after the exact source asset is validated and independently reviewed. Do not bulk-produce additional National Mall modules or landmark art under ART-006B.
