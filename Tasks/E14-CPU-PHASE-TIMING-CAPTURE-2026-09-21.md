# E14 bounded packet: main-thread phase timing capture, 2026-09-21

Status: bounded implementation packet for draft PR #9. Adds opt-in main-thread phase attribution around Astral's existing Win32 loop. It does not establish native GUI performance, GPU timing, profiler parity, an accepted budget, or independent acceptance.

## Authority and baseline

Repository `LucasKazaki/AnimeRPG`, branch `repair/2026-09-20-r0-runner-safety`, observed pre-packet head `bf513e043741dd2b358fd40ddb4c5b607eda87ba`, stacked on PR #6 base `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`.

The prior E14 packets provide bounded whole-frame wall-clock capture plus package-bound analysis. Native 3,600-sample GUI collection remains delegated to the registered Windows executor. This packet advances one independent E14 subrequirement: main-thread phase attribution for message pumping, update/control work, GDI render submission and the existing frame wait. The capability roadmap remains on separate unmerged PR #8; E14 stays partial and E17 blocked. Issue #7 remains open and R0 must not be invoked.

## Primary research, accessed 2026-09-21

- Unreal Engine 5.8 Timing Insights: https://dev.epicgames.com/documentation/unreal-engine/timing-insights-in-unreal-engine . Per-frame performance data includes distinct CPU/GPU tracks and detailed thread/event timelines.
- Unreal Engine 5.8 Timing Panel: https://dev.epicgames.com/documentation/unreal-engine/using-the-timing-panel-in-unreal-insights-for-unreal-engine . CPU/GPU timing events are organized on thread tracks.
- Unreal Engine 5.8 Trace: https://dev.epicgames.com/documentation/unreal-engine/trace-in-unreal-engine-5 . Trace channels include named CPU timers, Frame, Gpu and RenderCommands.
- Unity 6.0 ProfilerMarker: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/Unity.Profiling.ProfilerMarker.html . Named markers instrument arbitrary code blocks and can provide per-frame timings.
- Unity 6.0 CPU Usage Profiler: https://docs.unity3d.com/6000.0/Documentation/Manual/ProfilerCPU.html . Main-thread time is divided into categories such as rendering and scripts with timeline/hierarchy inspection.

Applicability: Astral's existing whole-frame interval cannot identify where the main thread spends time. Four flat phase intervals are a useful first attribution layer but are materially narrower than UE/Unity profiling. Public documentation only; no proprietary source copied and no dependency imported.

## Allowed paths

- `Engine/Core/FramePhaseTimingCapture.h`
- `Engine/Core/FramePhaseTimingCapture.cpp`
- `Engine/Platform/Win32Application.cpp`
- `Tests/FramePhaseTimingCaptureTests.cpp`
- `Tests/FrameTiming/CMakeLists.txt`
- `CMakeLists.txt`
- `.github/workflows/frame-timing-validation.yml`
- `Tasks/E14-CPU-PHASE-TIMING-CAPTURE-2026-09-21.md`
- `Docs/QA/E14-CPU-PHASE-TIMING-CAPTURE-2026-09-21.md`

No renderer implementation, gameplay behavior, graphics API, dependency, scheduler, packaging policy or content change is admitted. Stop on deterministic test/build failure, unexpected diff outside these paths, branch-head movement before publication, or evidence overwrite. Rollback is deletion of the new capture/test/task/QA files plus restoration of the four integration files.

## Implementation contract

`FramePhaseTimingCapture` is disabled unless `ASTRAL_FRAME_PHASE_TIMING_CSV` names a fresh absolute path whose parent exists. Optional controls: `ASTRAL_FRAME_PHASE_TIMING_WARMUP_FRAMES` default 120 and `ASTRAL_FRAME_PHASE_TIMING_MAX_SAMPLES` default 36000, hard cap 1000000.

For each measured Win32 main-loop iteration record contiguous `std::chrono::steady_clock` wall intervals:

1. `message_pump_ms`: loop entry through Win32 message draining.
2. `update_control_ms`: existing Clock tick, title/input/game-state/control work.
3. `render_submit_ms`: existing GDI acquire/clear/render/release work.
4. `frame_wait_ms`: existing `Sleep(1)` through wake-up.
5. `loop_total_ms`: sum of those four intervals.

Do not move or remove existing work to improve numbers. Timestamp calls occur only when enabled. Output permanently labels `cpu_scope=Win32_main_thread_only`, `gpu_timing=unavailable`, and `acceptance_claim=none`; `render_submit_ms` is not GPU time. Reject negative/non-finite phases and all-zero measured rows; discard warmup before validation; bound samples; record saturation; refuse overwrite of final or `.partial` evidence.

## Verification and sandbox evidence

Portable tests must cover paths/sample limits, existing evidence, warmup/schema, invalid values, all-zero rows, saturation, empty capture, output races and environment parsing. Existing whole-frame timing tests and analyzer remain required. Hosted Windows must compile the real `AstralGame` and run the complete non-GUI Debug/Release suite; that is not native GUI measurement.

Disposable Linux partial fixture commands:

```text
g++ -std=c++17 -Wall -Wextra -Wpedantic -I/tmp/astral_phase Engine/Core/FramePhaseTimingCapture.cpp Tests/FramePhaseTimingCaptureTests.cpp -o phase_tests
./phase_tests
clang++ -std=c++17 -Wall -Wextra -Wpedantic -fsanitize=address,undefined -fno-omit-frame-pointer -I/tmp/astral_phase Engine/Core/FramePhaseTimingCapture.cpp Tests/FramePhaseTimingCaptureTests.cpp -o phase_tests_san
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ./phase_tests_san
```

Both executions: `FRAME PHASE TIMING CAPTURE TESTS: PASS (9 groups)`, exit 0. The sandbox has no Windows cross-toolchain, so hosted Windows must establish Win32 integration.

## Native handoff and claim boundary

After the exact package passes existing prerequisite and M10 package-smoke gates on the registered Windows executor, enable both streams for the same frozen workload using separate fresh absolute files, warmup 120 and max samples 3600:

```text
ASTRAL_FRAME_TIMING_CSV=<whole-frame.csv>
ASTRAL_FRAME_TIMING_WARMUP_FRAMES=120
ASTRAL_FRAME_TIMING_MAX_SAMPLES=3600
ASTRAL_FRAME_PHASE_TIMING_CSV=<phase.csv>
ASTRAL_FRAME_PHASE_TIMING_WARMUP_FRAMES=120
ASTRAL_FRAME_PHASE_TIMING_MAX_SAMPLES=3600
```

Retain package/revision hashes, machine/CPU/RAM/GPU/driver identity, resolution, window/VSync settings, exact command/environment, UTC times, stdout/stderr and raw CSV hashes. Retain a matched capture-off control before claiming instrumentation overhead. GPU timestamps, worker/render-thread attribution, RAM/VRAM budgets, approved thresholds, matched UE5/Unity 3D and genuine-2D workloads, clean-machine launch, recovery stress, the 86,400-second soak and independent acceptance remain unresolved.

## Next useful action

If hosted integration is green, run the defined 3,600-sample native package capture with this phase stream enabled. A later bounded packet can validate/analyze the phase CSV. Do not choose parity thresholds before matched reference workloads are frozen and measured.
