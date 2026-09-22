# E14 bounded packet: CPU frame-interval capture, 2026-09-21

Status: implementation prepared and portable sandbox verified; hosted Windows verification pending at packet creation. Native package/GPU acceptance and independent review remain pending.

## Authority, dependency, and baseline

Repository: `LucasKazaki/AnimeRPG`. This packet reuses draft PR #9 on branch
`repair/2026-09-20-r0-runner-safety`; the observed branch head before this packet was
`95b6d612abcd88b777728e4af8fe8e764daa7e93`, stacked on PR #6 base
`e12e6c559bf776ffc9c715c809a517f8e02ce5d5`.

This is an E14 profiling/evidence packet. It does not invoke R0, merge, release,
deploy, install a dependency, change the GDI graphics architecture, restart paused
game content, or modify the Company Runtime. Issue #7 remains open and its native
Windows plus independent-review requirements are not satisfied by this packet.

The capability roadmap is still unmerged on PR #8. This packet advances only the
E14 evidence subrequirement for bounded CPU frame-interval capture. E00-E13 and
E15-E17 remain unchanged; E16's full feature-catalogue audit remains incomplete.
No broad UE5/Unity parity claim follows.

## Reproducible gap

Astral's main loop already calls `Clock::Tick()` once per frame and displays an
approximately one-second FPS average, but it retains no frame-by-frame timing
samples. That cannot produce p50/p95/p99 distributions, bind a later benchmark to
raw timing evidence, or expose individual stalls. The E14 roadmap explicitly
requires reproducible frame-time evidence before performance comparisons.

This packet records one narrow metric: the wall-clock interval in milliseconds
between consecutive `Clock::Tick()` calls. In the current Win32 loop that interval
includes work occurring between ticks, including simulation, GDI rendering,
message processing and the existing `Sleep(1)`. It is not a GPU timestamp, not an
isolated main-thread work duration, not render-thread timing, and not Present-wait
attribution.

## Primary-source research, accessed 2026-09-21

1. Epic Games, Unreal Engine 5.8, **Timing Insights**:
   https://dev.epicgames.com/documentation/en-us/unreal-engine/timing-insights-in-unreal-engine
   Timing Insights retains frame-by-frame data and exposes separate CPU/GPU tracks
   and timing events. Applicability: Astral needs raw per-frame evidence before a
   future profiler can attribute spikes. License constraint: documentation was used
   only as a behavioral reference; no Unreal source or proprietary implementation
   was copied.
2. Epic Games, Unreal Engine 5.8, **Timing Panel**:
   https://dev.epicgames.com/documentation/en-us/unreal-engine/using-the-timing-panel-in-unreal-insights-for-unreal-engine
   Applicability: CPU/GPU attribution is a later E14 requirement and is explicitly
   not claimed by this CPU interval packet.
3. Unity Technologies, Unity 6.0, **FrameTimingManager**:
   https://docs.unity3d.com/6000.0/Documentation/ScriptReference/FrameTimingManager.html
   It captures multiple frames and exposes CPU/GPU timing data. Applicability:
   Astral's bounded capture should retain multiple raw samples, but this packet does
   not claim Unity-equivalent timing detail.
4. Unity Technologies, Unity 6.0, **FrameTiming** and `cpuFrameTime`:
   https://docs.unity3d.com/6000.0/Documentation/ScriptReference/FrameTiming.html
   https://docs.unity3d.com/6000.0/Documentation/ScriptReference/FrameTiming-cpuFrameTime.html
   Unity distinguishes total CPU frame time from main/render-thread and GPU timing.
   Applicability: Astral's CSV is therefore labeled `cpu_frame_interval_ms` and
   `gpu_timing=unavailable` rather than laundering this interval into GPU or detailed
   thread timing.

The implementation keeps Astral's existing C++17 `std::chrono::steady_clock`
source instead of changing timer architecture. No external library is added.

## Allowed paths

Only these paths are admitted for this packet:

- `CMakeLists.txt`
- `Engine/Core/Clock.h`
- `Engine/Core/Clock.cpp`
- `Engine/Core/FrameTimingCapture.h`
- `Engine/Core/FrameTimingCapture.cpp`
- `Tests/FrameTimingCaptureTests.cpp`
- `Tests/FrameTiming/CMakeLists.txt`
- `.github/workflows/frame-timing-validation.yml`
- `Tasks/E14-CPU-FRAME-TIMING-CAPTURE-2026-09-21.md`
- `Docs/QA/E14-CPU-FRAME-TIMING-CAPTURE-2026-09-21.md`

Rollback is deletion/reversion of only this packet's added paths and the small
Clock/CMake registrations. Stop on any deterministic test failure, unexpected
source-head movement, unrelated dirty state, or evidence overwrite condition.

## Contract and implementation

Default execution remains measurement-off. If `ASTRAL_FRAME_TIMING_CSV` is unset,
`Clock::Tick()` keeps its existing return semantics and no timing file is created.
A requested capture requires an absolute output path whose parent already exists.
The final path and its `.partial` companion must not already exist.

Optional settings:

- `ASTRAL_FRAME_TIMING_WARMUP_FRAMES`, default `120`, unsigned decimal.
- `ASTRAL_FRAME_TIMING_MAX_SAMPLES`, default `36000`, range `1..1000000`.

Measured intervals are kept in a bounded pre-reserved buffer. Warmup frames are
not evidence and do not poison the run if an immediate first clock interval is
zero. A non-finite or non-positive post-warmup interval blocks publication. Hitting
the sample cap is explicit in metadata and never grows the buffer beyond the cap.

Normal `Clock` destruction writes a complete `.partial` file, flushes and closes
it, then publishes the final path through a hard link. This intentionally fails if
another file has appeared at the final path, avoiding POSIX rename-overwrite races.
If publication or partial cleanup fails, the capture reports failure to stderr and
must not be treated as accepted evidence.

CSV metadata identifies schema, milliseconds, exact wall-clock interval semantics,
`std::chrono::steady_clock` as the source, warmup/sample limits, saturation,
`gpu_timing=unavailable`, and `acceptance_claim=none`. Rows contain
`frame_index,cpu_frame_interval_ms` with six decimal places.

## Acceptance tests for this packet

Portable contract tests must cover:

- empty/relative/missing-parent path rejection;
- zero and over-hard-cap sample limits;
- existing final/partial evidence rejection;
- warmup exclusion and bounded measured samples;
- discarded warmup values not becoming measured evidence;
- invalid measured interval blocking;
- explicit saturation without buffer growth;
- no-sample publication rejection;
- output race/no-overwrite behavior;
- actual `Clock` environment integration and destructor flush;
- malformed environment setting rejection.

Required source-level gates are C++17 Debug, optimized Release, and Clang
ASan+UBSan with leak checks using `Tests/FrameTiming`. The top-level target is
registered through `astral_add_test`, so the existing hosted Windows Debug and
Release non-GUI CTest lanes must execute it without weakening Release assertions.

## Sandbox execution

The sandbox used a dedicated git worktree `automation/e14-frame-timing` built from
a partial source fixture containing the exact pre-packet Clock files plus the new
capture/test files. It was not a full clone and is not Windows/native GPU evidence.
External build roots were `/mnt/data/frame5-Debug`, `/mnt/data/frame5-Release`, and
`/mnt/data/frame5-sanitize`.

Executed successfully after final self-review:

```text
cmake -S Tests/FrameTiming -B /mnt/data/frame5-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build /mnt/data/frame5-Debug --parallel
ctest --test-dir /mnt/data/frame5-Debug --output-on-failure -V

cmake -S Tests/FrameTiming -B /mnt/data/frame5-Release -DCMAKE_BUILD_TYPE=Release
cmake --build /mnt/data/frame5-Release --parallel
ctest --test-dir /mnt/data/frame5-Release --output-on-failure -V

cmake -S Tests/FrameTiming -B /mnt/data/frame5-sanitize -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=clang++ -DASTRAL_FRAME_TIMING_SANITIZERS=ON
cmake --build /mnt/data/frame5-sanitize --parallel
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  ctest --test-dir /mnt/data/frame5-sanitize --output-on-failure -V
```

All three CTest runs passed `11/11` named contract groups in one executable, with
exit code 0. These tests use the production `Clock.cpp` and capture implementation,
not a mock. Sanitizers reported no failure in this bounded fixture.

## Native/local handoff and claim boundary

Hosted CI may establish cross-platform contracts plus real MSVC Debug/Release
build/test compatibility. It cannot establish real Astral GUI frame timing because
GitHub's hosted lane does not own the required interactive desktop and this packet
does not launch the GUI there.

The registered local executor should first satisfy the existing package prerequisite
and M10 package-smoke gates for the exact admitted package. For an initial E14
capture, use an exclusively owned supported Windows desktop and a fresh external
evidence directory. A proposed, not accepted performance threshold, fixture is:

```powershell
$env:ASTRAL_FRAME_TIMING_CSV = '<absolute-external-evidence-path>\\idle-default-scene.csv'
$env:ASTRAL_FRAME_TIMING_WARMUP_FRAMES = '120'
$env:ASTRAL_FRAME_TIMING_MAX_SAMPLES = '3600'
& '<exact-admitted-package>\\AstralGame.exe'
```

Leave the default procedural scene idle with no input until more than 3600
post-warmup ticks have occurred, then close normally so the Clock destructor can
publish evidence. Retain exact package/revision hashes, command/environment,
Windows/CPU/RAM/GPU/driver identity, client resolution/window mode/display refresh,
UTC timestamps, CSV SHA-256, `astral.log`, and the existing benchmark manifest.
The output should show `samples_saturated=1` if the cap was actually exceeded.

The 120-frame warmup and 3600-sample cap are fixture parameters, not approved UE5/
Unity parity thresholds. Quantify instrumentation overhead with a matched capture-off
control before using results for comparative claims. Later E14 work still needs GPU
and finer CPU attribution, RAM/VRAM, allocation/stall evidence, matched 3D and 2D
workloads, percentile analysis, and approved budgets. E15 clean-machine launch,
issue #7 independent/native acceptance, stress/recovery, and the 86,400-second soak
remain separate gates.
