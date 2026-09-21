# E14 bounded process-memory capture packet, 2026-09-21

Status: admitted bounded instrumentation packet on draft PR #9. This packet adds opt-in process-memory evidence for benchmark runs. It does not invoke R0, establish a RAM budget, prove a memory leak is absent, measure VRAM, merge/release/deploy anything, install a dependency, change the graphics architecture, or restart paused game-content work.

## Identity and dependency

- Repository: `LucasKazaki/AnimeRPG`.
- Owned branch / existing draft PR: `repair/2026-09-20-r0-runner-safety`, PR #9.
- Pre-packet branch head: `c3429ac0e576f2301564f9318caad22f7d27008c`.
- Stacked dependency retained by PR #9: PR #6 / `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`.
- Issue #7 remains open. The historical R0 runner must not be invoked by this packet.
- Capability-map impact: E14 `Profiling and budgets`, process RAM evidence only. E00-E13 and E15-E17 remain unresolved according to their existing evidence; this packet does not alter their acceptance state.

This packet is dependency-ready as an E14 instrumentation improvement because it does not depend on R0 execution or a graphics-backend change. Native benchmark acceptance still depends on the registered Windows executor, an exclusively owned interactive desktop, exact package provenance, and later independent review.

## Reproducible gap

Astral's current E14 capture can record whole-frame and Win32 main-thread phase wall times, but the production application has no per-frame process-memory stream for the same frozen benchmark interval. The existing external soak sampler is useful for long-run process telemetry, but it does not provide a benchmark-loop-owned, frame-indexed RAM stream that can be bound alongside the timing CSVs.

Acceptance for this packet is therefore limited to creating a fail-closed, bounded process-memory evidence stream with truthful semantics and regression coverage. No threshold is approved here.

## Allowed paths

Only these paths may change in this packet:

1. `Engine/Core/ProcessMemoryCapture.h`
2. `Engine/Core/ProcessMemoryCapture.cpp`
3. `Engine/Core/Clock.h`
4. `Engine/Core/Clock.cpp`
5. `Tests/ProcessMemoryCaptureTests.cpp`
6. `Tests/FrameTiming/CMakeLists.txt`
7. `CMakeLists.txt`
8. `.github/workflows/frame-timing-validation.yml`
9. `Tasks/E14-PROCESS-MEMORY-CAPTURE-2026-09-21.md`
10. `Docs/QA/E14-PROCESS-MEMORY-CAPTURE-2026-09-21.md`

The initial task draft named `Win32Application.cpp`, but inspection showed that `Clock::Tick()` already owns the frame index and whole-frame capture. The packet was narrowed before application integration so process sampling can share that existing frame identity without touching the larger platform loop. `Win32Application.cpp` is not modified by this packet.

No dependency, renderer/API, game-content, Company Runtime, scheduler, repository-permission, release, or deployment path is authorized.

## Primary-source research

Sources rechecked on 2026-09-21 before implementation:

1. Microsoft Learn, `PROCESS_MEMORY_COUNTERS_EX` (`psapi.h`): https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-process_memory_counters_ex
   - `WorkingSetSize` is the current working-set size in bytes; `PeakWorkingSetSize` is its lifetime peak. `PrivateUsage` is process commit charge, the total private committed memory for the running process.
   - Applicability: Astral can retain OS process-level resident and private-commit evidence without claiming allocator ownership or leak attribution.
2. Microsoft Learn, `GetProcessMemoryInfo`: https://learn.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-getprocessmemoryinfo
   - Retrieves process memory counters and returns zero on failure.
   - Applicability: use the current Astral process handle and `PROCESS_MEMORY_COUNTERS_EX`; sampler failure must block evidence publication.
3. Epic Games, Unreal Engine 5.8, Memory Insights: https://dev.epicgames.com/documentation/unreal-engine/memory-insights-in-unreal-engine
   - UE tracks allocation/free events, callstacks, LLM tags, memory growth/decline, and leak-oriented queries.
   - Applicability: Astral's OS counters are a much narrower baseline and must not be called equivalent allocator profiling.
4. Epic Games, Unreal Engine 5.8, Low-Level Memory Tracker: https://dev.epicgames.com/documentation/unreal-engine/using-the-low-level-memory-tracker-in-unreal-engine
   - UE can tag engine/OS allocations and emit memory CSV data.
   - Applicability: longer-term Astral parity requires tagged allocation tracking, not only process totals.
5. Unity 6.0 / 6000.0, Memory Profiler package 1.1.9: https://docs.unity3d.com/6000.0/Documentation/Manual/com.unity.memoryprofiler.html
   - Unity provides allocation-focused memory profiling for Unity 6000.0.
   - Applicability: Astral's process counters are only an initial measurable RAM layer.

No proprietary engine source is copied. No third-party package is imported. The Windows implementation uses documented OS APIs and the Windows SDK already required by the native project. `Psapi.lib` is a Windows system import library, not a new redistributed dependency.

## Implementation contract

Add an opt-in `ProcessMemoryCapture` with environment configuration:

- `ASTRAL_PROCESS_MEMORY_CSV`: absolute fresh output path; absence means disabled.
- `ASTRAL_PROCESS_MEMORY_WARMUP_FRAMES`: default `120`.
- `ASTRAL_PROCESS_MEMORY_SAMPLE_EVERY_FRAMES`: default `1`, bounded to `[1, 1000000]`.
- `ASTRAL_PROCESS_MEMORY_MAX_SAMPLES`: default `36000`, bounded to `[1, 1000000]`.

On Windows, sampled frames use `GetProcessMemoryInfo(GetCurrentProcess(), PROCESS_MEMORY_COUNTERS_EX)` and retain:

- frame index;
- current working-set bytes;
- peak working-set bytes;
- `PrivateUsage` / private commit-charge bytes;
- page-fault count.

The stream must label its scope and limitations explicitly: current process only, OS counters, allocator attribution unavailable, VRAM unavailable, leak detection not established, no performance-budget claim, and no acceptance claim. Output uses a fresh `.partial` file followed by no-overwrite publication, matching the existing E14 evidence style. Any measured invalid sample or production-sampler failure blocks final publication. Warmup and non-stride frames are ignored before validation.

`Clock` owns both the existing whole-frame stream and the new process-memory capture. On each `Tick()`, both receive the same monotonically increasing frame index before it advances. The memory sample therefore represents process state at that frame's `Clock::Tick()` boundary, not allocator attribution or GPU residency. Capture remains off unless explicitly requested.

## Verification contract

Portable contract tests must cover:

- invalid/relative paths and sample limits;
- existing final/partial output refusal;
- warmup and sampling stride;
- exact scope/claim metadata;
- invalid working-set/peak relation;
- saturation marker and bounded storage;
- no-sample refusal;
- output race refusal without overwrite;
- sampler-source misuse/failure guard;
- environment-disabled state.

Windows coverage must additionally execute the real `GetProcessMemoryInfo` production path against the test process and publish a fresh receipt. Register the native test with `astral_add_test`. Extend the existing portable timing subproject so GNU Debug, optimized Release, and Clang ASan+UBSan/leak checking exercise the same production capture source and tests.

Required full hosted regression after publication:

- the updated portable profiling workflow;
- full Windows Debug and Release non-GUI builds/tests;
- existing R0 safety tests without invoking R0;
- existing package/provenance/reproducibility workflow;
- tracked-tree cleanliness.

## External outputs and local handoff

All configure/build output must remain outside the source tree. Hosted CI uses runner temporary directories. The registered local executor should use distinct external Debug/Release roots and a fresh absolute evidence directory.

After prerequisite/package-smoke gates pass for the exact admitted package, the intended native benchmark is a frozen procedural 3D run with 120 warmup frames and 3,600 measured frames, collecting whole-frame timing, phase timing, and process-memory CSVs together plus a matched capture-off control. Retain source/package SHA, machine/OS/toolchain/CPU/RAM/GPU/driver identity, resolution/window/VSync configuration, exact environment/command, UTC timestamps, stdout/stderr, runtime logs, raw evidence hashes, and captured images where required by the existing benchmark controls.

That native run is not performed by this coordinator and is not implied by this task file.

## Rollback and stop conditions

- Stop on the first deterministic compile/test/invariant failure until it is understood and repaired within these allowed paths.
- Do not weaken an existing acceptance or claim guard to make a check pass.
- If the real Windows sampler cannot be verified in hosted CI, preserve the failure and do not claim production-memory evidence.
- Rollback is limited to reverting this packet's commits/paths. Do not rewrite branch history or discard another worker's changes.
- Keep PR #9 draft, issue #7 open, R0 uninvoked, and all native/clean-machine/24-hour-soak/independent-acceptance gates unresolved unless separate direct evidence exists.
