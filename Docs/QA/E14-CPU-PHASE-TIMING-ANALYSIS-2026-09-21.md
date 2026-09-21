# E14 QA: main-thread phase timing analysis, 2026-09-21

Status: implementation checkpoint for draft PR #9. No native Windows GUI benchmark, accepted performance budget, matched Unreal/Unity result, or independent acceptance is claimed by this record.

## Scope and provenance

- Repository: `LucasKazaki/AnimeRPG`
- Branch: `repair/2026-09-20-r0-runner-safety`
- Pre-packet head: `6fe02ad75d943a048398d781091ef6c65d00379f`
- Stacked base: `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`
- Packet: `Tasks/E14-CPU-PHASE-TIMING-ANALYSIS-2026-09-21.md`

This packet is restricted to five allowed paths: the new phase analyzer, its Python regression suite, the existing frame-timing workflow, this QA record, and the task contract. It does not edit Engine/Game/CMake source, alter the renderer or graphics API, import a dependency, change package authority, touch Company Runtime state, restart content work, merge, release, deploy, or invoke R0.

PR #9 remained open/draft at packet admission and issue #7 remained open. The capability map is still on separate unmerged PR #8, where E14 profiling remains partial and E17 comparative acceptance remains unresolved.

## Research basis

Primary documentation accessed 2026-09-21:

- Unreal Engine 5.8 Timing Insights: https://dev.epicgames.com/documentation/unreal-engine/timing-insights-in-unreal-engine
- Unreal Engine 5.8 Timing Panel: https://dev.epicgames.com/documentation/unreal-engine/using-the-timing-panel-in-unreal-insights-for-unreal-engine
- Unreal Engine 5.8 Timers and Counters: https://dev.epicgames.com/documentation/unreal-engine/using-the-timers-and-counters-tabs-in-unreal-insights-for-unreal-engine
- Unity 6.0 `ProfilerMarker`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/Unity.Profiling.ProfilerMarker.html
- Unity 6.0 Profiler API: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/Profiling.Profiler.html
- Unity 6.0 CPU Usage Profiler: https://docs.unity3d.com/6000.0/Documentation/Manual/ProfilerCPU.html

Applicability: UE exposes per-thread CPU/GPU event tracks and aggregate/export tooling; Unity exposes named markers plus timeline/hierarchy views and warns that profiling instrumentation affects performance. Astral's current phase capture is intentionally narrower: four flat Win32 main-thread wall-clock intervals. This analyzer makes that limited evidence reproducible and fail-closed; it does not turn it into profiler parity.

## Implementation behavior

`Scripts/analyze_frame_phase_timing_capture.py` validates only the exact phase-capture schema currently emitted by Astral. It requires fixed schema/metric/timing/scope/claim metadata, the exact six-column header, canonical non-negative six-decimal millisecond values, post-warmup contiguous frames, bounded sample/saturation metadata, and a positive loop total.

The four serialized phase durations must sum to serialized `loop_total_ms` within at most 3 microseconds. This tolerance exists only to cover independent six-decimal serialization rounding of the four phase values and total; it is not a general tolerance for altered evidence.

For valid data the analyzer computes min, mean, p50, p95, p99, max and sum for message-pump, update/control, GDI render-submission, frame-wait and loop-total intervals, plus aggregate phase shares of loop total. `render_submit_ms` is explicitly main-thread GDI submission work, not GPU execution time, and phase share is not CPU utilization.

The bound-analysis path re-verifies the existing production benchmark manifest and release/package binding, requires the exact expected commit and `AstralGame.exe` SHA-256, requires exactly one `cpu_phase_timing_csv` evidence role, rechecks evidence size/hash, rejects unsafe/symlinked evidence paths, and writes only a fresh analysis file. It retains hard-false claims for performance budget, comparative parity, instrumentation overhead, GPU timing, worker/render-thread attribution and independent acceptance.

## Disposable sandbox verification

A disposable Linux partial-source fixture contained the exact analyzer/test files but intentionally omitted repository `benchmark_manifest.py` and `release_manifest.py`. Commands:

```text
python -m py_compile /tmp/analyze_frame_phase_timing_capture.py /tmp/test_frame_phase_timing_analysis.py
cd /tmp && python test_frame_phase_timing_analysis.py
```

Results before publication:

- compile check: PASS, exit 0;
- 13 tests discovered;
- 10 parser/output/mutation tests: PASS;
- 3 production-binding tests: intentional SKIP because repository manifest modules were absent from the partial fixture;
- overall: `OK (skipped=3)`, exit 0, approximately 0.079 s;
- 250 seeded one-byte mutations either failed closed or revalidated against the complete schema/invariant set.

This is tooling execution only. It is not Windows evidence, native GUI execution, GPU evidence, a package launch, or an independent review.

## Hosted verification gate

The complete repository workflow must run `Scripts/test_frame_phase_timing_analysis.py` so all 13 tests execute, including integration with production `release_manifest.py` and `benchmark_manifest.py`, tampered raw-evidence rejection and exact candidate identity checks. The existing whole-frame analyzer tests plus Debug/optimized Release C++ capture contracts and Clang ASan+UBSan/leak checks must remain green. No test may be weakened to obtain a pass.

Hosted source/build success still does not establish the registered local Windows GUI measurement, capture overhead, clean-machine launch, GPU timing, RAM/VRAM budget, continuous 86,400-second soak or independent acceptance.

## Registered local executor handoff

After the exact package passes the existing prerequisite and M10 package-smoke gates, the registered Windows executor should run the already-defined frozen procedural 3D workload for 3,600 post-warmup samples with both whole-frame and phase streams enabled, plus a matched capture-off control. Preserve source/package hashes, machine/OS/toolchain/CPU/RAM/GPU/driver identity, resolution/window/VSync configuration, exact command/environment, UTC times, stdout/stderr, logs and raw evidence hashes.

Bind the immutable phase CSV into the benchmark manifest using role `cpu_phase_timing_csv`, then run this analyzer against that exact package/manifest/evidence set. Preserve the resulting JSON separately and do not edit raw CSV evidence to satisfy validation.

## Remaining claim boundary and next action

GPU timestamps, hierarchical or worker/render-thread attribution, RAM/VRAM budgets, approved thresholds, matched UE5/Unity 3D and genuine-2D workloads, clean-machine package launch, failure-recovery stress, the required 86,400-second soak and independent acceptance remain unresolved.

Next: obtain hosted complete-repository verification for this exact candidate. If green, the next native action remains the 3,600-sample registered-Windows run and immutable phase analysis. If native execution is still unavailable, a later independent E14 packet can research GPU timestamp feasibility for Astral's current graphics architecture without changing that architecture or adding dependencies.
