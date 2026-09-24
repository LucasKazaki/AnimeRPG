# ART-006D QA: National Mall Blender round-trip contract

Date: 2026-09-24  
Loop: `astral-art-hourly-20260922`  
Status: **source tooling verified; native Blender execution not run**

## Scope reviewed

This QA packet covers only the ART-006D Blender round-trip driver, independent standard-library verifier, focused regressions and the art-facing Blender/glTF profile. It does not claim that Blender, Khronos glTF-Validator or Astral Engine executed against the National Mall source asset.

## Tool qualification evidence

Official Blender sources were rechecked during this pass. Blender 5.2 LTS released 2026-07-14, Blender 5.2.2 LTS released 2026-09-15, and 5.2 LTS support runs through July 2028. Blender's 5.2 manual/API documents glTF import/export, explicit UV/normal/tangent/material controls, +Y-up export, embedded glTF and background command-line operation.

Source URLs are pinned in `Docs/Art/BLENDER-GLTF-ROUNDTRIP-PROFILE-v0.1.md`.

## Author verification executed in sandbox

The Blender driver itself cannot execute in this sandbox because `bpy`/Blender is not installed and this task has no dependency-install authority. Its Python syntax was compiled without importing `bpy`.

The independent verifier was exercised against generated self-contained glTF fixtures that mirror ART-006B's seven semantic instances and three material families.

Executed:

```text
python Scripts/test_blender_roundtrip_national_mall_panel.py
# PASS: 18/18 Blender round-trip verifier tests

python -m py_compile Scripts/blender_roundtrip_national_mall_panel.py Scripts/verify_blender_roundtrip_national_mall_panel.py Scripts/test_blender_roundtrip_national_mall_panel.py
# exit 0
```

Regression coverage includes:

- valid seven-instance semantic round-trip;
- stale ART-006B source hash;
- wrong Blender patch version;
- JSON boolean substituted into numeric Blender version tuple;
- input, output-glTF and `.blend` receipt hash tampering;
- JSON boolean substituted for mesh object count;
- imported-object inventory drift;
- export-profile drift;
- unknown receipt root field;
- missing semantic scene instance;
- world-dimension drift with output receipt re-pinned;
- world-center drift with output receipt re-pinned;
- missing tangent attribute;
- material-binding drift;
- external buffer URI;
- non-triangle primitive mode.

## Verification model

The verifier intentionally does not require byte-identical glTF output after Blender. It decodes embedded glTF buffers/accessors, traverses the active scene hierarchy with matrix/TRS transforms, computes each named instance's world-space AABB, and compares center/dimensions with the ART-006B source within `1e-4` metre. It separately requires indexed triangle primitives, `POSITION`, `NORMAL`, `TANGENT`, `TEXCOORD_0`, semantic material-name preservation and embedded buffers.

This means accessor/buffer repacking by Blender is allowed, while visible transform/scale/material or required-attribute drift is not.

## Not run / not claimed

- Blender 5.2.2 executable availability on Lucas's workstation;
- Blender import, `.blend` save or glTF export;
- official Khronos glTF-Validator execution;
- Astral import or rendering;
- collision, LOD, navigation or streaming behavior;
- frame-time, RAM or VRAM measurement;
- independent visual-art approval;
- UE5, Unity, Genshin Impact or Zenless Zone Zero parity.

## Next acceptance

Through the registered local execution path, prove Blender 5.2.2 is present, run the exact ART-006D background command into a fresh evidence directory, retain shell-level exit/hash evidence, then run the independent verifier and ART-006C Khronos evidence gate. Only after that should the asset status advance to `dcc_roundtrip_verified_not_astral_imported`.
