# E14 benchmark stream coherence QA checkpoint, 2026-09-21

## Checkpoint identity

- Repository: `LucasKazaki/AnimeRPG`.
- Existing owned draft PR: #9, branch `repair/2026-09-20-r0-runner-safety`.
- Pre-packet head: `6c6cbc95bb88810b13f5ab127d3b618363e05810`.
- Hosted implementation candidate: `ee9eb1008521fa1ce5f3b0d91cb7b6753b8263ab`.
- Issue #7 remains open. The historical R0 runner was not invoked.
- PR #9 remains draft and unmerged. Nothing was released, deployed, installed, or published.

## Selected gap and implementation result

Astral already had separate provenance-bound analyzers for whole-frame CPU intervals, Win32 main-thread phases, and Windows process-memory counters. It did not have one fail-closed receipt proving that all three raw streams belonged to the same verified benchmark/package identity and covered the same requested measured frame interval.

This packet adds `Scripts/verify_benchmark_stream_coherence.py`. It re-runs the production benchmark verifier, requires the exact admitted commit and `AstralGame.exe` hash, requires exactly one `cpu_frame_timing_csv`, `cpu_phase_timing_csv`, and `process_memory_csv`, independently resolves and hashes each file, and reuses the three production stream parsers.

The verifier requires identical warmup values; whole-frame and phase streams must have identical contiguous start/end/count plus compatible sample-cap/saturation state. Process memory may be downsampled, but every sample scheduled by its declared frame stride must be present within the timing interval and its final frame must be the exact final scheduled frame at or before the timing end. `local_native` provenance additionally requires `Windows_GetProcessMemoryInfo_PROCESS_MEMORY_COUNTERS_EX`; the contract fixture source cannot be promoted to native evidence.

The output receipt binds the benchmark descriptor, candidate/package identity, workload/protocol/environment/reference-version metadata, and exact path/size/SHA-256 of all three streams. Performance, RAM, VRAM, leak-free, GPU timing, parity, instrumentation-overhead, clean-machine, and independent-acceptance fields remain hard-false.

## Research basis

Primary sources were rechecked on 2026-09-21 before implementation:

1. Epic Games, Unreal Engine 5.8, Timing Insights: https://dev.epicgames.com/documentation/unreal-engine/timing-insights-in-unreal-engine
   - CPU/GPU activity is analyzed as related per-thread tracks within one trace session and selected time/frame ranges.
   - Applicability: related Astral evidence should retain one benchmark/session identity and explicit measured frame interval.
2. Epic Games, Unreal Engine 5.8, Trace: https://dev.epicgames.com/documentation/unreal-engine/trace-in-unreal-engine-5
   - Trace channels include Frame, CPU, GPU, memory-related channels, bookmarks, and screenshots in shared trace context.
   - Applicability: this packet copies no UE internals; it adopts only the evidence principle that related channels need common provenance.
3. Unity 6.0 / 6000.0, `FrameTimingManager`: https://docs.unity3d.com/cn/6000.0/ScriptReference/FrameTimingManager.html
   - The API captures/accesses timing data for multiple frames.
4. Unity 6.0 / 6000.0, `FrameTimingManager.CaptureFrameTimings`: https://docs.unity3d.com/jp/current/ScriptReference/FrameTimingManager.CaptureFrameTimings.html
   - Only complete valid finished frames are captured, so retained counts may vary.
   - Applicability: Astral verifies actual retained ranges instead of assuming requested counts imply coherent evidence.
5. Unity 6.0 / 6000.0, `Profiler.EmitSessionMetaData`: https://docs.unity3d.com/cn/6000.0/ScriptReference/Profiling.Profiler.EmitSessionMetaData.html
   - Session metadata is associated with an entire profiler capture; per-frame metadata is separate.
   - Applicability: Astral's immutable benchmark descriptor serves as session-level provenance for the three streams.
6. Unity 6.0 Profiler API: https://docs.unity3d.com/cn/6000.0/ScriptReference/Profiling.Profiler.html
   - Profiling itself can reduce application performance.
   - Applicability: a coherent receipt does not establish instrumentation overhead.
7. Unity 6.0 Performance Testing API 3.2.0: https://docs.unity3d.com/cn/6000.0/Manual/com.unity.test-framework.performance.html
   - Performance testing collects configuration metadata.
   - Applicability: Astral retains environment/protocol labels, but operator-supplied labels still require machine receipts.

No proprietary UE/Unity source was copied and no dependency was added.

## Exact changed paths and published identities

Relative to `6c6cbc95bb88810b13f5ab127d3b618363e05810`, candidate `ee9eb1008521fa1ce5f3b0d91cb7b6753b8263ab` changes exactly four paths:

1. `.github/workflows/frame-timing-validation.yml`
2. `Scripts/test_benchmark_stream_coherence.py`
3. `Scripts/verify_benchmark_stream_coherence.py`
4. `Tasks/E14-BENCHMARK-STREAM-COHERENCE-2026-09-21.md`

This QA follow-up adds only `Docs/QA/E14-BENCHMARK-STREAM-COHERENCE-2026-09-21.md`.

Published Git blob IDs for the implementation candidate:

- verifier: `da59229b01095071924c0bd884ed67b1a8fa07ca`;
- regression suite: `b5afa03e9aec96e3313ea97db79854f01936d589`;
- profiling workflow: `7cce963c75eb317c559aa730c29f3eae33987be3`;
- bounded task: `9ceea03a4cbb4a79f07e5464538d6f2aebd159d5`.

No Engine/Game/CMake source, renderer/API, dependency, package authority, Company Runtime, scheduler, permission, release, deployment, or content path changed.

## Regression contract

`Scripts/test_benchmark_stream_coherence.py` contains 13 bounded cases covering:

- coherent equal-rate streams;
- valid process-memory frame-stride downsampling;
- warmup mismatch rejection;
- whole-frame/phase interval mismatch rejection;
- missing scheduled process-memory samples;
- timing capture-limit mismatch;
- native-provenance fixture-source laundering rejection;
- exclusive no-overwrite output and hard-false acceptance fields;
- real production release/benchmark-manifest plus all-three-parser integration;
- raw timing evidence tamper rejection;
- expected candidate and executable-hash mismatch rejection;
- manifest-bound phase range mismatch rejection; and
- local-native benchmark rejection when process-memory evidence is only a caller-supplied contract fixture.

## Sandbox evidence

The container could not clone GitHub directly because its network path returned `Could not resolve host: github.com`; no local/shared project worktree was touched. A disposable partial fixture under `/tmp/astral-coherence` used the exact files later published.

Commands:

```text
python -m py_compile verify_benchmark_stream_coherence.py test_benchmark_stream_coherence.py
python test_benchmark_stream_coherence.py
```

Result: 13 tests discovered. The eight dependency-free alignment/provenance/output tests passed. Five full repository production-binding tests were intentionally skipped because the partial fixture omitted the repository manifest/analyzer modules. Runtime was approximately 0.001 s. Full repository integration is supplied by hosted CI below.

Exact local SHA-256 / Git blob identity before publication:

- verifier SHA-256 `82f2d08dc7494424a19a3866eecf675b6c297f28443ce2cee05b889d4e78610b`, Git blob `da59229b01095071924c0bd884ed67b1a8fa07ca`;
- tests SHA-256 `1186b80bce979354df33fd6b6fb1520d4c0ba480b664c63b0c417f6a06b878ba`, Git blob `b5afa03e9aec96e3313ea97db79854f01936d589`;
- workflow SHA-256 `faad6a4c89351ff335b89889a7feed35a8be5d8bef2238d1240ef194b3867ccd`, Git blob `7cce963c75eb317c559aa730c29f3eae33987be3`;
- task SHA-256 `6ba392caf719a417e1182e10739354d63f693e28d612b5778f19ea1865f7bcc1`, Git blob `9ceea03a4cbb4a79f07e5464538d6f2aebd159d5`.

## Hosted exact-source verification

All three pull-request workflows for `ee9eb1008521fa1ce5f3b0d91cb7b6753b8263ab` completed successfully.

### Profiling portability

- Run `35588765068`, job `106298135233`, Ubuntu 24.04.
- PASS: profiling analyzer/provenance step, which now executes `python Scripts/test_benchmark_stream_coherence.py` in the full repository alongside the three existing analyzer suites.
- PASS: production-linked profiling contracts in Debug and optimized Release.
- PASS: Clang AddressSanitizer + UndefinedBehaviorSanitizer profiling contracts with leak checking.
- Job conclusion: success.

### Windows Debug / Release regression

- Run `35588765073`, job `106298157281`, `windows-2022`.
- PASS: R0 runner safety contracts only; R0 itself was not invoked.
- PASS: PE dependency, prerequisite/runtime/bootstrap, and Release assertion/CTest safety contracts.
- PASS: actual Visual Studio 2022 x64 Debug build and deterministic Debug tests.
- PASS: actual Release build, runtime dependency/prerequisite/compatibility checks, deterministic Release tests, milestone verifiers, and clean tracked-tree gate.
- Job conclusion: success.

This packet changes no C++ source, so this lane is regression evidence, not a native GUI benchmark.

### Release / package / provenance regression

- Run `35588765066`, job `106298154993`, `windows-2022`.
- PASS: manifest, runtime receipt, restart-stress, continuous-soak contract, soak-analysis, benchmark-manifest, and PE reproducibility contract steps.
- PASS: repeated Release builds classified byte-identical.
- PASS: fresh Release build, exact staged-package verification, hosted benchmark-manifest fixture, and clean tracked-tree gate.
- Job conclusion: success.

The hosted fixture remains contract/provenance evidence only. It is not a native GUI/performance/RAM run, clean-machine package launch, real soak, or parity measurement.

## Capability-to-evidence map after this packet

No capability is promoted to comparable or independently accepted.

| Capability area | Evidence after this packet | Remaining material gap |
| --- | --- | --- |
| runtime / jobs / memory | R0 safety contracts; frame-indexed process-memory capture/analyzer | native R0 acceptance; real job system; allocator/tag attribution; approved memory budgets |
| scene ownership / serialization | existing inventory only | stable IDs, ownership/lifetime rules, versioned round trips, corruption recovery |
| asset pipeline | bounded mesh validation remains on separate draft PR #8 | importer/reimport/cache/dependency/cook lifecycle plus native acceptance |
| GPU rendering / materials | existing Win32/GDI baseline | approved modern GPU backend/material system, GPU timing, matched image evidence |
| lighting / shadows / reflections | incomplete inventory | implementation and matched quality/cost evidence |
| large-world streaming / detail | incomplete inventory | terrain, streaming, LOD/HLOD and large-scene memory evidence |
| animation | incomplete inventory | skeletal import/runtime/blending/state/tooling evidence |
| physics / collision | incomplete inventory | bodies/queries/layers and stable 2D/3D stress evidence |
| AI / navigation | incomplete inventory | navigation/pathfinding/behavior/cancellation/budget evidence |
| audio | incomplete inventory | decode/stream/mix/spatial/device recovery/tooling evidence |
| UI / editor tools | incomplete inventory | inspectors, gizmos, undo/redo, editor/runtime workflow |
| genuine 2D | explicitly required | sprites/batching/layers/tilemaps/camera/physics plus matched reference workflow |
| networking | incomplete inventory | schema/authority/replication/transport/loss/reorder/prediction/disconnect evidence |
| profiling | deterministic package provenance; whole-frame/phase/process-memory capture and analyzers; new cross-stream coherence receipt contract | native measured workload, GPU timing, thread/task/allocator attribution, RAM/VRAM budgets, instrumentation overhead |
| packaging / platforms | deterministic Release/package contract tooling | native package launch, clean machine, platform matrix, recovery stress, real 24-hour soak |
| remaining catalogue | terrain/foliage, VFX/particles, cinematics, scripting/reflection/plugins, input/replay, accessibility/localization and extra platforms remain explicit | research, implementation, tests and matched workflows for each |
| integrated parity | not established | matched versioned Astral/UE5/Unity 3D and 2D workloads with correctness, performance, memory, reliability, tooling and independent acceptance |

## Native executor handoff and unresolved acceptance

This coordinator did not operate Company Runtime or Lucas's PCs. The registered Windows executor can now collect the already specified frozen procedural 3D workload with 120 warmup frames, 3,600 whole-frame and phase samples, process-memory sampling on its declared stride, and a matched capture-off control. All three raw streams must be entries in the same benchmark manifest. Run `Scripts/verify_benchmark_stream_coherence.py` before interpreting the individual timing or memory analyses.

Retain source/package hashes, raw CSVs, manifest/analysis/coherence hashes, machine/OS/toolchain/CPU/RAM/GPU/driver identity, resolution/window/VSync, exact command/environment, UTC timestamps, stdout/stderr, exit codes, runtime logs, and required images.

Still unresolved: native 3,600-frame measurements; instrumentation overhead; GPU timing and VRAM; allocator/thread/task attribution; approved CPU/RAM/VRAM budgets; matched UE5/Unity 3D and genuine-2D workloads; clean-machine launch; real recovery stress; 86,400-second soak; independent review/acceptance; and overall engine parity.

## Single next useful action

Run the frozen native 3D benchmark on the registered Windows executor and require one benchmark manifest plus a passing cross-stream coherence receipt before accepting the timing or memory observations. If that native gate remains unavailable, the next coordinator pass should advance an independent E14 research/test packet rather than fabricate native progress or repeatedly poll unchanged local state.
