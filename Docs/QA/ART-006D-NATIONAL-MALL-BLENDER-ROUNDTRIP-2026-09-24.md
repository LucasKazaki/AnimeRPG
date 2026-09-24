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

The review that completed on `78c9acbc2dbbac312d0c9ca6cd4b0c82555b1b86` surfaced four remaining issues: one P1 regression in the committed GPU-instancing error-text expectation, plus P2 gaps for same-AABB world rotation, morph-target deformation, and extra `COLOR_0` rendering attributes. Verifier commit `738511005b5737b2693392d6fd64dec994e6a07a` and regression commit `6591b226e1760b83d022b43fa2aabef5b6c52397` repair them by:

1. restoring a distinct `GPU instancing unsupported` failure path while keeping punctual-light failures separate;
2. comparing the complete inherited 3x4 world transform within the fixed `1e-4` tolerance in addition to transform parity;
3. rejecting primitive morph `targets`, mesh `weights`, and node `weights` across the complete bounded glTF;
4. requiring the exact four rendering attributes `POSITION`, `NORMAL`, `TANGENT`, and `TEXCOORD_0`, so unverified vertex colors, skinning attributes, and other extras fail closed;
5. adding three committed hidden regressions for 180-degree world rotation, morph deformation, and vertex-color injection, taking the additive hidden suite from seven to ten cases.

The four newest review threads remain a gate until the repair is replied to, resolved, and independently re-reviewed on the final exact head.

## Author verification executed in sandbox

Before publication, the repaired verifier was syntax-compiled and exercised with a focused in-memory fixture matching the committed test fixture's seven semantic nodes, three materials, embedded indexed triangle data and fixed receipt profile. The valid path returned `dcc_roundtrip_verified_not_astral_imported`; independent mutations for a 180-degree `Lawn` rotation, morph target with nonzero weight, `COLOR_0`, and `EXT_mesh_gpu_instancing` each failed with the intended repaired gate. This targeted probe is author evidence only and does not replace the repository suites or independent review.

The committed verification commands remain:

```text
python Scripts/test_blender_roundtrip_national_mall_panel.py
python Scripts/test_blender_roundtrip_national_mall_panel_hidden_payloads.py
python -m py_compile Scripts/blender_roundtrip_national_mall_panel.py Scripts/verify_blender_roundtrip_national_mall_panel.py Scripts/test_blender_roundtrip_national_mall_panel.py Scripts/test_blender_roundtrip_national_mall_panel_hidden_payloads.py
```

The original committed suite remains 37 cases. The hidden-payload suite now contains ten cases, for 47 focused committed cases total. A fresh exact-head hosted run is required after these repairs; prior workflow run `35991975091` / #1055 passed on `78c9ac...` but is historical for later heads.

## Verification model

The verifier does not require byte-identical glTF output. It decodes embedded buffers/accessors, validates the fixed source and receipt identities, scans the complete node and mesh collections for disabled or unverified payloads, traverses the active scene hierarchy, derives world-space instance bounds from referenced vertices, checks full inherited world transforms and handedness, compares material semantics, and compares canonical winding-preserving triangle/attribute signatures for the seven semantic mesh instances. The fixed spatial/material/attribute tolerance is `1e-4`.

The bounded output rejects GPU instancing, punctual lights, node cameras, morph targets/weights and rendering attributes outside the four-source-attribute profile. Optional animation/image/texture/camera collections must be absent or exact empty arrays. This gate is intentionally stricter than a filename/count check but is still only a DCC source-workflow gate. It cannot establish Astral compatibility.

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
