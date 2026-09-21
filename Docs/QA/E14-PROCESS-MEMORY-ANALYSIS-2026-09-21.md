# E14 process-memory analysis QA checkpoint, 2026-09-21

## Checkpoint identity

- Repository: `LucasKazaki/AnimeRPG`.
- Existing owned draft PR: #9, branch `repair/2026-09-20-r0-runner-safety`.
- Pre-packet branch head: `bbd0ad8cdd75f62b509ad0fdd15a7cb86ac1cdc7`.
- Implementation candidate verified by hosted CI: `a5fd9bc70a73a88d95691cee0907bbf90b595f03`.
- This QA file is the documentation-only follow-up. Its own commit is not substituted for the implementation candidate in the receipts below.
- Issue #7 remains open. The historical R0 runner was not invoked.
- PR #9 remains draft. Nothing was merged, released, deployed, installed, or published to end users.

## Selected gap and result

The prior E14 packet added a frame-indexed Windows process-memory CSV, but Astral had no provenance-bound analyzer for that stream. This packet adds `Scripts/analyze_process_memory_capture.py` and a 13-test regression suite, then wires the suite into the existing profiling portability workflow.

The analyzer accepts only the exact process-memory schema emitted by the production capture, re-verifies the existing release and benchmark manifests, requires the expected candidate commit and exact `AstralGame.exe` SHA-256, requires exactly one `process_memory_csv` benchmark evidence entry, independently re-hashes the raw CSV, and rejects path traversal/symlink/claim/schema tampering.

For each measured stream it reports descriptive min/mean/p50/p95/p99/max, first/last decile medians, last-minus-first decile median, and ordinary-least-squares slope per 1,000 frame indices. Page faults additionally retain first/last/delta. The raw CSV has no per-sample timestamps, so the analyzer deliberately labels the trend axis as `frame_index` and does not fabricate bytes-per-second or faults-per-second rates.

Acceptance fields remain hard-false for RAM budget, leak freedom, VRAM budget, allocator attribution, comparative parity, instrumentation overhead and independent acceptance. A green receipt therefore describes one bound process-level observation only.

## Primary-source research

Sources were rechecked on 2026-09-21 before implementation.

1. Microsoft Learn, `PROCESS_MEMORY_COUNTERS`: https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-process_memory_counters
   - `PageFaultCount` is the number of page faults, `PeakWorkingSetSize` is peak working-set bytes, and `WorkingSetSize` is current working-set bytes.
   - Applicability: within one short process capture, the analyzer rejects a decreasing declared lifetime peak or page-fault count and rejects a current working set above the declared peak.
2. Microsoft Learn, `PROCESS_MEMORY_COUNTERS_EX`: https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-process_memory_counters_ex
   - `PrivateUsage` is process Commit Charge, the total private memory committed for the running process.
   - Applicability: private-commit statistics are not allocator ownership, RSS, VRAM or a leak verdict.
3. Microsoft Learn, `GetProcessMemoryInfo`: https://learn.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-getprocessmemoryinfo
   - This is the API used by the production capture. This packet analyzes serialized evidence and does not call the API itself.
4. Epic Games, Unreal Engine 5.8, Memory Insights: https://dev.epicgames.com/documentation/unreal-engine/memory-insights-in-unreal-engine
   - UE exposes allocation/free events, callstacks, LLM tags, live allocations and growth/decline/leak-oriented queries.
   - Applicability: Astral's process totals and frame trends remain materially below UE memory-profiler attribution depth.
5. Epic Games, Unreal Engine 5.8, Low-Level Memory Tracker: https://dev.epicgames.com/documentation/unreal-engine/using-the-low-level-memory-tracker-in-unreal-engine
   - LLM uses scoped tags to account for engine and OS allocations and can emit CSV values.
   - Applicability: tagged allocator accounting remains a separate Astral gap.
6. Unity 6.0 / 6000.0, Memory Profiler package 1.1.9: https://docs.unity3d.com/6000.0/Documentation/Manual/com.unity.memoryprofiler.html
   - Unity describes Memory Profiler as allocation-focused tooling for memory investigation across small through AAA projects.
   - Applicability: this Astral analyzer is an initial process-level evidence layer, not Unity Memory Profiler parity.

No proprietary UE/Unity source was copied. No third-party package was added. The analyzer uses Python standard-library modules only.

## Changed paths

Relative to `bbd0ad8cdd75f62b509ad0fdd15a7cb86ac1cdc7`, the implementation candidate changes exactly four paths:

1. `.github/workflows/frame-timing-validation.yml`
2. `Scripts/analyze_process_memory_capture.py`
3. `Scripts/test_process_memory_analysis.py`
4. `Tasks/E14-PROCESS-MEMORY-ANALYSIS-2026-09-21.md`

This documentation follow-up adds only:

5. `Docs/QA/E14-PROCESS-MEMORY-ANALYSIS-2026-09-21.md`

No Engine/Game/CMake source, renderer/API, dependency, content, package-authority or Company Runtime path changed.

Published exact-source Git blobs at the implementation candidate:

- `Scripts/analyze_process_memory_capture.py`: `b67dbf72551f1adef160500be3de21bc9e939930`.
- `Scripts/test_process_memory_analysis.py`: `b191cf230a52f850d665bcd36754e01d5fd732c0`.

## Regression contract

The 13-test process-memory analyzer suite covers:

- known percentiles, first/last windows and exact frame-index slopes;
- missing/duplicate/unknown metadata;
- claim and sample-source laundering;
- noncanonical, negative, floating and textual numeric fields;
- warmup/stride violations;
- current working set above peak and decreasing lifetime peak;
- decreasing page-fault count;
- sample caps, saturation and incomplete descriptive captures;
- 250 deterministic single-character mutations that either fail closed or fully revalidate fixed claim boundaries;
- exclusive no-overwrite JSON output;
- real repository release/benchmark-manifest integration;
- raw-evidence tampering rejection;
- expected candidate and executable-hash mismatch rejection.

## Sandbox evidence

The sandbox could not clone the public GitHub repository because its container DNS path returned `Could not resolve host: github.com`. No shared/local project worktree was touched. A disposable partial fixture under `/tmp/astral-mem-analysis` was instead created from the exact intended analyzer/test bytes before publication.

Attempted and passed:

```text
python -m py_compile analyze_process_memory_capture.py test_process_memory_analysis.py
python test_process_memory_analysis.py
```

Result: 13 tests discovered, 10 parser/output tests PASS, 3 production-binding tests intentionally SKIP because the partial fixture does not contain repository `benchmark_manifest` / `release_manifest` modules. Runtime: 0.070 s. The fixture also ran all 250 deterministic mutation cases inside the passing mutation test.

Exact pre-publication file SHA-256 values:

- analyzer: `ef5cc3a8eaa4efe7e4e3aa9040ee461129bfa1aedbc694911950e10b523079b6` (20,882 bytes);
- tests: `338bec3b799ff703f5bd22aefeab041558f94211ac716227d6b58b619dcc3847` (14,526 bytes).

`git hash-object` on those local bytes produced the same Git object IDs later published for both files, so the partial parser tests are byte-identical to the published analyzer/test files. They are still not a substitute for full repository integration, which is supplied by hosted CI below.

## Hosted verification of the exact implementation candidate

All three pull-request workflows for candidate `a5fd9bc70a73a88d95691cee0907bbf90b595f03` completed successfully.

### Profiling portability

- Run `35583434824`, job `106281230916`, Ubuntu 24.04.
- PASS: checkout.
- PASS: `Verify profiling analyses and production provenance binding`, including the new 13-test process-memory suite against the full repository, where production-manifest integration is available rather than skipped.
- PASS: Debug and optimized Release production-linked profiling capture contracts.
- PASS: Clang ASan+UBSan profiling contracts with leak checking.
- Job conclusion: success.

### Windows Debug / Release regression

- Run `35583434685`, job `106281231540`, `windows-2022`.
- PASS: R0 safety contracts only; R0 itself was not invoked.
- PASS: PE dependency, Windows prerequisite/runtime/bootstrap, Release assertion/CTest safety contracts.
- PASS: real Visual Studio 2022 x64 Debug build and deterministic Debug tests.
- PASS: real Release build, dependency/prerequisite/runtime checks and deterministic Release tests.
- PASS: milestone verifiers and clean tracked-tree gate.
- Job conclusion: success.

This packet changes no C++ source, so this Windows lane is regression evidence, not a new native GUI/process-memory benchmark.

### Release / package / provenance regression

- Run `35583434676`, job `106281230932`, `windows-2022`.
- PASS: release-manifest, runtime-receipt, restart-stress, continuous-soak contract, soak-analysis, benchmark-manifest and PE reproducibility contract steps.
- PASS: build the same Release candidate twice, classified byte-identical.
- PASS: fresh Release build, exact staged-package verification, hosted benchmark-manifest fixture and clean-tree gate.
- Job conclusion: success.

The hosted benchmark fixture remains schema/provenance evidence only. It is not a native performance/RAM run, clean-machine package launch, real soak or parity measurement.

## Capability-to-evidence map after this packet

No capability is promoted to comparable or independently accepted.

| Capability area | Evidence after this packet | Remaining material gap |
| --- | --- | --- |
| runtime / jobs / memory | R0 safety contracts; frame-indexed OS process-memory capture; provenance-bound descriptive analyzer | native R0 acceptance; real job system; allocator/tag attribution; approved memory budgets |
| scene ownership / serialization | existing inventory only | stable IDs, lifetime/ownership rules, versioned round trips and corruption recovery |
| asset pipeline | bounded mesh validation remains on separate draft PR #8 | importer/reimport/cache/dependency/cook lifecycle and native acceptance |
| GPU rendering / materials | existing Win32/GDI baseline | approved modern GPU backend/material system, GPU timing and matched image evidence |
| lighting / shadows / reflections | incomplete inventory | implementations plus matched quality/cost evidence |
| large-world streaming / detail | incomplete inventory | terrain, streaming, LOD/HLOD/large-scene memory evidence |
| animation | incomplete inventory | skeletal import/runtime/blending/state/tooling evidence |
| physics / collision | incomplete inventory | broad/narrow phase, queries/bodies/layers, stable 2D/3D stress evidence |
| AI / navigation | incomplete inventory | navigation/pathfinding/behavior execution/cancellation/budget evidence |
| audio | incomplete inventory | decode/stream/mix/spatial/device recovery/tooling evidence |
| UI / editor tools | incomplete inventory | scene/asset inspectors, gizmos, undo/redo and editor/runtime workflow |
| genuine 2D | explicitly required | sprites/batching/layers/tilemaps/2D camera/physics plus matched reference workflow |
| networking | incomplete inventory | schema/authority/replication/transport/loss/reorder/prediction/disconnect evidence |
| profiling | package provenance; deterministic builds; whole-frame/phase timing; process-memory capture; new process-memory descriptive analysis | native measured workloads, cross-stream coherence, GPU timing, thread/task/allocator attribution, RAM/VRAM budgets, overhead measurement |
| packaging / platforms | deterministic Release/package contract tooling | native package launch, clean machine, platform matrix, actual recovery stress and 24-hour soak |
| remaining catalogue | terrain/foliage, VFX/particles, cinematics, scripting/reflection/plugins, input/replay, accessibility/localization and extra platforms remain explicit | research, implementation, tests and matched workflows for each |
| integrated parity | not established | matched versioned Astral/UE5/Unity 3D and 2D workloads with correctness, performance, memory, reliability, tooling and independent acceptance |

## Native executor handoff

This coordinator did not execute Lucas's PCs or Company Runtime. Once the registered Windows executor owns an exclusive interactive desktop and the existing prerequisite/package gates pass, the frozen procedural 3D benchmark can collect 120 warmup frames plus 3,600 measured frames with whole-frame, phase and process-memory streams. The process-memory evidence entry in the benchmark manifest must use role `process_memory_csv`.

Then run:

```powershell
python Scripts/analyze_process_memory_capture.py `
  --benchmark-manifest <benchmark.json> `
  --package-root <exact-package-root> `
  --release-manifest <release-manifest.json> `
  --evidence-root <immutable-evidence-root> `
  --expected-commit <40-hex-admitted-revision> `
  --expected-executable-sha256 <64-hex-AstralGame.exe-sha256> `
  --output <fresh-process-memory-analysis.json>
```

Retain source/package hashes, manifests, raw CSV and analysis hashes, machine/OS/toolchain/CPU/RAM/GPU/driver identity, resolution/window/VSync settings, exact command/environment, UTC timestamps, stdout/stderr, runtime logs, exit codes and required images. A matched capture-off control is still required before instrumentation overhead can be evaluated.

## Limits and unresolved acceptance

This pass establishes analyzer code and hosted contracts only. It establishes none of the following:

- native 3,600-frame RAM behavior on Lucas's hardware;
- a RAM budget or proof of leak freedom;
- allocator/tag/callstack attribution;
- VRAM or GPU timing;
- instrumentation overhead;
- matched UE5/Unity 3D or genuine-2D comparison runs;
- clean-machine package launch;
- real failure-recovery stress;
- real 86,400-second soak;
- independent review/acceptance;
- engine parity or completion.

## Single next coordinator action

Before accepting a future multi-stream native benchmark receipt, add one bounded E14 cross-stream coherence verifier that proves the whole-frame timing, main-thread phase timing and process-memory evidence belong to the same benchmark descriptor/candidate/package and expected frame interval. It should reject mismatched warmup/sample ranges, hashes or candidate identities and keep all performance, RAM/VRAM, leak-free, parity and independent-acceptance claims false. The registered local executor may perform the already defined native benchmark independently under existing controls; this coordinator must not operate it.
