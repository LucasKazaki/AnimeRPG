# E14 bounded benchmark capture-pair verification packet, 2026-09-21

Status: admitted E14 verification implementation packet on owned draft PR #9. This packet adds a machine-verifiable pairing gate for the already-required profiled run and matched capture-off control. It does not run Astral on Lucas's PCs, invoke R0, merge/release/deploy, add dependencies, change the graphics API, change Company Runtime state, restart paused content work, measure instrumentation overhead, or establish performance/parity acceptance.

## Identity and dependency

- Repository: `LucasKazaki/AnimeRPG`.
- Existing owned branch / draft PR: `repair/2026-09-20-r0-runner-safety`, PR #9.
- Baseline: `b8ad0b8c259bd4cf7965efe41c1410b2cf255f31`.
- Dependencies already present on this branch: package/benchmark manifest binding, Windows environment evidence, fixed simulation, exact frame termination, client-area/resolution checks, presentation-policy coherence, timing/phase/process-memory capture and analysis, and cross-stream coherence.
- Issue #7 remains open. The historical R0 runner is not authorized for this packet.
- Capability impact: E14 profiling/benchmark evidence only. No production engine or renderer path is changed.

## Reproducible gap

The native handoff already requires a separate capture-off control so profiler overhead can eventually be measured, but the repository has no verifier that proves the profiled run and control are the same benchmark. Two individually valid manifests could differ in workload, reference version, machine label, environment, fixed rate, frame count, resolution, VSync/presentation policy, package identity, or other run-control state and still be compared manually.

This packet adds a fail-closed pairing receipt. Acceptance requires:

1. Both manifests independently pass the existing package/benchmark verifier and run-control verifier.
2. Candidate/package identity, workload, run protocol, environment, reference versions, and provenance are exactly equal.
3. The normalized run-control reports are equal for fixed rate, warmup/measured durations and frame counts, client area, presentation policy, input suppression, and exact termination.
4. The profiled manifest contains exactly one each of `cpu_frame_timing_csv`, `cpu_phase_timing_csv`, and `process_memory_csv`.
5. The control manifest contains none of those profiling-stream roles.
6. The output explicitly keeps `instrumentation_overhead_verified=false`. Role absence is not treated as proof that every profiling environment variable was absent at process launch.
7. No profiling value is interpreted here. The existing stream-coherence verifier remains required for the profiled run, and later native analysis is required before an overhead result can be claimed.

## Current primary-source research, accessed 2026-09-21

Behavior references only. No proprietary source is copied and no external dependency is imported.

1. Epic, Unreal Engine 5.8, "Trace in Unreal Engine 5":
   https://dev.epicgames.com/documentation/unreal-engine/trace-in-unreal-engine-5
   - Trace channels select which events are emitted and control trace data rate.
   - Applicability: capture configuration is a benchmark variable and should be explicit when comparing profiled and control runs.
2. Epic, Unreal Engine 5.8, "Developer Guide to Tracing in Unreal Engine":
   https://dev.epicgames.com/documentation/unreal-engine/developer-guide-to-tracing-in-unreal-engine
   - Trace channels constrain emitted events to reduce CPU and memory use.
   - Applicability: an instrumented measurement cannot simply be assumed equivalent to an instrumentation-disabled control.
3. Epic, Unreal Engine 5.8, "Using the Trace Control Tab in Unreal Insights":
   https://dev.epicgames.com/documentation/unreal-engine/using-the-trace-control-tab-in-unreal-insights-for-unreal-engine
   - Selected trace channels can be paused/resumed; stat named events provide additional metrics at additional overhead.
   - Applicability: capture state and overhead need explicit evidence.
4. Unity 6.0, `UnityEngine.Profiling.Profiler`:
   https://docs.unity3d.com/6000.0/ScriptReference/Profiling.Profiler.html
   - Unity states that using the Profiler negatively affects application performance and that disabling Development Build runs faster.
   - Applicability: Astral needs a matched control before attributing measured cost to instrumentation.
5. Unity 6.0, `BuildOptions.EnableDeepProfilingSupport`:
   https://docs.unity3d.com/6000.0/ScriptReference/BuildOptions.EnableDeepProfilingSupport.html
   - Deep profiling inserts additional method checks and can significantly slow the Player.
   - Applicability: profiling overhead is a measured quantity, not a zero-cost assumption.

Licensing: public documentation is used only to define behavior/measurement requirements. This packet copies no Unreal Engine, Unity, or Microsoft implementation code and adds no third-party library.

## Allowed paths

Implementation may change only:

1. `Scripts/verify_benchmark_capture_pair.py`
2. `Scripts/test_benchmark_capture_pair.py`
3. `.github/workflows/frame-timing-validation.yml`
4. `Tasks/E14-BENCHMARK-CAPTURE-PAIR-2026-09-21.md`

After verification, `Docs/QA/E14-BENCHMARK-CAPTURE-PAIR-2026-09-21.md` may be added as an evidence-only follow-up. No Engine, Game, CMake, renderer/API, package-authority, scheduler, dependency, content, or R0-runner path is authorized.

## Verification contract

- `python -m py_compile Scripts/verify_benchmark_capture_pair.py Scripts/test_benchmark_capture_pair.py`.
- `python Scripts/test_benchmark_capture_pair.py` in a complete repository checkout.
- Regression cases must include valid matched pair, missing/duplicate profiled roles, contaminated control, workload/reference/environment/machine mismatches, protocol/run-control mismatch, evidence tampering, output no-overwrite, and byte/output-parent bounds.
- The profiling workflow must execute the new suite and retain all existing timing, phase, memory, stream-coherence, environment, run-control, Debug, optimized Release, ASan, and UBSan checks.
- Never weaken existing schemas, hashes, timeouts, claim boundaries, or acceptance tests to make the pair gate pass.
- Hosted CI is contract evidence only. It is not a native performance result or independent acceptance.

## Stop, rollback, and native handoff

Stop at the first deterministic regression outside the four allowed implementation paths. Rollback is a revert of this packet only, with no force push or destructive cleanup.

If this packet verifies, the registered Windows executor should generate two fresh benchmark manifests against the same exact package and machine: one profiled run containing the three profiling-stream evidence roles and one capture-off control omitting those roles. Both must retain their own run-control and environment evidence. Run this pair verifier after the existing per-run package/run-control/environment checks and after profiled stream coherence. Preserve exact launch command/environment receipts because this pair gate cannot prove profiling environment variables were absent merely from manifest role absence.

Only after a valid pair exists should a later bounded packet calculate instrumentation overhead. GPU timing, active render-adapter proof, VRAM, approved frame-time/RAM budgets, matched UE5/Unity 3D and genuine-2D workloads, clean-machine launch, recovery stress, the actual 86,400-second soak, and independent acceptance remain unresolved.
