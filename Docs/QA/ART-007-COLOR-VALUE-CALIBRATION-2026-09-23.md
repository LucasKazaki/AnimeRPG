# ART-007 author QA

Assessment: **source/reference calibration only.** This pass defines reproducible palette/value/LUT diagnostics and checks their data contracts. It does not establish Astral tonemapping, HDR, color grading, renderer correctness, native GPU behavior, visual parity, or independent art approval.

Primary-source research used Epic UE 5.8 color-grading/tonemapper guidance, Unity 6 URP tonemapping/color-adjustment guidance, and W3C relative-luminance/contrast guidance as a UI screening reference. No engine/vendor assets were copied.

## Review repair, 2026-09-23

The first independent source review found four defects in exact head `e891abe6090d4b1b23684ceb0209bff8a6bd807e`: malformed checked-in PNG bytes, unbounded decompression dimensions, sparse palette/ramp semantic checks, and a grayscale preview calculated directly from gamma-encoded sRGB values. Head `8b87654f036dfd0e7804281b966d973b8a7296ac` repaired those algorithms, but the follow-up exact-head review found three remaining acceptance defects: tracked PNG bytes still did not match the repaired manifest, tests never exercised the repository artifact boundary, and the verifier did not validate the manifest's `ui_screening` records.

Head `5fbb1e8acf9c979702dca9c509d9499fdfda45ae` removed the stale derived `Generated/` pack, made the generator plus `expected-manifest.json` the source contract, added repository-boundary regressions, and required exact UI-screening metadata. Its exact-head Windows CI passed, but the next independent review found two more source-contract issues: raw newline-sensitive hashing/pin comparison could fail on Windows `core.autocrlf=true` checkouts, and a repinned manifest could falsely claim runtime validation because `status` was not semantically checked.

Head `34ed461572de3da860c63fb9a2358210e7ebffb6` canonicalized source and expected-manifest text line endings to LF before hashing or pin comparison and required exact source-only manifest status `art_reference_source_validated_not_runtime`. Its exact-head Windows CI passed. The subsequent independent review found two evidence defects: the stale-pin regression changed `status`, so the semantic status check could make the test pass without exercising expected-manifest enforcement, and the durable toolchain table still reported the original seven-test count.

Head `a90be95389e04a5dffa2f31212e3a66c636572ed` changed `test_stale_pin` to alter only `color_note`, metadata that remains semantically acceptable to the verifier, and required the exact `expected manifest pin` failure. `TOOLCHAIN.md` was corrected to sixteen tests. Exact-head Windows CI `35805036809` passed. The next independent review found one remaining boundary defect: the canonical `color-roles.json.status` itself could be changed to a runtime-validation claim and then repinned because the generator and verifier did not require the source contract's own source-only status.

The current repair adds an explicit source-status boundary to both generator and verifier. `color-roles.json.status` must be exactly `proposed_art_reference_not_runtime`. Two new regressions independently prove that the generator rejects a false runtime source status and that the verifier rejects the same false source status even when the source hash and expected manifest are deliberately repinned to match it. No generated PNG bytes or pinned hashes changed.

## Author verification executed on repaired source fixture

```text
python Scripts/generate_color_value_calibration.py --source Content/Calibration/ColorValue/color-roles.json --output <new-dir>
PASS: 3 color/value calibration PNGs; astral-color-calibration-2

python Scripts/generate_color_value_calibration.py --source Content/Calibration/ColorValue/color-roles.json --output <same-dir> --check
PASS

python Scripts/verify_color_value_calibration.py <same-dir> --source Content/Calibration/ColorValue/color-roles.json --expected-manifest Content/Calibration/ColorValue/expected-manifest.json
PASS: {'roles': 8, 'png_files': 3, 'ui_pairs': 4}

python Scripts/test_color_value_calibration.py
18/18 passed

python -m py_compile Scripts/generate_color_value_calibration.py Scripts/verify_color_value_calibration.py Scripts/test_color_value_calibration.py
PASS
```

Fresh generated hashes remain pinned by `expected-manifest.json`:
- `palette_card.png`: `4fa15493ed523ec5624f325d0ebe2b4cd45a814a1150a364464da10f9fcf0e5c` (2287 bytes)
- `neutral_lut_16.png`: `b9c2e13d85416055d65c6eb9d9a42e297515123c52b8dfd2a9d040458e1b256a` (6969 bytes)
- `value_ramp_16.png`: `7c61e5e17eb2873dd0ae3fc861cb5e51881a08c2492962f2e553f8698c810226` (362 bytes)
- expected manifest: `730bd19f4c1f7cb8296991d3cd868144e403dade1055c0984924159792de3ff4`
- canonical LF `color-roles.json`: `21e44310b4de21e085736b1a49a17705e6794285db7bede76ff0423322f96a22`

The 18-test suite includes absence of a stale source-tree derived pack, fresh-pack equality with the checked-in pin, a semantically valid but unpinned manifest rejection, CRLF-checkout contract portability, generator and verifier source-status rejection, valid temporary output, exact regeneration, overwrite refusal, repinned false manifest-runtime-status rejection, contrast rejection, full-pixel semantic corruption in swatch/preview/ramp regions, LUT replacement, oversized-dimension rejection before inflation, corrected vermilion luminance preview, and repinned false UI-screening metadata rejection.

Author inspection of a freshly generated palette confirmed the intended swatches, relative-luminance preview region and 16-step value ladder. This is author inspection only. Fresh exact-head hosted CI and a fresh independent review are required after publication of this repair.
