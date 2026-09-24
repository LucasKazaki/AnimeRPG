# ART-006C QA: Khronos glTF validator evidence gate

## Evidence boundary

This packet qualifies and tests a report adapter for the official Khronos glTF-Validator. It does **not** claim that Khronos glTF-Validator, Blender, or Astral Engine executed against the ART-006B asset in this pass.

## Upstream evidence checked 2026-09-24

Primary sources:

- `KhronosGroup/glTF-Validator` README: validator purpose, glTF 2.0 target, JSON report output, CLI shape, stdout option, resource validation default, and error return-code behavior.
- `KhronosGroup/glTF-Validator/docs/validation.schema.json`: report fields and issue-count/message structure used by the adapter.
- upstream repository metadata: Apache-2.0 license; repository observed active, last pushed 2026-09-18 during this qualification.
- Blender 4.5 LTS manual was checked only to preserve the later DCC boundary: Blender's glTF importer/exporter supports meshes, materials, UVs, normals and tangents, but Blender was not installed or run in this sandbox.

## Sandbox verification

Executed against the authored ART-006C files before publication:

```text
python Scripts/test_khronos_gltf_validator_report.py
# PASS: 18/18 Khronos glTF report adapter tests

python -m py_compile \
  Scripts/verify_khronos_gltf_validator_report.py \
  Scripts/test_khronos_gltf_validator_report.py
# exit 0
```

The regression suite covers a valid report plus fail-closed behavior for boolean-as-integer counts, validator errors, warnings above the default limit, truncated output, wrong target URI, malformed validator semver, summary/message count mismatch, boolean severity, wrong glTF version, external resources, missing/boolean/unknown resource storage, unknown root fields, pointer-plus-offset ambiguity, wrong asset SHA-256, and the actual CLI adapter entry point.

## Not run / not claimed

- official Khronos glTF-Validator executable or npm package;
- hosted drag-and-drop validation;
- Blender import/export round trip;
- Astral import or rendering;
- collision, LOD, nav, streaming, GPU/frame-time/RAM/VRAM measurements;
- independent visual-art approval;
- UE5/Unity/Genshin/ZZZ parity.

## Next action

After source review, run the official Khronos validator on the exact ART-006B asset through an already-authorized workstation/runtime if the executable is available. Capture executable version/path, asset SHA-256, raw JSON report, command, exit code, and this adapter's result. If the executable is absent, keep the gate `not_run` rather than installing a dependency under this task.
