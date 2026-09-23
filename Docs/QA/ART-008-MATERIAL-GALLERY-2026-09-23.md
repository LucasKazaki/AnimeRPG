# ART-008 neutral material gallery author QA

Date: 2026-09-23  
Loop: `astral-art-hourly-20260922`  
Status: source-validated candidate under exact-head re-review, not imported or runtime-approved.

## Delivered

ART-008 adds a machine-readable neutral gallery spec plus deterministic glTF generation and an independent verifier. The derived scene contains four matched sphere/cube stations, five PBR materials including the floor, one fixed camera, two white directional lights, and no image textures. This is an art-side source fixture, not a renderer feature or an Astral screenshot.

Eleven earlier exact-head review rounds repaired tangent handedness, camera/light rotation and transform validation, closed materials and evidence fields, per-triangle normals, matched geometry and source dimensions, morph/animation overrides, manifest evidence boundaries, POSITION format/bounds, canonical source-only intent, canonical topology, all-mesh tangent-frame orientation, and nested `extras` handling.

The twelfth independent review at `3c6b56a4bef3d0152598e40bc18c7b5a5464e456` found two additional P2 gaps. First, non-floor derivative-frame alignment used a threshold of only `1e-4`, so a cube tangent rotated 80 degrees toward its bitangent could remain unit length, normal-orthogonal, correctly handed and still pass. Second, POSITION had an exact accessor-format check but NORMAL, TANGENT, TEXCOORD_0 and indices did not, allowing a repinned invalid semantic such as FLOAT VEC4 NORMAL to be decoded and accepted.

This pass closes both paths. Non-floor tangent and reconstructed-bitangent alignment must now each exceed `0.95` against the triangle position/UV derivative frame, while the floor retains `0.9999`. The verifier also checks exact generator-owned semantic formats before decoding: POSITION FLOAT VEC3, NORMAL FLOAT VEC3, TANGENT FLOAT VEC4, TEXCOORD_0 FLOAT VEC2, and indices UNSIGNED_SHORT SCALAR, with `normalized=true` rejected for these exact-format FLOAT vertex attributes.

Six focused negatives were added, bringing the suite definition from 36 to 42 tests: an 80-degree cube tangent rotation, invalid NORMAL VEC4, invalid TANGENT VEC3, invalid TEXCOORD_0 VEC3, invalid index VEC2, and a FLOAT NORMAL accessor carrying `normalized=true`.

## Source research

The contract remains based on Khronos glTF 2.0.1. Core mesh semantic rules require POSITION and NORMAL as VEC3, TANGENT as VEC4, TEXCOORD as VEC2, and indices as SCALAR; FLOAT accessors do not use `normalized=true`. ART-008 intentionally narrows formats further to the exact deterministic generator output, FLOAT TEXCOORD_0 and UNSIGNED_SHORT indices, rather than claiming to accept every glTF-valid encoding.

Primary references retained for this task:
- https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
- https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual
- https://docs.blender.org/manual/en/5.2/addons/scene_gltf2.html

## Threshold qualification

A source-level calculation using the canonical generator geometry, 12 latitude segments and 24 longitude segments, measured the worst authored sphere tangent alignment against the triangle UV derivative at approximately `0.9914448614` and the worst reconstructed-bitangent alignment at approximately `0.9588133133`. A `0.95` minimum therefore retains the canonical source fixture with a small measured margin while rejecting the reviewed 80-degree rotated tangent, whose directional dot is approximately `0.1736481777`. This is source-contract qualification only, not a runtime visual-quality measurement.

## Verification state

Fresh evidence for this twelfth-review repair:
- remote branch bytes were re-read after the verifier/test writes and the new exact-format checks and `0.95` threshold are present;
- the six new regression definitions are present on the branch;
- the canonical-source tangent threshold calculation described above was independently recomputed in the sandbox;
- the previous exact head `3c6b56a4bef3d0152598e40bc18c7b5a5464e456` had passed hosted Windows workflow `35859627983` before these twelfth-review edits.

A fresh complete 42/42 focused regression execution, Python compilation of the edited scripts, fresh exact-head hosted CI, and fresh independent exact-head review are still required. The current hosted workflow does not itself substitute for `Scripts/test_material_gallery_gltf.py`, so no 42/42 claim is made in this pass until that suite is actually executed.

The generator, canonical source, expected manifest and generated artifact bytes are unchanged:
- canonical `gallery-spec.json`: `517833a990db74f97d8046aa7fafa2d2d73859538d59c3c41ff4a8a7fb63f530`;
- generated `material_gallery.gltf`: `d0bca093cfc52b59816a14ff98cc90b8684e7e7f664d0b687b394be5aa166af1`;
- generated size: 38,604 bytes;
- expected/generated manifest: `ba94405d30eaf4cd7f259b59f9c01e77fbe672527c2a1747061cd3f912c5c3b2`.

The generated glTF remains intentionally untracked; `expected-manifest.json` pins the exact derived output.

## Publication verification

This repair remains within ART-008 ownership. Implementation changes are limited to the verifier, regression suite, task/QA and art-hourly continuation records. Source spec, generator and expected manifest are unchanged. Draft PR #33 remains the integration surface. No engine, renderer, gameplay, CMake, workflow or dependency path is changed.

Fresh exact-head hosted Windows CI and an independent exact-head review are required after the final documentation head is established. Passing source checks does not clear those separate gates.

## Evidence boundaries

Not run: Blender export round-trip, Astral import, Astral rendering, workstation Windows/GPU native capture, performance/memory measurement, or independent visual art review.

Therefore this pass establishes only a repaired source contract and targeted source-analysis evidence. It does not establish `imported`, `runtime_verified`, `art_approved`, or parity with Unreal, Unity, Genshin Impact, Zenless Zone Zero, or professional material-review workflows.
