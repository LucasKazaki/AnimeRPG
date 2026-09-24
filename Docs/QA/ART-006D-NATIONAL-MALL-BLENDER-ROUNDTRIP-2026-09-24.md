# ART-006D QA: National Mall Blender round-trip contract

Date: 2026-09-24  
Loop: `astral-art-hourly-20260922`  
Status: **source tooling repaired; native Blender execution not run**

## Scope reviewed

This packet covers only the ART-006D Blender background driver, independent standard-library verifier, focused regressions and the art-facing Blender/glTF profile. It does not claim that Blender, Khronos glTF-Validator or Astral Engine executed against the National Mall source asset.

## Tool qualification evidence

Blender 5.2.2 LTS remains the pinned DCC for this representative round trip. Official Blender 5.2 release, manual, API and command-line sources are recorded in `Docs/Art/BLENDER-GLTF-ROUNDTRIP-PROFILE-v0.1.md`. No dependency was installed or downloaded by this task.

## Independent review repair history

The initial Codex review of `cb0de51ff52119ab6a9e790faedcc6ebfadcda52` found three issues. Later repairs pinned the ART-006B source independently, derived bounds from indexed vertices, and compared visible material semantics.

Subsequent exact-head reviews tightened the fixed `1e-4` tolerance, winding/topology and attribute payload checks, receipt type checking, exact active-scene inventory, transform parity and full inherited transforms, GPU-instancing/light/camera rejection, optional collection types, morph rejection, exact rendering attributes across reachable and unreachable primitives, and regression independence. The repaired head `1393c786527c4ef29f59b3766ec47d19aceac36f` restored the original center/dimension error contract while covering inherited parent transforms and hidden primitive payloads.

The exact-head review of `1393c786527c4ef29f59b3766ec47d19aceac36f` found two P2 issues. The following repair applied every primitive's accessor/index decoder contract to unreachable meshes and replaced component quantization with pairwise `1e-4` triangle/corner matching. This added unreachable accessor/index regressions plus a positive `0.00006` attribute-drift case that must remain accepted.

The fresh review of exact head `2c9813bc53d20a797dfc67f23ec753c3a8c94b3f` then found one remaining P2: an accessor could use bytes wholly inside the binary payload while its enclosing `bufferView.byteLength` extended past the end of the embedded buffer. The current repair closes that gap by requiring the full declared bufferView range, `byteOffset + byteLength`, to stay inside its referenced buffer before any accessor decode. A dedicated unreachable-normal regression clones a valid view, extends only its declared length past EOF while keeping the used accessor bytes in range, and requires fail-closed rejection.

## Source-only verification executed for the current repair

A clean sandbox reconstruction of the branch scripts ran the packet's primary suite, the repaired hidden suite, and Python compilation. Results:

```text
python Scripts/test_blender_roundtrip_national_mall_panel.py
PASS: 37/37 Blender round-trip verifier tests

python Scripts/test_blender_roundtrip_national_mall_panel_hidden_payloads.py
PASS: 24/24 Blender hidden-payload verifier tests

python -m py_compile Scripts/blender_roundtrip_national_mall_panel.py Scripts/verify_blender_roundtrip_national_mall_panel.py Scripts/test_blender_roundtrip_national_mall_panel.py Scripts/test_blender_roundtrip_national_mall_panel_hidden_payloads.py
exit 0
```

The two suites now provide 61 focused cases total. These are source/tooling tests only. They do not establish Blender execution or Astral runtime behavior.

## Verification model

The verifier does not require byte-identical glTF output. It validates the pinned ART-006B source and evidence receipt, embedded buffers, complete declared bufferView ranges, accessor/index formats and payload bounds for every mesh primitive including unreachable content, active-scene inventory, inherited world transforms and handedness, indexed world bounds, material semantics, and winding-preserving triangle/corner payloads using the fixed pairwise `1e-4` tolerance.

The bounded output also rejects GPU instancing, punctual lights, node cameras, morph targets/weights and rendering attributes outside `POSITION`, `NORMAL`, `TANGENT`, and `TEXCOORD_0`. Optional animation/image/texture/camera collections must be absent or exact empty arrays. This remains a DCC source-workflow gate only and cannot establish Astral compatibility.

## Hosted and independent gates

Windows workflow `36003846237`, run #1090, completed successfully on earlier exact head `2c9813bc53d20a797dfc67f23ec753c3a8c94b3f`. That evidence is historical after the current source repair. A fresh exact-head Windows workflow and independent source review are required for the new candidate before this PR can leave draft state.

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
