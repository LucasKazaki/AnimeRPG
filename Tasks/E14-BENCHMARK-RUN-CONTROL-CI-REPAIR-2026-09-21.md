# E14 benchmark run-control CI repair packet, 2026-09-21

Status: bounded regression-repair packet on draft PR #9. This is not a new engine feature. It repairs hosted verification failures introduced by the benchmark run-control candidate and stops once the exact candidate is green or a new deterministic blocker is found.

## Baseline and dependency

- Repository: `LucasKazaki/AnimeRPG`.
- Owned branch / draft PR: `repair/2026-09-20-r0-runner-safety`, PR #9.
- Benchmark run-control implementation baseline: `e52289f8f18a90c6b21ca4c7d174b829850d18c1`.
- First Windows failure: run `35606960562`, job `106356293648`. Debug build passed, but deterministic Debug CTest remained in the test step for the full 60-second non-runtime timeout and failed before Release.
- Diagnostic/stability commit `3dd02cb181da05d409dca922bd58df2d0fe696ec` synchronized the narrow and wide Windows CRT environment in the test harness and changed assertion failure from `abort()` to bounded process exit. Windows run `35608156814`, job `106360284341`, still hit the same 60-second Debug CTest timeout.
- Repair commit `fc0ff7d3cd9c84a088aa620ebdc8091880efd3f5` closes the receipt input stream before removing its temporary directory. Windows run `35608525667`, job `106361530889`, then passed the full Debug and Release deterministic CTest steps in about one second each. This establishes the Windows-specific open-file cleanup as the reproduced test-harness blocker. The same run then exposed a second deterministic regression: `Scripts/verify_milestone3.py` failed because its historical literal marker `GetAsyncKeyState('W')` no longer matches the benchmark-safe `keyDown('W')` wrapper in `Win32Application.cpp`.
- Issue #7 remains open and R0 remains uninvoked.

## Research basis

Primary sources rechecked 2026-09-21:

- Microsoft `_putenv_s`, `_wputenv_s`: https://learn.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2013/eyw7eyfw(v=vs.120) . The narrow and wide CRT environment families use `_environ` and `_wenviron`; explicitly driving both views in a Windows test avoids relying on cross-view synchronization behavior.
- Microsoft Windows environment variables: https://learn.microsoft.com/en-us/windows/win32/procthread/environment-variables . Process environment state is distinct from user/system environment configuration; this packet changes only the test process state.
- Microsoft file-handle behavior is platform-specific. The regression was established empirically by the hosted sequence above: keeping the receipt stream open through temporary-directory cleanup timed out under Windows, while closing it before cleanup made the same Debug/Release CTests complete immediately. No production acceptance claim is inferred from this test repair.

## Allowed paths

This repair packet may change only:

1. `Tests/BenchmarkRunControlTests.cpp`
2. `Scripts/verify_milestone3.py`
3. `Tasks/E14-BENCHMARK-RUN-CONTROL-CI-REPAIR-2026-09-21.md`
4. `Docs/QA/E14-BENCHMARK-RUN-CONTROL-CI-REPAIR-2026-09-21.md` after verification

No Engine/Game/CMake/workflow/runtime/package/dependency/renderer/content path is authorized by this repair packet.

## Repair contract

- Preserve the 60-second native test timeout. Do not raise or remove it.
- Preserve the benchmark controller's fail-closed behavior and exact-frame semantics.
- Keep the receipt read handle closed before Windows cleanup so tests do not depend on POSIX unlink semantics.
- Keep narrow and wide CRT test environment views synchronized because production benchmark control uses the wide Windows environment for receipt-path fidelity.
- Replace the stale M3 literal-input marker with semantic markers proving that movement still enters through the current input abstraction: the `keyDown` wrapper must call `GetAsyncKeyState(virtualKey)`, movement must call `keyDown('W')`, and the wrapper must honor `benchmarkRunControl.SuppressLiveInput()`.
- Do not satisfy the verifier with comments or dead strings. The markers must identify executable source expressions in `Win32Application.cpp`.
- Do not invoke R0, merge, release, deploy, install dependencies, touch Company Runtime state, or claim native benchmark acceptance.

## Verification

The exact repaired head must satisfy:

- profiling portability workflow green;
- benchmark environment workflow green;
- Windows Server 2022 Debug and Release builds green;
- Windows deterministic Debug and Release CTests green without changing the 60-second timeout;
- all three static milestone verifiers green;
- Release/package/provenance/reproducibility workflow green;
- clean tracked-tree gate green.

If a new deterministic failure appears, stop and repair only if it remains inside these allowed paths. Otherwise preserve the blocker for the next pass.

## Next action after verification

Return to the parent E14 benchmark run-control packet. The next useful external gate remains the registered-Windows frozen procedural 3D benchmark with 120 warmup frames, 3,600 measured frames, matching timing/phase/memory streams, benchmark environment receipt, run-control receipt, cross-stream coherence verification, and a matched capture-off control. GPU timing/active-adapter proof, VRAM, approved budgets, matched UE5/Unity 3D and genuine-2D workloads, clean-machine launch, recovery stress, the real 86,400-second soak, and independent acceptance remain unresolved.
