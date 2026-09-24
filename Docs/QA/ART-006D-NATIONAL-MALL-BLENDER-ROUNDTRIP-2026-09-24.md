# ART-006D QA: National Mall Blender round-trip contract

Date: 2026-09-24  
Loop: `astral-art-hourly-20260922`  
Status: **source tooling repaired and sandbox-verified; native Blender execution not run**

## Scope reviewed

This packet covers only the ART-006D Blender background driver, independent standard-library verifier, focused regressions and the art-facing Blender/glTF profile. It does not claim that Blender, Khronos glTF-Validator or Astral Engine executed against the National Mall source asset.

## Tool qualification evidence

Blender 5.2.2 LTS remains the pinned DCC for this representative round trip. Official Blender 5.2 release, manual, API and command-line sources are recorded in `Docs/Art/BLENDER-GLTF-ROUNDTRIP-PROFILE-v0.1.md`. No dependency was installed or downloaded by this task.

## Independent review repairs

The initial Codex review of `cb0de51ff52119ab6a9e790faedcc6ebfadcda52` found three issues. They were repaired before `64ffc277811b8cb8f95578b862384f0593f5841f`: the verifier owns the ART-006B source hash independently, computes bounds from indexed vertices, and compares visible material semantics.

A second exact-head Codex review of `64ffc277811b8cb8f95578b862384f0593f5841f` found one P1 and four P2 issues. Those were repaired on `fb2867ae5f3151e27c7cfaa2b109379c705e9fc1` by fixing the acceptance tolerance, winding-preserving triangle signatures, attribute payload validation, receipt type checks, and exact active-scene mesh inventory.

A third exact-head Codex review of `fb2867ae5f3151e27c7cfaa2b109379c705e9fc1` found two P2 issues. Head `2ad3bab391088419349cc27125f10df555efee0a` repaired reflected-transform parity and root/reachable-node GPU-instancing acceptance.

The exact-head review of `2ad3bab391088419349cc27125f10df555efee0a` then found three additional P2 issues. Verifier repair commit `732537429dd555972098dd66d8fc926e81c58a77` addresses all three:

1. `EXT_mesh_gpu_instancing` is rejected in declarations, root payloads and every node extension container, including unreachable nodes.
2. `KHR_lights_punctual` is rejected in declarations, root payloads and every node extension container because the fixed exporter profile has `export_lights=false`; node `camera` payloads are likewise rejected for `export_cameras=false`.
3. Optional `animations`, `images`, `textures`, and `cameras` collections may be absent or actual empty JSON arrays only. Falsy non-array substitutions fail closed.

All three review threads were answered and resolved after the verifier repair.

## Author verification executed in sandbox

The repaired verifier was exercised with a supplemental local harness containing the prior semantic/evidence cases plus seven focused hidden-payload/type cases:

```text
python test_blender_roundtrip_national_mall_panel.py
# PASS: 44/44 Blender round-trip verifier tests

python -m py_compile verify_blender_roundtrip_national_mall_panel.py test_blender_roundtrip_national_mall_panel.py
# exit 0
```

The seven supplemental cases cover unreachable-node GPU instancing, punctual-light payloads, node cameras, and wrong-type `animations` / `images` / `textures` / `cameras` collections.

The repository's persisted `Scripts/test_blender_roundtrip_national_mall_panel.py` is still the prior 37-case suite at this candidate. A direct connector write of the expanded test file was blocked before mutation, so the 44-case supplemental harness is sandbox evidence only and is not represented as committed regression coverage. No existing repository tests were removed or weakened.

The Windows workflow for pre-repair head `2ad3bab...`, run `35988421764` / #1040, completed successfully. Any hosted run on an older head is historical after this repair. A fresh exact-head hosted run and a fresh independent review are required before this PR leaves draft.

## Verification model

The verifier does not require byte-identical glTF output. It decodes embedded buffers/accessors, validates the fixed source and receipt identities, scans the complete node collection for disabled exporter payloads, traverses the active scene hierarchy, derives world-space instance bounds from referenced vertices, checks world-transform handedness, compares material semantics, and compares canonical winding-preserving triangle/attribute signatures for the seven semantic mesh instances. The fixed spatial/material/attribute tolerance is `1e-4`.

The bounded output additionally rejects GPU instancing, punctual lights and node cameras, and requires optional animation/image/texture/camera collections to be absent or exact empty arrays. This gate is intentionally stricter than a filename/count check but is still only a DCC source-workflow gate. It cannot establish Astral compatibility.

## Not run / not claimed

- Blender 5.2.2 executable availability or `bpy` execution on Lucas's workstation;
- Blender import, `.blend` save or glTF export;
- official Khronos glTF-Validator execution on the round-trip output;
- Astral import or rendering;
- collision, LOD, navigation or streaming behavior;
- frame-time, RAM or VRAM measurement;
- independent visual-art approval;
- UE5, Unity, Genshin Impact or Zenless Zone Zero parity.

## Next acceptance

First require a fresh exact-head hosted check and independent source review of this repaired packet. Then use the registered local execution path to prove Blender 5.2.2 availability, run the exact background command into a fresh evidence directory, preserve shell-level command/version/exit/hash evidence, and run this verifier plus the ART-006C Khronos evidence gate. Only real native outputs may advance the asset to `dcc_roundtrip_verified_not_astral_imported`.
