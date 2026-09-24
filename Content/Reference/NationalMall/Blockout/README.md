# National Mall core-panel blockout module

Status: **source validated, not imported into Astral**.

This directory contains one real glTF 2.0 blockout module for a typical National Mall lawn panel. It is deliberately smaller in scope than a finished environment scene. The purpose is to prove a clean reference-to-editable-3D path before producing landmarks, props, foliage, or full-world dressing.

The module is derived from the reviewed `mall-core-axis` entry in `../reference-ledger.json` and pins that ledger's Git blob (`ba8ec205d2aecfa4b2ace15c13c71fb9932cb7f4`). Source measurements stay visible in feet and are converted at this authoring boundary with exactly `1 ft = 0.3048 m`.

## What the glTF contains

`mall_core_panel_blockout.gltf` is a self-contained text glTF with an embedded binary buffer. It contains a real 24-vertex / 36-index unit box mesh with POSITION, NORMAL, TANGENT and TEXCOORD_0 data, reused through three semantic mesh/material bindings and seven node instances:

- one 137.16 m x 51.816 m lawn envelope;
- four 10.668 m gravel-path envelopes around the lawn;
- two 39.624 m tree-grove envelopes outside the paths.

The local asset convention is right-handed glTF, +Y up, +Z forward, -X right. Here +Z means the panel's longitudinal authoring axis. It is **not** a claim that the module has survey-accurate geographic placement or that Astral currently imports glTF.

The grove strips deliberately use the outer path length as a blockout simplification. No tree count, species placement, collision, landmark placement, textures, terrain grading, cross-street spacing, or runtime material response is authored in this packet.

## Files and verification

- `panel-module-source.json`: closed authoring contract and reference conversions.
- `mall_core_panel_blockout.gltf`: inspectable source-only 3D output.
- `expected-manifest.json`: exact hash/size/count/dimension pin.
- `Scripts/generate_national_mall_panel_blockout.py`: deterministic standard-library generator.
- `Scripts/verify_national_mall_panel_blockout.py`: independent structural and semantic verifier.
- `Scripts/test_national_mall_panel_blockout.py`: focused positive/negative regressions.

Run from the repository root:

```text
python Scripts/generate_national_mall_panel_blockout.py --source Content/Reference/NationalMall/Blockout/panel-module-source.json --gltf Content/Reference/NationalMall/Blockout/mall_core_panel_blockout.gltf --manifest Content/Reference/NationalMall/Blockout/expected-manifest.json --check
python Scripts/verify_national_mall_panel_blockout.py --source Content/Reference/NationalMall/Blockout/panel-module-source.json --gltf Content/Reference/NationalMall/Blockout/mall_core_panel_blockout.gltf --manifest Content/Reference/NationalMall/Blockout/expected-manifest.json
python Scripts/test_national_mall_panel_blockout.py
python -m py_compile Scripts/generate_national_mall_panel_blockout.py Scripts/verify_national_mall_panel_blockout.py Scripts/test_national_mall_panel_blockout.py
```

Passing these commands establishes only deterministic source geometry and provenance. It does not establish Blender round-trip behavior, Astral import/rendering, collision, LODs, texture/material correctness, native performance, artist approval, or parity with Unreal, Unity, Genshin Impact, or Zenless Zone Zero.
