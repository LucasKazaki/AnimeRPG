# QA: R0 package soak telemetry analysis, 2026-09-20

## Candidate and scope

Baseline: PR #9 head `6be62ba27ef754db33d136a1160fa0bbb497e15b`.

Bounded packet: `Tasks/R0-PACKAGE-SOAK-ANALYSIS-2026-09-20.md`.

This pass adds post-soak evidence validation and descriptive resource-trend analysis only. It does not run Astral, invoke R0, alter engine/game/CMake code, install dependencies, or approve the 24-hour soak.

## Research checked

Primary sources accessed September 20, 2026:

- Unreal Engine 5.8 Memory Insights: https://dev.epicgames.com/documentation/unreal-engine/memory-insights-in-unreal-engine
- Unity 6.0 Memory Profiler package 1.1.9: https://docs.unity3d.com/6000.0/Documentation/Manual/com.unity.memoryprofiler.html
- Unity 6.0 MemoryProfiler API: https://docs.unity3d.com/6000.0/ScriptReference/Unity.Profiling.Memory.MemoryProfiler.html
- Microsoft Process Working Set: https://learn.microsoft.com/en-us/windows/win32/procthread/process-working-set
- Microsoft PROCESS_MEMORY_COUNTERS_EX: https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-process_memory_counters_ex

The implementation therefore reports OS-level trends while explicitly refusing allocator-level or leak-free claims.

## Sandbox evidence before publication

Disposable Linux/Python sandbox, no repository worktree and no Windows native package:

- `python -m py_compile analyze_package_soak_telemetry.py test_package_soak_telemetry_analysis.py`: PASS, exit 0.
- `python test_package_soak_telemetry_analysis.py`: **9/9 PASS**, exit 0.
- Covered: exact receipt/telemetry hash binding, summary binding, expected revision/executable binding, non-monotonic elapsed rejection, boolean metric rejection, stale/mutated telemetry rejection, false acceptance-claim rejection, inconsistent 24-hour claim rejection, descriptive metric statistics, and CLI JSON output/claim guards.
- Analyzer local SHA-256: `4358a6388b70f5e73c8abe48264b80ffb367a56cafc4a0d3d3a3533f6226c73e`.
- Test local SHA-256: `86a67c34b1f3afc117fdb63e71d148d410478bf5c3b34bf7173f4c5d3699efac`.

The environment printed an unrelated `artifact_tool` spreadsheet-runtime warmup warning during Python startup; both requested commands returned exit code 0 and the unittest suite reported `OK`. No Astral or package process was started.

## Expected hosted gate

`.github/workflows/release-manifest-validation.yml` will run `python Scripts/test_package_soak_telemetry_analysis.py` on `windows-2022` alongside the existing manifest/runtime/restart/continuous-soak contract suites. Hosted success is contract evidence only. It is not a native 24-hour soak, memory budget result, leak proof, package launch, clean-machine test, or independent review.

## Remaining local and independent gates

The registered local executor still needs the exact admitted Windows package, prerequisite/bootstrap evidence, package-bound runtime smoke, and the actual 86,400-second continuous run on an exclusively owned supported desktop. Then independent QA should run this analyzer against the retained receipt/JSONL and inspect any growth with engine/allocator-level tools before making a soak decision.

`required_24h_soak_verified`, RAM/VRAM/frame-time budgets, `memory_leak_free_verified`, clean-machine compatibility, owned-desktop verification, and independent acceptance remain unresolved.
