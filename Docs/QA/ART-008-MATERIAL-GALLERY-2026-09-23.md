# ART-008 neutral material gallery author QA

Date: 2026-09-23  
Loop: `astral-art-hourly-20260922`  
Status: source-validated candidate, not imported or runtime-approved.

## Delivered

ART-008 adds a machine-readable neutral gallery spec plus deterministic glTF generation and an independent verifier. The derived scene contains four matched sphere/cube stations, five PBR materials including the floor, one fixed camera, two white directional lights, and no image textures. This is an art-side source fixture, not a renderer feature or an Astral screenshot.

Earlier exact-head reviews repaired floor tangent handedness, source-derived camera/light rotations, full floor tangent-frame validation, transform overrides that could reverse camera/light local -Z, uncontracted material rendering properties, incomplete per-triangle normal validation, divergent per-station geometry, source-dimension drift, unsupported glTF evidence claims, mesh/primitive morph overrides, animation transform overrides, and supplemental manifest evidence fields.

The eighth exact-head review at `67e4e286345531eb4dee10475b60a5f796bc55fd` found three further P2 contract gaps. First, the approved `intent` key was present but its value was not checked, so manifest-only verification could carry runtime/import/art/parity claims inside that string. Second, `schema_version: true` passed the numeric equality check because Python booleans compare equal to integers. Third, sphere/cube tangents were checked for unit length and handedness but not orthogonality to their paired normals, allowing a zero reconstructed bitangent.

This pass closes all three paths. The verifier now requires the exact source-only manifest intent, requires `schema_version` to have exact Python `int` type and value 1, and requires `abs(dot(normal, tangent.xyz)) < 1e-4` at every mesh vertex before any floor-specific derivative checks. Three dedicated negative regressions cover the false intent string, boolean schema version, and a cube tangent deliberately made parallel to its normal. The focused suite remains 30 tests.

A follow-up author self-audit caught a test-ordering issue introduced by the stronger intent check: the existing expected-manifest-pin negative test had changed `intent`, so it would now fail semantically before reaching the pin comparison. That would stop the suite from independently proving the byte-level pin gate. The regression now rewrites the same semantically valid manifest using different indentation only. Manifest semantics therefore pass, while `--expected-manifest` must still reject the byte drift specifically with `expected manifest pin`.

## Source research

The contract remains based on Khronos glTF 2.0.1. Tangent `xyz` is a normalized tangent direction while `w` carries handedness for reconstructing the bitangent, so an orthogonal normal/tangent pair is required for a non-degenerate tangent frame. Animations, morph targets, and mesh weights are valid core glTF features but remain outside this immutable calibration fixture.

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
