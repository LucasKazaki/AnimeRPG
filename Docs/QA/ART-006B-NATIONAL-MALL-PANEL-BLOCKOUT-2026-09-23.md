# ART-006B QA: National Mall core-panel source blockout

**Date:** 2026-09-23  
**Loop:** `astral-art-hourly-20260922`  
**Parent:** PR #45 exact head `00f30f6973f852bf1f53d64b25075a916bbb7f42`  
**Evidence class:** sandbox source-generation/validation only

## Delivered source asset

The packet creates one self-contained glTF 2.0 blockout module for a typical National Mall core lawn panel. It is actual triangle mesh data, not a rendered image or a claimed Astral screenshot.

Source-backed authoring dimensions:

- lawn: 137.16 m longitudinal x 51.816 m cross-axis;
- path width: 10.668 m;
- grove-envelope width: 39.624 m;
- outer path footprint: 158.496 m x 73.152 m;
- total cross-axis module envelope: 152.4 m.

The generated glTF is 6,947 bytes, with SHA-256 `6c51463332199c65bcfbde04ee8e5883e03a94aba710980eebfaa6945f2759b7`. It contains one canonical 24-vertex / 36-index unit cube payload, three semantic mesh/material bindings, and seven node instances.

The verifier owns a separate canonical geometry contract rather than deriving its expected payload from the generator. It decodes and checks the exact POSITION, NORMAL, TANGENT, TEXCOORD_0 and index payloads, fixed buffer-view/accessor layout, triangle winding, geometric normals, UV-derived +U tangents, and `cross(N,T) * w` +V bitangent handedness. This independently catches generator-side geometry defects such as the repaired bottom-face tangent-sign error.

## Exact sandbox verification

Executed against the final candidate file contents before publication:

```text
python Scripts/generate_national_mall_panel_blockout.py --source Content/Reference/NationalMall/Blockout/panel-module-source.json --gltf Content/Reference/NationalMall/Blockout/mall_core_panel_blockout.gltf --manifest Content/Reference/NationalMall/Blockout/expected-manifest.json --check
PASS: deterministic National Mall panel blockout matches pinned files

python Scripts/verify_national_mall_panel_blockout.py --source Content/Reference/NationalMall/Blockout/panel-module-source.json --gltf Content/Reference/NationalMall/Blockout/mall_core_panel_blockout.gltf --manifest Content/Reference/NationalMall/Blockout/expected-manifest.json
PASS: source-only National Mall panel blockout {'nodes': 7, 'materials': 3, 'vertices': 24, 'indices': 36}

python Scripts/test_national_mall_panel_blockout.py
PASS: 17/17 National Mall panel blockout tests

python -m py_compile Scripts/generate_national_mall_panel_blockout.py Scripts/verify_national_mall_panel_blockout.py Scripts/test_national_mall_panel_blockout.py
exit 0
```

The 17 regressions cover valid content, JSON boolean schema confusion, false runtime status, conversion drift, ledger-provenance drift, root-profile expansion, external buffer URIs, node scale drift, node/mesh ownership drift, material drift, decoded geometry corruption, repinned tangent-handedness corruption, tangent-accessor contract drift, false manifest status, unknown manifest fields, derived-dimension drift, and stale deterministic output detection.

## Final local file pins

- source JSON: `5ec46c554172b0f83859ed6185df07bbd2d35cb89f13fe66d9ac10246d9005b1`
- glTF: `6c51463332199c65bcfbde04ee8e5883e03a94aba710980eebfaa6945f2759b7`
- expected manifest: `7bb35caa97898bf6cdac69670e3d773c7ea6fde4dd279c0fc6ff0077c98d5f30`
- generator: `5fc74009b76cdae7c3f23ee903a83624a413b5c5918495b978bb6756c665959b`
- verifier: `391511820bdd634c16cf27f8a58d5dffa20c95796edd68ee7fc18b4038ad0a08`
- tests: `b48ad4ad38f923ffc3cb0f7d18850dec55158cff3147a1420320c6760a501fdc`

## Evidence boundary and remaining gates

Not run or claimed:

- Blender or another DCC import/export round trip;
- Khronos validator executable;
- Astral import, scene placement or rendering;
- actual grass, gravel, foliage or texture production;
- collision, LOD, occlusion, streaming or nav setup;
- native Windows/GPU frame-time, RAM or VRAM measurement;
- artist or independent visual approval;
- geographic survey accuracy;
- UE5, Unity, Genshin Impact or Zenless Zone Zero parity.

The next dependency-ready step after source review is a DCC round trip of this exact glTF or, when engine ownership supplies the supported path, Astral import/render acceptance. Bulk Mall asset production remains premature until one of those end-to-end paths is proven.
