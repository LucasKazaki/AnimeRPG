# ART-009 Starter Solid Primitives v1 QA

Date: 2026-09-24  
Scope: source-only sandbox verification for a new generic starter-content asset packet.

## Delivered asset

A freshly generated `starter_solid_primitives_v1.gltf` contains four original indexed solid meshes with a single neutral material. The generated glTF is intentionally not checked in; the checked-in expected manifest pins its exact bytes:

| Primitive | Vertices | Indices | Nominal size |
|---|---:|---:|---|
| Cube | 24 | 36 | 1 x 1 x 1 m |
| UV sphere | 151 | 672 | 1 m diameter |
| Cylinder | 70 | 192 | 1 m diameter x 2 m high |
| Plane | 4 | 6 | 1 x 1 m |

Total: 249 vertices, 906 indices. These are functional source meshes, not render screenshots or engine-installed assets.

## Pinned source/output identity

- source contract SHA-256: `12473281a43b46c8a41e04d9515c4d2b692387ba90ca175dcdd6e690c727b4cf`
- glTF SHA-256: `e79789761f295c2ae73094cc6643d1747e5fb6852eaca627a972d7e475c36b17`
- expected manifest SHA-256: `653d120860b9fcf248d2c50ab3b880d98d26a69b4c681ec212a3b39d8f90f71e`
- generator Git blob: `3b77c487c7a20ea444155bab8453d4a54ac1e85a`
- verifier Git blob: `3871d8dda7971766408580b4063a895748a7c27f`
- tests Git blob: `4b6ac10b6ec4e1ee7a1117466da013ac1598327e`

## Review-driven repair

The first independent review on candidate `3a8b174a4cba0e07bf368cd6369b937e8d608417` identified three P2 semantic-validation gaps. The repaired verifier now:

1. derives each triangle's expected tangent and bitangent from position and UV derivatives, then checks the supplied tangent direction plus `TANGENT.w` handedness;
2. checks the declared per-shape UV mapping policies directly, including cube/plane corners, sphere seam and latitude/longitude parameterization, and cylinder side/cap mappings;
3. applies strict JSON-integer validation to every attribute accessor binding so `false` cannot alias integer accessor `0` in Python equality semantics.

The regression suite adds one negative test for each first-review finding, increasing focused coverage from 27 to 30 cases. A follow-up independent review then found that two opposing bad vertex tangents could cancel in the triangle-average tangent check. The verifier now checks every supplied vertex tangent directly against the independently derived triangle `dP/du` direction before retaining the average tangent/bitangent check as supplemental consistency evidence. The `0.975` per-vertex dot threshold is intentionally above the 16-segment sphere/cylinder chord case observed analytically (`cos(11.25 degrees) ~= 0.980785`) while decisively rejecting the reported approximately +/-80 degree cancellation case (`cos(80 degrees) ~= 0.173648`). This pass adds an explicit 31st regression that mutates cube tangent vertices 0 and 2 to opposing approximately +/-80 degree directions. That exact cancellation construction must now fail semantically even if asset bytes and the manifest are repinned together.

## Cross-platform line-ending repair

An independent clean Windows worktree on candidate `f9bfce5eacd94b8524f94e453eb4a0d490bfd3e2` exposed a deterministic portability defect: 30 focused cases passed, while `test_valid_packet` failed because Git's CRLF checkout changed the raw source bytes from the repository LF identity `12473281a43b46c8a41e04d9515c4d2b692387ba90ca175dcdd6e690c727b4cf` to `b96a2bc2b6140419fb6da23c817cc34f6d7fdfdc86c7885e0c3580eff2426da3`. The asset semantics were unchanged; the raw-byte source hash was platform-sensitive.

The generator and independent verifier now hash the parsed source contract serialized as canonical JSON bytes instead of hashing checkout line endings. The checked-in expected manifest remains pinned to the canonical LF identity. A 32nd focused regression rewrites the same source contract with CRLF line endings, regenerates the packet, and requires the exact checked-in manifest plus semantic verifier to accept it. This is a determinism repair only; no asset geometry, material, runtime state, or integration claim changed.

## Sandbox execution

The following sandbox evidence was recorded on repaired candidate `d0c61b25f2ac3e29a0b50882933d1f77c58a88fb`, before the follow-up per-vertex tangent repair. It reproduced the pinned generator output byte-for-byte (`26,172` bytes, SHA-256 `e79789761f295c2ae73094cc6643d1747e5fb6852eaca627a972d7e475c36b17`). Do not reuse the 30/30 result below as exact-head acceptance for the follow-up verifier-only repair:

```text
python Scripts/test_starter_solid_primitives.py
PASS: 30/30 starter solid primitive tests

python -m py_compile Scripts/generate_starter_solid_primitives.py Scripts/verify_starter_solid_primitives.py Scripts/test_starter_solid_primitives.py
exit 0
```

The 30 focused tests include deterministic/stale-output behavior, overwrite refusal, source-schema boolean confusion, false runtime status, manifest hash and JSON-integer checks, checked-in manifest pin enforcement, external-buffer rejection, unexpected rendering attributes, material drift including bool-as-number substitution, scene bool-as-index substitution, UV/normal/tangent corruption, index bounds, reversed winding, sphere/cylinder/plane geometry drift, bufferView target/range changes, stale accessor bounds, unknown glTF root fields, UV-policy drift, official-reference drift, semantically wrong orthonormal tangents, collapsed-but-in-range UVs, and bool-as-accessor-binding substitution.

## Gate state

The current generator/verifier/regression repair still requires the packet's exact generator/check/verifier/regression/compile commands on the exact branch head plus a fresh independent review. The focused suite inventory is now 32 cases, but no 32/32 execution result is claimed until those exact commands run on this head. The earlier exact-head Windows lane on `f9bfce5e...` was green but did not execute this focused Python suite; it therefore does not close the new source-determinism gate.

## Evidence boundary

This pass did not execute Blender, glTF-Validator, Astral Engine, a native Windows GPU runtime, collision, physics, LOD generation, editor registration or independent visual review. The asset remains `source_validated_not_imported`.

A green hosted build or source regression suite cannot promote it to `imported`, `runtime_verified`, or `art_approved`.
