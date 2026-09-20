# R0 package restart-stress evidence, 2026-09-20

Status: portable contract tests passed in a deliberately partial sandbox fixture; hosted Windows integration is pending for the exact published candidate. No native GUI restart sequence ran in the sandbox.

## Scope and evidence boundary

This packet adds packaging QA only. It does not modify Astral Engine or game code, invoke R0, install dependencies, launch a hosted GUI smoke, approve a restart-stress threshold, or satisfy the required 24-hour soak. The production primitive being orchestrated is the existing package-bound `run_package_runtime_smoke.verify_and_run` wrapper from PR #9.

The research basis is current UE 5.8 Gauntlet/Automation documentation and Unity 6.0 Test Framework/Performance Testing documentation, all accessed September 20, 2026. Epic's automation stack supports monitored packaged sessions and content-stress workflows; Unity exposes PlayMode automation and a released performance-testing extension. Astral's new tool is intentionally narrower: repeat the exact existing packaged runtime smoke in independent fresh working directories and retain failure-safe receipts.

## Sandbox verification

The sandbox could not clone the private repository or run Windows GUI code. It used the exact new harness/test source plus a minimal local stub exposing only the production wrapper's function identity, timeout constants, and error type. This means the sandbox results are unit/contract evidence for the new harness, not integration evidence for `run_package_runtime_smoke.py`.

Executed:

```text
python -m py_compile run_package_restart_stress.py test_package_restart_stress.py
python test_package_restart_stress.py
```

Result: **11/11 tests passed**, exit 0.

Covered contracts:

- bounded integer iteration count;
- fresh/disjoint stress root;
- package and smoke hash mismatch rejection before stress-root creation;
- unique runtime directory and JSON receipt per completed iteration;
- fail-fast behavior at the first bad cycle;
- preservation of earlier receipts after launch interruption;
- preservation of earlier receipts after interruption during the inter-cycle wait;
- inter-cycle delay only between successful iterations;
- dependency-injected fake native claims cannot upgrade the overall native-sequence claim;
- the production package-smoke function is the default runner;
- summary JSON output retains false native/24-hour-soak claims in contract mode.

No package, executable, GUI, VC runtime, frame-time, RAM/VRAM, or soak result was produced by this sandbox test.

## Hosted Windows gate

The PR workflow must run `Scripts/test_package_restart_stress.py` against the real repository modules on `windows-2022`. It may compile/stage the Release package for the existing manifest gate, but it must **not** execute `M10RuntimeSmoke.exe` because hosted CI does not establish an exclusively owned interactive desktop. The hosted result must therefore keep all native restart/soak acceptance claims false.

## Registered-local next action

Once the exact candidate has prerequisite and single-launch package-smoke acceptance on the registered supported Windows desktop, run the command in `Tasks/R0-PACKAGE-RESTART-STRESS-2026-09-20.md` with a new empty stress root. Retain each per-cycle JSON/runtime directory plus the final summary. Stop at the first failure and diagnose it before starting a new clean evidence set.

Even a long repeated-launch pass is not the required 24-hour continuous runtime soak. E15 remains partial, E14 performance budgets remain unmeasured, issue #7 stays open, and merge/release still require registered-local evidence plus independent review.

Sandbox source SHA-256 before publication:

- `run_package_restart_stress.py`: `390d424c8d06dcf68657584bfb6c6e8b3bc29e04ee92068d578903e5de8ee5df`
- `test_package_restart_stress.py`: `a8b842ade0c96fa4fb0c0710a0b944a16f6f886e1c924d18cf76a51eaaf87432`

After publication, compare the Git blobs or fetched source bytes before treating the hosted results as evidence for this exact sandbox-tested implementation.
