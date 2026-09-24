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

A fresh review of `918ec775c73c1e6b6f91fe7fcdf2ba973be6fa80` found one P1 and three P2 issues. Head `1393c786527c4ef29f59b3766ec47d19aceac36f` repaired all four by preserving the committed center/dimension error contract, extending exact attribute-name checks to unreachable mesh primitives, splitting morph regressions by branch, and exercising inherited parent-transform composition directly.

The next exact-head review of `1393c786527c4ef29f59b3766ec47d19aceac36f` found two P2 issues. This repair addresses both:

1. every mesh primitive is now passed through the same accessor decoder/contract gate before active-scene traversal, including unreachable meshes. Float attribute accessor indices/formats, nonzero matching counts, buffer-view offsets/lengths, decoded finite payloads, unsigned scalar index accessors, nonempty triangle-multiple index payloads, vertex-range bounds and material references all fail closed before an unreachable primitive can be ignored by scene traversal;
2. topology comparison no longer quantizes each component independently into `1e-4` buckets. It preserves material grouping, triangle winding and cyclic-corner equivalence, then uses pairwise absolute `1e-4` comparisons with bipartite triangle matching. This accepts legitimate values that differ by less than the documented tolerance even when they straddle an arbitrary quantization boundary.

The hidden suite adds distinct regressions for unreachable invalid/incorrect `NORMAL` accessors, zero attribute counts, buffer-bound escapes, invalid index formats, zero index counts, plus a positive `0.00006` normal-component drift that must remain accepted under the `1e-4` gate.

## Source-only verification commands

The exact commands for this repaired head are:

```text
python Scripts/test_blender_roundtrip_national_mall_panel.py
python Scripts/test_blender_roundtrip_national_mall_panel_hidden_payloads.py
python -m py_compile Scripts/blender_roundtrip_national_mall_panel.py Scripts/verify_blender_roundtrip_national_mall_panel.py Scripts/test_blender_roundtrip_national_mall_panel.py Scripts/test_blender_roundtrip_national_mall_panel_hidden_payloads.py
```

The original suite remains 37 cases. The hidden suite now contains 23 cases, for 60 focused cases total. Fresh exact-head hosted execution and independent review are required after this repair; earlier pass receipts remain historical evidence for earlier heads only.

## Verification model

The verifier does not require byte-identical glTF output. It decodes embedded buffers/accessors, validates the fixed source and receipt identities, scans the complete node and mesh collections for disabled or unverified payloads, validates every primitive's accessor/index payload contract even when unreachable, traverses the active scene hierarchy, derives world-space instance bounds from referenced vertices, checks full inherited world transforms and handedness, compares material semantics, and compares winding-preserving triangle/corner payloads with the fixed pairwise tolerance for the seven semantic mesh instances. The fixed spatial/material/attribute tolerance is `1e-4`.

The bounded output rejects GPU instancing, punctual lights, node cameras, morph targets/weights and rendering attributes outside the four-source-attribute profile. Optional animation/image/texture/camera collections must be absent or exact empty arrays. This gate is intentionally stricter than a filename/count check but is still only a DCC source-workflow gate. It cannot establish Astral compatibility.

## Hosted and independent gates

A fresh exact-head Windows workflow and fresh independent review are required after the newest repair commits. Do not reuse workflow or review evidence from `1393c786...` or older heads for the repaired candidate.

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
