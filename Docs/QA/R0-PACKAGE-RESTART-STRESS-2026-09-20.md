# R0 package restart-stress evidence, 2026-09-20

Status: portable contract tests and exact-published-source hosted Windows contract tests pass. No native GUI restart sequence has been run by this packet. Registered-local repeated-launch evidence, the separate 24-hour continuous soak, performance budgets, and independent acceptance remain pending.

## Scope and evidence boundary

This packet adds packaging QA only. It does not modify Astral Engine or game code, invoke R0, install dependencies, launch a hosted GUI smoke, approve a restart-stress threshold, or satisfy the required 24-hour soak. The production primitive being orchestrated is the existing package-bound `run_package_runtime_smoke.verify_and_run` wrapper from PR #9.

The research basis is UE 5.8 Gauntlet/Automation documentation and Unity 6.0 Test Framework/Performance Testing documentation, accessed September 20, 2026. Epic's automation stack supports monitored packaged sessions and content-stress workflows; Unity exposes PlayMode automation and a released performance-testing extension. Astral's new tool is intentionally narrower: repeat the exact existing packaged runtime smoke in independent fresh working directories and retain failure-safe receipts.

## Sandbox verification

The sandbox could not clone the private repository or run Windows GUI code. It used the new harness/test source plus a minimal local stub exposing only the production wrapper's function identity, timeout constants, and error type. This makes the sandbox result unit/contract evidence for the new harness, not integration evidence for `run_package_runtime_smoke.py`.

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

Prepublication sandbox SHA-256 values were:

- `run_package_restart_stress.py`: `390d424c8d06dcf68657584bfb6c6e8b3bc29e04ee92068d578903e5de8ee5df`
- `test_package_restart_stress.py`: `a8b842ade0c96fa4fb0c0710a0b944a16f6f886e1c924d18cf76a51eaaf87432`

The published harness Git blob is `5f0661c61a87dd11edcfd7f5285c00a7aa164de7` and matches the implementation exercised before publication. The published test-file Git blob is `c573d48031d48e49e15371870611c4a379ca1fc4`; its exact published bytes were exercised by the hosted Windows run below. Therefore the hosted run, not the prepublication local test-file hash, is the authoritative receipt for the published test suite.

## Hosted Windows verification

GitHub Actions **Release manifest integrity** run `35541470122`, job `106159816715`, completed **SUCCESS** on Windows Server 2022 / `windows-2022` image `20260913.307.1`. GitHub checked out PR merge ref `877c2d41f067d23ec4dda5137447607afa8598c7`, which contains PR #9 head `8c78496fcf418dc452fea2a450e9597b2c71960d` merged into its current base for pull-request validation.

Exact hosted results:

- release-manifest contracts: **7/7 passed**;
- package runtime-smoke wrapper contracts: **11/11 passed**;
- exact published package restart-stress contracts: **11/11 passed**;
- the restart suite explicitly retained `repeated_native_package_launch_sequence_verified=false` and `required_24h_soak_verified=false`;
- VS2022 x64 Release configure/build passed with Windows SDK `10.0.26100.0`, MSVC `19.44.35228.0`, toolset `14.44.35207`;
- staged hosted `AstralGame.exe`: **69,632 bytes**, SHA-256 `be26a6414690a8f81ee6d9c773d77699f5856c969b638cc362ecffaa3e4bae57`;
- schema-v2 manifest aggregate: `ba3b6a51119eb6187911fa31f3555357e9ae249f45112e8d0024eaa08573ff3d`;
- package-manifest verification and clean tracked-tree check passed;
- the hosted package receipt correctly kept package launch, clean-machine compatibility, and independent acceptance false.

The separate **Windows build and deterministic tests** run `35541470106`, job `106159818369`, also completed **SUCCESS** for the same candidate. It passed every existing R0 safety, PE dependency, Windows prerequisite, runtime-environment, runtime-compatibility, Redistributable-bootstrap, Release assertion/CTest safety, VS2022 configure, Debug build/test, Release build/test, static milestone-verifier, and clean-tree step.

Hosted CI deliberately did **not** execute `M10RuntimeSmoke.exe`. GitHub-hosted Windows does not establish an exclusively owned interactive desktop and is not registered-local native acceptance.

## Registered-local next action

Once the exact admitted candidate has prerequisite and single-launch package-smoke acceptance on the registered supported Windows desktop, run the command in `Tasks/R0-PACKAGE-RESTART-STRESS-2026-09-20.md` with a new empty stress root. A 100-cycle sequence is a useful initial stress sample, not a hardcoded acceptance threshold. Retain each per-cycle JSON/runtime directory plus the final summary and stop at the first failure before starting another clean evidence set.

Even a long repeated-launch pass is not the required 24-hour continuous runtime soak. E15 remains partial, E14 frame-time/RAM/VRAM budgets remain unmeasured, issue #7 stays open, and merge/release still require registered-local evidence plus independent review.
