# E14 benchmark capture-pair verification evidence, 2026-09-21

Status: bounded E14 verification implementation complete for this packet. Hosted contract/regression gates passed. Native Windows benchmark measurements, instrumentation-overhead measurement, clean-machine acceptance, the 86,400-second soak, and independent review remain unresolved. This record does not authorize R0, merge, release, deployment, dependency changes, graphics-API changes, Company Runtime changes, or resumed game-content work.

## Identity

- Repository: `LucasKazaki/AnimeRPG`.
- Owned branch / draft PR: `repair/2026-09-20-r0-runner-safety`, PR #9.
- Base before this packet: `b8ad0b8c259bd4cf7965efe41c1410b2cf255f31`.
- Implementation candidate: `95fbaf1c2505e97c30a7800e52521dcbe0c6dd34`.
- Commit message: `E14: verify matched profiled and capture-off benchmark pairs`.
- PR state after candidate publication: open, draft, mergeable, unmerged.
- Issue #7 remains open. The historical R0 runner was not invoked.

The exact candidate is one commit ahead of the packet baseline and changes only:

1. `.github/workflows/frame-timing-validation.yml`
2. `Scripts/test_benchmark_capture_pair.py`
3. `Scripts/verify_benchmark_capture_pair.py`
4. `Tasks/E14-BENCHMARK-CAPTURE-PAIR-2026-09-21.md`

`git compare` for `b8ad0b8...95fbaf1` reports four paths, 669 additions, zero deletions. No Engine, Game, CMake, renderer, graphics API, dependency, package-authority, scheduler, content, or R0-runner path changed.

Published Git blob identities:

- verifier: `53ec8f477085c391b4c6b5f2d5bb2662b8557ef2`
- regression suite: `bacc3771ff90c8ff75df87a697e8cd775afe6fc8`
- task contract: `a78df0db91a31242c106679a372c6a917467f144`
- profiling workflow: `4cfe8721d72d054b66cd157db1671f5dc289362e`

The local disposable-source files used for syntax/YAML checks produced these same Git blob hashes.

## Reproducible gap repaired

The E14 native handoff has required a separately measured capture-off control, but before this packet the repository had no machine-verifiable rule proving that the profiled run and its control were actually the same benchmark. Two manifests could each be valid while differing in package, workload, run protocol, machine/environment labels, reference-engine versions, fixed simulation rate, warmup/sample duration, exact frame counts, client area, or presentation policy.

`Scripts/verify_benchmark_capture_pair.py` now:

- independently verifies both benchmark manifests against the same exact staged package/release manifest;
- independently runs the existing benchmark run-control verifier for both runs;
- requires exact equality for candidate/package identity, workload, run protocol, environment, reference versions, and provenance;
- requires equality of normalized run-control fields including fixed Hz, warmup/measured frames and seconds, exact completion, client area/control, window mode, VSync request, GDI presentation backend, pacing label, input suppression, and termination mode;
- requires exactly one `cpu_frame_timing_csv`, one `cpu_phase_timing_csv`, and one `process_memory_csv` role in the profiled manifest;
- requires all three profiling-stream roles to be absent from the control manifest;
- writes a fresh, no-overwrite pairing receipt with both descriptor hashes and the matched protocol/environment identity;
- permanently leaves instrumentation-overhead, performance/RAM/VRAM budgets, GPU timing, parity, clean-machine compatibility, and independent acceptance false.

This is deliberately a pairing gate, not an overhead calculator. A control manifest without profiling roles does not by itself prove that every `ASTRAL_*` profiling environment variable was absent when the process launched. Retained launch command/environment evidence is still required, and a future runtime capture-state receipt would make that fact machine-verifiable.

## Current primary-source research

Accessed 2026-09-21. These sources define behavior/measurement requirements only; no proprietary implementation code was copied and no external dependency was imported.

1. Epic Games, Unreal Engine 5.8, `Trace in Unreal Engine 5`
   - https://dev.epicgames.com/documentation/unreal-engine/trace-in-unreal-engine-5
   - Trace channels select emitted event types and control trace data rate.
   - Applicability: capture configuration is a benchmark variable, not an invisible constant.
2. Epic Games, Unreal Engine 5.8, `Developer Guide to Tracing in Unreal Engine`
   - https://dev.epicgames.com/documentation/unreal-engine/developer-guide-to-tracing-in-unreal-engine
   - Trace channels constrain emitted events to reduce CPU and memory use.
   - Applicability: profiled and non-profiled runs should not be assumed cost-equivalent.
3. Epic Games, Unreal Engine 5.8, `Using the Trace Control Tab in Unreal Insights`
   - https://dev.epicgames.com/documentation/unreal-engine/using-the-trace-control-tab-in-unreal-insights-for-unreal-engine
   - Trace channels can be paused/resumed; stat named events add metrics at additional overhead.
   - Applicability: measurement state and its overhead need explicit evidence.
4. Unity 6.0, `UnityEngine.Profiling.Profiler`
   - https://docs.unity3d.com/6000.0/ScriptReference/Profiling.Profiler.html
   - Unity explicitly states that the Profiler negatively affects application performance and that disabling Development Build makes the application run faster.
   - Applicability: Astral needs a matched control before attributing a timing delta to profiling.
5. Unity 6.0, `BuildOptions.EnableDeepProfilingSupport`
   - https://docs.unity3d.com/6000.0/ScriptReference/BuildOptions.EnableDeepProfilingSupport.html
   - Deep profiling inserts additional checks and can significantly slow the Player.
   - Applicability: profiling overhead must be measured, not assumed to be zero.

## Verification evidence

### Sandbox/source checks

Attempted in the disposable Linux sandbox against the exact files later published:

- `python -m py_compile /mnt/data/astral_e14_capture_pair/verify_benchmark_capture_pair.py /mnt/data/astral_e14_capture_pair/test_benchmark_capture_pair.py`
  - PASS, exit 0.
- YAML parse of the modified profiling workflow using `yaml.safe_load`
  - PASS.
- `git hash-object` on all four source files
  - PASS; all hashes exactly matched the published Git blobs above.
- `git ls-remote https://github.com/LucasKazaki/AnimeRPG.git refs/heads/repair/2026-09-20-r0-runner-safety`
  - ATTEMPTED, failed because the sandbox could not resolve `github.com`.
  - Consequence: a complete repository unit-test run was not possible in that sandbox. Hosted GitHub Actions supplied the complete-checkout execution below. The DNS failure is not treated as a product or test failure.

### Hosted profiling contract

GitHub Actions run `35638626528`, job `106462199923`, `portable-frame-timing`, completed successfully.

The job checked out PR merge commit `2f124f2e4368d4f7ed5c049098967909c4b94f1b`, which merges implementation candidate `95fbaf1c2505e97c30a7800e52521dcbe0c6dd34` into the unchanged PR base `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`.

New capture-pair suite:

- `python Scripts/test_benchmark_capture_pair.py`
- 8 tests PASS in 0.208 seconds.
- Cases cover valid matched pair, missing/duplicate profiled roles, contaminated control, workload/reference/environment/machine mismatches, protocol/run-control mismatch, evidence tampering, CLI no-overwrite, manifest-size limit, and output-parent safety.

Existing profiling verification also remained green:

- frame-timing analysis: 12 tests PASS;
- phase-timing analysis: 13 tests PASS;
- process-memory analysis: 13 tests PASS;
- stream coherence: 13 tests PASS;
- environment binding: 17 tests PASS;
- run-control verification: 15 tests PASS.

Production-linked C++ contract fixtures remained green:

- GNU 13.3.0 Debug: 5/5 CTests PASS.
- GNU 13.3.0 optimized Release: 5/5 CTests PASS.
- Clang 18.1.3 ASan + UBSan with leak detection: 5/5 CTests PASS.
- Existing native contract group counts remained 11 frame-timing, 9 phase-timing, 9 process-memory, 7 simulation-timestep, and 9 benchmark-run-control groups.

This hosted lane proves contract/regression behavior. It does not produce a native Astral performance measurement.

### Hosted Windows regression

GitHub Actions run `35638626503`, job `106462199878`, `MSVC Debug and Release`, completed successfully.

Passed steps include:

- R0 safety contracts without invoking the R0 runner;
- PE dependency, prerequisite, runtime-environment, runtime-compatibility, and Redistributable-bootstrap contracts;
- Visual Studio 2022 x64 configure;
- actual Debug build and deterministic Debug tests;
- actual Release build, dependency/prerequisite/runtime checks, deterministic Release tests;
- static milestone verifiers;
- clean tracked-tree gate.

This is hosted Windows regression evidence only. It is not Lucas's registered local Windows machine, GUI/desktop evidence, or independent acceptance.

### Release/package regression

GitHub Actions run `35638626535`, job `106462200381`, `windows-package-manifest`, completed successfully.

The existing release/package, runtime-receipt, restart-stress-contract, continuous-soak-contract, soak-analysis, benchmark-manifest, reproducibility-diagnostic, exact-package, provenance, and clean-tree gates all passed. The repeated Release builds were classified as byte-identical.

No release was published or deployed.

### Benchmark-environment regression

GitHub Actions run `35638626549` completed successfully:

- Windows CIM job `106462200359`: PASS, including a real CIM receipt from the ephemeral hosted Windows runner.
- Portable contracts job `106462200467`: PASS.

The hosted CIM result describes only the GitHub runner. It is not evidence about Lucas's PCs.

## Capability-to-evidence map after this packet

| Capability area | Current evidence level | Still required for comparison acceptance |
|---|---|---|
| Runtime / jobs / memory | Partial runtime contracts plus process-level memory capture; no mature job system/allocator attribution | allocator/tag attribution, task/job scheduling evidence, budgets, stress |
| Scene ownership / serialization | Prototype-level only | versioned scene ownership/serialization workflows, corruption/recovery tests |
| Asset pipelines | Bounded mesh-loader work exists on separate unmerged PR #8 | complete import/cook/cache/reload pipeline and production-scale fixtures |
| GPU rendering / materials | Current production path remains Win32 GDI wireframe | real GPU API/backend, materials/shaders, GPU timing, adapter proof, VRAM |
| Lighting / shadows / reflections | Gap | implemented features plus matched correctness/performance scenes |
| Large-world streaming / detail | Gap | streaming/LOD/HLOD/world-origin or equivalent evidence |
| Animation | Gap | skeleton/clip/blend/state/root-motion/tooling evidence |
| Physics / collision | Gap beyond prototype behavior | production collision/solver/query correctness, stress and determinism evidence |
| AI / navigation | Gap | navigation generation/query/path-following and scale evidence |
| Audio | Gap | mixer/spatialization/assets/streaming/tooling evidence |
| UI / editor tools | Minimal runtime UI only | editor workflows, inspection, authoring, undo/redo and runtime UI evidence |
| Genuine 2D | Unresolved | true 2D render/physics/animation/tile/UI workflow and matched reference scene |
| Networking | Gap | replication/session/serialization/latency/recovery evidence |
| Profiling | Improved: frame, phase, process memory, provenance, stream coherence, environment/run control, and now matched capture-pair contract | runtime capture-state proof, measured instrumentation overhead, GPU timing, approved budgets |
| Packaging / platforms | Hosted Windows package/provenance contracts only | clean-machine package launch, additional approved platforms, install/update/uninstall evidence |
| Terrain / foliage | Gap | researched measurable requirements and implementation |
| Particles / VFX | Gap | researched measurable requirements and implementation |
| Cinematics | Gap | sequence/camera/timeline/audio workflow evidence |
| Scripting / reflection / plugins | Gap | runtime/editor extension model, safety/versioning evidence |
| Input / replay | Live input exists; benchmark suppression proven | input abstraction, remap/device coverage, deterministic replay evidence |
| Accessibility / localization | Gap | localization pipeline, text/input accessibility, settings evidence |
| Profiling comparison catalogue | Pairing prerequisite now stronger | matched UE5/Unity 3D and 2D workloads on specified hardware/settings |
| Reliability / recovery | Contract-level hosted checks; issue #7 still open | registered-local stress/recovery, clean-machine checks, actual 86,400-second soak |
| Independent acceptance | Not established | separate reviewer/worker evidence for final gates |

No row is removed or downgraded to make the comparison target easier.

## Native handoff

The registered Windows executor may now create a profiled/control pair, but should not interpret overhead unless launch state is retained precisely.

Both runs must use:

- identical candidate/package bytes;
- identical machine and driver state;
- `ASTRAL_SIMULATION_FIXED_HZ=60`;
- `ASTRAL_BENCHMARK_MODE=1`;
- `ASTRAL_BENCHMARK_WARMUP_FRAMES=120`;
- `ASTRAL_BENCHMARK_MEASURED_FRAMES=3600`;
- one identical manifest-declared client resolution for both runs;
- `run_protocol.vsync=false`;
- separate fresh run-control receipt paths;
- 2.0 seconds descriptor warmup and 60.0 seconds descriptor sample duration;
- exact command line, complete relevant environment snapshot, source SHA, executable SHA-256, machine/toolchain/driver identity, UTC start/end timestamps, stdout/stderr, exit code, and artifact hashes.

Profiled run additionally enables and retains:

- whole-frame CSV, warmup 120, max samples 3600;
- phase-timing CSV, warmup 120, max samples 3600;
- process-memory CSV, warmup 120, declared sampling stride and sufficient max samples.

Control run must omit the whole-frame, phase-timing, and process-memory capture environment variables. Its manifest must omit the corresponding three evidence roles. Do not delete the raw launch-environment receipt merely because the pair verifier passes.

Required post-run order:

1. verify each package/benchmark manifest;
2. verify each run-control receipt;
3. verify the native Windows environment receipt for each manifest;
4. run stream coherence for the profiled run;
5. run `verify_benchmark_capture_pair.py` against the profiled and control manifests;
6. only then consider a separate bounded instrumentation-overhead analysis packet.

Example pair command shape after the two manifests exist:

```text
python Scripts/verify_benchmark_capture_pair.py ^
  --profiled-manifest <profiled-benchmark.json> ^
  --control-manifest <control-benchmark.json> ^
  --package-root <exact-staged-package> ^
  --release-manifest <exact-staged-package>\MANIFEST.json ^
  --profiled-evidence-root <profiled-evidence-root> ^
  --control-evidence-root <control-evidence-root> ^
  --output <fresh-pair-receipt.json>
```

The output is pairing evidence only. `instrumentation_overhead_verified` remains false.

## Limits and next action

Not established by this packet:

- actual profiling overhead;
- proof that the control process launched with all profiling environment variables absent;
- native GPU timestamps or presentation timing;
- active-render-adapter identity;
- VRAM;
- approved frame-time/RAM/VRAM budgets;
- matched UE5/Unity 3D or genuine-2D performance;
- clean-machine compatibility;
- stress/recovery acceptance;
- the actual 86,400-second soak;
- independent review or acceptance.

Single next useful coordinator action: add a bounded, package/run-control-bound profiling capture-state receipt that records at process startup whether whole-frame timing, phase timing, and process-memory capture were enabled, their warmup/limits/stride, and their output paths/hashes without leaking unrelated environment data. Make the capture-pair verifier require `profiled=enabled` and `control=disabled` receipts before any later instrumentation-overhead analyzer can promote `instrumentation_overhead_verified`.
