# ART-CALIBRATION-GLTF-2026-09-22

Loop: `astral-art-hourly-20260922`

Objective: create and validate one original asymmetric glTF 2.0 fixture that gives the engine worker an exact triangle/UV/normal/tangent/material/axis contract without changing renderer/importer code.

Base dependency: PR #23 head `0bda198c20b33dfaf3fa7cce129e61be0b556272`. This task is stacked on that art branch so it can update the existing art-hourly state without duplicating it.

Allowed paths:
- `Content/Calibration/AsymmetricSurface/`
- `Docs/Art/ASSET-CALIBRATION-SPEC-v0.1.md`
- `Docs/QA/ART-CALIBRATION-GLTF-2026-09-22.md`
- `Docs/Agents/art-hourly/BACKLOG.json`
- `Docs/Agents/art-hourly/STATE.json`
- `Docs/Agents/art-hourly/TOOLCHAIN.md`
- `Scripts/generate_asymmetric_calibration_gltf.py`
- `Scripts/verify_asymmetric_calibration_gltf.py`
- `Scripts/test_asymmetric_calibration_gltf.py`
- this task

Forbidden: Engine/Renderer/Platform/editor/gameplay/CMake/workflow edits, dependencies, installs, R0, local-runtime changes, releases, deployment, or merging another worker's changes.

Acceptance:
- deterministic generator and exact `--check`;
- independent standard-library verifier;
- generated source contains triangles, POSITION/NORMAL/TANGENT/TEXCOORD_0, unsigned-short indices, two materials, two nodes, and embedded orientation PNG;
- asymmetric marker exposes mirror/handedness mistakes;
- negative regressions reject coordinate-contract/material/sampler/manifest changes;
- all source claims remain explicit that Astral cannot import/render the asset yet.

Commands:
```text
python Scripts/generate_asymmetric_calibration_gltf.py --output <new-dir>
python Scripts/generate_asymmetric_calibration_gltf.py --output <same-dir> --check
python Scripts/verify_asymmetric_calibration_gltf.py <same-dir>/asymmetric_surface.gltf --manifest <same-dir>/manifest.json
python Scripts/test_asymmetric_calibration_gltf.py
python -m py_compile Scripts/generate_asymmetric_calibration_gltf.py Scripts/verify_asymmetric_calibration_gltf.py Scripts/test_asymmetric_calibration_gltf.py
```
