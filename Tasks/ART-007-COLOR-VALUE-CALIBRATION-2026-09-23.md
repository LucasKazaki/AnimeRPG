# ART-007 - color/value calibration references

## Objective
Create one deterministic, reviewable color/value reference pack for the proposed art bible without changing renderer, engine, gameplay, build, dependencies, or runtime configuration.

Base dependency: exact ART-003 head `e81f32b8545954b30a205a969bb6f90f0661f7f2`. Use a new art-owned branch/worktree. This packet may add or update only the ART-007 files, `Docs/Agents/art-hourly/STATE.json`, `BACKLOG.json`, and `TOOLCHAIN.md`. Do not merge under this packet.

Derived PNGs are deliberately generated outside the source tree. `color-roles.json`, the generator, verifier, tests, and `expected-manifest.json` are the source contract. Do not check in a second generated pack that can drift from the pin.

Text-contract portability rule: source and expected-manifest hashing/comparison canonicalize CRLF or CR line endings to LF before hashing or byte comparison. This keeps the artifact contract stable on Windows checkouts with `core.autocrlf=true` without broadening this packet to repository-wide `.gitattributes` changes. Generated `manifest.json` remains deterministic LF output.

## Acceptance

```text
python Scripts/generate_color_value_calibration.py --source Content/Calibration/ColorValue/color-roles.json --output <new-dir>
python Scripts/generate_color_value_calibration.py --source Content/Calibration/ColorValue/color-roles.json --output <new-dir> --check
python Scripts/verify_color_value_calibration.py <new-dir> --source Content/Calibration/ColorValue/color-roles.json --expected-manifest Content/Calibration/ColorValue/expected-manifest.json
python Scripts/test_color_value_calibration.py
python -m py_compile Scripts/generate_color_value_calibration.py Scripts/verify_color_value_calibration.py Scripts/test_color_value_calibration.py
```

Accept only if all pass, the fresh generated manifest equals the checked-in expected manifest after the documented newline canonicalization, and the regression suite confirms there is no stale source-tree `Generated/` pack. The suite must also prove CRLF-checkout portability and reject a repinned false runtime-validation status. Record fresh generated-file hashes. No runtime color-management or art-approval claim.
