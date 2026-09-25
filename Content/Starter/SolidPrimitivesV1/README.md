# Astral Starter Solid Primitives v1

Status: `source_validated_not_imported`.

This packet adds four original, inspectable glTF 2.0 starter primitives for Astral Engine authoring and future tutorial use:

- 1 m cube, 24 vertices / 36 indices
- 1 m diameter UV sphere, 151 vertices / 672 indices
- 1 m diameter x 2 m height cylinder, 70 vertices / 192 indices
- 1 m x 1 m plane, 4 vertices / 6 indices

All four carry `POSITION`, `NORMAL`, `TANGENT`, and `TEXCOORD_0`, use indexed `TRIANGLES`, and share one neutral nonmetal material. Its `[0.62, 0.64, 0.68, 1.0]` glTF `baseColorFactor` is explicitly a **linear** factor; it is not named or treated as sRGB-encoded texture data. The generated file is self-contained with an embedded binary buffer. Units are metres. The source contract uses the art pipeline convention `+Y` up, `+Z` forward, `-X` right in a right-handed coordinate system.

## Why these four first

The current Unity manual exposes Cube, Sphere, Capsule, Cylinder, Plane, and Quad as built-in primitive/placeholder objects. Unreal Engine 5.8 Modeling Mode documents Box, Sphere, Cylinder, Cone, Torus, Arrow, Rectangle, Disc, and Stairs predefined shapes. This v1 packet intentionally implements only the overlapping, high-use cube/sphere/cylinder/plane subset. It is not a claim of parity with either engine.

Official reference scope observed 2026-09-24:

- Unity: https://docs.unity3d.com/Manual/PrimitiveObjects.html
- Unreal Engine 5.8: https://dev.epicgames.com/documentation/unreal-engine/predefined-shapes-in-unreal-engine

No Epic or Unity meshes, textures, code, or content are copied.

## Files

- `source-contract.json`: closed source/provenance/budget contract
- generated `starter_solid_primitives_v1.gltf`: deterministic self-contained source asset produced into a fresh output directory
- `expected-manifest.json`: checked-in exact pin for the generated source/glTF hashes and aggregate counts
- `Scripts/generate_starter_solid_primitives.py`: deterministic generator
- `Scripts/verify_starter_solid_primitives.py`: independent semantic verifier
- `Scripts/test_starter_solid_primitives.py`: focused positive/negative regressions

## Verification

Generate into a fresh output directory, then compare the fresh manifest to the checked-in pin:

```text
python Scripts/generate_starter_solid_primitives.py \
  --source Content/Starter/SolidPrimitivesV1/source-contract.json \
  --gltf <fresh-dir>/starter_solid_primitives_v1.gltf \
  --manifest <fresh-dir>/manifest.json

python Scripts/generate_starter_solid_primitives.py \
  --source Content/Starter/SolidPrimitivesV1/source-contract.json \
  --gltf <fresh-dir>/starter_solid_primitives_v1.gltf \
  --manifest <fresh-dir>/manifest.json \
  --check

python Scripts/verify_starter_solid_primitives.py \
  --source Content/Starter/SolidPrimitivesV1/source-contract.json \
  --gltf <fresh-dir>/starter_solid_primitives_v1.gltf \
  --manifest <fresh-dir>/manifest.json \
  --expected-manifest Content/Starter/SolidPrimitivesV1/expected-manifest.json

python Scripts/test_starter_solid_primitives.py
```

The generated glTF is intentionally not checked into the repository; its exact bytes are pinned by `expected-manifest.json`. These commands establish only the source contract. They do not establish Blender round-trip behavior, Khronos validator acceptance, Astral import/rendering, collision, editor registration, tutorial installation, runtime performance, or visual-art approval.
