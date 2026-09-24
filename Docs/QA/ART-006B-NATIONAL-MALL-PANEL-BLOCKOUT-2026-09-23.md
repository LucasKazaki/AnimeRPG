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

The generated glTF is 6,948 bytes, with SHA-256 `5cb742530dda8b474bf4009d26274ce01755fd5033f38d8f2c0d7a3e6e5b95b9`. It contains one canonical 24-vertex / 36-index unit cube payload, three semantic mesh/material bindings, and seven node instances.

## Exact sandbox verification

Executed against the final files before publication:

```text
python Scripts/generate_national_mall_panel_blockout.py --source Content/Reference/NationalMall/Blockout/panel-module-source.json --gltf Content/Reference/NationalMall/Blockout/mall_core_panel_blockout.gltf --manifest Content/Reference/NationalMall/Blockout/expected-manifest.json --check
PASS: deterministic National Mall panel blockout matches pinned files

python Scripts/verify_national_mall_panel_blockout.py --source Content/Reference/NationalMall/Blockout/panel-module-source.json --gltf Content/Reference/NationalMall/Blockout/mall_core_panel_blockout.gltf --manifest Content/Reference/NationalMall/Blockout/expected-manifest.json
PASS: source-only National Mall panel blockout {'nodes': 7, 'materials': 3, 'vertices': 24, 'indices': 36}

python Scripts/test_national_mall_panel_blockout.py
PASS: 15/15 National Mall panel blockout tests

python -m py_compile Scripts/generate_national_mall_panel_blockout.py Scripts/verify_national_mall_panel_blockout.py Scripts/test_national_mall_panel_blockout.py
exit 0
```

The 15 regressions cover valid content, JSON boolean schema confusion, false runtime status, conversion drift, ledger-provenance drift, root-profile expansion, external buffer URIs, node scale drift, node/mesh ownership drift, material drift, decoded geometry corruption, false manifest status, unknown manifest fields, derived-dimension drift, and stale deterministic output detection.

## Final local file pins

- source JSON: `5ec46c554172b0f83859ed6185df07bbd2d35cb89f13fe66d9ac10246d9005b1`
- glTF: `5cb742530dda8b474bf4009d26274ce01755fd5033f38d8f2c0d7a3e6e5b95b9`
- expected manifest: `9d09f96610b37c18b13eb5df0bfe64f93a53ed7fd4c33818d28541469cfe8684`
- generator: `7a76176eecfd81033516387700657617efefb252b4322df331840b565b3808ee`
- verifier: `a4265585786e9e1e7d87d72cec717b637ac60dcf34a0ee4572963eea71491c4d`
- tests: `63e80c420aaea3530d11bed502b589a237bb97c74261a15978a5bf5558a32c81`

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
