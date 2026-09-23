# ART-008 neutral material gallery author QA

Date: 2026-09-23  
Loop: `astral-art-hourly-20260922`  
Status: source-validated candidate, not imported or runtime-approved.

## Delivered

ART-008 adds a machine-readable neutral gallery spec plus deterministic glTF generation and an independent verifier. The derived scene contains four matched sphere/cube stations, five PBR materials including the floor, one fixed camera, two white directional lights, and no image textures. This is an art-side source fixture, not a renderer feature or an Astral screenshot.

Earlier exact-head reviews repaired floor tangent handedness, source-derived camera/light rotations, full floor tangent-frame validation, transform overrides that could reverse camera/light local -Z, uncontracted material rendering properties, incomplete per-triangle normal validation, divergent per-station geometry, source-dimension drift, unsupported glTF evidence claims, mesh/primitive morph overrides, animation transform overrides, supplemental manifest evidence fields, false manifest intent, boolean schema-version acceptance, non-orthogonal tangent frames, and false POSITION accessor bounds.

The tenth exact-head review at `604470e9d971f64b131610dfad75e0735efc3a43` found two additional P2 contract gaps. First, POSITION semantics were not explicitly constrained to the glTF-required FLOAT `VEC3` accessor format before decoding, so a repinned non-VEC3 accessor could be interpreted through the generic accessor reader. Second, the source `capture_intent` could be rewritten to carry unsupported import/runtime/art/parity claims while still matching the generated `extras.astral_contract` field.

This pass closes both paths. Every POSITION semantic is now checked for `componentType == 5126` and `type == VEC3` before payload decoding. The verifier also requires the canonical source `capture_intent` sentence exactly, and the generated contract must carry that same canonical sentence. Two focused regressions cover a sphere POSITION accessor changed to `VEC4` and a source capture intent rewritten to claim Astral import, runtime verification, art approval and parity. The focused suite definition is now 33 tests.

## Source research

The contract remains based on Khronos glTF 2.0.1. Vertex POSITION attributes use FLOAT `VEC3` accessors, and POSITION accessor `min` / `max` metadata must describe the decoded vertex extents. Tangent `xyz` is a normalized tangent direction and `w` carries handedness for reconstructing the bitangent. Animations, morph targets, and mesh weights are valid core glTF features but remain outside this immutable calibration fixture.

Primary references retained for this task:
- https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
- https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual
- https://docs.blender.org/manual/en/5.2/addons/scene_gltf2.html

## Verification state

Previous ninth-repair sandbox evidence remains historical only:
- fresh generation: PASS;
- exact generator `--check`: PASS;
- independent verifier with generated manifest plus expected-manifest pin: PASS;
- Python compilation of generator, verifier and regression suite: PASS;
- all 31 then-defined regressions: PASS across bounded isolated batches.

For the current tenth-review repair, remote source inspection confirms the verifier checks the new POSITION format and canonical capture-intent gates before the affected downstream checks, and the regression list contains both new negative cases. A fresh exact-head hosted Windows CI run and a fresh independent Codex review are required after the final branch head is established. The repository workflow does not by itself establish Astral runtime or visual-art acceptance, and this record does not claim a fresh 33/33 local execution receipt.

The generator, canonical source, expected manifest and generated artifact bytes are unchanged:
- canonical `gallery-spec.json`: `517833a990db74f97d8046aa7fafa2d2d73859538d59c3c41ff4a8a7fb63f530`;
- generated `material_gallery.gltf`: `d0bca093cfc52b59816a14ff98cc90b8684e7e7f664d0b687b394be5aa166af1`;
- generated size: 38,604 bytes;
- expected/generated manifest: `ba94405d30eaf4cd7f259b59f9c01e77fbe672527c2a1747061cd3f912c5c3b2`.

The generated glTF remains intentionally untracked; `expected-manifest.json` pins the exact derived output.

## Publication verification

This repair remains within ART-008 ownership. Implementation changes are limited to the verifier, regression suite, and art-owned records. Source spec, generator and expected manifest are unchanged. Draft PR #33 remains the integration surface. No engine, renderer, gameplay, CMake, workflow or dependency path is changed.

## Evidence boundaries

Not run: Blender export round-trip, Astral import, Astral rendering, workstation Windows/GPU native capture, performance/memory measurement, or independent visual art review.

Therefore this pass establishes only source-contract hardening. It does not establish `imported`, `runtime_verified`, `art_approved`, or parity with Unreal, Unity, Genshin Impact, Zenless Zone Zero, or professional material-review workflows.
