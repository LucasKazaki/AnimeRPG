# ART-008 neutral material gallery author QA

Date: 2026-09-23  
Loop: `astral-art-hourly-20260922`  
Status: source-validated candidate under exact-head re-review, not imported or runtime-approved.

## Delivered

ART-008 provides an original machine-readable neutral material-gallery spec, deterministic glTF generation, and an independent standard-library verifier. The derived source scene contains four matched sphere/cube stations, five PBR materials including the floor, one fixed camera, two white directional lights, and no image textures. It is an art-side source fixture, not an Astral runtime screenshot or renderer feature.

Thirteen earlier exact-head review rounds repaired tangent handedness and frame validation, camera/light rotations and transform closure, material/evidence contracts, per-triangle normals, matched geometry and dimensions, morph/animation overrides, manifest boundaries, POSITION metadata, canonical topology, semantic accessor formats, meaningful non-floor tangent alignment, complete canonical payload comparison, and deterministic metadata-keyset closure.

The fourteenth independent review at `5baec92ac55329f4f462386c6fb113d2eb15b452` found three additional P2 gaps:

1. Accessor references could use JSON `true` because Python's `bool` is a subclass of `int`, allowing an invalid glTF index to compare equal to accessor 1.
2. Index accessor `min` / `max` metadata was required to exist but was not compared with the decoded index payload, so a repinned fixture could declare contradictory bounds.
3. A repinned fixture could prepend two bytes to the embedded buffer and shift every bufferView by two bytes. The decoded payloads remained canonical, but FLOAT vertex accessors were misaligned for glTF import.

This repair closes all three paths. `exact_index` now requires `type(value) is int` plus an in-range value for indexed accessor and bufferView references, while other fixed references use explicit integer-type checks. Index accessors now require one-component integer bounds exactly equal to the decoded minimum and maximum. Accessor decoding and contract checks now enforce component-size alignment, and vertex-attribute bufferViews require 4-byte alignment.

The generator was also corrected to emit truthful index bounds as `min(idx)` and `max(idx)` instead of assuming every indexed mesh references vertices `0..len(pos)-1`. This exposed a stale pin: the canonical 12x24 sphere has 325 stored vertices but its pole-trimmed triangle list references indices 1 through 323, while the old generated accessor declared 0 through 324. The corrected generator therefore changes only the declared sphere index bounds, not the geometry payload, file size, scene composition, or art intent.

Three focused negative regressions cover boolean NORMAL accessor references, false index bounds, and a two-byte global bufferView misalignment. The focused suite definition is now 47 tests.

## Source verification

Fresh hosted evidence for code head `00f3d3e4fc5f946841174a324a6fbfa186787faf`:
- GitHub-hosted Windows workflow `35881340721` passed;
- checkout, Debug and Release builds, deterministic Debug/Release tests, runtime/dependency policy checks, static milestone verifiers, and final clean-tree verification all passed;
- hosted CI does not invoke `Scripts/test_material_gallery_gltf.py`, so this receipt is not represented as a 47/47 focused ART-008 run.

Focused-suite evidence boundary for this repair:
- 47 focused regressions are defined on the candidate branch;
- the preceding 44-test head passed all 44 focused regressions in two bounded sandbox batches;
- a fresh 47-test execution is not claimed in this pass because the available sandbox could not resolve GitHub for a clean exact-head checkout;
- the three new regressions remain subject to focused execution and fresh independent source review on the final documentation head.

Pin reconciliation was checked with an independent local deterministic reconstruction of the published standard-library generator. As a sanity check on the reconstruction, using the old assumed index bounds reproduced the previously pinned 38,604-byte glTF SHA-256 exactly: `d0bca093cfc52b59816a14ff98cc90b8684e7e7f664d0b687b394be5aa166af1`. Applying the current generator's `min(idx)` / `max(idx)` behavior keeps the glTF at 38,604 bytes but produces corrected SHA-256 `e229624b789733eabc0955a61fc769e4b2a25dbb40a99ed8e8b83773fb080b43`. The repinned expected manifest is 646 bytes with SHA-256 `50fa14e850e20deacbfc7f1a04f42ee2a4c13c1a1d7b9af36f5d1ad833632d7e`. Canonical source SHA-256 remains `517833a990db74f97d8046aa7fafa2d2d73859538d59c3c41ff4a8a7fb63f530`.

This reconstruction validates the deterministic pin transition, not the repository test runner. A clean focused execution is still required before claiming 47/47.

## Publication and evidence boundaries

This repair remains within ART-008 ownership. No `Engine/`, renderer, editor, gameplay, CMake, workflow, dependency, deployment, release, or local Company Runtime path is changed. Draft PR #33 remains the integration surface and must stay unmerged under this task.

Not run: Blender round-trip, Astral import, Astral rendering, native workstation GPU capture, frame-time/RAM/VRAM measurement, or independent visual-art approval.

Therefore the candidate remains `proposed_art_reference_not_runtime` / `source_validated_not_imported`. It does not establish `imported`, `runtime_verified`, `art_approved`, or parity with Unreal Engine, Unity, Genshin Impact, Zenless Zone Zero, or professional production-art workflows.