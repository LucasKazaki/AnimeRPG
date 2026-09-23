# ART-008 neutral material gallery author QA

Date: 2026-09-23  
Loop: `astral-art-hourly-20260922`  
Status: source-validated candidate, not imported or runtime-approved.

## Delivered

ART-008 adds a machine-readable neutral gallery spec plus deterministic glTF generation and an independent verifier. The derived scene contains four matched sphere/cube stations, five PBR materials including the floor, one fixed camera, two white directional lights, and no image textures. This is an art-side source fixture, not a renderer feature or an Astral screenshot.

Earlier exact-head reviews repaired floor tangent handedness, source-derived camera/light rotations, full floor tangent-frame validation, transform overrides that could reverse camera/light local -Z, uncontracted material rendering properties, incomplete per-triangle normal validation, divergent per-station geometry, source-dimension drift, and unsupported evidence claims.

The fifth exact-head review at `b14511647d72b789a9523fa3cd49667ecf8d0215` found one further P2 acceptance gap: valid glTF morph targets plus non-zero mesh weights could alter one station after the verifier checked only the base accessors, breaking the matched-geometry review while still passing after manifest repinning.

This pass closes that path by making each gallery mesh and primitive a closed contract. Meshes may contain only `name` and `primitives`; primitives may contain only `attributes`, `indices`, `material`, and `mode`. That rejects `weights`, morph `targets`, and other unapproved mesh/primitive features before geometry comparison. Two new negative regressions cover the exact weighted-morph bypass and an unweighted primitive morph target. The focused suite is now 25 tests.

## Source research

The contract remains based on Khronos glTF 2.0.1. Morph targets and mesh weights are valid core glTF features, but they are outside this calibration fixture because the review requires every station to render the same immutable source geometry. Closing those optional fields is therefore an Astral fixture rule, not a claim that glTF itself forbids morphing.

Primary references retained for this task:
- https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
- https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual
- https://docs.blender.org/manual/en/5.2/addons/scene_gltf2.html

## Verification state

Fresh exact-head hosted CI and exact-head independent review are required after this repair. The generator, canonical source, and pinned generated artifact are unchanged:

- canonical `gallery-spec.json`: `517833a990db74f97d8046aa7fafa2d2d73859538d59c3c41ff4a8a7fb63f530`
- generated `material_gallery.gltf`: `d0bca093cfc52b59816a14ff98cc90b8684e7e7f664d0b687b394be5aa166af1`
- generated size: 38,604 bytes
- expected/generated manifest: `ba94405d30eaf4cd7f259b59f9c01e77fbe672527c2a1747061cd3f912c5c3b2`

The generated glTF remains intentionally untracked; `expected-manifest.json` pins the exact derived output.

## Publication verification

This repair remains within ART-008 ownership. Implementation changes are limited to the verifier and regression suite; source spec, generator, and expected manifest are unchanged. Draft PR #33 remains the integration surface. Exact-head hosted CI and a new exact-head independent source review are separate gates before marking the PR ready for review.

## Evidence boundaries

Not run: Blender export round-trip, Astral import, Astral rendering, Windows/GPU native capture outside hosted CI, performance/memory measurement, or independent visual art review.

Therefore this pass establishes only source-contract hardening. It does not establish `imported`, `runtime_verified`, `art_approved`, or parity with Unreal, Unity, Genshin Impact, Zenless Zone Zero, or professional material-review workflows.
