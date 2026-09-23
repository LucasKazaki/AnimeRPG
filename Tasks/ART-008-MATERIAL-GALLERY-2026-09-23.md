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
- generated runtime status stays `source_validated_not_imported`;
- canonical source `capture_intent` and generated manifest `intent` must equal their exact source-only sentences;
- generated manifest is a closed schema-version-1 contract, and schema version must be a JSON integer exactly equal to 1;
- exact generator-owned glTF profile for required semantics: POSITION FLOAT VEC3, NORMAL FLOAT VEC3, TANGENT FLOAT VEC4, TEXCOORD_0 FLOAT VEC2, and indices UNSIGNED_SHORT SCALAR;
- deterministic accessor, bufferView, buffer, camera, perspective, extension, mesh, primitive, material and scene object key sets are closed to the generator-owned profile, so optional text or metadata fields cannot carry unsupported runtime/import/art/parity claims;
- all required accessors and bufferViews are referenced by the calibration geometry, with no unused metadata containers admitted;
- each POSITION accessor declares finite three-component `min` and `max` values matching decoded payload;
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
- negative regressions cover material/light/camera contracts, statuses and evidence claims, nested `extras`, nested accessor `name` evidence claims, manifest schema/intent, texture insertion, bounds and semantic formats, topology, full canonical cube payload, geometry dimensions/sharing, normals/tangent frames, morphs/animation and transform overrides;
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

The thirteenth independent review of PR #33 found two P2 gaps at head `49ef9d92057a577f55095f31d7d0f201c70bcb0d`: a cube could collapse to six copies of one face while preserving counts/extents/topology, and optional nested `name` metadata could carry unsupported evidence claims. The repair compares complete decoded sphere/cube/floor payloads with generator-owned geometry and closes deterministic object key sets plus referenced accessor/bufferView coverage. Two focused negatives raise the suite definition from 42 to 44 tests.
