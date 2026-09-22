# ART calibration glTF author QA, 2026-09-22

Assessment: **review findings repaired and source fixture passes bounded author validation; exact-head hosted/independent re-review and runtime import/render remain pending.** Current Astral still has no proven glTF/solid-mesh texture path.

## Research and review basis

- Khronos glTF 2.0.1 specification was rechecked for this repair. It requires each accessor to fit within its referenced bufferView, defines triangle winding through node-transform determinant with positive determinant using counter-clockwise faces, defines `TANGENT.w` as tangent-basis handedness, and computes bitangent as `cross(normal.xyz, tangent.xyz) * tangent.w`.
- Independent Codex review of PR #26 head `9f9ff0fbc3dd6fd93d95d8b86803162d7087d7cd` reported five source-contract defects: marker winding/tangent handedness, missing accessor-within-bufferView bounds, missing scene/node mesh ownership checks, incomplete embedded-PNG validation, and failure to consume the checked-in expected manifest.
- Latest `main` observed while repairing review findings: `b3a2b1bf8f2b0c356d5b352006c48cb86532426b`. This repair does not modify engine/game-owned paths or rebase the stacked art dependency.

## Repair

Generator version is now `astral-calibration-gltf-2`. The front/handedness marker uses indices `[0,2,1]`, produces a +Z geometric/front-face normal, and encodes tangent `(1,0,0,-1)` so its UV-derived bitangent is -Y as required by the fixture.

The verifier now:
- bounds each accessor inside its own bufferView and bounds every bufferView inside the decoded buffer;
- rejects sparse/strided forms not emitted by this fixture instead of partially interpreting them;
- verifies exact scene membership and named node-to-mesh ownership;
- verifies marker positions, indices, normals, tangents, UVs, winding, and handedness;
- parses the complete embedded PNG chunk stream, checks CRCs and chunk bounds/order, requires IDAT/IEND, and performs bounded decompression to the expected 16 x 16 RGB8 payload;
- compares the generated manifest bytes with the checked-in `Content/Calibration/AsymmetricSurface/expected-manifest.json` pin.

## Author verification executed

Executed in an isolated Python sandbox against the repaired candidate:

```text
python Scripts/generate_asymmetric_calibration_gltf.py --output <new-dir>
PASS: asymmetric glTF calibration fixture

python Scripts/generate_asymmetric_calibration_gltf.py --output <same-dir> --check
PASS: asymmetric glTF calibration fixture

python Scripts/verify_asymmetric_calibration_gltf.py <same-dir>/asymmetric_surface.gltf --manifest <same-dir>/manifest.json --expected-manifest Content/Calibration/AsymmetricSurface/expected-manifest.json
PASS: {'block_vertices': 24, 'block_indices': 36, 'marker_vertices': 3}

python Scripts/test_asymmetric_calibration_gltf.py
13/13 passed

python -m py_compile Scripts/generate_asymmetric_calibration_gltf.py Scripts/verify_asymmetric_calibration_gltf.py Scripts/test_asymmetric_calibration_gltf.py
PASS
```

Generated `asymmetric_surface.gltf` remains 6681 bytes and now has SHA-256 `fb99014c48c9b19a63cdc96dbc383098bae5fcc884a8192a3fc5285b0e704f6d`.
Checked-in expected manifest SHA-256 is `74b89eeb51281f9fbbb4f6397f799dea5032af931e3b5e1fa770a03bd937229c`.
Repaired source SHA-256 values:
- generator `b319ae5f21c96c3a92e822e57e92961ebc203e1781f16d68930fc6c4b907ebaf`
- verifier `950f9958ced8dbf5f93f6429469fb408fa589460ea2bd88778437777a5a4632b`
- tests `b9a621b63619b2642eae10a26fe0dce909568ecfe233847f158af09e0c38d8b6`

Regression coverage now includes valid pinned output; rehashed coordinate/material/sampler corruption; marker-index contract; accessor escape from its bufferView; swapped node ownership; changed scene membership; corrupted PNG CRC; truncated PNG; stale expected-manifest pin; unrehased byte reformat; and generator overwrite refusal.

## Evidence boundaries

These are author/sandbox checks, not an independent re-review of the repaired head. Fresh hosted checks and exact-head independent review must run after publication. No official Khronos validator executable was installed or run; no Blender export round trip; no Astral runtime, Windows GPU, native screenshot, material-v2 binding, collision, mip/compression, measured performance, or art-quality acceptance. No UE5/Unity/Genshin/ZZZ parity claim is made.
