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
- negative regressions cover material/light/camera contracts, statuses and evidence claims, nested `extras`, nested accessor `name` evidence claims, manifest schema/intent, texture insertion, bounds and semantic formats, canonical index bounds, boolean accessor references, accessor alignment, topology, full canonical cube payload, geometry dimensions/sharing, normals/tangent frames, morphs/animation and transform overrides;
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

The fourteenth independent review of PR #33 at `5baec92ac55329f4f462386c6fb113d2eb15b452` found three P2 gaps: JSON booleans could satisfy accessor references because Python treats `True` as an integer; index accessors only required `min`/`max` keys rather than truthful payload bounds; and a repinned embedded buffer could shift every bufferView by two bytes while preserving decoded payloads but violating glTF alignment rules.

The current repair introduces exact integer index/reference validation, verifies decoded index minima/maxima against declared accessor bounds, enforces component-size and 4-byte vertex-attribute alignment before decoding, emits truthful generator index bounds with `min(idx)` / `max(idx)`, and adds three dedicated negative regressions. The focused suite definition is now 47 tests. Exact-head hosted Windows workflow `35881340721` passed on code head `00f3d3e4fc5f946841174a324a6fbfa186787faf`. A fresh 47-test focused source-suite execution is not claimed in this pass because the available sandbox could not resolve GitHub for a clean checkout; hosted CI does not substitute for that suite or for independent re-review.

The generator correction exposed and repaired a stale pin. The canonical 12x24 sphere stores vertices 0 through 324, but its pole-trimmed triangle list actually references indices 1 through 323. The old accessor bounds `[0,324]` were therefore false metadata. `expected-manifest.json` now pins the corrected deterministic 38,604-byte glTF SHA-256 `e229624b789733eabc0955a61fc769e4b2a25dbb40a99ed8e8b83773fb080b43`; the repinned manifest SHA-256 is `50fa14e850e20deacbfc7f1a04f42ee2a4c13c1a1d7b9af36f5d1ad833632d7e`. These hashes were derived by an independent deterministic reconstruction that reproduced the historical glTF hash exactly before applying the bounds correction. The repository's 47-test runner still requires fresh execution.