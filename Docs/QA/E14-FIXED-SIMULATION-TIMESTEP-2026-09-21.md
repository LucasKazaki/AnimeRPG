# E14 fixed simulation timestep QA, 2026-09-21

## Checkpoint

- Repository: `LucasKazaki/AnimeRPG`.
- Existing draft PR: #9, branch `repair/2026-09-20-r0-runner-safety`.
- Pre-packet head: `c8d1f094c55f1d8eeb1994b58869e35103f256db`.
- Implementation candidate: `0889dfb3d7aad053c050d6dc913d855400dafe7f` (`E14: add optional fixed simulation timestep`).
- PR state after hosted verification: open, draft, mergeable, unmerged.
- Issue #7 remains open. R0 was not invoked.
- No renderer/API, dependency, Company Runtime state, release/deployment authority, or paused content changed.

## Gap and implementation

Before this packet, `Clock::Tick()` used one wall-clock delta for both profiling and simulation progression. `Win32Application` consumes that return value for movement and gameplay-system updates, so slower and faster benchmark runs could advance different simulated time over the same rendered-frame count.

The packet adds `SimulationTimeStep`, configured only by optional `ASTRAL_SIMULATION_FIXED_HZ`:

- unset preserves the existing variable simulation delta;
- complete unsigned decimal values in `[1, 1000]` select one fixed simulation delta of `1 / Hz` per render-loop tick;
- malformed, signed, whitespace-padded, suffixed, overflowed, zero and out-of-range values fail closed;
- `Clock` throws on invalid configuration rather than silently changing benchmark semantics;
- frame-timing capture continues to record the measured `steady_clock` wall interval;
- process-memory capture keeps the same frame index; and
- `ElapsedSeconds()` remains wall time.

This is deliberately **not** a catch-up accumulator, physics substep scheduler, fixed real-time execution guarantee, replay system, or performance/parity claim. If rendering falls behind, simulation time can fall behind real time because this packet performs one fixed simulation step per render-loop iteration.

## Primary-source basis

Rechecked 2026-09-21:

- Unreal Engine 5.8 `UCatchupFixedRateCustomTimeStep`: https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/TimeManagement/UCatchupFixedRateCustomTimeStep . Epic treats engine timestep policy separately from platform time and documents explicit synchronization/catch-up behavior. Astral does not copy that implementation and does not implement catch-up here.
- Unreal Engine 5.8 Physics Sub-Stepping: https://dev.epicgames.com/documentation/unreal-engine/physics-sub-stepping-in-unreal-engine . UE remains variable-frame-rate while physics benefits from smaller fixed steps, with explicit CPU/stability tradeoffs. Astral does not claim physics equivalence.
- Unity 6.0 `Time`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/Time.html . Unity separates per-render-frame `deltaTime` from `fixedDeltaTime` for fixed-rate updates.
- Unity 6.0 fixed-timestep physics guidance: https://docs.unity3d.com/6000.0/Documentation/Manual/physics-optimization-cpu-frequency.html . Unity documents timestep frequency as an accuracy/performance tradeoff, not a free performance improvement.

No proprietary engine source was copied and no dependency was added.

## Changed paths

GitHub compare from `c8d1f094...` to implementation candidate `0889dfb3...` reports exactly one fast-forward commit, ahead by one, with only these nine paths changed:

1. `.github/workflows/frame-timing-validation.yml`
2. `CMakeLists.txt`
3. `Engine/Core/Clock.cpp`
4. `Engine/Core/Clock.h`
5. `Engine/Core/SimulationTimeStep.cpp`
6. `Engine/Core/SimulationTimeStep.h`
7. `Tasks/E14-FIXED-SIMULATION-TIMESTEP-2026-09-21.md`
8. `Tests/FrameTiming/CMakeLists.txt`
9. `Tests/SimulationTimeStepTests.cpp`

Published Git blobs checked directly after the write include:

- `Engine/Core/SimulationTimeStep.cpp`: `14a6ebc9bb0437090008719031da880248c4ec2b`
- `Engine/Core/SimulationTimeStep.h`: `f1efb1f2f566a044b9a3c68fcd85fb8e50c90dc9`
- `Engine/Core/Clock.cpp`: `c5063380bd1595b620267705d94b9ea0a13d0869`
- `Tests/SimulationTimeStepTests.cpp`: `ae798c60772630e6693f0de9fd29501f1760eced`

The branch update was non-force. No concurrent branch commit was discarded.

## Sandbox/compiler evidence

Before publication, the exact production `SimulationTimeStep.cpp` implementation was compiled in a disposable Linux fixture with a class-level regression harness under C++17 warnings-as-errors. GCC passed. Clang ASan+UBSan with leak checking also passed. That pre-publication harness covered variable mode, accepted rates, malformed rates, wall-delta independence and reset behavior.

The final production-linked seven-group test additionally exercises the real `Clock` integration and malformed-configuration failure path. That final test was verified in the complete hosted repository rather than claiming that the partial sandbox contained the full repository dependencies. The container could not resolve `github.com`, so it was not represented as a clone or native Astral runtime environment.

## Hosted verification for exact implementation candidate

All pull-request workflows associated with implementation head `0889dfb3d7aad053c050d6dc913d855400dafe7f` completed successfully.

### Profiling capture portability

- Run `35599850072`, Ubuntu job `106333106071`: PASS.
- Existing profiling/provenance analysis tests passed.
- Portable CMake Debug and optimized Release profiling contracts passed with the new `AstralSimulationTimeStepTests` target included.
- Clang AddressSanitizer + UndefinedBehaviorSanitizer with leak checking passed with the same production sources.

### Windows build and deterministic tests

- Run `35599849854`, Windows Server 2022 job `106333105254`: PASS.
- Existing R0-safety, PE/prerequisite/runtime/bootstrap and test-safety contracts passed.
- The actual VS2022 x64 root project configured successfully.
- Real `AstralGame` Debug and Release builds passed.
- Deterministic non-GUI Debug and Release CTest steps passed with `SimulationTimeStepTests` registered in the root project.
- Static milestone verifiers and tracked-tree cleanliness passed.

This is hosted Windows compile/test evidence. It is not an interactive GUI run, local GPU evidence, or independent review.

### Release/package/provenance regression

- Run `35599849903`, Windows job `106333106254`: PASS.
- Manifest, runtime receipt, restart-stress contract, continuous-soak contract, soak-analysis contract, benchmark-manifest and PE diagnostic checks passed.
- Two clean Release candidate builds remained byte-identical.
- Exact hosted package staging/manifest verification and hosted benchmark-manifest fixture binding passed.
- Clean tracked-tree gate passed.

These are contract/hosted package checks. The continuous-soak step validates the soak evidence machinery, not an actual 86,400-second native soak.

### Benchmark-environment regression

- Run `35599849983`: PASS.
- Portable contracts job `106333105981`: PASS.
- Windows CIM job `106333105689`: PASS, including real hosted-runner CIM capture.

That CIM evidence describes only the ephemeral GitHub runner, not Lucas's hardware.

## Capability-to-evidence map checkpoint

- Runtime/jobs/memory: this packet adds a bounded simulation-time policy primitive; a general job system and allocator/tag/callstack accounting remain open.
- Scene ownership/serialization: unchanged and below comparison target.
- Asset pipelines: separate E0 candidate remains unmerged.
- GPU rendering/materials, lighting/shadows/reflections, large-world streaming/detail, animation, physics/collision, AI/navigation, audio, UI/editor tools, genuine 2D, networking: no parity evidence added here.
- Profiling: wall-clock CPU intervals, Win32 main-thread phases, process-memory samples, provenance/coherence/environment evidence and deterministic package checks remain intact. Fixed simulation progression can now be separated from measured wall time for future controlled workloads.
- Packaging/platforms: hosted deterministic Release/package evidence remains green; clean-machine and broader platform acceptance remain open.
- Remaining catalogue: terrain/foliage, particles/VFX, cinematics, scripting/reflection, input/replay, accessibility/localization and additional platforms remain explicit open rows.
- Comparative acceptance: still blocked on matched native workloads, approved thresholds, stress/recovery, the real 24-hour soak and independent acceptance.

## Native gate and single next action

Do **not** run or interpret the frozen native 3D comparison with fixed simulation mode yet. Although a future executor can set:

```powershell
$env:ASTRAL_SIMULATION_FIXED_HZ = "60"
```

`benchmark_manifest.py` does not yet machine-bind that setting, live `GetAsyncKeyState` input remains active, and the runtime has no exact benchmark frame-termination controller. A command transcript alone is insufficient to make those hidden workload variables coherent.

The single next useful coordinator packet is therefore a bounded benchmark run-control/provenance implementation that:

1. binds the fixed simulation setting into the benchmark descriptor/verification chain;
2. suppresses live player/action input only when explicit benchmark mode is enabled;
3. enforces an exact warmup + measured-frame termination contract without weakening existing capture caps; and
4. adds production-linked tests proving normal interactive behavior is unchanged when benchmark mode is absent.

Only after that packet is verified should the registered Windows executor collect the frozen 3D native benchmark. GPU timestamps/active-adapter proof, VRAM, accepted performance/RAM budgets, matched UE5/Unity 3D and genuine-2D workloads, clean-machine launch, recovery stress, the real 86,400-second soak, and independent acceptance remain unresolved.
