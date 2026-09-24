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

A third exact-head Codex review of `fb2867ae5f3151e27c7cfaa2b109379c705e9fc1` found two additional P2 issues. This repair addresses both:

1. Each semantic mesh instance now records the handedness of its full inherited world transform. A negative-scale reflection that preserves center, dimensions and local topology is rejected as `transform parity drift` instead of being accepted with reversed world-space winding.
2. The bounded profile now rejects `EXT_mesh_gpu_instancing` both when declared at the glTF root and when attached to a scene node. One mesh-bearing node can therefore no longer hide multiple rendered instances behind a single semantic node count.

The verifier also rejects newly introduced camera payloads in the bounded output scope and validates that extension declaration lists and node extension containers have the expected JSON types before applying the instancing rule.

## Author verification executed in sandbox

Executed after the third review repair:

```text
python Scripts/test_blender_roundtrip_national_mall_panel.py
# PASS: 37/37 Blender round-trip verifier tests

python -m py_compile Scripts/verify_blender_roundtrip_national_mall_panel.py Scripts/test_blender_roundtrip_national_mall_panel.py
# exit 0
```

Regression coverage now includes the earlier source/hash/manifest/version/resource/material/transform cases plus fixed-tolerance enforcement, type-invalid receipt arrays/settings, unexpected and duplicate mesh instances, winding reversal, zero/mismatched attribute counts, normal/UV payload drift, reflected world transforms, node-level GPU instancing and root-level GPU-instancing declarations.

The prior exact-head Windows workflow `35986951177` (run #1035) passed on `fb2867ae...`, but this repair changes the candidate head. That run is historical evidence only. A new exact-head hosted run and a fresh independent review are required before this PR leaves draft.

## Verification model

The verifier does not require byte-identical glTF output. It decodes the embedded buffers/accessors, traverses the active scene hierarchy, derives world-space instance bounds from referenced vertices, checks world-transform handedness, rejects GPU-instanced duplicates, compares material semantics, and compares canonical winding-preserving triangle/attribute signatures for the seven semantic mesh instances. The fixed spatial/material/attribute tolerance is `1e-4`.

This bounded gate is intentionally stricter than a filename/count check but is still only a DCC source-workflow gate. It cannot establish Astral compatibility.

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
