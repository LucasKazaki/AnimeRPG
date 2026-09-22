# QA: R0 package soak telemetry analysis, 2026-09-20

## Candidate and scope

Baseline: PR #9 head `6be62ba27ef754db33d136a1160fa0bbb497e15b`.

Bounded packet: `Tasks/R0-PACKAGE-SOAK-ANALYSIS-2026-09-20.md`.

Final implementation candidate tested by hosted CI: `e9777e843e2b9bbe350705482a1b81e3a6dac6e8`.

This pass adds post-soak evidence validation and descriptive resource-trend analysis only. It does not run Astral, invoke R0, alter engine/game/CMake code, install dependencies, or approve the 24-hour soak.

## Research checked

Primary sources accessed September 20, 2026:

- Unreal Engine 5.8 Memory Insights: https://dev.epicgames.com/documentation/unreal-engine/memory-insights-in-unreal-engine
- Unity 6.0 Memory Profiler package 1.1.9: https://docs.unity3d.com/6000.0/Documentation/Manual/com.unity.memoryprofiler.html
- Unity 6.0 MemoryProfiler API: https://docs.unity3d.com/6000.0/ScriptReference/Unity.Profiling.Memory.MemoryProfiler.html
- Microsoft Process Working Set: https://learn.microsoft.com/en-us/windows/win32/procthread/process-working-set
- Microsoft PROCESS_MEMORY_COUNTERS_EX: https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-process_memory_counters_ex

The implementation therefore reports OS-level trends while explicitly refusing allocator-level or leak-free claims. UE Memory Insights and Unity's Memory Profiler can attribute allocations at a substantially deeper level; Astral does not yet have an equivalent allocator snapshot/callsite system.

## Sandbox evidence before publication

Disposable Linux/Python sandbox, no repository worktree and no Windows native package:

- `python -m py_compile analyze_package_soak_telemetry.py test_package_soak_telemetry_analysis.py`: PASS, exit 0.
- `python test_package_soak_telemetry_analysis.py`: **11/11 PASS**, exit 0, final rerun completed in about 0.6 seconds.
- Covered: exact receipt/telemetry hash binding, summary binding, expected revision/executable binding, non-monotonic elapsed rejection, boolean metric rejection, stale/mutated telemetry rejection, non-boolean pass-state rejection, false acceptance-claim rejection, inconsistent 24-hour claim rejection, synthetic-evidence laundering rejection, descriptive metric statistics, and CLI JSON output/claim guards.
- Analyzer final SHA-256: `9bc4459857f64fea1ed02c28e91d9ca8b0dcd13a089203dd84dae7cdd41c8411`; published Git blob `74be86d7507917549aaed3baf859f54aeabf19dd`.
- Test final SHA-256: `ca756242d99250f5579f27e5e6515cc4676f76427aa076f552e2d63493345feb`; published Git blob `6468135a391c287469d8352c8efe148ddbe31f17`.

The final self-review found and repaired an evidence-laundering edge case before closing the packet: an arbitrary truthy `passed` value or synthetic contract receipt must never be allowed to carry a 24-hour native-duration observation. The analyzer now requires a boolean pass state, a recognized evidence kind, boolean native-observation fields, and, for `required_24h_duration_observed=true`, a passed `native_continuous_package_soak` receipt with at least 86,400 seconds requested and elapsed, native uptime/RAM observations, and post-soak package integrity.

No Astral or package process was started in the sandbox.

## Hosted Windows evidence

Exact candidate `e9777e843e2b9bbe350705482a1b81e3a6dac6e8` ran on GitHub `windows-2022`.

Release-manifest workflow run `35547520654`, job `106176018062`: **SUCCESS**. The hosted job passed the existing manifest, package-runtime, restart-stress, and continuous-soak contract steps, then passed the new `Verify package soak-analysis contracts` step. It also built the Release executable, staged and verified the exact hosted package bytes, and passed the clean tracked-tree check.

Windows build/deterministic workflow run `35547520644`, job `106176025279`: **SUCCESS**. The exact same candidate passed the existing R0 safety, PE dependency, prerequisite, runtime-environment, runtime-compatibility, Redistributable-bootstrap, Release assertion/CTest safety, VS2022 x64 configure, Debug build/tests, Release build/tests, runtime dependency/prerequisite checks, milestone verifiers, and clean tracked-tree gate.

Hosted CI validates the analyzer contracts and confirms the branch still builds/tests. It did not generate a 24-hour native soak, establish a RAM/VRAM/frame-time budget, prove leak freedom, run a clean-machine package launch, establish exclusive interactive-desktop ownership, or provide independent review.

## Remaining local and independent gates

The registered local executor still needs the exact admitted Windows package, prerequisite/bootstrap evidence, package-bound runtime smoke, and the actual 86,400-second continuous run on an exclusively owned supported desktop. Then independent QA should run this analyzer against the retained receipt/JSONL and inspect any material resource growth with engine/allocator-level tools before making a soak decision.

`required_24h_soak_verified`, RAM/VRAM/frame-time budgets, `memory_leak_free_verified`, clean-machine compatibility, owned-desktop verification, and independent acceptance remain unresolved. R0 remains uninvoked by this pass.
