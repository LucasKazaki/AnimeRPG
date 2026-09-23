# ART-008-MATERIAL-GALLERY-2026-09-23

Loop: `astral-art-hourly-20260922`

Objective: create and independently validate one original neutral material-response gallery source fixture that can become the first repeatable Astral material-review scene after the engine worker provides a solid-mesh/material/light import path.

Base dependency: PR #28 exact head `087c673f716800198272915777681c61a748a490`.
This task is stacked on that art branch and does not merge or modify engine/game work.

Allowed paths:
- `Content/Calibration/MaterialGallery/`
- `Docs/QA/ART-008-MATERIAL-GALLERY-2026-09-23.md`
- `Docs/Agents/art-hourly/BACKLOG.json`
- `Docs/Agents/art-hourly/STATE.json`
- `Docs/Agents/art-hourly/TOOLCHAIN.md`
- `Scripts/generate_material_gallery_gltf.py`
- `Scripts/verify_material_gallery_gltf.py`
- `Scripts/test_material_gallery_gltf.py`
- this task

Forbidden: `Engine/`, renderer, editor, gameplay, CMake, workflow, dependencies, software installs, local Company Runtime execution, R0, releases, deployment, paid services, force-pushes, or merging another worker's changes.

## Source decisions

- glTF 2.0.1 source scene, metres, +Y up / +Z forward / -X right;
- four matched material stations, each shown on the same immutable sphere and cube geometry;
- one neutral floor, fixed 16:9 perspective camera at 50 degrees vertical FOV;
- white `KHR_lights_punctual` directional key/fill at 1000/250 lux;
- exact source-only capture intent: `Neutral material-response comparison. Absolute exposure and tonemapping are runtime-owned and are not encoded by glTF.`;
- no image textures and no baked lighting in the gallery fixture;
- source values remain reference-only, with exposure/tonemapping runtime-owned.

## Acceptance

- deterministic generation plus exact `--check`;
- independent standard-library verifier;
- canonical source status is `proposed_art_reference_not_runtime`;
- source `schema_version` must be a genuine JSON integer exactly equal to 1; JSON booleans and floating-point lookalikes are rejected independently by generator and verifier;
- generated runtime status stays `source_validated_not_imported`;
- canonical source `capture_intent` and generated manifest `intent` must equal their exact source-only sentences;
- generated manifest is a closed schema-version-1 contract, and schema version must be a JSON integer exactly equal to 1;
- all glTF index/reference fields used by the fixture must be real JSON integers, not booleans that Python could otherwise treat as integers;
- exact generator-owned glTF profile for required semantics: POSITION FLOAT VEC3, NORMAL FLOAT VEC3, TANGENT FLOAT VEC4, TEXCOORD_0 FLOAT VEC2, and indices UNSIGNED_SHORT SCALAR;
- deterministic accessor, bufferView, buffer, camera, perspective, extension, mesh, primitive, material and scene object key sets are closed to the generator-owned profile, so optional text or metadata fields cannot carry unsupported runtime/import/art/parity claims;
- all required accessors and bufferViews are referenced by the calibration geometry, with no unused metadata containers admitted;
- each POSITION accessor declares finite three-component `min` and `max` values matching decoded payload;
- each index accessor declares one-component integer `min` and `max` values matching the decoded index payload;
- bufferView/accessor byte offsets satisfy component-size alignment and the glTF 4-byte vertex-attribute alignment requirement;
- finite triangle geometry, bounded indices, exact generator-owned index topology, outward winding and consistent indexed vertex normals;
- every vertex tangent is normalized, normal-orthogonal, and its tangent plus reconstructed bitangent agrees with position/UV derivatives; non-floor alignment must exceed 0.95 and floor alignment 0.9999;
- all sphere stations share one canonical sphere binding and all cube stations share one canonical cube binding;
- decoded POSITION/NORMAL/TANGENT/TEXCOORD_0/index payloads for sphere, cube and floor must match the generator-owned procedural geometry within `1e-6` for floating payloads, preventing repeated/relocated faces that preserve counts, extents and topology;
- decoded sphere radius, cube half extent and floor extents match the source specification;
- exact generator-owned mesh names and station/material/node ownership;
- gallery meshes contain only `name` and `primitives`, and primitives only `attributes`, `indices`, `material`, and `mode`;
- top-level glTF animations are rejected;
- neutral-review materials permit only approved `name` and exact `pbrMetallicRoughness` fields;
- root `extras` contains only `astral_contract`, that contract contains only approved source-only evidence fields, and nested glTF `extras` are rejected;
- camera and directional-light nodes reject scale, matrix or other transform overrides and use source-derived rotations;
- no images/textures/samplers;
- pinned source hash and generated glTF hash in `expected-manifest.json`;
- negative regressions cover source/manifest schema typing, material/light/camera contracts, statuses and evidence claims, nested `extras`, nested accessor `name` evidence claims, manifest intent, texture insertion, bounds and semantic formats, canonical index bounds, boolean accessor references, accessor alignment, topology, full canonical cube payload, geometry dimensions/sharing, normals/tangent frames, morphs/animation and transform overrides;
- no claim of Astral import, runtime rendering, native GPU evidence or art approval.

## Verification commands

```text
python Scripts/generate_material_gallery_gltf.py --source Content/Calibration/MaterialGallery/gallery-spec.json --output <new-dir>
python Scripts/generate_material_gallery_gltf.py --source Content/Calibration/MaterialGallery/gallery-spec.json --output <same-dir> --check
python Scripts/verify_material_gallery_gltf.py <same-dir>/material_gallery.gltf --source Content/Calibration/MaterialGallery/gallery-spec.json --manifest <same-dir>/manifest.json --expected-manifest Content/Calibration/MaterialGallery/expected-manifest.json
python Scripts/test_material_gallery_gltf.py
python -m py_compile Scripts/generate_material_gallery_gltf.py Scripts/verify_material_gallery_gltf.py Scripts/test_material_gallery_gltf.py
```

## Current bounded repair

The fourteenth repaired candidate reached exact head `7e750d2b16371d5fe6d3edd62fd7731b2d700da1`. GitHub-hosted Windows workflow `35882948488` passed on that exact head, and the requested Codex review completed with no new inline finding observed plus a Codex bot `+1` reaction. The hosted workflow still does not execute `Scripts/test_material_gallery_gltf.py`.

A subsequent bounded source-contract audit found one additional fail-closed gap: both `load_source` implementations compared `schema_version == 1` without checking the JSON value's actual type. In Python, `True == 1`, so a source document using `"schema_version": true` could pass the source-schema gate even though the manifest gate already rejected the same type confusion.

The current repair makes both the generator and independent verifier require `type(schema_version) is int` and value exactly `1`. It adds `test_source_schema_bool_semantics`, which exercises both entry points against a boolean schema value. The focused suite definition is now 48 tests. An isolated executable predicate check confirmed parsed JSON `true` and numeric `1.0` are rejected while integer `1` is accepted. This isolated check is not a substitute for running the repository's complete 48-test suite.

The gallery payload and pins are unchanged by this schema-only repair. Canonical source SHA-256 remains `517833a990db74f97d8046aa7fafa2d2d73859538d59c3c41ff4a8a7fb63f530`; `expected-manifest.json` continues to pin the corrected deterministic 38,604-byte glTF SHA-256 `e229624b789733eabc0955a61fc769e4b2a25dbb40a99ed8e8b83773fb080b43`, with expected-manifest SHA-256 `50fa14e850e20deacbfc7f1a04f42ee2a4c13c1a1d7b9af36f5d1ad833632d7e`.

PR #33 remains draft and unmerged. A fresh exact-head hosted workflow, focused 48-test execution, and independent source re-review are still required before marking it ready for review. Astral import/render, Blender round-trip, native GPU/performance evidence and independent visual-art approval remain separate later gates.
