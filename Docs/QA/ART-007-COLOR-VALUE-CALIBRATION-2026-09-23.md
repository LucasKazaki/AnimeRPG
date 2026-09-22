# ART-007 author QA

Assessment: **source/reference calibration only.** This pass defines reproducible palette/value/LUT diagnostics and checks their data contracts. It does not establish Astral tonemapping, HDR, color grading, renderer correctness, native GPU behavior, visual parity, or independent art approval.

Primary-source research used Epic UE 5.8 color-grading/tonemapper guidance, Unity 6 URP tonemapping/color-adjustment guidance, and W3C relative-luminance/contrast guidance as a UI screening reference. No engine/vendor assets were copied.

## Review repair, 2026-09-23

The first independent source review found four defects in exact head `e891abe6090d4b1b23684ceb0209bff8a6bd807e`: malformed checked-in PNG bytes, unbounded decompression dimensions, sparse palette/ramp semantic checks, and a grayscale preview calculated directly from gamma-encoded sRGB values. The repair regenerates the exact checked-in PNGs, bounds dimensions and inflated bytes before decompression, scans every palette/ramp pixel, and computes relative luminance in linear light before re-encoding a neutral sRGB preview. Generator version is now `astral-color-calibration-2`.

## Author verification executed on repaired source fixture

```text
python Scripts/generate_color_value_calibration.py --source Content/Calibration/ColorValue/color-roles.json --output <new-dir>
PASS: 3 color/value calibration PNGs; astral-color-calibration-2

python Scripts/generate_color_value_calibration.py --source Content/Calibration/ColorValue/color-roles.json --output <same-dir> --check
PASS

python Scripts/verify_color_value_calibration.py <same-dir> --source Content/Calibration/ColorValue/color-roles.json --expected-manifest Content/Calibration/ColorValue/expected-manifest.json
PASS: {'roles': 8, 'png_files': 3, 'ui_pairs': 4}

python Scripts/test_color_value_calibration.py
11/11 passed

python -m py_compile Scripts/generate_color_value_calibration.py Scripts/verify_color_value_calibration.py Scripts/test_color_value_calibration.py
PASS
```

Repaired generated hashes:
- `palette_card.png`: `4fa15493ed523ec5624f325d0ebe2b4cd45a814a1150a364464da10f9fcf0e5c`
- `neutral_lut_16.png`: `b9c2e13d85416055d65c6eb9d9a42e297515123c52b8dfd2a9d040458e1b256a`
- `value_ramp_16.png`: `7c61e5e17eb2873dd0ae3fc861cb5e51881a08c2492962f2e553f8698c810226`
- generated and checked-in expected manifest: `730bd19f4c1f7cb8296991d3cd868144e403dade1055c0984924159792de3ff4`
- `color-roles.json`: `21e44310b4de21e085736b1a49a17705e6794285db7bede76ff0423322f96a22`

The 11-test suite includes valid output, exact regeneration, overwrite refusal, stale-manifest rejection, contrast rejection, full-pixel semantic corruption in swatch/preview/ramp regions, LUT replacement, oversized-dimension rejection before inflation, and a regression for the corrected vermilion luminance preview.

Author inspection of the regenerated palette confirmed the intended swatches, relative-luminance preview region and 16-step value ladder. This is author inspection only. Exact-head hosted CI and a fresh independent review are required after publication of the repair.
