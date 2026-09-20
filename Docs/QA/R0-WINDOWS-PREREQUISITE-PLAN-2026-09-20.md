# R0 Windows prerequisite policy evidence, 2026-09-20

Status: portable policy tests and hosted Windows verification passed for the published
candidate. Clean-machine package execution and independent review remain separate gates.

## Baseline and observed dependency evidence

Implementation baseline was PR #9 head `3a27de91e2cfecf40b19639134a39e93ac807bb1`.
Policy implementation was published as `adaccc7b402756ef0cb13068eb6fb1d08a2917ff`.
The immediately preceding hosted Release inventory recorded AMD64 `AstralGame.exe`
SHA-256 `6b808fc5145d7d420c46c6660c94c8cad042074f0e541cd5356c5a9e1b5a1969` importing
`MSVCP140.dll`, `VCRUNTIME140.dll`, and `VCRUNTIME140_1.dll`, with no Debug CRT imports.
That evidence requires a VC runtime deployment decision, not a portability claim.

## Implemented policy

`Scripts/plan_windows_prerequisites.py` converts a schema-1 PE dependency report into
an explicit package prerequisite record. For current x64 VC runtime imports it selects
`central_vc_redist`, identifies the Microsoft Visual C++ v14 Redistributable family,
records the official Microsoft download/deployment references, and keeps these fields
false: `installer_bundled`, `installer_executed`, `app_local_fallback_selected`,
`static_runtime_switch_selected`, and `clean_machine_compatibility_verified`.

The planner checks that classified runtime names are actually present in the report's
`all_imports`, rejects unknown PE machine types, and refuses a dependency report that
has already marked clean-machine compatibility true. This preserves the separation
between static import evidence and actual target-machine execution.

## Portable execution receipt

Executed in a disposable Linux sandbox using Python 3, without third-party packages:

```text
python -m py_compile plan_windows_prerequisites.py test_windows_prerequisite_plan.py
python test_windows_prerequisite_plan.py
```

Result: **8/8 tests passed**, exit 0.

A second CLI run used a JSON fixture matching the prior hosted Astral Release evidence:
AMD64 plus `MSVCP140.dll`, `VCRUNTIME140.dll`, and `VCRUNTIME140_1.dll`, image SHA-256
`6b808fc5145d7d420c46c6660c94c8cad042074f0e541cd5356c5a9e1b5a1969`. The planner
returned exit 0 and selected `central_vc_redist`, architecture `x64`, with clean-machine
verification and installer execution/bundling all false.

These are policy/parser checks, not Windows launch evidence and not a test of the
Microsoft installer itself.

## Hosted Windows receipt

GitHub Actions run https://github.com/LucasKazaki/AnimeRPG/actions/runs/35522471810,
job `106108814814`, completed **SUCCESS** for candidate
`adaccc7b402756ef0cb13068eb6fb1d08a2917ff` on `windows-2022`.

Verified steps included:

- R0 safety contracts: pass.
- PE dependency inspector contracts: pass.
- Windows prerequisite planner contracts: pass, **8/8** portable planner tests inside
  the published test file.
- Release assertion/CTest safety contracts: pass.
- Visual Studio 2022 x64 configure: pass.
- full Debug build and deterministic non-GUI tests: pass.
- full Release build and deterministic non-GUI tests: pass.
- real Release `AstralGame.exe` dependency inspection: pass.
- real Release prerequisite planning: pass.
- explicit current-policy checks: `central_vc_redist`, `x64`, no installer bundled or
  executed, and clean-machine compatibility still false: pass.
- static milestone verifiers and tracked-tree cleanliness check: pass.

The hosted run proves that the planner accepts the freshly compiled Release dependency
report and that the reviewed current policy remains central v14 redistributable
deployment. It does not prove that a redistributable installer is available, licensed
for a particular delivery mechanism, installed on a target, or sufficient for a clean
machine launch.

## Remaining acceptance

No Microsoft binary was downloaded, installed, copied, bundled, or executed. The R0
runner was not invoked. A clean Windows machine was not available to this pass.
Before E15 clean-machine acceptance, the registered local path still needs an approved
redistributable bootstrap/delivery mechanism and a finished-package launch on a
supported Windows host with exact OS/runtime/package evidence. Interactive native
smokes, independent review, and the required soak remain open.
