# Astral calibration asset specification v0.1

Status: **source-validated handoff, runtime import/render not implemented**  
Loop: `astral-art-hourly-20260922`  
Asset ID: `ART-003-asymmetric-surface`

## Purpose

This fixture is the first solid-mesh source contract for the art/engine boundary. It is intentionally small enough to debug by inspection while still detecting the common mistakes that a symmetric cube misses: axis conversion, handedness/mirroring, face winding, UV orientation, tangent basis, material assignment, embedded image decode, scene-node ownership, and unit scale.

The source fixture is generated as a self-contained glTF 2.0 asset with an embedded binary buffer and embedded 16x16 orientation PNG. Astral does not import glTF today. Passing the Python validator means only that the source fixture obeys this project's expected contract, not that Astral can render it.

## Source convention

Khronos glTF 2.0.1 defines a right-handed coordinate system with +Y up, +Z forward, -X right, and metres for linear distances. Positive-determinant triangle meshes use counter-clockwise winding. `POSITION`, `NORMAL`, `TANGENT`, and `TEXCOORD_0` are standard vertex attributes. Tangent XYZ is normalized; tangent W is +/-1 and the bitangent is `cross(normal, tangent.xyz) * tangent.w`.

Primary reference: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html

Blender 5.2's glTF exporter exposes Y-up conversion and can export UVs, normals, and tangents. That makes Blender a suitable later authoring source, but no Blender install/export round trip is claimed in this packet.

Primary reference: https://docs.blender.org/manual/en/5.2/addons/scene_gltf2.html

## Fixture geometry

`AsymmetricBlock` is a rectangular prism:

- X width: 1.0 m, from -0.5 to +0.5
- Y height: 1.5 m, from 0.0 to 1.5
- Z depth: 2.0 m, from -1.0 to +1.0
- bottom sits at Y=0
- +Z is the intended front

The block uses 24 vertices, four per face, so every face has unambiguous UVs and tangent frames. It contains 36 unsigned-short indices, six per face.

`ForwardRightMarker` is a separate triangle placed at Z=1.01, in front of the +Z face. Its point extends toward +X. Because glTF defines -X as right, this deliberately provides a handedness test: an importer that silently treats +X as right or mirrors the asset will visibly disagree with the fixture contract.

## UV and material test

The block uses `TEXCOORD_0` with each face mapped over the full 0..1 square. Its material binds an embedded 16x16 nearest-filtered orientation texture:

- top-left: red
- top-right: green
- bottom-left: blue
- bottom-right: yellow
- top and left image borders: black

The marker uses a second magenta material. This makes material-index swaps visible independently of UV errors.

Sampler wrap mode is CLAMP_TO_EDGE on both axes. This is a coordinate/orientation fixture, not a tiling-material test.

## Engine-worker acceptance path

The engine-owned importer/render path should load this exact generated file without preprocessing it into an undocumented private convention. If conversion is needed, conversion belongs at the import boundary and must be covered by tests.

Minimum importer acceptance:

1. two named nodes are present and keep their mesh ownership;
2. block bounds are exactly X [-0.5,0.5], Y [0,1.5], Z [-1,1] within float tolerance;
3. marker remains beyond +Z front and points toward +X;
4. 24 block vertices and 36 block indices are preserved or produce equivalent indexed geometry;
5. supplied normals remain unit length and match face winding;
6. tangent XYZ remains normalized and W remains +/-1;
7. UVs remain in [0,1] and the rendered orientation texture is not vertically or horizontally mirrored;
8. block receives material 0 and marker receives material 1;
9. unit scale is one metre per source unit after node transforms;
10. invalid/truncated buffers, indices, accessors, and image data fail safely.

Minimum renderer acceptance under a fixed neutral-light scene:

- front (+Z) face is identifiable;
- marker is visible in front of the front face and on the expected +X side;
- all four UV quadrants appear in the expected orientation;
- back-face culling agrees with source winding;
- no NaN, missing-material fallback, or uninitialized tangent behavior;
- a captured frame identifies exact source SHA, importer build, GPU/driver, and camera transform.

## Neutral review camera

Proposed first camera for screenshots, not an engine requirement:

- target: (0, 0.75, 0)
- eye: (3.2, 2.4, 4.5)
- vertical FOV: 45 degrees
- no post-process color grading
- neutral white key light plus low-intensity fill
- fixed exposure

A second screenshot should use eye (-3.2, 2.4, 4.5) to expose accidental mirroring.

## What this does not prove

This packet does not prove glTF import, PBR rendering, normal-map correctness, mip generation, compression, collision, LODs, skinning, animation, editor placement, package loading, UE5/Unity parity, or production-art quality. It is a deliberately narrow source fixture and handoff.
