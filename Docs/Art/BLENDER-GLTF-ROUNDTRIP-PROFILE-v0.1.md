# Blender glTF round-trip profile v0.1

Status: **adopted source-workflow profile, native execution still required**  
Loop: `astral-art-hourly-20260922`  
Task: `ART-006D`

This profile defines one bounded, reproducible DCC round-trip for Astral Engine source assets before any asset is described as DCC-validated. It does not add Blender as a repository dependency and does not claim Astral runtime support for glTF.

## Qualified tool and version

Primary DCC for this profile: **Blender 5.2.2 LTS**.

Current official Blender evidence rechecked on 2026-09-24:

- Blender 5.2 LTS initially released 2026-07-14 and is supported through July 2028.
- Blender 5.2.2 LTS was released 2026-09-15.
- The Blender 5.2 glTF add-on is enabled by default and supports meshes, materials, textures, cameras, punctual lights, extras, animation and skinning.
- The Blender 5.2 exporter exposes explicit controls for UVs, normals, tangents, materials, +Y-up glTF output, animation, GPU instancing and embedded glTF output.
- Blender 5.2 command-line/background execution is an official supported automation workflow.

Official sources:

- https://www.blender.org/releases/5-2/
- https://docs.blender.org/manual/en/5.2/addons/scene_gltf2.html
- https://docs.blender.org/manual/en/5.2/advanced/command_line/index.html
- https://docs.blender.org/api/5.2/bpy.ops.import_scene.html
- https://docs.blender.org/api/5.2/bpy.ops.export_scene.html
- https://docs.blender.org/api/5.2/bpy.ops.wm.html

## ART-006D import profile

The native driver imports the ART-006B self-contained `mall_core_panel_blockout.gltf` into a factory-clean Blender scene with these relevant settings:

- no vertex merge;
- imported normals preserved;
- `BLENDER` bone heuristic, although ART-006B contains no skeleton;
- scene extras imported;
- no collection wrapper requested by the importer;
- imported material slots kept separate;
- created objects selected for deterministic inspection.

The driver requires the seven expected National Mall mesh-object names and three expected blockout materials before it writes any accepted receipt.

## ART-006D export profile

The round-trip glTF must use the following fixed settings:

| Setting | Value | Reason |
|---|---:|---|
| format | `GLTF_EMBEDDED` | one inspectable, self-contained text artifact |
| UVs | on | preserve `TEXCOORD_0` |
| normals | on | preserve surface shading basis |
| tangents | on | preserve tangent-space contract |
| materials | `EXPORT` | preserve semantic material bindings |
| images | `NONE` | ART-006B has no textures; do not invent image payloads |
| cameras | off | source has none |
| lights | off | source has none |
| extras | on | preserve source metadata if Blender retains it |
| +Y up | on | emit standard glTF orientation |
| apply modifiers | off | no silent geometry bake |
| animation | off | source has none |
| GPU instancing | off | avoid introducing an extension during this calibration round-trip |

The editable `.blend` is saved from the imported scene before glTF export. Both the `.blend` and re-exported `.gltf` remain native evidence artifacts until reviewed. They are not automatically committed.

## Acceptance contract

A successful native DCC pass requires all of the following:

1. Blender reports exactly version `5.2.2` and background mode.
2. The source glTF SHA-256 still matches ART-006B's checked-in expected manifest.
3. Blender imports exactly the seven expected named mesh objects and three expected materials.
4. The driver writes a fresh `.blend`, fresh embedded `.gltf`, and JSON receipt without overwriting existing outputs.
5. The standard-library verifier independently hashes all three resources.
6. The re-exported glTF keeps all seven named scene instances, required `POSITION`, `NORMAL`, `TANGENT`, `TEXCOORD_0` attributes, indexed triangle primitives and semantic material bindings.
7. For every named instance, world-space center and world-space dimensions match the ART-006B source within `1e-4` metre.
8. The round-trip output contains no animation, image or texture payload that was absent from the source.
9. Native shell evidence separately records Blender command, executable/version, working directory and process exit code.

The verifier compares functional scene semantics rather than requiring byte-identical glTF output. Blender is allowed to repack buffers/accessors or hierarchy representation as long as the externally meaningful geometry, transforms, attributes and material assignments survive.

## Evidence stages

- `source_validated_not_imported`: ART-006B current state.
- `dcc_roundtrip_executed_not_astral_imported`: Blender driver completed and wrote its receipt.
- `dcc_roundtrip_verified_not_astral_imported`: independent ART-006D verifier accepted the receipt and round-trip files.
- `imported`: reserved for an actual Astral importer receipt.
- `runtime_verified`: reserved for actual Astral rendering/runtime checks.
- `art_approved`: reserved for visual/creative approval after runtime evidence.

ART-006D can only establish the second and third stages. It cannot clear engine-owned import, rendering, native performance or visual-art approval gates.
