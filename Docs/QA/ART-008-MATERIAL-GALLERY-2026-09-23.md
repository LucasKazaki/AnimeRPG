# ART-008 neutral material gallery author QA

Date: 2026-09-23  
Loop: `astral-art-hourly-20260922`  
Status: source-validated candidate, not imported or runtime-approved.

## Delivered

ART-008 adds a machine-readable neutral gallery spec plus deterministic glTF generation and an independent verifier. The derived scene contains four matched sphere/cube stations, five PBR materials including the floor, one fixed camera, two white directional lights, and no image textures. This is an art-side source fixture, not a renderer feature or an Astral screenshot.

Ten earlier exact-head review rounds repaired tangent handedness, camera/light rotation and transform validation, closed materials and evidence fields, per-triangle normals, matched geometry and source dimensions, morph/animation overrides, manifest evidence boundaries, POSITION format/bounds, and canonical source-only intent.

The eleventh independent review at `3b7d1600ed1f053465f0dfccd85414d48ced859c` found three additional P2 gaps. First, a repinned cube could replace its canonical faces with one triangle repeated twelve times while preserving counts and decoded extents. Second, sphere/cube tangent handedness could be flipped even though tangent length and normal orthogonality remained valid. Third, valid glTF `extras` on nested objects could carry unsupported runtime/art evidence claims outside the already-closed root contract.

This pass closes all three paths. The verifier derives the exact generator-owned sphere/cube/floor index payloads and requires the decoded indices to match them. It derives tangent and bitangent orientation from each triangle's positions and UVs for every mesh, with a stricter orientation threshold retained for the floor. It also recursively rejects nested glTF `extras` everywhere outside the approved root `extras.astral_contract`. Dedicated repinned negatives cover repeated cube triangles, non-floor `TANGENT.w=-1`, and camera `extras` carrying `runtime_verified` / `art_approved`. The focused suite now contains 36 tests.

## Source research

The contract remains based on Khronos glTF 2.0.1. POSITION uses FLOAT `VEC3`; POSITION accessor bounds describe decoded geometry; tangent `xyz` plus `w` reconstruct a tangent-space bitangent; and application-specific `extras` may appear on glTF objects, which is why this intentionally closed calibration fixture now rejects nested extras rather than treating them as evidence-neutral.

Primary references retained for this task:
- https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
- https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual
- https://docs.blender.org/manual/en/5.2/addons/scene_gltf2.html

## Verification state

Fresh sandbox evidence for this eleventh-review repair:
- Python compilation of generator, verifier and regression suite: PASS;
- canonical generation / expected-manifest pin checks are included in the focused suite and passed;
- tests 1-18 of the 36-test focused suite: PASS in one bounded isolated batch;
- tests 19-36 of the focused suite: PASS in a second bounded isolated batch;
- repeated-triangle cube mutation: rejected with `canonical indices`;
- non-floor handedness mutation: rejected with `mesh tangent frame`;
- nested camera evidence extras: rejected with `nested extras unsupported`.

The 36 tests were executed in two bounded batches because one uninterrupted invocation exceeded the available sandbox command window. This is source-validation evidence only, not native/Astral evidence.

The generator, canonical source, expected manifest and generated artifact bytes are unchanged:
- canonical `gallery-spec.json`: `517833a990db74f97d8046aa7fafa2d2d73859538d59c3c41ff4a8a7fb63f530`;
- generated `material_gallery.gltf`: `d0bca093cfc52b59816a14ff98cc90b8684e7e7f664d0b687b394be5aa166af1`;
- generated size: 38,604 bytes;
- expected/generated manifest: `ba94405d30eaf4cd7f259b59f9c01e77fbe672527c2a1747061cd3f912c5c3b2`.

The generated glTF remains intentionally untracked; `expected-manifest.json` pins the exact derived output.

## Publication verification

This repair remains within ART-008 ownership. Implementation changes are limited to the verifier, regression suite, task/QA and art-hourly continuation records. Source spec, generator and expected manifest are unchanged. Draft PR #33 remains the integration surface. No engine, renderer, gameplay, CMake, workflow or dependency path is changed.

Fresh exact-head hosted Windows CI and an independent exact-head review are still required after the final documentation head is established. Passing source tests does not clear those separate gates.

## Evidence boundaries

Not run: Blender export round-trip, Astral import, Astral rendering, workstation Windows/GPU native capture, performance/memory measurement, or independent visual art review.

Therefore this pass establishes only source-contract hardening. It does not establish `imported`, `runtime_verified`, `art_approved`, or parity with Unreal, Unity, Genshin Impact, Zenless Zone Zero, or professional material-review workflows.
