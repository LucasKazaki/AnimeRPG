# E14 bounded benchmark run-control and provenance packet, 2026-09-21

Status: admitted independent E14 benchmark-reproducibility packet on draft PR #9. This packet controls only an explicit benchmark run. Normal interactive operation remains unchanged when benchmark mode is absent. It does not invoke R0, run Astral on Lucas's PCs, establish a performance budget or parity, merge/release/deploy, add dependencies, change the renderer/API, or restart paused content work.

## Identity and dependency

- Repository: `LucasKazaki/AnimeRPG`.
- Existing owned branch / draft PR: `repair/2026-09-20-r0-runner-safety`, PR #9.
- Pre-packet head: `fbdbfd5153250077b2024b99c9adcbd212a2aff9`.
- Dependency: the verified E14 fixed simulation timestep packet at implementation candidate `0889dfb3d7aad053c050d6dc913d855400dafe7f`, plus the existing frame/phase/memory capture, package provenance, environment receipt, and cross-stream coherence tooling on PR #9.
- PR #9 remains stacked on PR #6 / `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`.
- Issue #7 remains open. The historical R0 runner is not authorized for this packet.
- Capability impact: E14 benchmark control/provenance only. No engine feature becomes comparable or independently accepted.

## Reproducible gap

The previous packet can make simulation progression deterministic per rendered frame with `ASTRAL_SIMULATION_FIXED_HZ`, but a benchmark still had three uncontrolled variables:

1. the fixed simulation rate was present only in environment/command receipts rather than machine-bound benchmark evidence;
2. `Win32Application` sampled live physical keyboard state with `GetAsyncKeyState`, so operator input could alter the workload; and
3. the application had no exact warmup plus measured-frame stop condition, so nominally matched runs could execute different frame counts.

A benchmark result must not be interpreted until those variables are controlled and their settings are cryptographically bound into the existing benchmark descriptor/evidence chain.

## Primary-source research

Rechecked on 2026-09-21:

- Unreal Engine 5.8 Command-Line Arguments: https://dev.epicgames.com/documentation/unreal-engine/command-line-arguments-in-unreal-engine . Epic documents launch parameters for reproducible test/optimization runs, including resolution/framerate and custom key-value arguments parsed by project code. Applicability: benchmark execution parameters belong in an explicit runtime control surface rather than undocumented operator behavior.
- Unity 6.0 `Time`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/Time.html . Unity exposes `captureDeltaTime`/`captureFramerate` separately from real-time values. Applicability: a controlled simulation/capture rate can be distinct from wall-clock performance measurement.
- Microsoft `GetAsyncKeyState`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getasynckeystate . Microsoft documents that the function reports whether a physical key is currently down and notes desktop/UIPI conditions that can affect calls. Applicability: live physical key state is an uncontrolled workload input and must be suppressed for the frozen benchmark.
- Unity 6.0 `FrameTiming`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/FrameTiming.html . Unity separates CPU/GPU timing measurements from simulation-time policy. Applicability: Astral's benchmark controller must not replace its wall-clock timing evidence.

No proprietary source is copied and no dependency is added.

## Allowed paths

Implementation commit may change only:

1. `Engine/Core/BenchmarkRunControl.h`
2. `Engine/Core/BenchmarkRunControl.cpp`
3. `Engine/Core/Clock.h`
4. `Engine/Platform/Win32Application.cpp`
5. `Tests/BenchmarkRunControlTests.cpp`
6. `Scripts/verify_benchmark_run_control.py`
7. `Scripts/test_benchmark_run_control.py`
8. `CMakeLists.txt`
9. `Tests/FrameTiming/CMakeLists.txt`
10. `.github/workflows/frame-timing-validation.yml`
11. `Tasks/E14-BENCHMARK-RUN-CONTROL-2026-09-21.md`

After verification, `Docs/QA/E14-BENCHMARK-RUN-CONTROL-2026-09-21.md` may be added as an evidence-only follow-up. No renderer, scene/game-content, package authority, Company Runtime, scheduler, permission, deployment, or third-party dependency path is authorized.

## Runtime contract

Benchmark mode is opt-in and fail-closed:

- with `ASTRAL_BENCHMARK_MODE` absent and no other benchmark-control variables present, normal interactive behavior is unchanged;
- if any benchmark-control variable is present without `ASTRAL_BENCHMARK_MODE=1`, configuration is invalid;
- benchmark mode requires the already-configured `ASTRAL_SIMULATION_FIXED_HZ` to resolve to `[1,1000]`;
- `ASTRAL_BENCHMARK_WARMUP_FRAMES` accepts an unsigned decimal count;
- `ASTRAL_BENCHMARK_MEASURED_FRAMES` accepts an unsigned decimal count in `[1,1000000]`;
- warmup plus measured frames may not exceed 1,000,000;
- `ASTRAL_BENCHMARK_CONTROL_JSON` must be a fresh absolute output path in an existing directory.

When benchmark mode is enabled:

- all `GetAsyncKeyState` workload inputs are treated as up, including Escape, movement, combat, action, interaction, guard, and command keys;
- simulation remains one fixed delta per rendered frame at the actual `Clock` fixed rate;
- the application finishes exactly `warmup_frames + measured_frames` rendered frames and exits the run loop without executing another rendered frame;
- early window/process termination is an incomplete benchmark and must not publish a completion receipt;
- successful exact completion publishes a fresh no-overwrite JSON receipt via `.partial` plus hard-link publication;
- receipt fields bind the actual fixed simulation Hz, warmup/measured/total/completed frame counts, input suppression, and exact-frame-limit termination;
- performance budget, comparative parity, and independent acceptance fields remain false.

The existing benchmark manifest already hashes every evidence file into `benchmark_descriptor_sha256`. The new verifier requires exactly one `benchmark_run_control_json` evidence role, re-verifies the release package and benchmark manifest, re-hashes the receipt, validates its exact schema/claim boundaries, and reports the fixed rate/frame contract. Therefore the fixed simulation setting is machine-bound transitively to the benchmark descriptor through the receipt SHA-256 rather than trusted as prose.

## Verification contract

Sandbox / portable checks:

- compile the production `BenchmarkRunControl.cpp` and production-linked test under C++17 with warnings as errors;
- run all seven control groups under a non-optimized build;
- repeat under optimized Release with assertions disabled to prove the test harness itself is not compiled away;
- run Clang AddressSanitizer + UndefinedBehaviorSanitizer with leak checking;
- byte-compile the Python verifier/test before publication.

Hosted checks for the exact implementation candidate:

- profiling portability must run the new Python verifier regression suite and the portable C++ run-control target in Debug, optimized Release, and ASan+UBSan;
- Windows Server 2022 must compile the real modified `AstralGame` in Debug and Release and run all non-GUI CTests, including `BenchmarkRunControlTests`;
- existing release/package/provenance/reproducibility gates must remain green because root CMake and production Win32 runtime code changed.

Do not weaken an existing timing/capture test, lower a safety bound, or treat hosted execution as the native benchmark.

## Native handoff after hosted verification

Only after this packet verifies, the registered Windows executor may use a frozen control set such as:

```powershell
$env:ASTRAL_SIMULATION_FIXED_HZ = "60"
$env:ASTRAL_BENCHMARK_MODE = "1"
$env:ASTRAL_BENCHMARK_WARMUP_FRAMES = "120"
$env:ASTRAL_BENCHMARK_MEASURED_FRAMES = "3600"
$env:ASTRAL_BENCHMARK_CONTROL_JSON = "<fresh absolute evidence path>\\benchmark-control.json"
```

It must also enable the existing whole-frame, phase, and process-memory captures with matching frame boundaries/stride, retain environment/package/machine receipts, add the completion receipt to the benchmark manifest as role `benchmark_run_control_json`, then run both `verify_benchmark_run_control.py` and `verify_benchmark_stream_coherence.py`. A matched capture-off control remains required before instrumentation overhead can be interpreted.

## Stop, rollback, next action

Stop at the first deterministic compile/test/CI regression and repair only within the allowed paths. Do not bypass failures or invoke R0. Rollback is a revert of this bounded packet only, with no force push or destructive cleanup.

If this packet verifies, the single next useful action is the registered-Windows frozen procedural 3D benchmark using the exact package/control/evidence chain. GPU timestamps/active-adapter proof, VRAM, approved performance/RAM budgets, matched UE5/Unity 3D and genuine-2D workloads, clean-machine launch, recovery stress, the real 86,400-second soak, and independent acceptance remain separate unresolved gates.
