# ART-007 author QA

Assessment: **source/reference calibration only.** This pass defines reproducible palette/value/LUT diagnostics and checks their data contracts. It does not establish Astral tonemapping, HDR, color grading, renderer correctness, native GPU behavior, visual parity, or independent art approval.

Primary-source research used Epic UE 5.8 color-grading/tonemapper guidance, Unity 6 URP tonemapping/color-adjustment guidance, and W3C relative-luminance/contrast guidance as a UI screening reference. No engine/vendor assets were copied.

## Review repair, 2026-09-23

The first independent source review found four defects in exact head `e891abe6090d4b1b23684ceb0209bff8a6bd807e`: malformed checked-in PNG bytes, unbounded decompression dimensions, sparse palette/ramp semantic checks, and a grayscale preview calculated directly from gamma-encoded sRGB values. Head `8b87654f036dfd0e7804281b966d973b8a7296ac` repaired the algorithms, but the follow-up exact-head review found three remaining acceptance defects: tracked PNG bytes still did not match the repaired manifest, tests never exercised the repository artifact boundary, and the verifier did not validate the manifest's `ui_screening` records.

This repair removes the stale derived `Generated/` pack from source control. The generator plus `expected-manifest.json` are now the source contract; reviewers generate into a fresh external directory and verify that its manifest bytes exactly equal the pin. This avoids presenting stale binary derivations as source assets. The regression suite asserts that no `Content/Calibration/ColorValue/Generated` directory is tracked, verifies a fresh pack against the checked-in pin, and rejects repinned false UI-screening metadata. The independent verifier now requires the exact pair identities, thresholds, and rounded ratios computed from the source roles.

## Author verification executed on repaired source fixture

```text
python Scripts/generate_color_value_calibration.py --source Content/Calibration/ColorValue/color-roles.json --output <new-dir>
PASS: 3 color/value calibration PNGs; astral-color-calibration-2

python Scripts/generate_color_value_calibration.py --source Content/Calibration/ColorValue/color-roles.json --output <same-dir> --check
PASS

python Scripts/verify_color_value_calibration.py <same-dir> --source Content/Calibration/ColorValue/color-roles.json --expected-manifest Content/Calibration/ColorValue/expected-manifest.json
PASS: {'roles': 8, 'png_files': 3, 'ui_pairs': 4}

python Scripts/test_color_value_calibration.py
14/14 passed

python -m py_compile Scripts/generate_color_value_calibration.py Scripts/verify_color_value_calibration.py Scripts/test_color_value_calibration.py
PASS
```

Fresh generated hashes pinned by `expected-manifest.json`:
- `palette_card.png`: `4fa15493ed523ec5624f325d0ebe2b4cd45a814a1150a364464da10f9fcf0e5c` (2287 bytes)
- `neutral_lut_16.png`: `b9c2e13d85416055d65c6eb9d9a42e297515123c52b8dfd2a9d040458e1b256a` (6969 bytes)
- `value_ramp_16.png`: `7c61e5e17eb2873dd0ae3fc861cb5e51881a08c2492962f2e553f8698c810226` (362 bytes)
- expected manifest: `730bd19f4c1f7cb8296991d3cd868144e403dade1055c0984924159792de3ff4`
- `color-roles.json`: `21e44310b4de21e085736b1a49a17705e6794285db7bede76ff0423322f96a22`

The 14-test suite includes absence of a stale source-tree derived pack, fresh-pack equality with the checked-in pin, valid temporary output, exact regeneration, overwrite refusal, stale-manifest rejection, contrast rejection, full-pixel semantic corruption in swatch/preview/ramp regions, LUT replacement, oversized-dimension rejection before inflation, corrected vermilion luminance preview, and repinned false UI-screening metadata rejection.

Author inspection of a freshly generated palette confirmed the intended swatches, relative-luminance preview region and 16-step value ladder. This is author inspection only. The earlier Windows CI success for `8b87654f...` does not certify this newer repair. Fresh exact-head hosted CI and a fresh independent review are required after publication.
