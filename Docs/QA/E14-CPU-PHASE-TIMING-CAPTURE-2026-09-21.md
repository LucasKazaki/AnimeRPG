# E14 QA: main-thread phase timing capture, 2026-09-21

Status: implementation checkpoint for draft PR #9. Native Windows GUI measurement, performance acceptance and independent review are not established here.

## Candidate and scope

- Repository: `LucasKazaki/AnimeRPG`
- Branch: `repair/2026-09-20-r0-runner-safety`
- Pre-packet head: `bf513e043741dd2b358fd40ddb4c5b607eda87ba`
- Implementation commit: `eb3909a92a911467373ec13ffd955001861162b5`
- Stacked base: `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`
- Packet: `Tasks/E14-CPU-PHASE-TIMING-CAPTURE-2026-09-21.md`

The implementation is restricted to the packet's nine allowed paths. It adds an opt-in main-thread phase CSV capture, wires it around the existing Win32 loop, registers production-linked tests, and extends the existing portable frame-timing CI subproject. Renderer implementation, graphics API, gameplay contracts, dependencies, Company Runtime and release authority are unchanged.

Final implementation diff from the pre-packet head contains exactly those nine paths. GitHub classified the implementation commit as one commit ahead with no unrelated file changes.

Published implementation blob identities:

- `Engine/Core/FramePhaseTimingCapture.h`: `1db3b43f36c76a2cc329c9df51c262bdbf2a787b`
- `Engine/Core/FramePhaseTimingCapture.cpp`: `4c16320386dd90401462d238526ad369a2b4068a`
- `Engine/Platform/Win32Application.cpp`: `6053e0365856cecd0dee2c2acdff73d82b334de1`
- `Tests/FramePhaseTimingCaptureTests.cpp`: `457b2f18f1ea0375c59b935ad5802eb59213a831`
- `Tests/FrameTiming/CMakeLists.txt`: `96f2a99b830af4c4d15b8d96fae2b97d55b8e03e`
- root `CMakeLists.txt`: `97c1361df7ce9a57f6b9439c53e09790000a90bc`
- `.github/workflows/frame-timing-validation.yml`: `2dcc432fd5f35794e8e22a4a59ebf1224bd6bcd7`

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

No existing update, render or wait operation was moved. The implementation diff adds only conditional timing configuration/timestamps/recording/flush around the existing loop. When `ASTRAL_FRAME_PHASE_TIMING_CSV` is absent, phase timing is disabled.

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

The sandbox has no Windows SDK/cross-toolchain, so it did not establish Win32 integration locally.

## Hosted verification for exact implementation candidate

All three pull-request workflows associated with source head `eb3909a92a911467373ec13ffd955001861162b5` completed successfully.

### Frame timing portability

GitHub Actions run `35568848921`, job `106236002334`, Ubuntu `ubuntu-24.04`, conclusion success. The job reported success for:

- production frame-timing analyzer/provenance binding;
- Debug and optimized Release capture contracts with the portable CMake subproject, which now contains both the existing whole-frame suite and the new phase suite;
- Clang AddressSanitizer + UndefinedBehaviorSanitizer capture contracts with leak checking.

The structured Actions job record reports every executable step successful. The sandbox result above is the retained explicit `9 groups` count for the new suite; this record does not invent per-test counts not exposed by the structured hosted metadata.

### Windows production integration

GitHub Actions run `35568848925`, job `106236002333`, `windows-2022`, conclusion success. Successful steps include R0 parser/safety contracts, PE/runtime/prerequisite/bootstrap contracts, assertion/CTest safety, Visual Studio 2022 x64 configuration, real Debug build, deterministic Debug tests, real Release build, Release dependency/prerequisite/runtime checks, deterministic Release tests, static milestone verifiers and the final clean tracked-tree check. This establishes that the exact published Win32 integration compiles and passes the existing hosted non-GUI regression gates. It does not establish an interactive GUI performance measurement.

R0 itself was not invoked by this workflow; only its safety contracts ran.

### Package/provenance regression

GitHub Actions run `35568848935`, job `106236002511`, `windows-2022`, conclusion success. Manifest, package runtime-receipt, restart-stress, continuous-soak receipt, soak-analysis, benchmark-manifest and PE reproducibility diagnostic contracts all passed. The same Release candidate was built twice and the byte-identical classification step passed; package staging/exact-byte verification, hosted benchmark-manifest fixture and clean tracked-tree check also passed.

These are hosted contract/build results, not a clean-machine package launch, native package GUI smoke, continuous 86,400-second run or independent acceptance.

## Remaining claim boundary

Unresolved gates remain: interactive Windows GUI phase samples, matched capture-off overhead, GPU timestamps, worker/render-thread timelines, RAM/VRAM budgets, accepted thresholds, matched UE5/Unity 3D and genuine-2D workloads, clean-machine launch, failure-recovery stress, continuous 86,400-second soak and independent acceptance.

Issue #7 remains open. Green source/build tests do not authorize merge, release or use of the historical recovery runner.

## Registered local executor handoff

After the exact package passes the existing prerequisite and M10 package-smoke gates on the registered Windows executor, run the frozen 3,600-sample procedural workload with both whole-frame and phase streams enabled, warmup 120, separate fresh absolute CSVs, and a matched capture-off control. Preserve source/package hashes, exact command/environment, machine/OS/toolchain/CPU/RAM/GPU/driver identity, resolution/window/VSync settings, UTC times, stdout/stderr, `astral.log`, raw evidence hashes and captured images where applicable. `render_submit_ms` remains main-thread GDI work, not GPU execution time.

## Next action

The hosted integration gate is green. The next useful native action is the existing 3,600-sample package-bound procedural 3D measurement with this phase stream enabled; validate/analyze the immutable phase CSV in a separate bounded packet. Do not choose parity thresholds until matched reference workloads are frozen and measured.
