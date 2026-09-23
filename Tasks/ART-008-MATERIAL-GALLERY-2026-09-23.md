# ART-008-MATERIAL-GALLERY-2026-09-23

Loop: `astral-art-hourly-20260922`

Objective: create and independently validate one original neutral material-response
gallery source fixture that can become the first repeatable Astral material-review
scene after the engine worker provides a solid-mesh/material/light import path.

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

Forbidden: `Engine/`, renderer, editor, gameplay, CMake, workflow, dependencies,
software installs, local Company Runtime execution, R0, releases, deployment,
paid services, force-pushes, or merging another worker's changes.

Source decisions:
- glTF 2.0.1 source scene, metres, +Y up / +Z forward / -X right;
- four matched material stations, each shown on the same sphere and cube;
- one neutral floor, fixed 16:9 perspective camera at 50 degrees vertical FOV;
- white `KHR_lights_punctual` directional key/fill at 1000/250 lux;
- no image textures and no baked lighting in the gallery fixture;
- source values remain reference-only, with exposure/tonemapping runtime-owned.

Acceptance:
- deterministic generation plus exact `--check`;
- independent standard-library verifier;
- canonical source status is `proposed_art_reference_not_runtime`;
- generated runtime status stays `source_validated_not_imported`;
- finite triangle geometry with positions, normals, tangents, UVs and bounded indices;
- valid outward winding, normalized normals/tangents, and stable sphere/cube counts;
- exact station/material/node ownership and camera/light contract;
- no images/textures/samplers, preventing accidental baked-lighting review;
- pinned source hash and generated glTF hash in `expected-manifest.json`;
- negative regressions for light, material, camera, source/runtime status, texture
  insertion, accessor bounds, expected-manifest pinning and CRLF portability;
- no claim of Astral import, runtime rendering, native GPU evidence or art approval.

Commands:

```text
python Scripts/generate_material_gallery_gltf.py --source Content/Calibration/MaterialGallery/gallery-spec.json --output <new-dir>
python Scripts/generate_material_gallery_gltf.py --source Content/Calibration/MaterialGallery/gallery-spec.json --output <same-dir> --check
python Scripts/verify_material_gallery_gltf.py <same-dir>/material_gallery.gltf --source Content/Calibration/MaterialGallery/gallery-spec.json --manifest <same-dir>/manifest.json --expected-manifest Content/Calibration/MaterialGallery/expected-manifest.json
python Scripts/test_material_gallery_gltf.py
python -m py_compile Scripts/generate_material_gallery_gltf.py Scripts/verify_material_gallery_gltf.py Scripts/test_material_gallery_gltf.py
```
