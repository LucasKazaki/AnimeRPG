# E14 bounded benchmark duration-coherence packet, 2026-09-21

Status: admitted verification repair on draft PR #9. This packet closes a benchmark-protocol consistency hole before native E14 measurements are interpreted. It does not invoke R0, run Astral on Lucas's PCs, merge/release/deploy, add dependencies, change renderer/API behavior, change Company Runtime state, restart paused content work, or establish performance/parity acceptance.

## Identity and dependency

- Repository: `LucasKazaki/AnimeRPG`.
- Existing owned branch / draft PR: `repair/2026-09-20-r0-runner-safety`, PR #9.
- Pre-packet head: `b7c241db2f7e05a71eb98096fae3099fc30f5587`.
- Dependency: benchmark resolution-control candidate `5f60ea83490ead4c2544b026da5b8fb152995df7` and evidence-only follow-up at the pre-packet head.
- Issue #7 remains open. The historical R0 runner is not authorized for this packet.
- Capability impact: E14 profiling/benchmark provenance only. No engine feature, performance budget, or UE5/Unity parity claim is promoted.

## Reproducible gap

The benchmark descriptor already SHA-256-binds the run-control receipt, so fixed simulation rate and exact frame counts are transitively retained. However, `verify_benchmark_run_control.py` currently checks only width, height, and window mode against `run_protocol`. It does not prove that the descriptor's `warmup_seconds` and `sample_seconds` agree with the receipt's `simulation_fixed_hz`, `warmup_frames`, and `measured_frames`.

A descriptor can therefore claim a different warmup or sampling duration while still passing run-control verification, as long as the evidence hash is internally valid. That makes otherwise package-bound evidence semantically inconsistent and unsuitable for a matched UE5/Unity comparison.

Acceptance for this packet is fail-closed protocol coherence:

`warmup_seconds * simulation_fixed_hz == warmup_frames`

`sample_seconds * simulation_fixed_hz == measured_frames`

Both products must be exact integer frame counts. The verifier must reject inconsistent or fractional-frame protocols rather than rounding them.

## Primary-source research

Rechecked 2026-09-21:

- Unreal Engine 5.8 General Engine Settings: https://dev.epicgames.com/documentation/unreal-engine/general-engine-settings-in-the-unreal-engine-project-settings . Epic exposes `Use Fixed Frame Rate`, `Fixed Frame Rate`, and a custom time-step policy. Applicability: the time-step rate is a distinct benchmark control and must not be inferred from an unrelated duration label.
- Unreal Engine 5.8 `UCatchupFixedRateCustomTimeStep`: https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/TimeManagement/UCatchupFixedRateCustomTimeStep . Epic explicitly defines a fixed frame-rate time-step implementation. Applicability: fixed-rate semantics and elapsed-duration semantics are separate pieces of evidence.
- Unreal Engine 5.8 performance profiling introduction: https://dev.epicgames.com/documentation/unreal-engine/introduction-to-performance-profiling-and-configuration-in-unreal-engine . Epic treats frame time and frame rate as explicit performance quantities. Applicability: a matched benchmark must preserve the exact relationship between the admitted update rate, number of samples, and stated duration.
- Unity 6.0 `Time`: https://docs.unity3d.com/6000.0/ScriptReference/Time.html . Unity exposes `fixedDeltaTime` separately from per-frame `deltaTime` and capture frame rate. Applicability: Astral's fixed simulation rate must remain explicit and coherent with its measurement window.
- Unity 6.0 fixed-timestep optimization: https://docs.unity3d.com/6000.0/Manual/physics-optimization-cpu-frequency.html . Unity documents the frequency/performance tradeoff of fixed timestep changes. Applicability: silently changing fixed Hz changes the workload and therefore invalidates a comparison protocol.

No proprietary source is copied and no external dependency is added.

## Allowed paths

Implementation may change only:

1. `Scripts/verify_benchmark_run_control.py`
2. `Scripts/test_benchmark_run_control.py`
3. `Tasks/E14-BENCHMARK-DURATION-COHERENCE-2026-09-21.md`

After verification, `Docs/QA/E14-BENCHMARK-DURATION-COHERENCE-2026-09-21.md` may be added as an evidence-only follow-up. No Engine/Game/CMake/workflow/renderer/package-authority/scheduler/content path is authorized.

## Verification contract

- Add a deterministic regression that changes only `run_protocol.warmup_seconds` or `sample_seconds` while retaining a valid SHA-256-bound run-control receipt; verification must fail.
- Add a fractional-frame case such as a duration that does not map to an integer number of fixed steps; verification must fail rather than round.
- Keep a non-60-Hz coherent fixture valid so the verifier does not hardcode one benchmark rate.
- Preserve exact package/manifest/evidence hashing and all existing false acceptance claims.
- Byte-compile the verifier/tests and run the complete Python suite in a full checkout.
- Hosted profiling, Windows Debug/Release, release/package/provenance/reproducibility, and benchmark-environment gates must remain green for the exact implementation candidate.
- Do not weaken timeouts, schema checks, evidence hashes, or claim boundaries to make the packet pass.

## Stop, rollback, and native handoff

Stop at the first deterministic regression outside the three allowed implementation paths. Rollback is a revert of this packet only, with no force push or destructive cleanup.

If this repair verifies, the registered Windows executor may use the existing frozen procedural 3D benchmark protocol only when the run-control receipt and descriptor pass this duration-coherence gate along with the existing package, environment, resolution, and cross-stream checks. Retain 60 Hz fixed simulation, 120 warmup frames, 3,600 measured frames, timing/phase/memory streams, one manifest-declared client resolution, exact package/source/environment receipts, and a separate capture-off control. GPU timing/active-adapter proof, VRAM, approved budgets, matched UE5/Unity 3D and genuine-2D workloads, clean-machine launch, recovery stress, the real 86,400-second soak, and independent acceptance remain unresolved.
