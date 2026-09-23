# ART-008 neutral material gallery author QA

Date: 2026-09-23  
Loop: `astral-art-hourly-20260922`  
Status: source-validated candidate, not imported or runtime-approved.

## Delivered

ART-008 adds a machine-readable neutral gallery spec plus deterministic glTF generation and an independent verifier. The derived scene contains four matched sphere/cube stations, five PBR materials including the floor, one fixed camera, two white directional lights, and no image textures. This is an art-side source fixture, not a renderer feature or an Astral screenshot.

Earlier exact-head reviews repaired floor tangent handedness, source-derived camera/light rotations, full floor tangent-frame validation, transform overrides that could reverse camera/light local -Z, uncontracted material rendering properties, incomplete per-triangle normal validation, divergent per-station geometry, source-dimension drift, unsupported glTF evidence claims, mesh/primitive morph overrides, animation transform overrides, supplemental manifest evidence fields, false manifest intent, boolean schema-version acceptance, and non-orthogonal tangent frames.

The ninth exact-head review at `176b2f1c035ade1a6da3992cc6281240b9256bf7` found one additional P2 metadata gap. In supported manifest-only verification, a repinned fixture could replace a canonical POSITION accessor's declared `min` and `max` values with unrelated finite coordinates while leaving the decoded geometry unchanged. The previous verifier checked binary ranges and decoded source dimensions but did not compare glTF accessor bounds to the payload. A consumer using those declared bounds for culling could therefore make an otherwise accepted calibration scene disappear or remain visible incorrectly.

This pass closes that path. Every POSITION accessor now requires finite three-component `min` and `max` arrays whose values match component-wise extrema from the decoded float32 payload within `1e-6`. A dedicated negative regression changes the canonical sphere POSITION accessor bounds to `[100,100,100]` / `[101,101,101]`, repins the generated manifest, and requires rejection specifically with `position bounds`.

The stronger check initially shadowed the existing sphere-radius and cube-extent regressions because those tests changed binary positions without updating their glTF metadata. The regression helper was corrected to scale declared bounds together with the payload. Those tests therefore continue to reach and independently exercise `sphere radius` and `cube extent`; the new metadata regression independently exercises `position bounds`. The focused suite is now 31 tests.

## Source research

The contract remains based on Khronos glTF 2.0.1. POSITION accessor bounds are glTF metadata used to describe vertex extents, while tangent `xyz` is a normalized tangent direction and `w` carries handedness for reconstructing the bitangent. Animations, morph targets, and mesh weights are valid core glTF features but remain outside this immutable calibration fixture.

Primary references retained for this task:
- https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
- https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual
- https://docs.blender.org/manual/en/5.2/addons/scene_gltf2.html

## Verification state

Sandbox source verification for this repair:
- fresh generation: PASS;
- exact generator `--check`: PASS;
- independent verifier with generated manifest plus expected-manifest pin: PASS;
- Python compilation of generator, verifier and regression suite: PASS;
- repinned false POSITION `min`/`max`: correctly rejected with `position bounds`;
- all 31 focused regressions: PASS across bounded isolated batches. The execution environment's wall-time limit prevented one uninterrupted 31-test process, so no single-process 31/31 claim is made.

The generator, canonical source, expected manifest and generated artifact bytes are unchanged:
- canonical `gallery-spec.json`: `517833a990db74f97d8046aa7fafa2d2d73859538d59c3c41ff4a8a7fb63f530`;
- generated `material_gallery.gltf`: `d0bca093cfc52b59816a14ff98cc90b8684e7e7f664d0b687b394be5aa166af1`;
- generated size: 38,604 bytes;
- expected/generated manifest: `ba94405d30eaf4cd7f259b59f9c01e77fbe672527c2a1747061cd3f912c5c3b2`.

The generated glTF remains intentionally untracked; `expected-manifest.json` pins the exact derived output. Fresh exact-head hosted CI and a new exact-head independent review are still required after this repair.

## Publication verification

This repair remains within ART-008 ownership. Implementation changes are limited to the verifier, regression suite, and art-owned records. Source spec, generator and expected manifest are unchanged. Draft PR #33 remains the integration surface. No engine, renderer, gameplay, CMake, workflow or dependency path is changed.

## Evidence boundaries

Not run: Blender export round-trip, Astral import, Astral rendering, workstation Windows/GPU native capture, performance/memory measurement, or independent visual art review.

Therefore this pass establishes only source-contract hardening. It does not establish `imported`, `runtime_verified`, `art_approved`, or parity with Unreal, Unity, Genshin Impact, Zenless Zone Zero, or professional material-review workflows.
