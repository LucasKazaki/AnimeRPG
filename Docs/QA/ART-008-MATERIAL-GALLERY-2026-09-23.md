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

The first independent review corrected floor tangent handedness and source-derived
camera/light rotations. A second exact-head review found two further P2 bypasses:
a repinned floor could reverse tangent XYZ while keeping `w = -1`, and camera or
light nodes could add a negative scale that reversed their local -Z direction while
retaining the expected quaternion. This pass closes both gaps. The verifier now
derives the floor tangent and bitangent from position/UV derivatives for every floor
triangle and compares the complete tangent frame. Camera and directional-light
nodes must also have only the expected transform keys, so scale, matrix, or other
transform overrides cannot silently reverse the checked orientation.

Three new regressions cover reversed floor tangent direction plus camera and light
negative-scale overrides. The focused suite is now 17 tests.

## Source research

The contract remains based on the Khronos glTF 2.0.1 rules already recorded for
this task: cameras and `KHR_lights_punctual` directional lights operate along local
-Z, and tangent-space bitangents reconstruct from `cross(normal, tangent.xyz) *
tangent.w`. These are interchange semantics only and do not establish Astral
runtime support.

Primary references rechecked 2026-09-23:
- https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
- https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual
- https://docs.blender.org/manual/en/5.2/addons/scene_gltf2.html

## Author verification

Executed in an isolated sandbox fixture against the modified verifier/regression
bytes before publication, using a behavior-equivalent reconstruction of the checked
in deterministic generator. The reconstructed generator reproduced the exact
pinned 38,604-byte glTF and manifest hashes before the repaired verifier was tested.

```text
python Scripts/generate_material_gallery_gltf.py --source Content/Calibration/MaterialGallery/gallery-spec.json --output <new-dir>
# PASS: neutral material gallery source
python Scripts/generate_material_gallery_gltf.py --source Content/Calibration/MaterialGallery/gallery-spec.json --output <same-dir> --check
# PASS
python Scripts/verify_material_gallery_gltf.py <same-dir>/material_gallery.gltf --source Content/Calibration/MaterialGallery/gallery-spec.json --manifest <same-dir>/manifest.json --expected-manifest Content/Calibration/MaterialGallery/expected-manifest.json
# PASS: 5 materials, 4 stations, 325 sphere vertices, 1584 sphere indices, 2 lights, 50 degree vertical FOV
python Scripts/test_material_gallery_gltf.py
# PASS: 17/17
python -m py_compile Scripts/generate_material_gallery_gltf.py Scripts/verify_material_gallery_gltf.py Scripts/test_material_gallery_gltf.py
# PASS
```

Pinned SHA-256 remains unchanged:
- canonical `gallery-spec.json`: `517833a990db74f97d8046aa7fafa2d2d73859538d59c3c41ff4a8a7fb63f530`
- generated `material_gallery.gltf`: `d0bca093cfc52b59816a14ff98cc90b8684e7e7f664d0b687b394be5aa166af1`
- expected/generated manifest: `ba94405d30eaf4cd7f259b59f9c01e77fbe672527c2a1747061cd3f912c5c3b2`

The generated glTF remains intentionally untracked; `expected-manifest.json` pins
the exact derived output.

## Publication verification

The repair remains within ART-008 ownership. The Git blob IDs returned by the
publication writes exactly match the locally tested modified files:

- verifier: `99b85c1f83780d4eaaf3a36dcf48a79b890cc632`
- regression suite: `855c810bb42e3cbade39875e554e9f562409f0e2`

The source spec, deterministic generator, and expected manifest were not changed by
this pass. Draft PR #33 remains the integration surface. Fresh exact-head hosted CI
and a new independent review are still separate gates after this evidence update.

## Evidence boundaries

Not run: Blender export round-trip, Astral import, Astral rendering, Windows/GPU
native capture, performance/memory measurement, or independent visual art review.

Therefore this pass establishes only `source -> validated`. It does not establish
`imported`, `runtime_verified`, `art_approved`, or parity with Unreal, Unity,
Genshin Impact, Zenless Zone Zero, or professional material-review workflows.
