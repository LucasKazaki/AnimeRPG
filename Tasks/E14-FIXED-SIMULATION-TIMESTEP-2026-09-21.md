# E14 bounded fixed simulation timestep packet, 2026-09-21

Status: admitted independent E14 runtime/profiling reproducibility packet on draft PR #9 while the registered native 3D benchmark remains an external/local gate. This packet separates optional deterministic simulation progression from wall-clock performance measurement. It does not invoke R0, run Astral on Lucas's PCs, suppress live input, auto-exit a benchmark, establish performance budgets/parity, merge/release/deploy, install dependencies, change the graphics API, or restart paused content work.

## Identity and dependency

- Repository: `LucasKazaki/AnimeRPG`.
- Existing owned branch / draft PR: `repair/2026-09-20-r0-runner-safety`, PR #9.
- Pre-packet head: `c8d1f094c55f1d8eeb1994b58869e35103f256db`.
- Dependency: E14 wall-clock frame timing, phase timing, process-memory capture, benchmark provenance, coherence, and Windows environment evidence already present on PR #9.
- PR #9 remains stacked on PR #6 / `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`.
- Issue #7 remains open. The historical R0 runner is not authorized for this packet.
- Capability impact: E14 benchmark/runtime reproducibility infrastructure only. No capability becomes comparable or independently accepted.

## Reproducible gap

Before this packet, `Clock::Tick()` both measured wall-clock frame duration and returned that same variable duration to gameplay/simulation callers. `Win32Application` feeds the returned value into movement, combat timers, Shadowblade actions, and other update logic. Therefore two nominally identical benchmark runs that render at different speeds can advance different amounts of simulated time per rendered frame. A fixed frame-count comparison can then measure different simulation trajectories rather than only different execution cost.

The benchmark also needs true wall-clock intervals for performance evidence, so simply replacing the measured interval would corrupt profiling. The bounded repair must keep wall-clock capture and elapsed time unchanged while optionally returning a configured fixed simulation delta to callers.

## Primary-source research

Rechecked on 2026-09-21:

- Unreal Engine 5.8 `UCatchupFixedRateCustomTimeStep`: https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/TimeManagement/UCatchupFixedRateCustomTimeStep . Epic documents a fixed-frame-rate engine timestep with explicit platform-time synchronization and catch-up behavior. Applicability: confirms that simulation timestep policy is a distinct engine concern from raw platform time. Astral does not copy this implementation and does not implement catch-up in this packet.
- Unreal Engine 5.8 Physics Sub-Stepping: https://dev.epicgames.com/documentation/unreal-engine/physics-sub-stepping-in-unreal-engine . UE uses a variable frame rate while physics benefits from smaller fixed steps; sub-stepping trades CPU cost for stability. Applicability: supports separating render-frame duration from deterministic/stable simulation stepping. Astral does not add a physics substep scheduler here.
- Unity 6.0 `Time`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/Time.html . Unity exposes render-frame `deltaTime` separately from `fixedDeltaTime`, the in-game interval for fixed-rate updates. Applicability: supports preserving measured wall time while giving simulation code an explicit fixed interval.
- Unity 6.0 fixed-timestep physics guidance: https://docs.unity3d.com/6000.0/Documentation/Manual/physics-optimization-cpu-frequency.html . Unity documents the default 0.02 second fixed interval and the accuracy/performance tradeoff when changing fixed-update frequency. Applicability: rate selection is a benchmark/workload parameter, not a free performance claim.

No proprietary engine source is copied. No third-party dependency is added. The implementation uses only C++17 standard-library facilities already permitted by the project.

## Allowed paths

Only these paths may change in the implementation commit:

1. `Engine/Core/SimulationTimeStep.h`
2. `Engine/Core/SimulationTimeStep.cpp`
3. `Engine/Core/Clock.h`
4. `Engine/Core/Clock.cpp`
5. `Tests/SimulationTimeStepTests.cpp`
6. `CMakeLists.txt`
7. `Tests/FrameTiming/CMakeLists.txt`
8. `.github/workflows/frame-timing-validation.yml`
9. `Tasks/E14-FIXED-SIMULATION-TIMESTEP-2026-09-21.md`

After hosted verification, `Docs/QA/E14-FIXED-SIMULATION-TIMESTEP-2026-09-21.md` may be added as the evidence-only follow-up. No `Win32Application`, renderer, scene/game, package-authority, benchmark-manifest, Company Runtime, scheduler, permission, deployment, or content path is authorized in this packet.

## Implementation contract

- The default remains exactly variable simulation time: if `ASTRAL_SIMULATION_FIXED_HZ` is absent, `Clock::Tick()` returns the measured wall delta as before.
- `ASTRAL_SIMULATION_FIXED_HZ` accepts a complete unsigned decimal integer in `[1, 1000]` only. Malformed, signed, whitespace-padded, suffixed, overflowed, zero, and out-of-range values fail closed.
- A valid value makes `Clock::Tick()` return exactly `1 / fixed_hz` seconds of simulation delta once per render-loop tick.
- Frame-timing capture must continue recording the measured `steady_clock` wall interval. Process-memory capture keeps the same frame index. `ElapsedSeconds()` remains real wall time.
- Invalid fixed-step configuration throws during `Clock` construction rather than silently falling back to variable simulation time.
- This is deliberately one fixed simulation delta per render-loop tick. It is not a catch-up accumulator, not physics sub-stepping, and not proof of a real-time fixed update frequency if rendering falls behind.
- Existing timing/memory capture behavior and default runtime semantics must remain unchanged when the setting is absent.

## Verification contract

Sandbox / portable source checks:

- compile production `SimulationTimeStep.cpp` and the production-linked test under C++17 with warnings as errors;
- run the seven test groups covering variable default, valid boundary/common rates, malformed values, wall-delta independence, state reset, real `Clock` integration, and fail-closed malformed `Clock` configuration;
- run Debug and optimized Release through the portable `Tests/FrameTiming` subproject; and
- run Clang AddressSanitizer + UndefinedBehaviorSanitizer with leak checking.

Hosted checks for the exact implementation candidate:

- `windows-2022` must compile the real `AstralGame` in Debug and Release and run all non-GUI deterministic CTests, including `SimulationTimeStepTests`;
- the portable profiling workflow must run Debug, optimized Release, ASan and UBSan against the production sources; and
- existing release/package/provenance/reproducibility gates must remain green because root CMake changes.

Do not weaken tests, relax malformed-input rejection, or redefine wall-clock timing to make the gate pass.

## Native handoff and benchmark limitation

For a future frozen benchmark, a proposed deterministic simulation setting is:

```powershell
$env:ASTRAL_SIMULATION_FIXED_HZ = "60"
```

The local executor must retain this exact environment setting with the command receipt. However, **do not treat a native benchmark using this option as comparative evidence yet**. The current benchmark manifest does not have a machine-readable field binding `ASTRAL_SIMULATION_FIXED_HZ`, and live `GetAsyncKeyState` input plus exact benchmark termination are not controlled by this packet. Those gaps must be closed before matched Astral/UE/Unity benchmark interpretation.

## Stop, rollback, next action

Stop at the first deterministic compile/test/CI regression. Do not touch `Win32Application` or the renderer to work around a failure in this packet. Rollback is deletion/revert of only the allowed paths above; do not force-push or discard concurrent work.

If this packet verifies, the single next useful coordinator action is a bounded benchmark-run-control/provenance packet that binds the fixed simulation setting into the benchmark descriptor, suppresses live input for benchmark mode, and enforces an exact warmup/sample frame termination contract. Only after that control plane is verified should the registered Windows executor perform the frozen native 3D benchmark.
