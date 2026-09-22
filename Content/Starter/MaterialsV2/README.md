# Astral Starter Materials v2

This folder defines a deterministic source-material pack for future Astral texture/import testing. It is **not installed in the editor and not rendered by the current engine**.

The checked-in source of truth is the generator plus `expected-manifest.json`. Running the generator produces 16 PNGs whose exact byte sizes and SHA-256 values are pinned by that manifest.

## Contents generated

Four standalone 512 x 512 tileable material families:

- limestone
- concrete
- brushed steel
- asphalt

Each material produces:

- `*_basecolor.png`, sRGB source color with no directional lighting baked in;
- `*_normal.png`, linear tangent-space normal using a positive-Y convention;
- `*_orm.png`, linear R=occlusion, G=roughness, B=metallic;
- `*_height.png`, linear replicated-grayscale authoring height.

The generated pack contains 16 PNGs plus `manifest.json`. Every PNG is SHA-256 recorded. Files are original procedural sources generated with Python's standard library. No Epic, Unity, HoYoverse or third-party texture pixels are included.

## Why this is an upgrade from the first pack

The original starter pack used a 256 x 256 atlas containing sixteen 64 x 64 calibration swatches. Those were useful for format tests but are poor repeatable material sources because atlas filtering and mipmaps can bleed between tiles. V2 generates standalone 512 x 512 textures with exact seam continuity and independently validated map semantics.

This still does not establish professional material quality. The maps are procedural calibration/default sources, not scans or artist-finished hero surfaces. Runtime quality must be judged only after Astral supports textured solid rendering with a defined tangent basis and import path.

## Generate and verify

From the repository root, generate into a new directory outside the source tree:

```powershell
python Scripts/generate_starter_materials_v2.py --output ../AstralMaterialsV2
python Scripts/generate_starter_materials_v2.py --output ../AstralMaterialsV2 --check
python Scripts/verify_starter_materials_v2.py ../AstralMaterialsV2 --expected-manifest Content/Starter/MaterialsV2/expected-manifest.json
python Scripts/test_starter_materials_v2.py
```

The generator refuses to overwrite an existing destination. `--check` is read-only and regenerates expected bytes in memory before comparison. The pinned manifest lets CI or a local worker detect silent generator drift without storing 16 generated binary files in Git.

## Material contract

This pack intentionally aligns its map semantics with a common future interchange path:

- base color is sRGB;
- normal/ORM/height are linear data;
- normal is tangent space, positive Y;
- ORM is R occlusion, G roughness, B metallic;
- pure nonmetals use metallic 0, brushed steel uses metallic 1.

Khronos glTF 2.0.1 defines base-color sRGB, tangent-space +Y normals, roughness in G, metalness in B and occlusion in R. Blender 5.2 LTS documents the corresponding glTF metal/rough workflow. Astral does **not** import glTF yet, so this alignment is an importer-facing design choice, not runtime proof.

## Art review gate

Before any v2 material is promoted beyond `source_validated_not_imported`, capture:

1. a 3 x 3 tiled view to check repetition and seams;
2. neutral daylight and overcast material-sphere views;
3. grazing-light closeup to reveal normal/roughness problems;
4. measured texture memory and mip behavior in the actual Astral renderer;
5. an independent art review against a licensed human-authored comparison material.
