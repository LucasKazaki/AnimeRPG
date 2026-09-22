# ART calibration glTF author QA, 2026-09-22

Assessment: **source fixture and bounded verifier pass; runtime import/render not tested because current Astral has no glTF/solid-mesh texture path.**

Research basis checked this pass:
- Khronos glTF 2.0.1 registry/specification, current registry still identifies 2.0 as current and 2.0.1 as the specification patch level.
- Blender 5.2 manual glTF exporter documents Y-up conversion plus UV, normal, and tangent export.
- Current `main` at observation `e782f595696c046c2c394b636622f700a18d0425` exposes only `Engine/Assets/StaticMesh.cpp/.h`; no current glTF importer was found in `Engine/Assets`.

Executed in the isolated Python sandbox:

```text
python Scripts/generate_asymmetric_calibration_gltf.py --output <new-dir>
PASS: asymmetric glTF calibration fixture

python Scripts/generate_asymmetric_calibration_gltf.py --output <same-dir> --check
PASS: asymmetric glTF calibration fixture

python Scripts/verify_asymmetric_calibration_gltf.py <same-dir>/asymmetric_surface.gltf --manifest <same-dir>/manifest.json
PASS: {'block_vertices': 24, 'block_indices': 36, 'marker_vertices': 3}

python Scripts/test_asymmetric_calibration_gltf.py
6/6 passed

python -m py_compile ...
PASS
```

The host Python startup emitted an unrelated spreadsheet-runtime warmup timeout to stderr in this environment while these commands still returned exit 0 where listed. That warning is not fixture validation evidence and is not hidden.

Verifier checks include the declared +Y/+Z/-X coordinate contract, embedded buffer bounds, embedded PNG signature/dimensions, exact node/material ownership, required vertex attributes, finite values, unit normals/tangents, tangent W, UV range, index range, triangle winding, marker placement, sampler clamp mode, and manifest SHA/size.

Negative tests cover changed coordinate contract, swapped block material, changed wrap mode, manifest-breaking byte reformat, and generator overwrite refusal.

Limitations: no official Khronos validator executable was installed or run; no Blender export round trip was run; no Astral runtime, Windows GPU, native screenshot, material-v2 binding, collision, mip, compression, performance, or art-quality acceptance was run. These remain engine/local gates.
