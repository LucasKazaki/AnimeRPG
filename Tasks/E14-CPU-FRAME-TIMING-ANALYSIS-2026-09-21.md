# E14 bounded packet: package-bound CPU frame-timing analysis, 2026-09-21

Status: bounded implementation packet for draft PR #9. Native GUI measurement,
matched UE5/Unity execution, performance acceptance, GPU timing and independent
review remain outside this packet.

## Authority, dependency, and baseline

Repository: `LucasKazaki/AnimeRPG`. Reuse draft PR #9 on branch
`repair/2026-09-20-r0-runner-safety`. Observed pre-packet head:
`77721fd0947072052d890f27c7a96a00fb7c7c23`, stacked on PR #6 base
`e12e6c559bf776ffc9c715c809a517f8e02ce5d5`.

This packet follows `Tasks/E14-CPU-FRAME-TIMING-CAPTURE-2026-09-21.md`. The raw
capture already emits bounded `cpu_frame_interval_ms` CSV evidence but there is no
repository tool that independently validates that CSV, re-binds it to the existing
package/benchmark provenance, and derives deterministic descriptive percentiles.
Without that step, operators could calculate p50/p95/p99 from malformed, stale or
unbound rows and accidentally detach a performance statement from the exact package.

The capability roadmap remains on separate unmerged PR #8. This packet advances
only one E14 profiling/evidence subrequirement. It does not close E14 or modify
E00-E13/E15-E17. Issue #7 remains open; R0 must not be invoked by this packet.

## Primary-source research, accessed 2026-09-21

1. Epic Games, Unreal Engine 5.8, **Timing Insights**:
   https://dev.epicgames.com/documentation/unreal-engine/timing-insights-in-unreal-engine
   Timing Insights retains frame-by-frame performance data and separate CPU/GPU
   tracks, supports spike inspection and aggregated analysis over selected ranges.
   Applicability: Astral needs validated raw multi-frame evidence before later
   attribution and comparison. Documentation only; no Unreal source is copied.
2. Epic Games, Unreal Engine 5.8, **Timing Panel**:
   https://dev.epicgames.com/documentation/unreal-engine/using-the-timing-panel-in-unreal-insights-for-unreal-engine
   The panel separates CPU/GPU tracks and supports explicit time-range selection.
   Applicability: a single Astral wall-clock interval stream is a narrower first
   layer and must not be presented as thread/GPU profiler parity.
3. Epic Games, Unreal Engine 5.8, **Timers and Counters**:
   https://dev.epicgames.com/documentation/unreal-engine/using-the-timers-and-counters-tabs-in-unreal-insights-for-unreal-engine
   Timing Insights can export timing/statistics data from command-line analysis.
   Applicability: reproducible machine-readable analysis output is useful for
   automated evidence review rather than relying on screenshots alone.
4. Unity Technologies, Unity 6.0, **FrameTimingManager** and **FrameTiming**:
   https://docs.unity3d.com/6000.0/Documentation/ScriptReference/FrameTimingManager.html
   https://docs.unity3d.com/6000.0/Documentation/ScriptReference/FrameTiming.html
   Unity retains multiple frame timings and distinguishes total CPU frame time,
   main/render-thread work, Present wait and GPU time. Unity documents total CPU
   frame time as including waits/overhead between frames. Applicability: Astral's
   source metric is kept explicitly as a wall-clock frame interval and no finer
   attribution is inferred.
5. Unity Technologies, Unity 6.0, **Performance testing API 3.2.0**:
   https://docs.unity3d.com/6000.0/Documentation/Manual/com.unity.test-framework.performance.html
   The package collects performance results together with configuration metadata.
   Applicability: Astral analysis must remain bound to its benchmark descriptor,
   package bytes and retained evidence instead of emitting free-floating numbers.

No dependency is imported and no proprietary engine implementation is copied.

## Bounded scope and allowed paths

Allowed paths for this packet only:

- `Scripts/analyze_frame_timing_capture.py`
- `Scripts/test_frame_timing_analysis.py`
- `.github/workflows/frame-timing-validation.yml`
- `Tasks/E14-CPU-FRAME-TIMING-ANALYSIS-2026-09-21.md`
- `Docs/QA/E14-CPU-FRAME-TIMING-ANALYSIS-2026-09-21.md`

No Engine/Game/CMake source is admitted. Rollback is deletion of the two new
scripts/task/QA record plus restoration of the workflow file. Stop on deterministic
contract failure, branch-head movement before write, evidence overwrite, package or
benchmark verification failure, or an unexpected diff outside these paths.

## Implementation contract

Add a dependency-free Python analyzer that first invokes the existing production
`benchmark_manifest.verify_benchmark_manifest(...)`. The caller must also provide
the expected 40-hex candidate commit and expected 64-hex `AstralGame.exe` SHA-256;
the analyzer rejects a benchmark whose bound candidate differs. Exactly one
benchmark evidence entry with role `cpu_frame_timing_csv` is required.

The raw CSV parser is fail-closed. It accepts only the schema emitted by
`FrameTimingCapture.cpp`:

- every required metadata key appears exactly once and no unknown key is accepted;
- schema/metric/units/semantics/timing source must match production literals;
- `gpu_timing=unavailable` and `acceptance_claim=none` cannot be upgraded;
- warmup is unsigned; max samples is in `1..1000000`; saturation is `0|1`;
- header is exactly `frame_index,cpu_frame_interval_ms`;
- measured frame indices are unsigned, at/after warmup and strictly contiguous;
- intervals use the production fixed six-decimal representation and must be finite
  and positive;
- row count cannot exceed max samples, and a saturated capture must contain exactly
  max samples;
- evidence roots/components cannot be symlinks or escape their explicit root;
- the raw bytes/size must still match the benchmark evidence entry.

Calculate descriptive values only: sample count, first/last frame, sum, min, mean,
p50, p95, p99 and max. Percentiles use deterministic linear interpolation at
`position=(n-1)*p`. Do not accept a threshold argument and do not infer a budget.

The output must retain benchmark descriptor SHA-256, candidate commit/executable
hash, workload/protocol/provenance labels, and raw CSV path/size/hash. All acceptance
flags remain hard false, including performance budget, comparative parity,
instrumentation-overhead, GPU-timing and independent acceptance. Optional JSON
publication must use exclusive creation and refuse to overwrite an existing receipt.

## Verification requirements

Portable contract tests cover exact known percentile values, malformed/duplicate
metadata, claim laundering, non-finite/non-positive values, non-contiguous indices,
sample/saturation bounds, incomplete but valid descriptive samples, exclusive
output publication and 250 seeded byte mutations. A mutation may remain valid only
if it re-validates the full schema/invariants.

When the real repository modules are present, integration tests must use the actual
`release_manifest.py` and `benchmark_manifest.py` to build/verify a temporary package
and evidence chain. They then test exact-candidate binding and raw-evidence tamper
rejection. Partial sandbox fixtures may skip only these production-module integration
cases and must label the limitation.

Hosted `Frame timing capture portability` must run the Python analyzer suite against
the complete checkout before its existing C++17 Debug, optimized Release and
Clang ASan+UBSan/leak checks. Do not weaken any existing C++ gate.

## Sandbox evidence before publication

The automation sandbox used an isolated disposable directory rather than a full
repository clone, so production-manifest integration was unavailable there. It ran:

```text
python3 -m py_compile analyze_frame_timing_capture.py test_frame_timing_analysis.py
python3 test_frame_timing_analysis.py
```

Result: 12 discovered tests, 9 PASS and 3 explicitly SKIPPED production-binding
cases because `benchmark_manifest.py` was absent from the partial fixture; exit 0.
The parser and 250 seeded mutation checks executed. Hosted CI must run all 12 cases
against the real repository before this implementation can be called repository-
integrated.

## Native/local handoff and claim boundary

This packet does not run the GUI and therefore produces no native timing result.
After the registered Windows executor passes the exact package prerequisite and M10
runtime-smoke gates, capture the frozen procedural workload using the existing E14
capture packet and create/verify the benchmark manifest with the raw CSV role set to
`cpu_frame_timing_csv`. Then run this analyzer with the exact admitted commit and
`AstralGame.exe` SHA. Preserve the raw CSV unchanged alongside the JSON analysis,
package/release/benchmark manifests, machine/driver/resolution receipts and logs.

A native p50/p95/p99 is descriptive evidence only. Matched capture-off evidence is
still required to quantify instrumentation overhead. GPU timestamps, finer CPU
attribution, RAM/VRAM budgets, approved thresholds, matched 3D and genuine-2D
UE5/Unity scenes, clean-machine launch, load/unload and recovery stress, the real
86,400-second soak, and independent acceptance remain unresolved.

## Single next useful action after this packet

Once hosted integration is green, the next useful action is the registered local
3,600-sample package-bound procedural-scene capture already defined by the prior
packet. Bind its raw CSV to the benchmark manifest and run this analyzer. Do not
choose or pass a parity threshold until matched reference workloads are frozen and
measured.
