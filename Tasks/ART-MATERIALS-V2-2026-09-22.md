# ART-MATERIALS-V2: art bible and standalone material defaults

Date: 2026-09-22  
Loop: `astral-art-hourly-20260922`  
Original baseline: `main` at `755faabfb5f04d5bc07324d91cbceb261cdc1060`.  
Latest independently moving `main` observed during review repair: `e782f595696c046c2c394b636622f700a18d0425`.

## Operator authorization

On September 22, 2026 Lucas explicitly instructed this separate hourly art worker to perform bounded art-direction, creative-tooling, asset-production and verification work for AnimeRPG, continuing until paused or stopped. That newer operator instruction admits dependency-ready art direction, original source/default assets, creative QA and tool/workflow qualification in parallel with the engine and game workers. It does not grant renderer/editor/gameplay ownership, automatic merge authority, architecture changes, spending, installs, local execution, deployment or release.

This authorization is also recorded in `GAME_DEVELOPMENT_CONTROL.md` and `AGENTS.md` in this review-repair commit so the repository source-of-truth hierarchy no longer relies on this task packet to authorize itself.

## Objective

Deliver one dependency-safe art increment without changing renderer/editor/gameplay ownership: establish proposed visual rules and replace the first atlas-only calibration approach with a reproducible standalone source-material set suitable for future importer/render tests.

## Allowed additions and narrow authorization edits

- `Docs/Art/ART-BIBLE-v0.1.md`
- `Docs/Agents/art-hourly/README.md`
- `Docs/Agents/art-hourly/BACKLOG.json`
- `Docs/Agents/art-hourly/STATE.json`
- `Docs/Agents/art-hourly/TOOLCHAIN.md`
- `Content/Starter/MaterialsV2/README.md`
- `Content/Starter/MaterialsV2/expected-manifest.json`
- `Scripts/generate_starter_materials_v2.py`
- `Scripts/verify_starter_materials_v2.py`
- `Scripts/test_starter_materials_v2.py`
- `Docs/QA/ART-MATERIALS-V2-2026-09-22.md`
- this task
- `GAME_DEVELOPMENT_CONTROL.md`, only to record Lucas's explicit hourly art-worker authorization
- `AGENTS.md`, only to mirror that scoped operator authorization in agent rules

No renderer, editor, gameplay, CMake, workflow, dependency, R0 or scheduler change is admitted. Generated PNGs remain build/test outputs until a real asset pipeline exists.

## Sources rechecked

Accessed 2026-09-22:

- Epic UE 5.8 PBR: https://dev.epicgames.com/documentation/unreal-engine/physically-based-materials-in-unreal-engine
- Unity 6 URP Lit: https://docs.unity3d.com/6000.0/Manual/urp/prebuilt-shader-graphs-urp-lit.html
- Khronos glTF 2.0.1: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
- Blender 5.2 LTS: https://www.blender.org/releases/5-2/
- Blender glTF exporter: https://docs.blender.org/manual/en/latest/addons/scene_gltf2.html
- Genshin Impact official PlayStation page: https://www.playstation.com/en-us/games/genshin-impact/
- Zenless Zone Zero official PlayStation page: https://www.playstation.com/en-us/games/zenless-zone-zero/

## Review-repair acceptance

The first independent Codex review of `e1c4d0fecec704aa342c06e0dbcfaca34f3f979a` found one authorization issue and three material-tooling issues: unbounded PNG inflation, sparse metallic validation, and duplicated periodic endpoints. This bounded repair must:

- record the explicit operator authorization at the controlling repository level;
- bound decoded PNG allocation before inflation;
- validate every ORM metallic texel;
- sample the full periodic domain over `[0,1)` and wrap normal gradients modulo all 512 unique texels;
- add regressions for a non-grid metallic corruption, an inflation bomb, and duplicated-endpoint prevention;
- regenerate and pin the exact manifest after the generator version changes.

## Required checks

```text
python Scripts/generate_starter_materials_v2.py --output <new-dir>
python Scripts/generate_starter_materials_v2.py --output <same-dir> --check
python Scripts/verify_starter_materials_v2.py <same-dir> --expected-manifest Content/Starter/MaterialsV2/expected-manifest.json
python Scripts/test_starter_materials_v2.py
python -m py_compile Scripts/generate_starter_materials_v2.py Scripts/verify_starter_materials_v2.py Scripts/test_starter_materials_v2.py
```

A fixed-metadata temporary archive roundtrip was also used as an author-side packaging stress check in the initial pass, but no generated binary pack is admitted as repository source in this slice. This task cannot claim an Astral render, Windows runtime, importer integration, UE/Unity parity or independent art approval.
