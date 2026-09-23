# Neutral Material Gallery source fixture

Status: `source_validated_not_imported`

This directory contains the canonical art-side source contract for ART-008. The
checked-in files are `gallery-spec.json` and `expected-manifest.json`. The derived
`material_gallery.gltf` is generated into a fresh external directory and is not
tracked, so the source spec and pin cannot drift from a second checked-in copy.

The fixture is deliberately small but useful. Four material stations each use the
same UV sphere and hard-edged cube so roughness/metalness and highlight shape can
be compared on curved and planar geometry. The stations are MidGray dielectric,
NearWhite dielectric, NearBlack dielectric, and Chrome metal. A neutral floor,
fixed 50 degree vertical-FOV camera, and two white directional lights are included.

`gallery-spec.json` is an art reference, not an Astral runtime scene. It declares
metres, glTF +Y up / +Z forward / -X right, a 16:9 camera, a 1000 lux key and
250 lux fill, and source-only material factors. The fixture contains no image
textures and forbids baked lighting, so later runtime captures can distinguish
source material values from scene lighting and grading.

The values follow glTF 2.0 semantics. `baseColorFactor` is a linear multiplier,
the metallic-roughness material model is used, the perspective camera looks down
its local -Z axis, and KHR_lights_punctual directional-light intensity is in lux.
Blender 5.2's glTF exporter supports meshes, Principled BSDF materials, cameras,
and punctual lights, so the same contract is suitable for a later Blender
round-trip once local Blender execution and Astral import are actually available.

Primary references, rechecked 2026-09-23:
- Khronos glTF 2.0.1: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
- Khronos KHR_lights_punctual: https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual
- Blender 5.2 LTS glTF exporter: https://docs.blender.org/manual/en/5.2/addons/scene_gltf2.html

Generate and verify:

```text
python Scripts/generate_material_gallery_gltf.py --source Content/Calibration/MaterialGallery/gallery-spec.json --output <new-dir>
python Scripts/generate_material_gallery_gltf.py --source Content/Calibration/MaterialGallery/gallery-spec.json --output <same-dir> --check
python Scripts/verify_material_gallery_gltf.py <same-dir>/material_gallery.gltf --source Content/Calibration/MaterialGallery/gallery-spec.json --manifest <same-dir>/manifest.json --expected-manifest Content/Calibration/MaterialGallery/expected-manifest.json
python Scripts/test_material_gallery_gltf.py
```

Acceptance is still staged: source -> validated is exercised here. Imported,
runtime_verified, and art_approved remain false until the engine worker can import
and render the fixture and a separate art review judges real Astral captures.
