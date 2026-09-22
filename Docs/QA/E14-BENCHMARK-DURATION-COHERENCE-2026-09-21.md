# E14 benchmark duration-coherence verification, 2026-09-21

Status: bounded verification repair completed and green in hosted CI. Native registered-Windows benchmarking and independent review remain pending. This record advances E14 benchmark provenance only; it does not establish Unreal Engine 5 / Unity parity, a performance budget, or engine acceptance.

## Identity and bounded scope

- Repository: `LucasKazaki/AnimeRPG`.
- Draft PR: #9, branch `repair/2026-09-20-r0-runner-safety`.
- Fixed PR base: `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`.
- Pre-packet head: `b7c241db2f7e05a71eb98096fae3099fc30f5587`.
- Task-admission commit: `547dedebc4b7dede40c60ca58f7666f7f20d8608`.
- Verifier implementation commit: `ef2db37cd0711f0b80b573bfc51384584e167261`.
- Test / implementation candidate: `68bd1667edb3227313c3a3ab31715266adf02d88`.
- PR merge ref exercised by hosted jobs: `3912806fe61aa74a7eae8b73ceff733847a08e90`, merging the implementation candidate into the unchanged fixed base.
- GitHub compare `b7c241d...68bd166` reports three commits and exactly three changed paths: this task, the run-control verifier, and its Python regression suite. No Engine/Game/CMake/workflow/renderer/package-authority/Company-Runtime/content path changed.
- Issue #7 remains open. R0 was not invoked. No merge, release, deploy, install, local-PC process control, dependency addition, graphics-API change, or content restart occurred.

Published implementation blobs:

- `Scripts/verify_benchmark_run_control.py`: `d155184118105fda6947225bb2403d21f5ccb355`
- `Scripts/test_benchmark_run_control.py`: `f1a41da53a86ed7265c64c799722d8a0dfcd25bb`
- `Tasks/E14-BENCHMARK-DURATION-COHERENCE-2026-09-21.md`: `f70a1a824271fc8bfd552eec473d5634b91bdcf3`

## Reproducible verification defect

The benchmark manifest already SHA-256-binds the run-control receipt as evidence, and that receipt records `simulation_fixed_hz`, `warmup_frames`, and `measured_frames`. The run-control verifier also already checked the receipt against descriptor width, height, and window mode.

It did not, however, prove that the descriptor's `run_protocol.warmup_seconds` and `run_protocol.sample_seconds` described the same fixed-step intervals as the receipt. A manifest could therefore be internally hash-valid while saying, for example, `warmup_seconds=3.0` next to a valid 60 Hz / 120-frame receipt, which actually represents a two-second fixed-step warmup. That semantic mismatch would undermine a matched comparison even though every file hash remained valid.

The implementation adds a fail-closed duration/frame-rate coherence check:

```text
warmup_seconds * simulation_fixed_hz == warmup_frames
sample_seconds * simulation_fixed_hz == measured_frames
```

The verifier uses decimal arithmetic derived from the parsed JSON number representation and requires each product to be an exact integer frame count. It does not round a fractional fixed-step duration to make the protocol pass. A coherent non-60-Hz fixture remains valid, so this is not a hidden hardcoded 60 Hz rule.

The successful verification report now includes `benchmark_duration_protocol_coherent: true`, the descriptor warmup/sample seconds, and the receipt frame/rate fields. Performance, comparative parity, and independent-acceptance claims remain false. The limitation list now also preserves a separate unresolved point: `run_protocol.vsync` is descriptor-declared but not runtime-proven by the current GDI run-control receipt.

## Current primary-source research

Rechecked 2026-09-21:

- Epic Games, Unreal Engine 5.8 General Engine Settings: https://dev.epicgames.com/documentation/unreal-engine/general-engine-settings-in-the-unreal-engine-project-settings
- Epic Games, Unreal Engine 5.8 `UCatchupFixedRateCustomTimeStep`: https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/TimeManagement/UCatchupFixedRateCustomTimeStep
- Epic Games, Unreal Engine 5.8 performance profiling introduction: https://dev.epicgames.com/documentation/unreal-engine/introduction-to-performance-profiling-and-configuration-in-unreal-engine
- Unity 6.0 `Time`: https://docs.unity3d.com/6000.0/ScriptReference/Time.html
- Unity 6.0 fixed-timestep optimization: https://docs.unity3d.com/6000.0/Manual/physics-optimization-cpu-frequency.html

Epic exposes fixed frame rate and custom time-step controls separately from frame-time measurement. Unity exposes `fixedDeltaTime` separately from rendered-frame `deltaTime`, and documents that changing fixed-timestep frequency changes CPU cost and simulation behavior. These references support treating the simulation rate and admitted measurement duration as separate controls that must agree before results are compared. They do not supply Astral implementation code. No proprietary engine source was copied and no external dependency was added.

## Sandbox check

A disposable Linux helper fixture copied the new `_duration_frames` and duration-coherence logic and ran:

```text
python3 -m py_compile /tmp/e14_duration_coherence_fixture.py
python3 /tmp/e14_duration_coherence_fixture.py
```

Result: PASS. It covered coherent 60 Hz and 120 Hz cases, mismatched warmup and measured durations, a `60.01 s * 60 Hz` fractional-frame case, and boolean/string/NaN/infinity/negative malformed inputs.

This helper fixture was intentionally partial. It did not contain the full repository's package/manifest stack and is not presented as exact-source integration evidence. The complete hosted checkout below ran the published production verifier and regression suite.

## Hosted verification for implementation head `68bd1667...`

All four PR workflows associated with head `68bd1667edb3227313c3a3ab31715266adf02d88` completed successfully. GitHub's PR checkout exercised merge ref `3912806fe61aa74a7eae8b73ceff733847a08e90`; the PR base remained the fixed `e12e6c...` baseline.

### Profiling capture portability

- Run `35625212387`, job `106417803277`: PASS.
- The full production Python profiling/provenance step ran `Scripts/test_benchmark_run_control.py` and passed 14/14 tests.
- New passing cases include descriptor warmup/sample mismatch rejection after manifest re-hashing, fractional fixed-step duration rejection without rounding, and a coherent 120 Hz protocol to prove the check is rate-generic.
- Existing hash mutation, duplicate/missing evidence role, claim laundering, schema, client-area, exact-frame-count, CLI no-overwrite, and file-size-bound cases remained green.
- GNU 13.3.0 Debug C++ profiling contracts: 5/5 CTests PASS, including `BenchmarkRunControlTests: 9 groups passed`.
- GNU 13.3.0 optimized Release: 5/5 CTests PASS.
- Clang 18.1.3 ASan+UBSan with leak detection and halt-on-error: 5/5 CTests PASS.
- Existing frame timing, phase timing, process memory, and fixed-simulation production-linked tests remained green.

### Windows build and deterministic tests

- Run `35625212326`, job `106417879225`: PASS.
- R0 safety contract tests, PE dependency inspection, prerequisite/runtime compatibility, VC Redistributable bootstrap contracts, Release assertion/CTest safety, Visual Studio 2022 x64 configure, Debug build/tests, Release build/tests, static milestone verifiers, and final clean-tree verification all passed.
- The real `AstralGame` still built in both Debug and Release on the hosted Windows lane. This packet did not alter engine source.
- Passing R0 safety contract tests do not authorize or constitute execution of the historical R0 runner. Issue #7 remains open.

### Release/package/provenance

- Run `35625212258`, job `106417866782`: PASS.
- Release manifest, runtime-receipt, restart-stress, continuous-soak contract, soak analysis, benchmark-manifest, and PE reproducibility contract checks passed.
- Two same-candidate Release builds were classified as byte-identical.
- Exact hosted package staging/manifest verification and the hosted benchmark-manifest contract fixture passed, followed by the clean-tree check.
- These are hosted package/contract checks, not a clean-machine package launch and not the real 86,400-second soak.

### Benchmark environment

- Run `35625212292`: PASS.
- Windows CIM-capture job `106417802958`: PASS, including a real environment receipt for the ephemeral hosted Windows runner.
- Portable environment-contract job `106417803334`: PASS.
- The CIM data describes the hosted runner, not Lucas's PCs.

## Capability-to-evidence map impact

This packet changes only the confidence of E14 benchmark protocol evidence. The run-control receipt, benchmark descriptor, exact frame counts, fixed simulation rate, warmup duration, measured duration, requested/observed client area, package hash, environment receipt, and profiling-stream hashes now have a stronger consistency gate before a native result can be interpreted.

The broader catalogue remains unresolved or partial: runtime/jobs/memory-system breadth; scene ownership/serialization; asset pipeline breadth; GPU rendering/materials; lighting/shadows/reflections; large-world streaming/detail; animation; physics/collision; AI/navigation; audio; UI/editor tooling; genuine 2D; networking; GPU timestamps; active-render-adapter proof; VRAM; allocator/thread/task attribution; terrain/foliage; particles/VFX; cinematics; scripting/reflection; input/replay; localization/accessibility; additional platforms; approved performance/RAM/VRAM budgets; matched UE5/Unity reference scenes/workflows; clean-machine launch; recovery stress; real 86,400-second native soak; and independent acceptance.

A green build or a coherent duration contract is not engine parity.

## Registered-Windows native handoff

Use the existing frozen procedural 3D benchmark only from the registered local executor, preserving source SHA, package/release-manifest hash, machine/toolchain/driver identity, exact commands, stdout/stderr, exit codes, UTC timestamps, raw evidence hashes, and required screenshots. Do not run the historical R0 runner.

For the admitted 60 Hz / 120 warmup / 3,600 measured protocol, the benchmark descriptor must state exactly:

```text
run_protocol.warmup_seconds = 2.0
run_protocol.sample_seconds = 60.0
```

and benchmark launch controls must include:

```text
ASTRAL_BENCHMARK_MODE=1
ASTRAL_SIMULATION_FIXED_HZ=60
ASTRAL_BENCHMARK_CLIENT_WIDTH_PX=<exact run_protocol.width>
ASTRAL_BENCHMARK_CLIENT_HEIGHT_PX=<exact run_protocol.height>
ASTRAL_BENCHMARK_WARMUP_FRAMES=120
ASTRAL_BENCHMARK_MEASURED_FRAMES=3600
ASTRAL_BENCHMARK_CONTROL_JSON=<fresh absolute control-receipt path>

ASTRAL_FRAME_TIMING_CSV=<fresh absolute whole-frame CSV path>
ASTRAL_FRAME_TIMING_WARMUP_FRAMES=120
ASTRAL_FRAME_TIMING_MAX_SAMPLES=3600

ASTRAL_FRAME_PHASE_TIMING_CSV=<fresh absolute phase CSV path>
ASTRAL_FRAME_PHASE_TIMING_WARMUP_FRAMES=120
ASTRAL_FRAME_PHASE_TIMING_MAX_SAMPLES=3600

ASTRAL_PROCESS_MEMORY_CSV=<fresh absolute memory CSV path>
ASTRAL_PROCESS_MEMORY_WARMUP_FRAMES=120
ASTRAL_PROCESS_MEMORY_SAMPLE_EVERY_FRAMES=<manifest-declared stride>
ASTRAL_PROCESS_MEMORY_MAX_SAMPLES=<enough for every scheduled post-warmup sample, no saturation>
```

Use one manifest-declared windowed client resolution. Capture the Windows environment receipt first and bind it to the same benchmark descriptor/package. Build the benchmark manifest only from immutable raw evidence and exact hashes. Do not edit a receipt/CSV/manifest after collection to force agreement.

Then run the production verifier against fresh external analysis output:

```text
python Scripts/verify_benchmark_run_control.py <benchmark.json> <package-root> <release-MANIFEST.json> <evidence-root> --json <fresh-analysis-output.json>
```

The verifier must report `benchmark_duration_protocol_coherent: true`. Also require package/release verification, Windows environment binding, whole-frame/phase/process-memory analyzers, and cross-stream coherence to pass for the same candidate/package/frame interval. Run a separately packaged or otherwise exactly matched capture-off control under the existing instrumentation-overhead protocol before treating capture-on frame timings as performance evidence.

If the descriptor says anything other than 2.0 warmup seconds and 60.0 sample seconds for the 60 Hz / 120 / 3,600 frame contract, this new gate must fail. Do not weaken or round the protocol.

## Limits and next useful action

No registered-Windows GUI benchmark, actual GPU timing, active-render-adapter proof, VRAM measurement, accepted frame-time/RAM budget, matched Unreal/Unity workload, clean-machine launch, recovery-stress run, real 24-hour soak, or independent review happened in this pass. VSync remains descriptor-declared rather than runtime-proven in the current GDI benchmark path. Code review by the implementation author is not independent review.

Single next useful action: run the registered-Windows frozen procedural 3D benchmark only after generating a descriptor whose 2.0-second warmup and 60.0-second measured duration pass this coherence gate, then retain the complete package/environment/run-control/timing/phase/memory/coherence receipt chain plus the matched capture-off control. If native execution remains unavailable, the next coordinator repair should close another concrete benchmark-evidence gap, with runtime proof of the descriptor's VSync/presentation policy a higher-value target than adding unrelated engine features while E14 acceptance is still blocked.
