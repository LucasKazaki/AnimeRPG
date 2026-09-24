# ART-006D QA: National Mall Blender round-trip contract

Date: 2026-09-24  
Loop: `astral-art-hourly-20260922`  
Status: **source tooling repaired and sandbox-verified; native Blender execution not run**

## Scope reviewed

This QA packet covers only the ART-006D Blender round-trip driver, independent standard-library verifier, focused regressions and the art-facing Blender/glTF profile. It does not claim that Blender, Khronos glTF-Validator or Astral Engine executed against the National Mall source asset.

## Tool qualification evidence

Official Blender sources were rechecked during the original ART-006D pass. Blender 5.2 LTS released 2026-07-14, Blender 5.2.2 LTS released 2026-09-15, and 5.2 LTS support runs through July 2028. Blender's 5.2 manual/API documents glTF import/export, explicit UV/normal/tangent/material controls, +Y-up export, embedded glTF and background command-line operation.

Source URLs are pinned in `Docs/Art/BLENDER-GLTF-ROUNDTRIP-PROFILE-v0.1.md`.

## Independent review repair

Codex review of initial head `cb0de51ff52119ab6a9e790faedcc6ebfadcda52` found one P1 and two P2 defects:

1. The verifier trusted the caller-supplied ART-006B manifest hash, so a self-consistent substituted source+manifest pair could be accepted.
2. World bounds were computed from every POSITION entry, allowing unused extrema to hide re-indexed geometry drift.
3. Material equivalence checked names/bindings but not visible PBR/material properties.

The repaired verifier now:

- owns the ART-006B source SHA-256 `6c51463332199c65bcfbde04ee8e5883e03a94aba710980eebfaa6945f2759b7` independently and checks key manifest identity/count fields;
- computes each semantic instance AABB from the validated index stream only;
- constrains expected attribute/index accessor formats for this fixture;
- compares named material base color, metallic, roughness, emissive factor, alpha mode/cutoff and sidedness, and rejects unsupported material extensions;
- distinguishes material-property drift from material-binding drift.

## Author verification executed in sandbox

The Blender driver itself cannot execute in this sandbox because `bpy`/Blender is not installed and this task has no dependency-install authority. Its Python syntax remains a source-only check; actual `bpy` execution is still a native Company Runtime/local acceptance step.

The repaired independent verifier was exercised against generated self-contained glTF fixtures that mirror ART-006B's seven semantic instances and three material families.

Executed after the repair:

```text
python Scripts/test_blender_roundtrip_national_mall_panel.py
# PASS: 25/25 Blender round-trip verifier tests

python -m py_compile Scripts/verify_blender_roundtrip_national_mall_panel.py Scripts/test_blender_roundtrip_national_mall_panel.py
# exit 0
```

The Python host also emitted an unrelated spreadsheet-runtime warmup warning after startup; the ART-006D test and `py_compile` processes both exited 0.

Regression coverage now includes:

- valid seven-instance semantic round-trip;
- production ART-006B pin constant;
- stale source hash and a re-pinned self-consistent substituted source+manifest pair;
- manifest identity drift;
- wrong Blender patch version and JSON boolean numeric confusion;
- input, output-glTF and `.blend` receipt hash tampering;
- imported-object inventory and export-profile drift;
- unknown receipt root field;
- missing semantic scene instance;
- world-dimension and world-center drift with output receipt re-pinned;
- re-indexed triangle output that leaves unused POSITION extrema behind;
- missing tangent attribute;
- material-binding drift;
- base-color, roughness and sidedness material-property drift;
- external buffer URI;
- non-triangle primitive mode.

The initial `cb0de51...` hosted Windows workflow passed before these review repairs. A new exact-head hosted run and a fresh independent review are required for the repaired candidate before leaving draft.

## Verification model

The verifier intentionally does not require byte-identical glTF output after Blender. It decodes embedded glTF buffers/accessors, traverses the active scene hierarchy with matrix/TRS transforms, computes each named instance's world-space AABB from referenced vertices, and compares center/dimensions with the ART-006B source within `1e-4` metre. It separately requires indexed triangle primitives, expected attribute/index formats, embedded buffers, semantic material bindings and material-property preservation.

This allows ordinary buffer/accessor repacking by Blender while preventing substituted inputs, re-indexing that changes rendered geometry, or visible material changes from being reported as a verified round trip.

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

First require exact-head hosted checks and a fresh independent source review of the repaired packet. Then, through the registered local execution path, prove Blender 5.2.2 is present, run the exact ART-006D background command into a fresh evidence directory, retain shell-level exit/hash evidence, and run the independent verifier plus ART-006C Khronos evidence gate. Only after those real outputs exist should the asset status advance to `dcc_roundtrip_verified_not_astral_imported`.
