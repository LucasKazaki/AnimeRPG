# E14 CPU frame timing capture QA, 2026-09-21

## Checkpoint

This record closes one bounded implementation packet only: adding opt-in, bounded raw CPU frame-interval capture to Astral's existing `Clock` path so later E14 benchmark work can retain frame-by-frame evidence. It does not establish UE5/Unity performance parity, GPU timing, clean-machine compatibility, native interactive acceptance, or completion of E14.

- Repository: `LucasKazaki/AnimeRPG`
- Draft PR: #9, `repair/2026-09-20-r0-runner-safety` onto `audit/2026-09-19-test-safety-and-direction`
- Base/dependency revision: `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`
- Pre-packet branch head: `95b6d612abcd88b777728e4af8fe8e764daa7e93`
- Implementation commit: `a47e7c9b5b37f26ed0d4deb93adb99bca2a0ac26`
- Hosted PR synthetic merge revision used by Windows CI: `ba8032752a1e03e81d43e0ffc4cb2f279286b48a`
- Issue #7 remains open. R0 was not invoked.

The packet contract is `Tasks/E14-CPU-FRAME-TIMING-CAPTURE-2026-09-21.md`.

## Why this packet

Before this change, Astral's main loop called `Clock::Tick()` once per frame and retained only an approximately one-second FPS average for display. It did not retain raw per-frame timing samples, so a later benchmark could not calculate distributions such as p50/p95/p99, bind a performance claim to the actual samples, or inspect individual stalls.

This packet records one deliberately narrow metric: the wall-clock interval in milliseconds between consecutive `Clock::Tick()` calls. With the current Win32 loop, that interval spans the work between ticks, including simulation, GDI rendering, message processing and the existing `Sleep(1)`. It is not an isolated main-thread work duration, render-thread duration, GPU duration, or Present-wait attribution.

## Primary-source comparison research

Primary sources were read on 2026-09-21.

1. Epic Games, Unreal Engine 5.8, **Timing Insights**: https://dev.epicgames.com/documentation/en-us/unreal-engine/timing-insights-in-unreal-engine
   - Timing Insights retains frame-by-frame timing data and exposes CPU/GPU timing tracks and events.
   - Applicability: Astral needs raw per-frame evidence before later tooling can attribute stalls or compare distributions.
   - Constraint: documentation was used only as behavioral reference. No Unreal proprietary source was copied.
2. Epic Games, Unreal Engine 5.8, **Timing Panel in Unreal Insights**: https://dev.epicgames.com/documentation/en-us/unreal-engine/using-the-timing-panel-in-unreal-insights-for-unreal-engine
   - The Timing Panel supports detailed CPU/GPU track inspection and selected-interval analysis.
   - Applicability: this makes clear that Astral's single CPU interval metric is only a first profiling layer, not profiler parity.
3. Unity Technologies, Unity 6.0, **FrameTimingManager**: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/FrameTimingManager.html
   - Unity can capture multiple frame timings and expose CPU/GPU data.
   - Applicability: Astral likewise needs retained multi-frame samples, while remaining explicit about unavailable GPU data.
4. Unity Technologies, Unity 6.0, **FrameTiming** and `cpuFrameTime`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/FrameTiming.html and https://docs.unity3d.com/6000.0/Documentation/ScriptReference/FrameTiming-cpuFrameTime.html
   - Unity distinguishes total CPU frame time from more specific CPU/render/GPU measures.
   - Applicability: Astral labels its metric `cpu_frame_interval_ms` and records the exact wall-clock semantics rather than implying equivalent attribution.

No new dependency was imported and the production graphics API/architecture was not changed.

## Changed paths

The implementation commit changed only this packet's admitted paths:

- `CMakeLists.txt`
- `Engine/Core/Clock.h`
- `Engine/Core/Clock.cpp`
- `Engine/Core/FrameTimingCapture.h`
- `Engine/Core/FrameTimingCapture.cpp`
- `Tests/FrameTimingCaptureTests.cpp`
- `Tests/FrameTiming/CMakeLists.txt`
- `.github/workflows/frame-timing-validation.yml`
- `Tasks/E14-CPU-FRAME-TIMING-CAPTURE-2026-09-21.md`

Published Git blob identities:

- `CMakeLists.txt`: `8461f8783f003c680b7a9b3c5fd505cb0bc3cd33`
- `Engine/Core/Clock.h`: `c79d4ce462727be836a543ce1948cbf3b1c1b9e9`
- `Engine/Core/Clock.cpp`: `dae016f93a789b89da8ed40a9f8ddb0060836500`
- `Engine/Core/FrameTimingCapture.h`: `6b93957bd7b107f66f8847a0a4b3358ee941fa36`
- `Engine/Core/FrameTimingCapture.cpp`: `9b5112aa486959326eb27c506027f08d3c931eb3`
- `Tests/FrameTimingCaptureTests.cpp`: `cedb87ff95773f48f60181c5f431ffed609ee568`
- `Tests/FrameTiming/CMakeLists.txt`: `cc044b4773bb0992c6a7db3946e407c1bed3f648`
- `.github/workflows/frame-timing-validation.yml`: `266881ff5c28a43b45a912eaef05eeccfc4f5942`
- `Tasks/E14-CPU-FRAME-TIMING-CAPTURE-2026-09-21.md`: `edd8f0c3c3a5bd36a2fdc6871ae95de1d533b7c2`

`CMakeLists.txt` also retains the previous Release-only MSVC `/Brepro` setting unchanged.

## Implemented behavior

Capture is disabled by default. The existing `Clock::Tick()` return value and ordinary game execution remain measurement-off unless `ASTRAL_FRAME_TIMING_CSV` is supplied.

A requested capture accepts:

- `ASTRAL_FRAME_TIMING_CSV`: required absolute output path. Its parent must already exist.
- `ASTRAL_FRAME_TIMING_WARMUP_FRAMES`: optional unsigned decimal, default 120.
- `ASTRAL_FRAME_TIMING_MAX_SAMPLES`: optional integer from 1 through 1,000,000, default 36,000.

The capture pre-reserves a bounded sample vector. Warmup samples are discarded before measured-sample validity checks. A non-finite or non-positive post-warmup interval latches a failure and blocks final evidence publication. Reaching the configured sample cap sets an explicit saturation flag without growing the buffer further.

Evidence publication is fail-closed:

- final and `.partial` paths must not exist at configuration time;
- the complete CSV is written and flushed to `.partial` first;
- publication creates a hard link from the partial file to the final path, which fails rather than overwriting a competing final file;
- successful publication removes the partial path;
- no-sample, invalid-sample, repeated-flush and output-race cases fail.

The CSV identifies the schema, metric, units, exact wall-clock semantics, `std::chrono::steady_clock` timing source, warmup/cap/saturation values, `gpu_timing=unavailable`, and `acceptance_claim=none`, followed by `frame_index,cpu_frame_interval_ms` rows.

`Clock` configures the capture from environment in its constructor, records each enabled interval in `Tick()`, and publishes on normal destruction. Configuration or publication failure is reported to stderr and must not be treated as accepted benchmark evidence.

## Portable sandbox verification

The local sandbox used a dedicated disposable worktree/partial source fixture and external build roots. It was not a full repository clone, Windows GUI evidence, or GPU evidence.

Commands executed successfully after final self-review:

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

Result: Debug PASS, optimized Release PASS, and Clang ASan+UBSan PASS. The executable reports `FRAME TIMING CAPTURE TESTS: PASS (11 groups)` in each lane. No sanitizer failure was reported.

The tests exercise the production `Clock.cpp` and capture implementation rather than substituting a mock. The 11 groups cover invalid paths, unsafe sample limits, existing outputs, warmup exclusion, invalid measured intervals, saturation, empty evidence, output races, real `Clock` environment integration/destructor publication, and malformed environment values.

## Hosted CI verification

All CI below is tied to implementation commit `a47e7c9b5b37f26ed0d4deb93adb99bca2a0ac26`. Pull-request workflows actually checked out synthetic merge revision `ba8032752a1e03e81d43e0ffc4cb2f279286b48a`, the merge of that implementation into base `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`. This distinction is retained deliberately.

### Frame timing portability lane

- Workflow: `Frame timing capture portability`
- Run: `35561396672`
- Job: `106214803013`
- Runner: Ubuntu 24.04
- Conclusion: PASS
- Debug contract build/test: PASS
- optimized Release contract build/test: PASS
- Clang AddressSanitizer + UndefinedBehaviorSanitizer with leak detection: PASS

### Full Windows regression lane

- Workflow: `Windows build and deterministic tests`
- Run: `35561396630`
- Job: `106214802847`
- Runner: `windows-2022`, Windows Server 2022 `10.0.20348`, image `20260913.307.1`
- Conclusion: PASS
- C++ compiler: MSVC `19.44.35228.0`
- VC tools: `14.44.35207`
- Windows SDK: `10.0.26100.0`

New production-linked test coverage:

- Debug non-GUI CTest: 9/9 PASS, including `FrameTimingCaptureTests`
- Release non-GUI CTest: 9/9 PASS, including `FrameTimingCaptureTests`

Existing safety/regression gates also remained green:

- R0 runner safety: 13/13 PASS
- PE dependency inspector: 5/5 PASS
- Windows prerequisite planner: 8/8 PASS
- runtime-environment probe: 8/8 PASS
- runtime compatibility audit: 7/7 PASS
- Redistributable bootstrap contracts: 8 PASS, 1 intentional non-Windows-only case skipped on Windows
- Release assertion/CTest safety: 3/3 PASS
- static milestone verifiers: PASS
- clean tracked tree: PASS

The Release `AstralGame.exe` produced by this lane was 91,136 bytes with SHA-256:

`89dd35915c4ea4c30e9050e8d70052521421e9f9708aa425ca1084b6b3d54de5`

The hosted runtime evidence observed VC runtime DLL version `14.51.36247.0` against build tools `14.44.35207`; the version floor passed. Installer provenance, actual package launch, clean-machine compatibility and independent acceptance remained explicitly false.

### Existing package/reproducibility lane

- Workflow: `Release manifest integrity`
- Run: `35561396604`
- Job: `106214802836`
- Conclusion: PASS

Its existing manifest, package-smoke contract, restart-stress contract, continuous-soak contract, soak-analysis, benchmark-manifest and PE reproducibility diagnostic checks passed. The same Release candidate was built twice in that workflow and its byte-identical deterministic classification passed. The workflow staged and verified package bytes and confirmed a clean tracked tree. It did not launch the native GUI and therefore supplies no interactive performance result.

## Claim boundary and capability-map delta

This packet advances E14 from "no raw per-frame sample stream" to "bounded raw CPU frame-interval sample stream with fail-closed evidence publication and cross-platform contract coverage." It does not close E14.

Explicitly unresolved:

- no GPU timestamp/query instrumentation;
- no separation of simulation, renderer, message-pump, Present or wait durations;
- no RAM/VRAM budget measurement in this packet;
- no allocation/stall attribution;
- no p50/p95/p99 result from a native workload yet;
- no measured instrumentation-overhead comparison;
- no matched Astral/UE5/Unity 3D or genuine-2D benchmark scene result;
- no approved performance threshold;
- no clean-machine package launch;
- no registered local interactive-desktop evidence;
- no real 86,400-second soak acceptance;
- no independent review/acceptance;
- terrain, particles/VFX, cinematics, scripting/reflection, input/replay, accessibility/localization, additional platforms and the rest of the engine feature catalogue remain open.

The proposed 120-frame warmup and 3,600-sample first native fixture are measurement parameters only. They are not parity targets and not acceptance limits.

## Registered local executor handoff

After the exact admitted package passes the existing prerequisite and M10 package-smoke gates, use an exclusively owned supported Windows desktop and a fresh external evidence directory. Do not run this through R0 merely because this source packet passed CI.

```powershell
$env:ASTRAL_FRAME_TIMING_CSV = '<absolute-external-evidence-path>\idle-default-scene.csv'
$env:ASTRAL_FRAME_TIMING_WARMUP_FRAMES = '120'
$env:ASTRAL_FRAME_TIMING_MAX_SAMPLES = '3600'
& '<exact-admitted-package>\AstralGame.exe'
```

Leave the existing default procedural scene idle with no input until more than 3,600 post-warmup ticks have occurred, then close normally so `Clock` destruction can publish the CSV. Preserve:

- exact source/package revision and `AstralGame.exe` hash;
- command and environment values;
- Windows/CPU/RAM/GPU/driver identity;
- client resolution, window mode and display refresh;
- UTC start/end timestamps;
- CSV SHA-256 and the CSV itself;
- `astral.log`;
- the existing benchmark manifest and package evidence.

A matched capture-off control is required before using these samples for comparative claims, so instrumentation overhead can be quantified. A later analysis packet should calculate p50/p95/p99 and stall counts from the retained raw rows without modifying them.

## Single next useful action

Run the first package-bound native 3,600-sample idle procedural-scene capture on the registered Windows executor, paired with a matched capture-off control, then bind both results to the benchmark manifest and calculate descriptive frame-time percentiles. GPU timing and finer CPU attribution should remain separate later packets rather than being inferred from this wall-clock interval.