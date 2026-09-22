# Astral original starter fixtures v1

[Creative production plan](../../Docs/Planning/CREATIVE-PRODUCTION-2026-09-21.md)
· [QA evidence](../../Docs/QA/CREATIVE-STARTER-2026-09-21.md)

**32 wireframe meshes · 5 PNG source textures · 16 material recipes.**
The files are inside [`astral-starter-v1.tar.xz`](astral-starter-v1.tar.xz), not installed
in the editor. They are original procedural calibration/blockout fixtures, not
production art, a rigged mannequin, a playable template or UE5 parity.

Archive SHA-256:
`d9a72430e9da1ded0eca82783d135eb76971458fb3bc3834bf1465b2884ac7c5`

## What works now

`ASTRAL_MESH 1` files can be read by the current engine's `StaticMesh` loader.
The archive contains primitive forms, modular architecture and simple props:
cube, plane, sphere, cylinder, cone, capsule, ramp, pyramid, octahedron, disc,
floor, wall, doorway, window wall, square/round pillar, beam, two stairs, arch,
round platform, fence, crate, barrel, table, chair, bench, pedestal, bollard,
static training dummy, axis marker and grid.

The five PNGs are a 256×256 base-color atlas, a 256×256 ORM atlas, a 16×16 flat
normal, a 64×64 checker and a 64×64 UV-orientation diagnostic. The sixteen atlas
tiles are **64×64 prototype swatches**, not high-resolution finished surfaces.
Materials cover plaster, concrete, brick, limestone, ceramic, wood, painted wood,
steel, brushed metal, rubber, fabric, soil, grass proxy, sand, gray calibration
and missing material. The flat normal contains no surface relief. PNG color-space
policy is in the manifest, not embedded as an ICC profile.

Astral currently has no texture/material rendering path. No file in this pack
makes GDI render a textured solid, imports a triangle mesh or registers an editor
asset. Do not tell users to drag these into the current editor or press a pending
Play button. There are no loadable scenes, character rigs, animations, effects or
sounds in this archive.

## Source-level tutorial: inspect, verify, load

Use a clean task worktree of this candidate. Python 3.10+ is sufficient for
archive generation and validation; no package installation or network is needed.
Run commands from the repository root. Keep generated output outside the source.

```powershell
# Extract the small checked-in archive to a NEW sibling directory.
python -m tarfile -e Content/Starter/astral-starter-v1.tar.xz ../AstralStarterPack
python Scripts/verify_starter_content.py ../AstralStarterPack
python Scripts/generate_starter_content.py --output ../AstralStarterPack --check
python Scripts/test_starter_content.py
```

Expected validator counts: 32 meshes, 5 texture files and 16 material recipes.
`manifest.json` records all 38 other files and their SHA-256 hashes; it does not
self-hash. To generate the archive's contents from editable source instead:

```powershell
python Scripts/generate_starter_content.py --output ../AstralStarterPack-new
python Scripts/verify_starter_content.py ../AstralStarterPack-new
```

The generator refuses existing destinations. `--check` never writes. Exact
regeneration checks also compare PNG compressed bytes; a different zlib encoder
can require investigation even when decoded pixels agree. Never overwrite
unknown local work to silence that check.

For a portable loader check where a C++17 compiler is already available:

```sh
g++ -std=c++17 -Wall -Wextra -Werror -O2 -DNDEBUG -I. Engine/Assets/StaticMesh.cpp Tests/StarterContentProbe.cpp -o ../StarterContentProbe
python Scripts/verify_starter_content.py ../AstralStarterPack --probe ../StarterContentProbe
```

The probe uses the actual engine loader and compares its vertex/edge counts with
the manifest. It does not create a window, render a model or test Windows.
The probe is not part of the product CTest registration in this slice.

## Coordinate and texture rules

+Y is up. One unit is one metre **for this pack**, not a claim about all existing
world coordinates. Most objects are ground-centred; floors have their top at
Y=0, and stair bounds expose their offset. Always inspect the per-file bounds.
Meshes are vertices and edges, without triangles, UVs, normals, skin weights,
collision, snapping or LOD data. Closed-looking wireframes are not solid surfaces.

Atlas origin is top-left; each recipe gives an integer pixel rectangle. **Crop a
64×64 tile before enabling repeat or generating mipmaps**: the atlas has no gutters,
and naive filtered atlas sampling will bleed neighboring tiles. Color is sRGB;
normal/ORM are linear; ORM is R=occlusion, G=roughness, B=metallic. The normal is
positive-Y tangent-space by declared convention. UV diagnostic red increases in
the right half, green in the lower half, and the top/left border is blue. Its
orientation must be reconciled with a future importer, not guessed from a render.

## Rights and next gate

No Epic, Fab, scan-library or generated-service assets were copied. This patch
makes no new public redistribution-license grant and does not license the entire
repository. Preserve the project's ownership/approval policy. Professional
third-party acquisitions need individual provenance and license records.

Next: independent review and native acceptance of the inherited engine stack;
then an admitted texture/triangle/importer and scene-catalog packet. The production
plan defines the remaining UE-like library categories and quality tests.
