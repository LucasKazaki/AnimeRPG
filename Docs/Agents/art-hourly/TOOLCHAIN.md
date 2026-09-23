# AnimeRPG art toolchain status

Loop: `astral-art-hourly-20260922`  
Updated: 2026-09-23

Statuses are `adopted_for_source`, `candidate`, `source_fixture_ready`, `blocked_engine_dependency`, `not_revalidated`, or `rejected_for_now`. Adoption means an authoring/source decision only unless runtime evidence is named.

| Job | Tool / format | Status | Evidence and decision |
|---|---|---|---|
| Primary editable 3D DCC | Blender 5.2.2 LTS | `adopted_for_source` | Retained as the first editable 3D authoring choice when actually available locally. No installation or export receipt is claimed. Blender 5.2 documents glTF Y-up conversion plus UV, normal, and tangent export. Source: https://docs.blender.org/manual/en/5.2/addons/scene_gltf2.html |
| Future mesh/material interchange | glTF 2.0.1 | `source_fixture_ready` | ART-003 has a deterministic self-contained source fixture with triangles, UVs, normals, tangents, materials, named nodes and an embedded orientation image. Exact-head Windows CI passed and Codex re-review completed on `e81f32b...`; Astral still has no proven glTF importer. Source: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html |
| Blender export profile | Blender glTF 2.0 exporter | `candidate` | ART-003 defines the expected coordinate, UV, tangent, material and node result. Promote only after a local Blender export round trip and actual Astral importer test match the fixture contract. |
| Procedural default materials | Python standard library generators | `adopted_for_source` | Starter Materials v2 on PR #23 are source-validated. Runtime texture/material rendering remains unsupported. |
| Procedural calibration geometry | Python standard library glTF generator | `adopted_for_source` | ART-003 repaired fixture has 13/13 source regressions plus exact-head hosted/review gates complete. Runtime importer/render remains engine-owned. |
| Color/value review references | Python standard library PNG generator | `adopted_for_source` | ART-007 generates an eight-role palette card, own-layout neutral 16^3 LUT diagnostic and 16-step value ramp with a pinned manifest and 18 regressions. Generator and verifier both require source status `proposed_art_reference_not_runtime`; outputs remain art references, not runtime grading assets. |
| Runtime tonemapping / grading | Engine-owned future implementation | `blocked_engine_dependency` | Epic UE 5.8 separates project-wide film/tonemapping from scene-referred color correction; Unity 6 URP exposes Neutral/ACES and HDR calibration. Art requires reproducible neutral review exposure before approving a creative grade. No Astral implementation is claimed. |
| Concept / look-development generation | GPT Image / FLUX family | `not_revalidated` | Retained candidates only. Recheck exact model/version, rights, cost and reference consistency before a benchmark. |
| Image-to-3D generation | Hunyuan3D / TRELLIS / Meshy / Tripo-class tools | `not_revalidated` | Do not select a winner before ART-003 survives a real importer/render path. Then compare one current candidate against a Blender/manual baseline on the same prop brief. |
| Anime humanoid base authoring | VRoid Studio plus technical-art cleanup | `not_revalidated` | Retained candidate. VRM is not currently an Astral runtime format. Do not bulk-produce characters before rig/import proof. |
| Material authoring | Substance-type workflow | `candidate` | Useful after texture/PBR or stylized material rendering exists; procedural defaults establish channel and tiling tests without a software purchase. |
| Animation cleanup | Cascadeur / conventional Blender animation | `candidate` | Do not select until one skeleton, root-motion, socket and gameplay-event contract is accepted by the engine worker. |

## ART-003 engine handoff status

The glTF source fixture follows the right-handed +Y up, +Z forward, -X right contract, has explicit triangles/UVs/normals/tangents/materials, and includes a separate handedness marker. Exact-head hosted Windows CI `35778589314` passed at `e81f32b8545954b30a205a969bb6f90f0661f7f2`; exact-head Codex review completed with no new inline finding observed after repair. PR #26 is ready for review and unmerged. The engine worker owns actual solid-mesh/texture import and rendering.

## ART-007 color review decision

Do not encode the game's look as a single baked LUT. Keep asset color, scene lighting/exposure and creative grading separable. Epic UE 5.8 documents project-wide film controls and scene-referred color correction; Unity 6 URP documents Neutral/ACES tonemapping and HDR calibration. The ART-007 neutral LUT is an Astral-defined diagnostic layout only, not an Unreal/Unity-compatible runtime asset. WCAG 2.2's 4.5:1 text contrast threshold is used as a screening metric for four proposed text-like foreground/background pairs, not as a blanket game-accessibility claim.

## Next tool decision

Keep generated-3D model selection paused until the ART-003 asset is visible in Astral. In parallel, use ART-007 to define capture metadata and neutral review expectations so future material/character comparisons are made under repeatable color/exposure conditions rather than flattering ad hoc grades.
