# AnimeRPG art toolchain status

Loop: `astral-art-hourly-20260922`  
Updated: 2026-09-22

Statuses are `adopted_for_source`, `candidate`, `source_fixture_ready`, `blocked_engine_dependency`, `not_revalidated`, or `rejected_for_now`. Adoption means an authoring/source decision only unless runtime evidence is named.

| Job | Tool / format | Status | Evidence and decision |
|---|---|---|---|
| Primary editable 3D DCC | Blender 5.2.2 LTS | `adopted_for_source` | Retained as the first editable 3D authoring choice when actually available locally. No installation or export receipt is claimed. Blender 5.2 documents glTF Y-up conversion plus UV, normal, and tangent export. Source: https://docs.blender.org/manual/en/5.2/addons/scene_gltf2.html |
| Future mesh/material interchange | glTF 2.0.1 | `source_fixture_ready` | Khronos identifies glTF 2.0 as the current major version and 2.0.1 as the specification patch level. ART-003 now provides a deterministic self-contained source fixture with triangles, UVs, normals, tangents, materials, named nodes, and an embedded orientation image. Astral still has no glTF importer. Source: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html |
| Blender export profile | Blender glTF 2.0 exporter | `candidate` | ART-003 defines the expected coordinate, UV, tangent, material, and node result. Promote only after a local Blender export round trip and actual Astral importer test match the fixture contract. Source: https://docs.blender.org/manual/en/5.2/addons/scene_gltf2.html |
| Procedural default materials | Python standard library generators | `adopted_for_source` | Starter Materials v2 on PR #23 are source-validated. Exact-head hosted Windows CI at `0bda198c20b33dfaf3fa7cce129e61be0b556272` passed. Runtime texture/material rendering remains unsupported. |
| Procedural calibration geometry | Python standard library glTF generator | `adopted_for_source` | ART-003 generates an asymmetric block, handedness marker, embedded UV-orientation image, materials, and exact manifest. Independent standard-library validation and six regressions pass. |
| Concept / look-development generation | GPT Image / FLUX family | `not_revalidated` | Retained candidates only. Recheck exact current model/version, rights, cost, and reference consistency before a benchmark. |
| Image-to-3D generation | Hunyuan3D / TRELLIS / Meshy / Tripo-class tools | `not_revalidated` | Do not select a winner before ART-003 survives a real importer/render path. Then compare one current candidate against a Blender/manual baseline on the same prop brief. |
| Anime humanoid base authoring | VRoid Studio plus technical-art cleanup | `not_revalidated` | Retained candidate. VRM is not currently an Astral runtime format. Do not bulk-produce characters before rig/import proof. |
| Material authoring | Substance-type workflow | `candidate` | Useful after texture/PBR or stylized material rendering exists; procedural defaults establish channel and tiling tests without a software purchase. |
| Animation cleanup | Cascadeur / conventional Blender animation | `candidate` | Do not select until one skeleton, root-motion, socket, and gameplay-event contract is accepted by the engine worker. |

## ART-003 source calibration contract

The generated fixture follows glTF 2.0's right-handed convention: +Y up, +Z forward, -X right, with linear distances in metres. It includes `POSITION`, `NORMAL`, `TANGENT`, and `TEXCOORD_0`, unsigned-short triangle indices, two named mesh nodes, two materials, and an embedded orientation texture. The separate marker intentionally exposes mirror/handedness mistakes.

Current observed `main` still exposes only the existing `Engine/Assets/StaticMesh.cpp/.h` loader in `Engine/Assets`; no glTF importer is claimed. The engine worker owns any solid-mesh, texture, shader, and editor integration implementation.

## Next tool decision

Do not pick a character or image-to-3D production model next. First run ART-003 through an actual Astral solid-mesh plus texture importer/render path. Once that works, use one matched prop brief to compare Blender/manual authoring with exactly one revalidated generated-3D candidate. Measure cleanup time, topology/UV/tangent defects, import failures, visual review results, memory, and runtime cost per accepted asset.
