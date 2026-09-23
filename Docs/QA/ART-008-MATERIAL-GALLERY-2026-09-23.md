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

## Source research

Rechecked the current Khronos glTF registry and glTF 2.0.1 specification. glTF can
represent complete scenes with nodes, meshes, materials and cameras; the camera
looks down local -Z; the metallic-roughness model defines `baseColorFactor` as a
linear multiplier. The ratified `KHR_lights_punctual` extension defines
directional-light intensity in lux and light direction along local -Z. Blender
5.2 LTS documents export support for meshes, Principled BSDF materials, cameras
and punctual lights. These references justify the interchange/source contract
only, not Astral support.

Primary references rechecked 2026-09-23:
- https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
- https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual
- https://docs.blender.org/manual/en/5.2/addons/scene_gltf2.html

## Author verification

Executed in an isolated sandbox fixture:

```text
python Scripts/generate_material_gallery_gltf.py --source Content/Calibration/MaterialGallery/gallery-spec.json --output <new-dir>
# PASS: neutral material gallery source
python Scripts/generate_material_gallery_gltf.py --source Content/Calibration/MaterialGallery/gallery-spec.json --output <same-dir> --check
# PASS
python Scripts/verify_material_gallery_gltf.py <same-dir>/material_gallery.gltf --source Content/Calibration/MaterialGallery/gallery-spec.json --manifest <same-dir>/manifest.json --expected-manifest Content/Calibration/MaterialGallery/expected-manifest.json
# PASS: 5 materials, 4 stations, 325 sphere vertices, 1584 sphere indices, 2 lights, 50 degree vertical FOV
python Scripts/test_material_gallery_gltf.py
# PASS: 11/11
python -m py_compile Scripts/generate_material_gallery_gltf.py Scripts/verify_material_gallery_gltf.py Scripts/test_material_gallery_gltf.py
# PASS
```

Pinned SHA-256:
- canonical `gallery-spec.json`: `517833a990db74f97d8046aa7fafa2d2d73859538d59c3c41ff4a8a7fb63f530`
- generated `material_gallery.gltf`: `4ef57da77151c045a69a235246cef90ad85579242edae5ed24fd130b6e2c663a`
- expected/generated manifest: `929d9d52d52b37a2a0064f2c286bed03e7e5fecdbbd9f8a2e91fea39bd507f26`

The generated glTF is 38,604 bytes. It is intentionally not tracked as a second
derived source; `expected-manifest.json` pins the exact output.

## Publication verification

The branch was compared against ART-007 base `087c673f716800198272915777681c61a748a490`.
Only the 11 ART-008 allowed paths changed. The remote Git blob IDs for the five
locally tested contract/tool files match `git hash-object` on the sandbox copies:

- `gallery-spec.json`: `68241ebc51fa00aa90559f8b917fb1a6f8ec7cba`
- `expected-manifest.json`: `d8b6f84be2f56e07b901742cbab24779d00a37b7`
- generator: `f22b5ff6ca3a3d22853ecf2125d08e08b9384a81`
- verifier: `d49aefcdc2c03fc29ac2d215b228e5594e8e9bc6`
- regression suite: `94831860bdfbef779dce5a33904d532ba7dd756d`

Draft PR #33 is the scoped integration surface. Exact-head hosted CI and fresh
independent review are separate gates and must be rechecked after the final branch
head is established.

## Evidence boundaries

Not run: Blender export round-trip, Astral import, Astral rendering, Windows/GPU
native capture, performance/memory measurement, or independent visual art review.

Therefore this pass establishes only `source -> validated`. It does not establish
`imported`, `runtime_verified`, `art_approved`, or parity with Unreal, Unity,
Genshin Impact, Zenless Zone Zero, or professional material-review workflows.
