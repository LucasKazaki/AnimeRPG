# Blender glTF round-trip profile v0.1

Status: **adopted source-workflow profile, native execution still required**  
Loop: `astral-art-hourly-20260922`  
Task: `ART-006D`

This profile defines one bounded, reproducible DCC round trip for Astral Engine source assets before an asset is described as DCC-validated. It does not add Blender as a repository dependency and does not claim Astral runtime support for glTF.

## Qualified tool and version

Primary DCC: **Blender 5.2.2 LTS**.

Official Blender evidence rechecked for ART-006D on 2026-09-24:

- Blender 5.2 LTS released 2026-07-14 and is supported through July 2028.
- Blender 5.2.2 LTS released 2026-09-15.
- Blender 5.2 documents glTF import/export, explicit UV/normal/tangent/material controls, embedded glTF output and background command-line execution.

Official sources:

- https://www.blender.org/releases/5-2/
- https://docs.blender.org/manual/en/5.2/addons/scene_gltf2.html
- https://docs.blender.org/manual/en/5.2/advanced/command_line/index.html
- https://docs.blender.org/api/5.2/bpy.ops.import_scene.html
- https://docs.blender.org/api/5.2/bpy.ops.export_scene.html
- https://docs.blender.org/api/5.2/bpy.ops.wm.html

## ART-006D import profile

The native driver imports ART-006B's self-contained `mall_core_panel_blockout.gltf` into a factory-clean Blender scene. It requires the seven expected National Mall mesh-object names and three expected blockout materials before writing accepted execution evidence.

## ART-006D export profile

The round-trip glTF uses a fixed profile:

| Setting | Value | Reason |
|---|---:|---|
| format | `GLTF_EMBEDDED` | one self-contained text artifact |
| UVs | on | preserve `TEXCOORD_0` |
| normals | on | preserve surface shading basis |
| tangents | on | preserve tangent-space contract |
| materials | `EXPORT` | preserve semantic material bindings |
| images | `NONE` | ART-006B has no textures |
| cameras | off | source has none |
| lights | off | source has none |
| extras | on | preserve source metadata where supported |
| +Y up | on | emit standard glTF orientation |
| apply modifiers | off | no silent geometry bake |
| animation | off | source has none |
| GPU instancing | off | do not introduce an extension during calibration |

Receipt verification requires exact field types as well as values. JSON integer `1` is not accepted in place of boolean `true`, and imported object/material inventories must be arrays of strings rather than object-key lookalikes.

## Acceptance contract

A successful native DCC pass requires all of the following:

1. Blender reports exactly version `5.2.2` in background mode.
2. The source glTF SHA-256 matches ART-006B's verifier-owned source pin.
3. The imported inventory contains exactly seven expected named mesh objects and three expected materials.
4. The driver writes a fresh `.blend`, embedded `.gltf` and JSON receipt without overwriting prior evidence.
5. The verifier independently hashes input/output resources and enforces the fixed export-profile types and values.
6. The active exported scene contains exactly the seven expected mesh-bearing instances, with no extras or duplicates.
7. Each primitive is indexed triangles with float `POSITION`, `NORMAL`, `TANGENT`, `TEXCOORD_0` accessors, equal nonzero attribute counts and an unsigned scalar index accessor.
8. World center/dimensions match the source within the fixed, non-overridable `1e-4` metre tolerance, and each semantic instance preserves world-transform handedness so an AABB-preserving reflection cannot pass.
9. Canonical indexed-triangle signatures preserve winding and referenced position/normal/tangent/UV payloads. Triangle ordering and cyclic first-corner choice may vary, but connectivity, culling orientation and shading/UV semantics may not drift.
10. Semantic material bindings and visible PBR factors remain equivalent within the same fixed numeric tolerance.
11. The bounded output may not introduce `EXT_mesh_gpu_instancing` or `KHR_lights_punctual` through declarations, root payloads or any node, including unreachable nodes. Node camera payloads are also prohibited because cameras are disabled.
12. `animations`, `images`, `textures`, and `cameras` must be absent or actual empty JSON arrays. Empty objects, booleans, numbers and other falsy substitutes are invalid evidence.
13. Native shell evidence separately records the real Blender command, executable/version, working directory and process exit code.

The verifier compares functional scene semantics rather than requiring byte-identical glTF output. Blender may repack buffers/accessors without weakening the acceptance gate.

## Evidence stages

- `source_validated_not_imported`: ART-006B current state.
- `dcc_roundtrip_executed_not_astral_imported`: real Blender driver completed and wrote native outputs/receipt.
- `dcc_roundtrip_verified_not_astral_imported`: independent ART-006D verifier accepted those real files.
- `imported`: reserved for an actual Astral importer receipt.
- `runtime_verified`: reserved for actual Astral rendering/runtime checks.
- `art_approved`: reserved for visual/creative approval after runtime evidence.

ART-006D can only establish the second and third stages. It cannot clear engine-owned import, rendering, native performance or visual-art approval gates.
