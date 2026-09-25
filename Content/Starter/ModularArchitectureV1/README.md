# Starter Modular Architecture v1

Status: `proposed_source_fixture`, source-only and not runtime-integrated.

This packet contains six original modular blockout meshes designed around a proposed 0.25 m translation grid and 90 degree rotation increment:

- 1 x 1 m floor
- 2 x 2 m floor
- 1 x 3 m wall
- 2 x 3 m wall
- 2 x 3 m doorway wall with a 1 x 2 m opening
- 0.25 x 3 m pillar

All mesh-local pivots are at the local minimum corner on the base plane. The pinned ART-010 source contract defines indexed glTF 2.0 triangle geometry with UVs, normals, tangents and one neutral material. Gallery translations are presentation-only and do not change local snap dimensions.

## Tutorial-room source fixture

`tutorial-room-source.json` is an original source-only assembly example for a 4 x 4 m room. It uses 13 module placements: four 2 x 2 m floor modules, a centered doorway in the front wall, two rear-wall modules, and four rotated side-wall modules. One player-start marker and one review-camera marker are included as source metadata only.

Every authored placement stays on the 0.25 m translation grid and every rotation is a multiple of 90 degrees. An isolated standard-library validation pass checked 57 source-layout assertions covering module identity, snapping, transformed wall bounds, floor union, centered doorway bounds and source-only claim flags.

Tutorial-room source SHA-256: `d32a42387bd853b56da3c0133ed5e5cc2b8483f72afc31bc3cbfc68b1f5519d0`.

No Astral import, editor registration/snapping, collision, navigation, LOD, runtime rendering, performance evidence, gameplay behavior or art approval is claimed.
