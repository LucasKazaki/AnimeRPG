# ART-006C: Khronos glTF validator evidence gate

**Loop:** `astral-art-hourly-20260922`  
**Parent:** ART-006B / PR #49 exact head `c3571ed68c5b97d23d6acacecfeda8987d49a3ff`  
**Status:** bounded external-tool qualification and evidence-adapter packet; no DCC, Astral runtime, or visual-approval claim  
**Live `main` observed before branch creation:** `1a575690df46ba5cff7e00b01a15f4256a844113`

## Goal

Close one concrete tooling gap in the ART-006 representative asset path without installing software or pretending the official validator ran. ART-006B already has an independent project-local structural verifier. This packet adds a strict adapter for the official Khronos glTF-Validator JSON report so a future authorized workstation/CI invocation can produce evidence bound to the exact source asset rather than a screenshot or hand-copied summary.

The intended path becomes:

`ART-006B pinned glTF -> official Khronos validator -> JSON report -> this strict evidence adapter -> future DCC round trip -> future Astral import/render -> art/performance review`.

## Allowed paths

- `Scripts/verify_khronos_gltf_validator_report.py`
- `Scripts/test_khronos_gltf_validator_report.py`
- `Tasks/ART-006C-KHRONOS-GLTF-VALIDATOR-GATE-2026-09-24.md`
- `Docs/QA/ART-006C-KHRONOS-GLTF-VALIDATOR-GATE-2026-09-24.md`

Do not edit ART-006B source outputs, `Engine/`, renderer/editor/importer/gameplay code, CMake, workflows, dependencies, another worker's branch, or `Docs/Agents/art-hourly/*`.

## Tool qualification

KhronosGroup/glTF-Validator is the selected primary specification validator for this gate. Official upstream evidence checked 2026-09-24:

- upstream repository: `KhronosGroup/glTF-Validator`, active with a 2026-09-18 push observed during qualification;
- license: Apache-2.0;
- purpose: validate assets against glTF 2.0 and emit JSON reports with issue counts and asset statistics;
- CLI contract: `gltf_validator [options] <input>`, `--stdout` emits JSON to stdout, resource validation defaults on, and a non-zero return code indicates at least one error;
- upstream report schema requires `validatorVersion` and `issues`, with `numErrors`, `numWarnings`, `numInfos`, `numHints`, `messages`, and `truncated` inside `issues`;
- current upstream report implementation also emits asset statistics including animation/material counts, morph/skin/texture/default-scene flags, draw calls, total vertices/triangles and maximum UV/influence/attribute counts;
- upstream web frontend operates client-side, but this task does not upload or validate project assets through a hosted page.

No package, binary, Dart SDK, npm dependency, or external service is installed or invoked by this packet. The exact validator version available on Lucas's authorized workstation remains `not_measured` until a native receipt exists.

## Acceptance contract

The adapter must fail closed unless all of the following hold:

1. The report is structurally bounded to the known upstream fields used by this gate.
2. `validatorVersion` is a semver string and `mimeType` is `model/gltf+json`.
3. The report URI resolves to the exact target asset path, not merely the same filename.
4. Every issue count and severity is a real JSON integer, not a Python-equal boolean.
5. Message severities exactly reproduce the report summary counts and output is not truncated.
6. Error count is zero and warnings stay at or below the explicit limit, default zero.
7. glTF version is `2.0`.
8. The ART-006B self-contained asset has no validator-reported external resource, and every reported resource has a recognized storage mode.
9. Current upstream info statistic fields are accepted with strict integer/boolean typing when present.
10. The source asset SHA-256 equals the explicitly supplied expected pin. A detached report still requires a native invocation receipt to prove which bytes produced it.
11. Focused regressions and `py_compile` pass before publication.

## Future authorized validator command

When the Khronos executable is already available through an authorized workstation/runtime, run it without installing anything and retain the raw report. The upstream CLI supports stdout JSON; exact executable path/version must be captured in the native receipt. Run from the repository root so the report URI and adapter resolve the same input path. A representative command shape is:

```text
gltf_validator --stdout --all Content/Reference/NationalMall/Blockout/mall_core_panel_blockout.gltf > <evidence>/mall_core_panel_blockout.report.json
```

Then bind that report to ART-006B's pinned asset:

```text
python Scripts/verify_khronos_gltf_validator_report.py \
  --asset Content/Reference/NationalMall/Blockout/mall_core_panel_blockout.gltf \
  --report <evidence>/mall_core_panel_blockout.report.json \
  --expected-sha256 6c51463332199c65bcfbde04ee8e5883e03a94aba710980eebfaa6945f2759b7
```

Capture executable version/path, working directory, pre/post asset SHA-256, raw JSON report, command, exit code, and this adapter's result.

## Stop condition

Stop this packet once the adapter is tested, independently reviewed, and ready for integration review. Do not claim Khronos acceptance until the official validator itself has actually run against the exact ART-006B asset. Blender/DCC, Astral import/render, native GPU performance, visual-art approval, and top-tier parity remain separate gates.
