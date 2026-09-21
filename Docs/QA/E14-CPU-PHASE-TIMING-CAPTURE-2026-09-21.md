# E14 QA: main-thread phase timing capture, 2026-09-21

Status: implementation checkpoint for draft PR #9. Native Windows GUI measurement, performance acceptance and independent review are not established here.

## Candidate and scope

- Repository: `LucasKazaki/AnimeRPG`
- Branch: `repair/2026-09-20-r0-runner-safety`
- Pre-packet head: `bf513e043741dd2b358fd40ddb4c5b607eda87ba`
- Stacked base: `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`
- Packet: `Tasks/E14-CPU-PHASE-TIMING-CAPTURE-2026-09-21.md`

The implementation is restricted to the packet's nine allowed paths. It adds an opt-in main-thread phase CSV capture, wires it around the existing Win32 loop, registers production-linked tests, and extends the existing portable frame-timing CI subproject. Renderer implementation, graphics API, gameplay contracts, dependencies, Company Runtime and release authority are unchanged.

## Research basis

Primary documentation read on 2026-09-21:

- UE 5.8 Timing Insights: https://dev.epicgames.com/documentation/unreal-engine/timing-insights-in-unreal-engine
- UE 5.8 Timing Panel: https://dev.epicgames.com/documentation/unreal-engine/using-the-timing-panel-in-unreal-insights-for-unreal-engine
- UE 5.8 Trace: https://dev.epicgames.com/documentation/unreal-engine/trace-in-unreal-engine-5
- Unity 6.0 ProfilerMarker: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/Unity.Profiling.ProfilerMarker.html
- Unity 6.0 CPU Usage Profiler: https://docs.unity3d.com/6000.0/Documentation/Manual/ProfilerCPU.html

UE provides per-thread CPU/GPU event tracks; Unity provides named markers plus CPU timeline/hierarchy views. Astral's new evidence remains four flat Win32 main-thread intervals and explicitly leaves GPU timing unavailable. No proprietary source or third-party dependency was used.

## Implemented behavior

When explicitly enabled, `FramePhaseTimingCapture` records message-pump, update/control, GDI render-submission and existing `Sleep(1)` wait wall times plus their derived total. Rows are bounded and post-warmup only. Measured phases must be finite and non-negative with positive total duration. Evidence refuses overwrite, uses a `.partial` plus hard-link publication, and labels `cpu_scope=Win32_main_thread_only`, `gpu_timing=unavailable`, and `acceptance_claim=none`.

No existing update, render or wait operation was moved. The production loop only gains opt-in configuration, conditional `steady_clock` timestamps, recording and final evidence flush. When `ASTRAL_FRAME_PHASE_TIMING_CSV` is absent, phase timing is disabled.

## Sandbox verification

Linux disposable partial fixture, not a full clone or Windows environment:

```text
g++ -std=c++17 -Wall -Wextra -Wpedantic -I/tmp/astral_phase /tmp/astral_phase/Engine/Core/FramePhaseTimingCapture.cpp /tmp/astral_phase/Tests/FramePhaseTimingCaptureTests.cpp -o /tmp/astral_phase/phase_tests
/tmp/astral_phase/phase_tests
```

Result: `FRAME PHASE TIMING CAPTURE TESTS: PASS (9 groups)`, exit 0.

```text
clang++ -std=c++17 -Wall -Wextra -Wpedantic -fsanitize=address,undefined -fno-omit-frame-pointer -I/tmp/astral_phase /tmp/astral_phase/Engine/Core/FramePhaseTimingCapture.cpp /tmp/astral_phase/Tests/FramePhaseTimingCaptureTests.cpp -o /tmp/astral_phase/phase_tests_san
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 /tmp/astral_phase/phase_tests_san
```

Result: `FRAME PHASE TIMING CAPTURE TESTS: PASS (9 groups)`, exit 0, with no sanitizer or leak failure reported.

Covered groups: unsafe paths/sample bounds, existing outputs, warmup/schema, negative/non-finite values, zero-total rows, saturation, empty capture, publication race, and valid/malformed environment configuration.

The sandbox has no Windows SDK/cross-toolchain, so it cannot establish the Win32 integration build.

## Deferred verification

Before repository integration can be claimed:

- hosted portable Debug and optimized Release execution with both timing suites;
- hosted Clang ASan+UBSan/leak execution on the complete checkout;
- hosted Windows real `AstralGame` Debug/Release compilation and non-GUI regression tests;
- final changed-path and clean-tree checks.

Even after those pass, unresolved gates remain: interactive Windows GUI samples, matched capture-off overhead, GPU timestamps, worker/render-thread timelines, RAM/VRAM budgets, accepted thresholds, matched UE5/Unity 3D and genuine-2D workloads, clean-machine launch, failure-recovery stress, continuous 86,400-second soak and independent acceptance.

Issue #7 remains open and R0 was not invoked. Green source/build tests do not authorize merge, release or use of the historical recovery runner.

## Registered local executor handoff

After hosted integration is green and the exact package passes the existing prerequisite and M10 package-smoke gates, run the frozen 3,600-sample procedural workload with both whole-frame and phase streams enabled, warmup 120, separate fresh absolute CSVs, and a matched capture-off control. Preserve source/package hashes, exact command/environment, machine/OS/toolchain/CPU/RAM/GPU/driver identity, resolution/window/VSync settings, UTC times, stdout/stderr, `astral.log`, raw evidence hashes and captured images where applicable. `render_submit_ms` remains main-thread GDI work, not GPU execution time.

## Next action

First require hosted CI to compile and execute this exact candidate. If green, the next native action is the existing 3,600-sample package-bound procedural 3D measurement with this phase stream enabled; validate/analyze phase CSV in a separate bounded packet.
