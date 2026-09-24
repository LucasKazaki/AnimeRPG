# Modular Architecture V1 source kit

Status: `source_validated_not_imported`.

This is an original, deterministic solid-mesh starter kit for modular environment
blocking and importer qualification. It is a follow-up to the original
`ASTRAL_MESH 1` wireframe starter archive, not a replacement for the current
runtime loader and not an editor-installed asset pack.

## Delivered modules

Seven unique functional render modules are authored on a 0.25 m snap grid:

- `floor_4x4`: 4 m x 4 m floor tile, 0.25 m thick, top surface at local Y=0;
- `wall_4x3`: 4 m wide x 3 m high wall;
- `wall_2x3`: 2 m wide x 3 m high wall;
- `corner_2x2x3`: 2 m L-corner assembled without overlapping volumes;
- `doorway_4x3`: 4 m x 3 m frame with a clear 1.5 m x 2.25 m opening;
- `stairs_2x4x2`: 2 m wide, 4 m run, 2 m rise, eight 0.25 m steps;
- `pillar_0p5x3`: 0.5 m square x 3 m pillar.

Every render module has real indexed triangles plus POSITION, NORMAL, TANGENT and
TEXCOORD_0 data. UV scale is one UV unit per metre. The glTF also contains a
second, non-default `CollisionGallery` scene with one matching collision proxy per
module. The stair proxy is a lower-complexity walkable ramp prism rather than a
copy of the stepped render mesh. Collision nodes are source metadata only; Astral
does not currently consume this convention.

The default `RenderGallery` scene spaces modules apart for DCC inspection while
keeping each mesh's local pivot at its declared modular anchor. Gallery
translations are not baked into mesh positions.

## Coordinate, material and provenance contract

- units: metres;
- right-handed;
- up: +Y;
- forward: +Z;
- character/right direction under this project convention: -X;
- snap grid: 0.25 m;
- base UV density: 1 UV unit per metre;
- material 0: neutral opaque source material;
- material 1: collision-debug source material;
- no textures, images, external buffers, third-party meshes or proprietary engine
  content are included;
- geometry is original procedural project content.

The neutral materials are inspection aids, not production surface art. No baked
lighting is present.

## Verify from repository root

```powershell
python Scripts/generate_modular_architecture_v1.py `
  --source Content/Starter/ModularArchitectureV1/source.json `
  --gltf ../AstralModularArchitectureV1/modular_architecture_v1.gltf `
  --manifest ../AstralModularArchitectureV1/manifest.json

python Scripts/generate_modular_architecture_v1.py `
  --source Content/Starter/ModularArchitectureV1/source.json `
  --gltf ../AstralModularArchitectureV1/modular_architecture_v1.gltf `
  --manifest Content/Starter/ModularArchitectureV1/expected-manifest.json `
  --check

python Scripts/verify_modular_architecture_v1.py `
  --source Content/Starter/ModularArchitectureV1/source.json `
  --gltf ../AstralModularArchitectureV1/modular_architecture_v1.gltf `
  --manifest Content/Starter/ModularArchitectureV1/expected-manifest.json

python Scripts/test_modular_architecture_v1.py
```

The generator refuses to overwrite existing outputs unless `--check` is used.
For deliberate regeneration, write to fresh paths and review the resulting diff
instead of replacing the pinned fixture blindly.

## Evidence boundary

The derived glTF is generated into a fresh path outside the source tree and pinned by `expected-manifest.json`; this avoids maintaining a second checked-in derivation that can drift. The generated glTF is a source fixture, not evidence of Astral glTF import,
editor snapping, runtime collision, navmesh behavior, textured rendering, LOD,
streaming, native GPU performance, DCC round-trip stability or art approval. The
official Khronos validator and Blender remain separate acceptance gates. Do not
call this UE5/Unity parity from file counts or source validation alone.
