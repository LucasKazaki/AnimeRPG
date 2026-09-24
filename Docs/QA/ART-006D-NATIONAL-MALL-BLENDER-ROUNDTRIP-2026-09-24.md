# ART-006D QA: National Mall Blender round-trip contract

Date: 2026-09-24  
Loop: `astral-art-hourly-20260922`  
Status: **source tooling repaired; native Blender execution not run**

## Scope reviewed

This packet covers only the ART-006D Blender background driver, independent standard-library verifier, focused regressions and the art-facing Blender/glTF profile. It does not claim that Blender, Khronos glTF-Validator or Astral Engine executed against the National Mall source asset.

## Tool qualification evidence

Blender 5.2.2 LTS remains the pinned DCC for this representative round trip. Official Blender 5.2 release, manual, API and command-line sources are recorded in `Docs/Art/BLENDER-GLTF-ROUNDTRIP-PROFILE-v0.1.md`. No dependency was installed or downloaded by this task.

## Independent review repairs

The initial Codex review of `cb0de51ff52119ab6a9e790faedcc6ebfadcda52` found three issues. They were repaired before `64ffc277811b8cb8f95578b862384f0593f5841f`: the verifier owns the ART-006B source hash independently, computes bounds from indexed vertices, and compares visible material semantics.

A second exact-head Codex review of `64ffc277811b8cb8f95578b862384f0593f5841f` found one P1 and four P2 issues. Those were repaired on `fb2867ae5f3151e27c7cfaa2b109379c705e9fc1` by fixing the acceptance tolerance, winding-preserving triangle signatures, attribute payload validation, receipt type checks, and exact active-scene mesh inventory.

A third exact-head Codex review of `fb2867ae5f3151e27c7cfaa2b109379c705e9fc1` found two P2 issues. Head `2ad3bab391088419349cc27125f10df555efee0a` repaired reflected-transform parity and root/reachable-node GPU-instancing acceptance.

The exact-head review of `2ad3bab391088419349cc27125f10df555efee0a` then found three additional P2 issues. The subsequent repair rejected GPU instancing and punctual-light payloads on every node, including unreachable nodes, rejected node cameras, and required optional animation/image/texture/camera collections to be absent or actual empty arrays.

The review that completed on `78c9acbc2dbbac312d0c9ca6cd4b0c82555b1b86` surfaced four remaining issues: one P1 regression in the committed GPU-instancing error-text expectation, plus P2 gaps for same-AABB world rotation, morph-target deformation, and extra `COLOR_0` rendering attributes. Those were repaired by restoring a distinct GPU-instancing error path, comparing complete inherited 3x4 world transforms, rejecting primitive/mesh/node morph payloads across the bounded glTF, and requiring the exact four source rendering attributes.

A fresh review of `918ec775c73c1e6b6f91fe7fcdf2ba973be6fa80` found one P1 and three P2 issues. The current repair addresses all four:

1. the verifier checks center/dimensions/material binding before the stricter world-transform comparison, preserving the committed `transform/material binding drift` contract for the existing dimension and center regressions while still catching same-AABB orientation drift afterward;
2. every mesh primitive in the bounded glTF, including unreachable meshes, must use indexed `TRIANGLES` and exactly `POSITION`, `NORMAL`, `TANGENT`, and `TEXCOORD_0`; missing attributes preserve the existing `required attributes missing` contract, and extras fail as `unexpected rendering attributes`;
3. the hidden suite now exercises primitive `targets`, mesh `weights`, and node `weights` independently, including unreachable-content variants, rather than allowing one earlier rejection to mask another branch;
4. the inherited-world-transform regression now wraps `Lawn` beneath a rotated parent instead of rotating the semantic mesh locally, so ancestor transform composition is directly covered.

## Author verification executed in sandbox

The repaired repository-source copies were executed in a clean sandbox with the same standard-library fixture used by the committed tests. Results:

```text
python Scripts/test_blender_roundtrip_national_mall_panel.py
PASS: 37/37 Blender round-trip verifier tests

python Scripts/test_blender_roundtrip_national_mall_panel_hidden_payloads.py
PASS: 16/16 Blender hidden-payload verifier tests

python -m py_compile Scripts/blender_roundtrip_national_mall_panel.py Scripts/verify_blender_roundtrip_national_mall_panel.py Scripts/test_blender_roundtrip_national_mall_panel.py Scripts/test_blender_roundtrip_national_mall_panel_hidden_payloads.py
exit 0
```

The two suites now provide 53 focused cases total. These are source/tooling tests only. They do not establish Blender execution or Astral runtime behavior.

## Verification model

The verifier does not require byte-identical glTF output. It decodes embedded buffers/accessors, validates the fixed source and receipt identities, scans the complete node and mesh collections for disabled or unverified payloads, traverses the active scene hierarchy, derives world-space instance bounds from referenced vertices, checks full inherited world transforms and handedness, compares material semantics, and compares canonical winding-preserving triangle/attribute signatures for the seven semantic mesh instances. The fixed spatial/material/attribute tolerance is `1e-4`.

The bounded output rejects GPU instancing, punctual lights, node cameras, morph targets/weights and rendering attributes outside the four-source-attribute profile. Optional animation/image/texture/camera collections must be absent or exact empty arrays. This gate is intentionally stricter than a filename/count check but is still only a DCC source-workflow gate. It cannot establish Astral compatibility.

## Hosted and independent gates

A fresh exact-head Windows workflow and fresh independent review are required after the current repair commits. Earlier Windows workflow evidence applies only to earlier heads and is historical for the final repaired packet.

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

Require a fresh exact-head hosted check and independent source review of this repaired packet. Then use the registered local execution path to prove Blender 5.2.2 availability, run the exact background command into a fresh evidence directory, preserve shell-level command/version/exit/hash evidence, and run this verifier plus the ART-006C Khronos evidence gate. Only real native outputs may advance the asset to `dcc_roundtrip_verified_not_astral_imported`.
