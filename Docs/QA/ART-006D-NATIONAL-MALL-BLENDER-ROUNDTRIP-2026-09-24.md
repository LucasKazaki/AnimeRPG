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

A second exact-head Codex review of `64ffc277811b8cb8f95578b862384f0593f5841f` found one P1 and four P2 issues. The current repair addresses all five:

1. The acceptance tolerance is a verifier-owned constant, exactly `1e-4` metre. The Python API and CLI no longer accept a caller-supplied tolerance override.
2. The semantic gate compares canonicalized indexed triangles while preserving winding. Triangle order and cyclic first-corner rotation may change, but reversing winding or changing connectivity does not pass.
3. `NORMAL`, `TANGENT` and `TEXCOORD_0` are decoded, must have the same nonzero element count as `POSITION`, and their referenced payloads participate in each triangle signature.
4. Receipt inventories must be JSON arrays of strings with exact expected membership/count, and each export setting must have the exact expected JSON/Python type and value. Numeric `1`/`0` cannot substitute for booleans.
5. Every scene-reachable mesh-bearing node must be one of the seven expected semantic instances, and duplicate expected mesh-instance names are rejected.

The topology signature includes material binding plus per-corner quantized `POSITION`, `NORMAL`, `TANGENT` and `TEXCOORD_0` data at the same fixed `1e-4` comparison resolution used by the bounded geometry/material gate. This lets Blender repack accessors and reorder triangles without treating winding, shading-basis or UV drift as equivalent.

## Author verification executed in sandbox

Executed after the second review repair:

```text
python Scripts/test_blender_roundtrip_national_mall_panel.py
# PASS: 34/34 Blender round-trip verifier tests

python -m py_compile Scripts/verify_blender_roundtrip_national_mall_panel.py Scripts/test_blender_roundtrip_national_mall_panel.py
# exit 0
```

Regression coverage now includes the earlier source/hash/manifest/version/resource/material/transform cases plus fixed-tolerance enforcement, type-invalid receipt arrays/settings, unexpected and duplicate mesh instances, winding reversal, zero/mismatched attribute counts, and normal/UV payload drift.

Hosted Windows workflow `35979325879` (run #1025) passed on `64ffc277...`, but the second-review source repairs change the candidate head. That run is therefore historical evidence only. A new exact-head hosted run and a fresh independent review are required before this PR leaves draft.

## Verification model

The verifier does not require byte-identical glTF output. It decodes the embedded buffers/accessors, traverses the active scene hierarchy, derives world-space instance bounds from referenced vertices, compares material semantics, and compares canonical winding-preserving triangle/attribute signatures for the seven semantic mesh instances. The fixed spatial/material/attribute tolerance is `1e-4`.

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
