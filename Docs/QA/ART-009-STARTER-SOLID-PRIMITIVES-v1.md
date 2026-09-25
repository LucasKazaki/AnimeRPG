# ART-009 Starter Solid Primitives v1 QA

Date: 2026-09-24  
Scope: source-only verification for a new generic starter-content asset packet.

## Delivered asset

A freshly generated `starter_solid_primitives_v1.gltf` contains four original indexed solid meshes with one neutral material. The generated glTF is intentionally not checked in; the checked-in expected manifest pins its exact bytes.

| Primitive | Vertices | Indices | Nominal size |
|---|---:|---:|---|
| Cube | 24 | 36 | 1 x 1 x 1 m |
| UV sphere | 151 | 672 | 1 m diameter |
| Cylinder | 70 | 192 | 1 m diameter x 2 m high |
| Plane | 4 | 6 | 1 x 1 m |

Total: 249 vertices, 906 indices. These are functional source meshes, not render screenshots or engine-installed assets.

## Current pinned identity

Candidate code/data head before this documentation refresh: `d6cd5703985ef93249376cd9c07eaa76315d3556`.

- canonical source contract SHA-256: `ddf647df2aa7b5ef91d1bdca02384c52713cabc0f2d7e90c65e3d935663d5aa4`
- glTF SHA-256: `e79789761f295c2ae73094cc6643d1747e5fb6852eaca627a972d7e475c36b17`
- glTF bytes: `26,172`
- expected manifest SHA-256: `5ef9f93bcd31e0192dbff6e1a6c60f5820defd663efc51e81fd3913c8ca2f2a1`
- generator Git blob: `e196124dd73577f9758eec8791a39cd4307e8e29`
- verifier Git blob: `abb0b592aee0adb5f4c3dc9eb9454ccb541211fb`
- tests Git blob: `e283f4e04d4b90e56256b5bd9f38079b14ea63db`

The source identity is SHA-256 of canonical parsed JSON bytes, so LF and CRLF checkouts share one source pin. The checked-in expected-manifest comparison normalizes only CRLF and lone CR to LF. Spacing, key order, number spellings and all other bytes remain part of the manifest pin.

## Review-driven tangent, UV and schema repairs

The first independent review on candidate `3a8b174a4cba0e07bf368cd6369b937e8d608417` identified three P2 semantic-validation gaps. The repaired verifier:

1. derives each triangle's expected tangent and bitangent from position and UV derivatives, then checks tangent direction plus `TANGENT.w` handedness;
2. checks the declared per-shape UV mapping policies directly, including cube/plane corners, sphere seam and latitude/longitude parameterization, and cylinder side/cap mappings;
3. applies strict JSON-integer validation to every attribute accessor binding so `false` cannot alias integer accessor `0`.

A follow-up review found that opposing bad vertex tangents could cancel in the triangle-average check. The verifier now checks every supplied triangle-vertex tangent against the independently derived triangle `dP/du` direction before the average tangent/bitangent consistency checks. The focused suite contains an explicit opposing-tangent regression.

## Cross-platform line-ending repairs

Clean Windows verification exposed two portability defects in sequence: raw checkout bytes made the source hash line-ending sensitive, and raw expected-manifest comparison rejected a CRLF checkout of an otherwise identical pin. The generator and verifier now hash canonical parsed source JSON. The expected-manifest gate separately normalizes only line endings. Focused regressions cover both CRLF source and CRLF expected-manifest cases.

## Linear base-color contract repair

The source contract previously named the neutral material field `base_color_srgb` even though the generator copied those values directly to glTF `baseColorFactor`. The source field is now `base_color_linear_factor`, matching the intended glTF material semantics. The numeric factor remains `[0.62, 0.64, 0.68, 1.0]`, so this repair does not intentionally change generated visual values or glTF bytes.

Both generator and independent verifier require the new field name. A focused negative regression restores the obsolete `base_color_srgb` spelling and requires both validators to reject the contract. The focused suite inventory is now 34 cases.

## Output preflight repair

The generator previously checked and wrote its two destinations sequentially. With a fresh glTF destination and an already occupied manifest destination, it could write the glTF and only then refuse the manifest, leaving a half-generated packet. The repaired generator resolves and compares both destinations first, rejects aliases, refuses if either destination already exists, creates parent directories only after those checks, and then writes both files.

Two focused regressions cover the occupied-second-output case and aliased output paths. The focused suite inventory is now 36 cases. This repair changes generator safety behavior only; source identity, geometry, material values, generated glTF bytes, and runtime state are unchanged.

## Exclusive-create race repair

The preflight still had a time-of-check/time-of-use window because the final writes used overwrite-capable `write_bytes()`. Another process could create a destination after the `exists()` checks and have that file truncated by this generator. Output creation now uses exclusive `xb` mode. If a destination races into existence, generation refuses and removes any earlier output created by the current invocation so it does not leave a half packet.

A focused regression injects a manifest creation after preflight, requires refusal, preserves the other writer's bytes, and requires the generator-created glTF to be removed. The focused suite inventory is now 37 cases. Geometry, material values, canonical source identity, and generated glTF bytes are unchanged.

## Verification evidence

Historical exact-head clean-Windows verification at `d36c50e559d7aca9c9aee07136bd1630027edb43` recorded generator pass, generator `--check` pass, independent verifier pass for all four meshes, focused suite `PASS: 33/33`, Python compilation pass, `git diff --check` pass, and successful hosted Windows run `36053365291`. That result predates the linear base-color contract repair and is not reused as 34-case acceptance.

For linear-factor repair head `d6cd5703985ef93249376cd9c07eaa76315d3556`, GitHub Actions `Windows build and deterministic tests` run `36076296993` / #1118 completed successfully. That hosted workflow does not execute the focused ART-009 Python suite, so no 34/34 exact-head source-suite result is claimed here.

## Current gate state

The packet remains `source_validated_not_imported`. The exact exclusive-create repair head still needs fresh generator output, generator `--check`, independent verifier, focused 37/37 regression suite, Python compile, applicable hosted repository checks, and a fresh independent review with no unresolved blocking finding.

The older tangent review thread is stale relative to repaired source and regression coverage but still needs an explicit repair reply and resolution. A new independent review should target the final exact head after this QA refresh.

## Evidence boundary

This packet does not establish Blender/DCC round-trip behavior, Khronos glTF-Validator acceptance, Astral import/rendering, editor primitive registration, collision or physics, LODs, native GPU/performance evidence, tutorial installation, independent visual-art approval, or UE5/Unity/Genshin/ZZZ parity.

A green hosted build or source regression suite cannot promote it to `imported`, `runtime_verified`, or `art_approved`.
