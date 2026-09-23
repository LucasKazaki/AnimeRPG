# ART-008 neutral material gallery author QA

Date: 2026-09-23  
Loop: `astral-art-hourly-20260922`  
Status: source-validated candidate, not imported or runtime-approved.

## Delivered

ART-008 adds a machine-readable neutral gallery spec plus deterministic glTF generation and an independent verifier. The derived scene contains four matched sphere/cube stations, five PBR materials including the floor, one fixed camera, two white directional lights, and no image textures. This is an art-side source fixture, not a renderer feature or an Astral screenshot.

Earlier exact-head reviews repaired floor tangent handedness, source-derived camera/light rotations, full floor tangent-frame validation, transform overrides that could reverse camera/light local -Z, uncontracted material rendering properties, and incomplete per-triangle normal validation.

The latest exact-head review at `aad40d621c35ca20049d06260e78702fb2717603` found two additional P2 acceptance gaps:

1. a material station could be redirected to a separate same-count geometry accessor and repinned, so the four stations no longer had to use one canonical sphere/cube geometry source and source dimensions such as `sphere_radius` were not enforced;
2. `extras.astral_contract` could gain unsupported evidence claims such as `runtime_verified` or `art_approved` while retaining the approved status and still pass after repinning.

This pass closes both gaps. Sphere stations must share one canonical geometry accessor binding, cube stations must share one canonical binding, and the decoded sphere radius, cube half extent, and floor half extents are checked against the source specification. The glTF `extras` object is also closed to exactly `astral_contract`, and that contract must contain exactly the approved source-only fields. Supplemental runtime/art/parity claims are rejected independently of the manifest hash.

Four new negative regressions cover split station geometry binding, source sphere radius drift, source cube extent drift, and a repinned `art_approved` evidence claim. The focused suite is now 23 tests.

## Source research

The contract remains based on the Khronos glTF 2.0.1 rules already recorded for this task. Mesh/accessor indirection is valid glTF, so matched-review geometry must be an Astral fixture rule rather than assumed from the format. Likewise, glTF `extras` is extensible by design, so unsupported runtime or approval claims must be blocked by this fixture's evidence contract rather than inferred from generic glTF validity.

Primary references retained for this task:
- https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
- https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual
- https://docs.blender.org/manual/en/5.2/addons/scene_gltf2.html

## Verification state

Exact-head hosted verification is required after the verifier/test and evidence commits. The generator, canonical source, and pinned generated artifact are unchanged by this repair:

- canonical `gallery-spec.json`: `517833a990db74f97d8046aa7fafa2d2d73859538d59c3c41ff4a8a7fb63f530`
- generated `material_gallery.gltf`: `d0bca093cfc52b59816a14ff98cc90b8684e7e7f664d0b687b394be5aa166af1`
- generated size: 38,604 bytes
- expected/generated manifest: `ba94405d30eaf4cd7f259b59f9c01e77fbe672527c2a1747061cd3f912c5c3b2`

The generated glTF remains intentionally untracked; `expected-manifest.json` pins the exact derived output.

## Publication verification

This repair remains within ART-008 ownership. The implementation changes are limited to the verifier and regression suite; source spec, generator, and expected manifest are unchanged. Draft PR #33 remains the integration surface. Exact-head hosted CI and a new exact-head independent source review are separate gates before marking the PR ready for review.

## Evidence boundaries

Not run: Blender export round-trip, Astral import, Astral rendering, Windows/GPU native capture outside hosted CI, performance/memory measurement, or independent visual art review.

Therefore this pass establishes only source-contract hardening. It does not establish `imported`, `runtime_verified`, `art_approved`, or parity with Unreal, Unity, Genshin Impact, Zenless Zone Zero, or professional material-review workflows.
