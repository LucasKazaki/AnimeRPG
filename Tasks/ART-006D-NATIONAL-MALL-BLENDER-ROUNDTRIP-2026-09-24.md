# ART-006D: National Mall Blender round-trip contract

Owner: AnimeRPG Art Direction worker (`astral-art-hourly-20260922`)  
Base: ART-006C exact head `6b558a1f8deed3ca1b8463cb10534d01afa9f333`  
Scope: source-art/DCC qualification only

## Objective

Close the next dependency-ready gap in the representative National Mall asset path by making Blender 5.2.2 LTS round-trip execution reproducible and independently checkable for the ART-006B panel blockout.

This task does **not** install Blender, run Company Runtime, change Astral engine/importer code, or claim a Blender execution that has not happened.

## Allowed paths

- `Docs/Art/BLENDER-GLTF-ROUNDTRIP-PROFILE-v0.1.md`
- `Scripts/blender_roundtrip_national_mall_panel.py`
- `Scripts/verify_blender_roundtrip_national_mall_panel.py`
- `Scripts/test_blender_roundtrip_national_mall_panel.py`
- `Tasks/ART-006D-NATIONAL-MALL-BLENDER-ROUNDTRIP-2026-09-24.md`
- `Docs/QA/ART-006D-NATIONAL-MALL-BLENDER-ROUNDTRIP-2026-09-24.md`

Do not edit engine, renderer, gameplay, build, workflow, dependency or another worker's continuation-state files in this packet.

## Inputs

- `Content/Reference/NationalMall/Blockout/mall_core_panel_blockout.gltf`
- `Content/Reference/NationalMall/Blockout/expected-manifest.json`
- ART-006B expected source SHA-256 `6c51463332199c65bcfbde04ee8e5883e03a94aba710980eebfaa6945f2759b7`

## Deliverables

1. A Blender-native background driver pinned to Blender 5.2.2.
2. The driver saves one editable `.blend`, exports one embedded glTF and writes a strict JSON receipt.
3. A standard-library verifier that compares the DCC output with ART-006B's source semantics rather than byte equality.
4. Focused regressions for evidence tampering, version/type confusion, missing assets, transform drift, material drift, missing tangents, external buffers and non-triangle output.
5. A versioned art-facing round-trip profile based on current official Blender documentation.

## Native execution command

Run only through the registered workstation executor when Blender 5.2.2 availability is proven. Use a fresh evidence directory. Example from repository root:

```text
"C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup ^
  --python Scripts\blender_roundtrip_national_mall_panel.py -- ^
  --input Content\Reference\NationalMall\Blockout\mall_core_panel_blockout.gltf ^
  --output <fresh-evidence-dir>\mall_core_panel_blender_roundtrip.gltf ^
  --blend <fresh-evidence-dir>\mall_core_panel_roundtrip.blend ^
  --receipt <fresh-evidence-dir>\blender-roundtrip-receipt.json
```

Retain the actual executable path, `blender --version`, working directory, complete command, start/end timestamps, process exit code, stdout/stderr and pre/post source hash in the native receipt bundle. The script's JSON receipt is necessary but does not replace shell-level execution evidence.

Then run:

```text
python Scripts/verify_blender_roundtrip_national_mall_panel.py ^
  --source Content/Reference/NationalMall/Blockout/mall_core_panel_blockout.gltf ^
  --manifest Content/Reference/NationalMall/Blockout/expected-manifest.json ^
  --roundtrip <fresh-evidence-dir>/mall_core_panel_blender_roundtrip.gltf ^
  --blend <fresh-evidence-dir>/mall_core_panel_roundtrip.blend ^
  --receipt <fresh-evidence-dir>/blender-roundtrip-receipt.json
```

After ART-006C is integrated, run the official Khronos glTF-Validator on the round-trip output using the ART-006C evidence adapter. Do not install or download it under this task without separate approval/availability.

## Source-only verification commands

These do not execute Blender:

```text
python Scripts/test_blender_roundtrip_national_mall_panel.py
python -m py_compile Scripts/blender_roundtrip_national_mall_panel.py Scripts/verify_blender_roundtrip_national_mall_panel.py Scripts/test_blender_roundtrip_national_mall_panel.py
```

## Stop condition

This bounded packet stops when the scripts/profile/task/QA changes are published, the focused standard-library regression suite and Python compilation pass, hosted repository checks are observed, and independent exact-head source review is requested/completed according to the existing art-worker gate.

Native Blender execution is a separate future gate. A clean source packet must not be labeled `dcc_roundtrip_executed_not_astral_imported` until real Blender 5.2.2 outputs and shell receipts exist.
