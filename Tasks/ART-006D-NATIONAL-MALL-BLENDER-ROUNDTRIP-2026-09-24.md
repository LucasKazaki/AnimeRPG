# ART-006D: National Mall Blender round-trip contract

Owner: AnimeRPG Art Direction worker (`astral-art-hourly-20260922`)  
Base: ART-006C exact head `6b558a1f8deed3ca1b8463cb10534d01afa9f333`  
Scope: source-art/DCC qualification only

## Objective

Close the next dependency-ready gap in the representative National Mall asset path by making a Blender 5.2.2 LTS round trip reproducible and independently checkable for the ART-006B panel blockout.

This task does **not** install Blender, run Company Runtime, change Astral engine/importer code, or claim a Blender execution that has not happened.

## Allowed paths

- `Docs/Art/BLENDER-GLTF-ROUNDTRIP-PROFILE-v0.1.md`
- `Scripts/blender_roundtrip_national_mall_panel.py`
- `Scripts/verify_blender_roundtrip_national_mall_panel.py`
- `Scripts/test_blender_roundtrip_national_mall_panel.py`
- `Scripts/test_blender_roundtrip_national_mall_panel_hidden_payloads.py`
- `Tasks/ART-006D-NATIONAL-MALL-BLENDER-ROUNDTRIP-2026-09-24.md`
- `Docs/QA/ART-006D-NATIONAL-MALL-BLENDER-ROUNDTRIP-2026-09-24.md`

Do not edit engine, renderer, gameplay, build, workflow, dependency or another worker's continuation-state files in this packet.

## Inputs

- `Content/Reference/NationalMall/Blockout/mall_core_panel_blockout.gltf`
- `Content/Reference/NationalMall/Blockout/expected-manifest.json`
- ART-006B expected source SHA-256 `6c51463332199c65bcfbde04ee8e5883e03a94aba710980eebfaa6945f2759b7`

The verifier owns this ART-006B hash independently. A caller-supplied manifest cannot redefine the accepted source.

## Deliverables

1. A Blender-native background driver pinned to Blender 5.2.2.
2. One editable `.blend`, one embedded glTF and one strict JSON receipt when the driver is actually executed natively.
3. A standard-library verifier that compares DCC output with ART-006B scene semantics rather than byte equality.
4. Focused positive/negative regressions for evidence tampering, source substitution, receipt type confusion, scene inventory, complete inherited world transforms, topology/winding, exact rendering attributes and accessor payload contracts across reachable and unreachable mesh primitives, morph-target rejection, materials, disabled-export payloads, GPU-instancing rejection and buffer/profile violations.
5. A versioned art-facing Blender/glTF profile based on current official Blender documentation.

## Round-trip semantic gate

The post-DCC gate permits harmless buffer/accessor repacking and triangle-list reordering, but it does not infer equivalence from names, counts or bounding boxes alone.

For the bounded output it requires:

- exactly the seven expected active-scene mesh-bearing semantic instances, with no extra or duplicate mesh instances;
- indexed `TRIANGLES` only for every mesh primitive in the bounded glTF, including unreachable meshes;
- an exact primitive rendering-attribute set of float `POSITION`, `NORMAL`, `TANGENT` and `TEXCOORD_0` on every mesh primitive in the bounded glTF, with no unverified `COLOR_0`, skinning, or other extra rendering attributes, including unreachable meshes;
- valid in-range float attribute accessors, valid buffer views whose entire declared ranges remain inside their embedded buffers, valid accessor payload bounds, identical nonzero attribute counts, and unsigned-integer scalar index accessors with nonempty triangle-multiple index payloads whose indices stay within the decoded vertex range, for every mesh primitive including unreachable meshes;
- world-space center/dimensions derived from referenced vertices and matching the ART-006B source within the verifier-owned, non-overridable `1e-4` metre tolerance;
- the complete inherited 3x4 world transform for each semantic instance matching the ART-006B source within `1e-4`, in addition to matching transform orientation parity, so rotations, reflections, translations, scale and shear cannot preserve the AABB while changing rendered orientation;
- semantic material bindings and base-color, metallic, roughness, emissive, alpha and sidedness preservation;
- winding-preserving triangle/corner comparison that keeps triangle topology strict but compares position/normal/tangent/UV components pairwise with the full `1e-4` tolerance, rather than independently quantizing values into buckets;
- no primitive morph `targets`, mesh `weights`, or node `weights` anywhere in the bounded glTF, including unreachable content;
- exact JSON array/string types for imported names and exact types/values for every fixed export setting;
- no `EXT_mesh_gpu_instancing` declaration, root payload or node payload anywhere in the glTF, including unreachable nodes;
- no `KHR_lights_punctual` declaration, root payload or node payload because the fixed profile has `export_lights=false`;
- no node `camera` payload because the fixed profile has `export_cameras=false`;
- embedded buffers;
- `animations`, `images`, `textures`, and `cameras` either absent or present only as actual empty JSON arrays. Falsy non-array substitutes are invalid.

## Native execution command

Run only through the registered workstation executor when Blender 5.2.2 availability is proven. Use a fresh evidence directory. Example from repository root:

```text
"C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup ^
  --python Scripts\blender_roundtrip_national_mall_panel.py -- ^
  --input Content\Reference\NationalMall\Blockout\mall_core_panel_blockout.gltf ^
  --output <fresh-evidence-dir>\mall_core_panel_blender_roundtrip.gltf ^
  --blend <fresh-evidence-dir>\mall_core_panel_roundtrip.blend ^
  --receipt <fresh-evidence-dir>\blender-roundtrip-receipt.json
```

Retain executable path, `blender --version`, working directory, complete command, timestamps, process exit code, stdout/stderr and pre/post source hash in the native evidence bundle. The driver's JSON receipt is necessary but does not replace shell-level execution evidence.

Then run:

```text
python Scripts/verify_blender_roundtrip_national_mall_panel.py ^
  --source Content/Reference/NationalMall/Blockout/mall_core_panel_blockout.gltf ^
  --manifest Content/Reference/NationalMall/Blockout/expected-manifest.json ^
  --roundtrip <fresh-evidence-dir>/mall_core_panel_blender_roundtrip.gltf ^
  --blend <fresh-evidence-dir>/mall_core_panel_roundtrip.blend ^
  --receipt <fresh-evidence-dir>/blender-roundtrip-receipt.json
```

The verifier intentionally has no tolerance override. After ART-006C is integrated, run the official Khronos glTF-Validator on the round-trip output using the ART-006C evidence adapter. Do not install or download it under this task without separate approval/availability.

## Source-only verification commands

These do not execute Blender:

```text
python Scripts/test_blender_roundtrip_national_mall_panel.py
python Scripts/test_blender_roundtrip_national_mall_panel_hidden_payloads.py
python -m py_compile Scripts/blender_roundtrip_national_mall_panel.py Scripts/verify_blender_roundtrip_national_mall_panel.py Scripts/test_blender_roundtrip_national_mall_panel.py Scripts/test_blender_roundtrip_national_mall_panel_hidden_payloads.py
```

The two focused suites are additive: the original 37-case suite remains intact, and the hidden-payload suite now contains 24 regressions, for 61 focused cases total. The latest repairs validate accessor format/count/buffer/index contracts on unreachable primitives, reject a bufferView whose declared range exceeds the embedded buffer even if the accessor's used bytes still fit, and prove that an attribute drift of `0.00006` is accepted under the documented `1e-4` pairwise tolerance instead of being rejected by quantization bucket boundaries.

## Stop condition

This bounded packet stops when the scripts/profile/task/QA changes are published, both focused standard-library regression suites and Python compilation pass, hosted repository checks are observed, and independent exact-head source review is completed under the existing art-worker gate.

Native Blender execution is a separate future gate. A clean source packet must not be labeled `dcc_roundtrip_executed_not_astral_imported` until real Blender 5.2.2 outputs and shell receipts exist.
