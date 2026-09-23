# ART-008 neutral material gallery author QA

Date: 2026-09-23  
Loop: `astral-art-hourly-20260922`  
Status: source-validated candidate under exact-head re-review, not imported or runtime-approved.

## Delivered

ART-008 provides an original machine-readable neutral material-gallery spec, deterministic glTF generation, and an independent standard-library verifier. The derived source scene contains four matched sphere/cube stations, five PBR materials including the floor, one fixed camera, two white directional lights, and no image textures. It is an art-side source fixture, not an Astral runtime screenshot or renderer feature.

Twelve earlier exact-head review rounds repaired tangent handedness and frame validation, camera/light rotations and transform closure, material/evidence contracts, per-triangle normals, matched geometry and dimensions, morph/animation overrides, manifest boundaries, POSITION metadata, canonical topology, semantic accessor formats, and meaningful non-floor tangent alignment.

The thirteenth independent review at `49ef9d92057a577f55095f31d7d0f201c70bcb0d` found two additional P2 gaps:

1. A repinned cube could copy the first face's POSITION/NORMAL/TANGENT payload over all six hard-edged faces, update POSITION bounds, and still pass because counts, absolute extents, index topology, winding and tangent-frame checks were individually valid.
2. A referenced accessor could carry optional glTF `name` text such as unsupported runtime/art approval claims because nested `extras` were closed but other optional metadata fields were not.

This pass closes both paths. The verifier now reconstructs generator-owned sphere, hard-edged cube and floor geometry and compares their complete decoded POSITION/NORMAL/TANGENT/TEXCOORD_0/index payloads with the canonical procedural result. Floating payloads use a `1e-6` comparison tolerance. This preserves the useful independent semantic checks while preventing a geometrically different fixture from passing only because dimensions and topology look plausible.

The deterministic source profile is also closed further. Root fields, the embedded buffer, referenced bufferViews, required accessors, camera/perspective objects, the punctual-light extension wrapper, mesh names, and existing mesh/primitive/material/node contracts must match the generator-owned field sets. All accessors and bufferViews must be referenced by the calibration geometry. As a result, optional `name`, `extras`, unused accessor, or unused bufferView metadata cannot be used to attach unsupported evidence claims to a repinned fixture.

Two focused negative regressions were added, bringing the suite from 42 to 44 definitions: one clones the first cube face across all six faces while keeping its POSITION bounds self-consistent, and one injects an unsupported evidence claim into a referenced NORMAL accessor's optional `name` field.

## Source verification

Fresh sandbox evidence for this repair:
- deterministic generation passed;
- exact generator `--check` passed;
- independent verification with generated manifest plus pinned expected manifest passed;
- Python compilation passed for generator, verifier and regression suite;
- all 44 focused regressions passed across two bounded isolated batches, tests 1-22 and tests 23-44. One uninterrupted 44-test process is not claimed;
- the exact locally tested verifier Git blob is `ad5b3e34052cb987321178b270df3fe0d517d251`, matching the GitHub write receipt;
- the exact locally tested regression-suite Git blob is `78d8471f36422b0e07b93e73fa68f6a37e1426b8`, matching the GitHub write receipt.

The canonical source, generator output and expected manifest are unchanged:
- canonical `gallery-spec.json`: SHA-256 `517833a990db74f97d8046aa7fafa2d2d73859538d59c3c41ff4a8a7fb63f530`;
- generated `material_gallery.gltf`: 38,604 bytes, SHA-256 `d0bca093cfc52b59816a14ff98cc90b8684e7e7f664d0b687b394be5aa166af1`;
- expected/generated manifest: SHA-256 `ba94405d30eaf4cd7f259b59f9c01e77fbe672527c2a1747061cd3f912c5c3b2`.

The previous exact head `49ef9d92057a577f55095f31d7d0f201c70bcb0d` passed GitHub-hosted Windows workflow `35865932556` before these thirteenth-review edits. A fresh hosted workflow and fresh independent review are still required on the final documentation head. Hosted CI remains a separate gate and does not substitute for the focused source suite.

## Publication and evidence boundaries

This repair remains within ART-008 ownership. No `Engine/`, renderer, editor, gameplay, CMake, workflow, dependency, deployment, release, or local Company Runtime path is changed. Draft PR #33 remains the integration surface and must stay unmerged under this task.

Not run: Blender round-trip, Astral import, Astral rendering, native workstation GPU capture, frame-time/RAM/VRAM measurement, or independent visual-art approval.

Therefore the candidate remains `proposed_art_reference_not_runtime` / `source_validated_not_imported`. It does not establish `imported`, `runtime_verified`, `art_approved`, or parity with Unreal Engine, Unity, Genshin Impact, Zenless Zone Zero, or professional production-art workflows.
