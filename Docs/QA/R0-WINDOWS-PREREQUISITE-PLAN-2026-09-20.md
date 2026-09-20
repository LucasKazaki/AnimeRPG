# R0 Windows prerequisite policy evidence, 2026-09-20

Status: portable policy tests passed before publication. Hosted Windows verification
for the exact GitHub candidate remains required after commit.

## Baseline and observed dependency evidence

Baseline PR #9 head: `3a27de91e2cfecf40b19639134a39e93ac807bb1`.
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

## Hosted contract added

Windows CI now runs the eight planner tests. After building Release it reuses the real
`inspect_pe_dependencies.py` output from `AstralGame.exe`, creates a prerequisite plan,
and checks that the current dynamically linked build resolves to central VC v14
redistributable deployment while preserving the unverified/bundled/executed flags.
A change in runtime linkage therefore requires an explicit packaging-policy update.

## Remaining acceptance

No Microsoft binary was downloaded, installed, copied, bundled, or executed. The R0
runner was not invoked. A clean Windows machine was not available to this pass.
Before E15 clean-machine acceptance, the registered local path still needs an approved
redistributable bootstrap/delivery mechanism and a finished-package launch on a
supported Windows host with exact OS/runtime/package evidence. Interactive native
smokes, independent review, and the required soak remain open.
