# R0 package restart-stress evidence, 2026-09-20

## Why this packet exists

PR #9 now binds one native M10 runtime smoke to the exact packaged `AstralGame.exe`, but one successful launch/input/exit cycle does not exercise repeated process startup, shutdown, fresh working-directory creation, or package immutability across many cycles. This packet adds bounded restart-stress evidence without changing Engine/, Game/, CMake, the R0 runner, or the existing native smoke itself.

Baseline: PR #9 head `435b2bc358551f053f1aeb21f9ff255edad8e3f6`, stacked on PR #6. E15 remains partial and issue #7 remains open.

## Allowed paths

- `Scripts/run_package_restart_stress.py`
- `Scripts/test_package_restart_stress.py`
- `.github/workflows/release-manifest-validation.yml`
- `Tasks/R0-PACKAGE-RESTART-STRESS-2026-09-20.md`
- `Docs/QA/R0-PACKAGE-RESTART-STRESS-2026-09-20.md`

No Engine/, Game/, Tests/, CMake, R0-runner, runtime database, scheduler, dependency installation, package publication, architecture, merge, or release changes are authorized by this packet.

## Primary-source basis

Accessed September 20, 2026:

1. Epic Games, **Gauntlet Automation Framework Overview**, UE 5.8: https://dev.epicgames.com/documentation/unreal-engine/gauntlet-automation-framework-overview-in-unreal-engine . Gauntlet runs and monitors packaged-engine sessions and supports sequential, parallel, and dependent automated tests. Astral's bounded restart sequence is a much smaller packaging QA primitive, not a Gauntlet equivalent.
2. Epic Games, **Running Gauntlet Tests**, UE 5.8: https://dev.epicgames.com/documentation/unreal-engine/running-gauntlet-tests-in-unreal-engine . UE includes packaged-client boot/automation workflows and optional resume behavior after critical failures. Astral should retain exact per-cycle receipts and fail at the first bad cycle rather than treating one prior success as durable runtime evidence.
3. Epic Games, **Automation Test Framework**, UE 5.8: https://dev.epicgames.com/documentation/unreal-engine/automation-test-framework-in-unreal-engine . Epic distinguishes unit, feature, smoke, and content-stress tests and advises tests not to assume prior state. Astral therefore uses a new external runtime directory for every iteration.
4. Unity 6.0, **Test Framework**: https://docs.unity3d.com/6000.0/Documentation/Manual/com.unity.test-framework.html . Unity supports EditMode and PlayMode automation; this is a capability reference, not code to copy.
5. Unity 6.0, **Performance testing API**, package 3.2.0: https://docs.unity3d.com/6000.0/Documentation/Manual/com.unity.test-framework.performance.html . Unity's released performance-test extension collects performance and configuration metadata. Astral's restart harness does not yet collect comparable frame-time or hardware metrics and must not claim E14 performance parity.

No proprietary engine source is copied and no new third-party dependency is introduced.

## Acceptance contract

Add a dependency-free restart harness around the production `run_package_runtime_smoke.verify_and_run` implementation that:

- validates the expected 40-hex revision, exact package executable SHA-256, exact external M10 smoke SHA-256, manifest placement, package/smoke file identity, and timeout bounds before creating its stress-output root;
- requires a fresh empty stress root disjoint from the immutable package;
- allows only 2-1000 bounded iterations and a 0-3600 second pause between cycles;
- allocates a distinct fresh `runtime-XXXX` directory for each iteration;
- invokes the existing package-bound runtime-smoke wrapper for every iteration, preserving all of its manifest/hash/PASS-marker/process-tree checks;
- writes one JSON receipt per completed iteration and stops at the first failed cycle;
- preserves prior completed receipts when interrupted while launching an iteration or waiting between iterations;
- records total requested/completed cycles, elapsed time, failed iteration, per-cycle receipt paths and claim state;
- permits `repeated_native_package_launch_sequence_verified=true` only on real Windows when the production wrapper itself is used and every iteration independently reports `package_launch_verified=true`;
- keeps `restart_stress_acceptance_approved`, `required_24h_soak_verified`, `clean_machine_compatibility_verified`, `owned_interactive_desktop_verified`, and `independent_acceptance` false by design.

Hosted CI runs contract tests only. It must not execute GUI runtime smoke or claim a native restart sequence. A repeated-launch sequence is deliberately distinct from the required 24-hour soak because it starts and exits a fresh process each cycle rather than keeping one game process alive continuously.

## Local evidence handoff

After package manifest, prerequisite, runtime-compatibility, bootstrap, and one native package-smoke receipt all pass for the exact admitted candidate on an exclusively owned supported Windows desktop, use a fresh stress root and run a bounded sequence such as:

```powershell
python Scripts/run_package_restart_stress.py `
  <package>\MANIFEST.json <package> `
  --expected-commit <40-hex-admitted-revision> `
  --expected-executable-sha256 <independently-recorded-AstralGame-sha256> `
  --smoke-executable <external-build>\Release\M10RuntimeSmoke.exe `
  --expected-smoke-sha256 <independently-recorded-M10RuntimeSmoke-sha256> `
  --stress-root <fresh-external-stress-directory> `
  --iterations 100 `
  --interval-seconds 1 `
  --timeout-seconds 60 `
  --json <external-evidence>\package-restart-stress.json
```

The example count is a useful stress run, not a hardcoded acceptance threshold. Retain the summary JSON, every iteration JSON and runtime directory/log, machine/OS/toolchain identity, prerequisite receipts, package/smoke hashes, exact command, and timestamps. QA must decide whether the observed sequence is sufficient restart-stress evidence. The separate 24-hour continuous soak, performance/RAM/VRAM budgets, clean-machine acceptance, and independent review remain unresolved.

## Stop and rollback

Stop on malformed or mismatched hashes/revision, unsafe paths, a nonempty stress root, any package-smoke failure, package mutation, timeout, missing M10 PASS marker, interruption, or inability to preserve receipts. Do not skip a failed iteration or rerun into the same stress root. Rollback is the restart-stress commit(s) on PR #9. Do not invoke R0 merely because this harness exists.
