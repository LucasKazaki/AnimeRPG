# ART-007 - color/value calibration references

## Objective
Create one deterministic, reviewable color/value reference pack for the proposed art bible without changing renderer, engine, gameplay, build, dependencies, or runtime configuration.

Base dependency: exact ART-003 head `e81f32b8545954b30a205a969bb6f90f0661f7f2`. Use a new art-owned branch/worktree. This packet may add or update only the ART-007 files, `Docs/Agents/art-hourly/STATE.json`, `BACKLOG.json`, and `TOOLCHAIN.md`. Do not merge under this packet.

## Acceptance

```text
python Scripts/generate_color_value_calibration.py --source Content/Calibration/ColorValue/color-roles.json --output <new-dir>
python Scripts/generate_color_value_calibration.py --source Content/Calibration/ColorValue/color-roles.json --output Content/Calibration/ColorValue/Generated --check
python Scripts/verify_color_value_calibration.py Content/Calibration/ColorValue/Generated --source Content/Calibration/ColorValue/color-roles.json --expected-manifest Content/Calibration/ColorValue/expected-manifest.json
python Scripts/test_color_value_calibration.py
python -m py_compile Scripts/generate_color_value_calibration.py Scripts/verify_color_value_calibration.py Scripts/test_color_value_calibration.py
```

Accept only if all pass and the generated manifest bytes exactly equal the checked-in expected manifest. Record generated-file hashes. No runtime color-management or art-approval claim.
