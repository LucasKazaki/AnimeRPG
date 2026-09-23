# ART-008 neutral material gallery author QA

Date: 2026-09-23  
Loop: `astral-art-hourly-20260922`  
Status: source-validated candidate, not imported or runtime-approved.

## Delivered

ART-008 adds a machine-readable neutral gallery spec plus deterministic glTF
generation and an independent verifier. The derived scene contains four matched
sphere/cube stations, five PBR materials including the floor, one fixed camera,
two white directional lights, and no image textures. This is an art-side source
fixture, not a renderer feature or an Astral screenshot.

This repair pass corrected two exact source-contract defects found by independent
review. The floor tangent now uses `w = -1`, so glTF reconstructs its bitangent
along +Z for the authored +U/+X and +V/+Z UV directions. The verifier also derives
and checks the exact camera and light quaternions from the source spec instead of
accepting any normalized quaternion. Three focused regressions cover floor tangent
handedness, camera rotation, and directional-light rotation.

## Source research

Rechecked the current Khronos glTF registry and glTF 2.0.1 specification. glTF can
represent complete scenes with nodes, meshes, materials and cameras; the camera
looks down local -Z; tangent-space bitangents are reconstructed as
`cross(normal.xyz, tangent.xyz) * tangent.w`; and the metallic-roughness model
defines `baseColorFactor` as a linear multiplier. The ratified
`KHR_lights_punctual` extension defines directional-light intensity in lux and
light direction along local -Z. Blender 5.2 LTS documents export support for
meshes, Principled BSDF materials, cameras and punctual lights. These references
justify the interchange/source contract only, not Astral support.

Primary references rechecked 2026-09-23:
- https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
- https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual
- https://docs.blender.org/manual/en/5.2/addons/scene_gltf2.html

## Author verification

Executed in an isolated sandbox fixture against the exact generator/verifier/test
bytes published in this repair:

```text
python Scripts/generate_material_gallery_gltf.py --source Content/Calibration/MaterialGallery/gallery-spec.json --output <new-dir>
# PASS: neutral material gallery source
python Scripts/generate_material_gallery_gltf.py --source Content/Calibration/MaterialGallery/gallery-spec.json --output <same-dir> --check
# PASS
python Scripts/verify_material_gallery_gltf.py <same-dir>/material_gallery.gltf --source Content/Calibration/MaterialGallery/gallery-spec.json --manifest <same-dir>/manifest.json --expected-manifest Content/Calibration/MaterialGallery/expected-manifest.json
# PASS: 5 materials, 4 stations, 325 sphere vertices, 1584 sphere indices, 2 lights, 50 degree vertical FOV
python Scripts/test_material_gallery_gltf.py
# PASS: 14/14
python -m py_compile Scripts/generate_material_gallery_gltf.py Scripts/verify_material_gallery_gltf.py Scripts/test_material_gallery_gltf.py
# PASS
```

Pinned SHA-256:
- canonical `gallery-spec.json`: `517833a990db74f97d8046aa7fafa2d2d73859538d59c3c41ff4a8a7fb63f530`
- generated `material_gallery.gltf`: `d0bca093cfc52b59816a14ff98cc90b8684e7e7f664d0b687b394be5aa166af1`
- expected/generated manifest: `ba94405d30eaf4cd7f259b59f9c01e77fbe672527c2a1747061cd3f912c5c3b2`

The generated glTF remains 38,604 bytes. It is intentionally not tracked as a
second derived source; `expected-manifest.json` pins the exact output.

## Publication verification

The repair remains within the existing ART-008 path ownership. The remote Git blob
IDs returned by the publication writes match the locally tested files:

- `expected-manifest.json`: `8b8752ef393ed017aa8e87fd3f6f5964e0f6f78c`
- generator: `064d11cf4df02bcf7b8c76124e02e7936700e78d`
- verifier: `7a798d1924601877b0be3e4f3382befe32ffac2e`
- regression suite: `cf23110688d35e13fa4cff424f74d09163ef67d7`

Draft PR #33 remains the scoped integration surface. Exact-head hosted CI and a
fresh independent review are separate gates and must be rechecked after the final
repair/state head is established.

## Evidence boundaries

Not run: Blender export round-trip, Astral import, Astral rendering, Windows/GPU
native capture, performance/memory measurement, or independent visual art review.

Therefore this pass establishes only `source -> validated`. It does not establish
`imported`, `runtime_verified`, `art_approved`, or parity with Unreal, Unity,
Genshin Impact, Zenless Zone Zero, or professional material-review workflows.
