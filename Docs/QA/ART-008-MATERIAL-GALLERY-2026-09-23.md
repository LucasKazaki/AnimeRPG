# ART-008 neutral material gallery author QA

Date: 2026-09-23  
Loop: `astral-art-hourly-20260922`  
Status: source-validated candidate, not imported or runtime-approved.

## Delivered

ART-008 adds a machine-readable neutral gallery spec plus deterministic glTF
generation and an independent verifier. The derived scene contains four matched
sphere/cube stations, five PBR materials including the floor, one fixed camera,
two white directional lights, and no image textures. This is an art-side source
fixture, not a renderer feature or an Astral screenshot.

The first two independent-review rounds corrected floor tangent handedness,
source-derived camera/light rotations, complete floor tangent-frame validation,
and transform overrides that could reverse camera/light local -Z.

A third exact-head review at `0ebee02d7f81300d19bfa2ebdf6275e0a4b7707d`
found two further P2 acceptance gaps:

1. a material could add an uncontracted core rendering property such as
   `emissiveFactor` and still pass after repinning;
2. mesh validation compared each triangle face only with the first indexed vertex
   normal, so later triangle vertices could carry inverted normals and still pass.

This pass closes both gaps. Each material must now contain exactly `name` and
`pbrMetallicRoughness`; the nested PBR object was already required to match the
source factors exactly. Mesh validation now checks the geometric triangle normal
against all three indexed vertex normals, while retaining the existing unit-normal,
index-range, and tangent checks.

Two new negative regressions add an emissive factor to a repinned material and
invert cube-face normals on vertices 1-3 while leaving vertex 0 valid. The focused
suite is now 19 tests.

## Source research

The contract remains based on the Khronos glTF 2.0.1 rules already recorded for
this task. Core material properties such as emissive contribution affect rendered
material response, so source-only neutral-review materials must not silently gain
properties outside the approved contract. Mesh normals remain explicit vertex
attributes consumed by lighting and therefore must agree with the authored face
orientation for every indexed triangle vertex.

Primary references rechecked 2026-09-23:
- https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
- https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual
- https://docs.blender.org/manual/en/5.2/addons/scene_gltf2.html

## Verification state

The prior repaired candidate reproduced the pinned source artifact in an isolated
sandbox and passed fresh generation, exact regeneration, independent validation,
17/17 regressions, and Python compilation. That earlier evidence remains valid for
the unchanged generator/source artifact, but it does not prove the two new verifier
and regression edits.

Current exact-head verification must therefore execute the normal hosted workflow
before this repair can be considered complete. The source generator and pinned
artifact are unchanged by this pass:

- canonical `gallery-spec.json`: `517833a990db74f97d8046aa7fafa2d2d73859538d59c3c41ff4a8a7fb63f530`
- generated `material_gallery.gltf`: `d0bca093cfc52b59816a14ff98cc90b8684e7e7f664d0b687b394be5aa166af1`
- generated size: 38,604 bytes
- expected/generated manifest: `ba94405d30eaf4cd7f259b59f9c01e77fbe672527c2a1747061cd3f912c5c3b2`

The generated glTF remains intentionally untracked; `expected-manifest.json` pins
the exact derived output.

## Publication verification

The third-review repair remains within ART-008 ownership. The only implementation
paths changed are the verifier and its regression suite; the source spec,
deterministic generator, and expected manifest are unchanged. Draft PR #33 remains
the integration surface. A fresh exact-head hosted run and a fresh independent
review are separate gates after the final evidence/state commit.

## Evidence boundaries

Not run: Blender export round-trip, Astral import, Astral rendering, Windows/GPU
native capture outside hosted CI, performance/memory measurement, or independent
visual art review.

Therefore this pass establishes only source-contract hardening. It does not
establish `imported`, `runtime_verified`, `art_approved`, or parity with Unreal,
Unity, Genshin Impact, Zenless Zone Zero, or professional material-review workflows.
