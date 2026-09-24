# AnimeRPG art toolchain status

Loop: `astral-art-hourly-20260922`  
Updated: 2026-09-22

Statuses are `adopted_for_source`, `candidate`, `blocked_engine_dependency`, `not_revalidated`, or `rejected_for_now`. Adoption means an authoring/source decision only unless runtime evidence is named.

| Job | Tool / format | Status | Evidence and decision |
|---|---|---|---|
| Primary editable 3D DCC | Blender 5.2.2 LTS | `adopted_for_source` | Blender 5.2 LTS released 2026-07-14, 5.2.2 updated 2026-09-15, supported to 2028-07. Use as the first production DCC once locally available; do not claim installation. Source: https://www.blender.org/releases/5-2/ |
| Future mesh/material interchange | glTF 2.0.1 / GLB | `blocked_engine_dependency` | Khronos defines a real-time-oriented triangle/PBR format with base color, metallic/roughness, AO, tangent-space normal, skinning and animation. Astral has no glTF importer. Source: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html |
| Blender export profile | Blender glTF 2.0 exporter | `candidate` | Blender 5.2 manual documents triangle conversion, +Y tangent normals and ORM-compatible hookups. Accept only after an Astral importer calibration asset proves axes, tangents, UVs, skinning and animation. Source: https://docs.blender.org/manual/en/latest/addons/scene_gltf2.html |
| Procedural default materials | Python standard library generators | `adopted_for_source` | Deterministic, dependency-free starter materials v2 created and source-validated this pass. This is for defaults/calibration, not hero art. |
| Concept / look-development generation | GPT Image / FLUX family | `not_revalidated` | Retained from prior research. Recheck exact current model/version, rights, cost and reference consistency before a paid or local benchmark. |
| Image-to-3D generation | Hunyuan3D / TRELLIS / Meshy / Tripo-class tools | `not_revalidated` | Retained as candidate families only. No generation service ran this pass. Test one prop brief against Blender/manual baseline after importer contract exists. |
| Anime humanoid base authoring | VRoid Studio plus technical-art cleanup | `not_revalidated` | Retained candidate. VRM is not currently an Astral runtime format. Do not bulk-produce characters before rig/import proof. |
| Material authoring | Substance-type workflow | `candidate` | Useful after texture/PBR rendering exists; procedural defaults establish channel tests without buying software. |
| Animation cleanup | Cascadeur / conventional Blender animation | `candidate` | Do not choose until one skeleton/root-motion/event contract is accepted by the engine worker. |

## Current source-material convention

Starter-material v2 uses standalone 512 x 512 base-color, tangent-normal, ORM and height files. Base color is sRGB. Data maps are linear. ORM is R=occlusion, G=roughness, B=metallic. This agrees with glTF 2.0.1 and is easier to validate than the original atlas.

## Next tool decision

Do not choose a character-generation model next. First coordinate an engine-owned **triangle + UV + tangent + texture calibration importer/render path**. Once it exists, benchmark Blender 5.2.2 LTS manual/procedural authoring against exactly one current image-to-3D candidate on the same simple prop brief, recording cleanup time and runtime defects.
