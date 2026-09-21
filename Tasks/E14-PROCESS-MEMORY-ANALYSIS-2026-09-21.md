# E14 bounded process-memory analysis packet, 2026-09-21

Status: admitted bounded analysis packet on draft PR #9. This packet converts the already implemented frame-indexed process-memory CSV into provenance-bound descriptive evidence. It does not run Astral on Lucas's PCs, invoke R0, establish a RAM or VRAM budget, prove leak freedom, add allocator attribution, merge/release/deploy anything, add a dependency, change the graphics architecture, or restart paused game-content work.

## Identity and dependency

- Repository: `LucasKazaki/AnimeRPG`.
- Owned branch / existing draft PR: `repair/2026-09-20-r0-runner-safety`, PR #9.
- Pre-packet branch head: `bbd0ad8cdd75f62b509ad0fdd15a7cb86ac1cdc7`.
- Immediate implementation dependency: E14 process-memory capture candidate `abc91216322e8ce1fe4e1eea0ee3896551e1e996` and its QA follow-up at the pre-packet head.
- Stacked dependency retained by PR #9: PR #6 / `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`.
- Issue #7 remains open. The historical R0 runner must not be invoked by this packet.
- Capability-map impact: E14 process-memory analysis only. The rest of E00-E17, including native/independent acceptance, remains unresolved according to existing evidence.

## Reproducible gap

Astral can now emit a bounded CSV containing frame-indexed current working set, peak working set, private commit charge and page-fault count. The existing benchmark manifest can hash and bind arbitrary evidence files, but there is no parser/analyzer that checks the process-memory schema and claim metadata, re-verifies the exact package/benchmark identity, and emits first-versus-last-window and frame-trend statistics without converting those observations into a leak-free or budget claim.

Acceptance for this packet is therefore limited to a fail-closed analyzer plus regression/hosted verification. Native RAM measurements still require the registered Windows executor and later independent review.

## Allowed paths

Only these paths may change in this packet:

1. `Scripts/analyze_process_memory_capture.py`
2. `Scripts/test_process_memory_analysis.py`
3. `.github/workflows/frame-timing-validation.yml`
4. `Tasks/E14-PROCESS-MEMORY-ANALYSIS-2026-09-21.md`
5. `Docs/QA/E14-PROCESS-MEMORY-ANALYSIS-2026-09-21.md`

No Engine/Game/CMake source, renderer/API, dependency, package authority, Company Runtime state, scheduler, permission, release, deployment or content path is authorized.

## Primary-source research

Sources rechecked on 2026-09-21 before implementation:

1. Microsoft Learn, `PROCESS_MEMORY_COUNTERS` (`psapi.h`): https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-process_memory_counters
   - Defines `PageFaultCount` as the number of page faults, `PeakWorkingSetSize` as peak working-set bytes, and `WorkingSetSize` as current working-set bytes.
   - Applicability: the analyzer treats page faults and peak working set as cumulative/peak counters within one process capture and rejects decreases that contradict those field semantics.
2. Microsoft Learn, `PROCESS_MEMORY_COUNTERS_EX`: https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-process_memory_counters_ex
   - Defines `PrivateUsage` as process Commit Charge, the total private memory committed for the running process.
   - Applicability: report private-commit descriptive statistics without treating commit charge as allocator ownership, RSS, VRAM or a leak verdict.
3. Microsoft Learn, `GetProcessMemoryInfo`: https://learn.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-getprocessmemoryinfo
   - Retrieves the process memory counter structure used by the production capture.
   - Applicability: this analyzer verifies the serialized evidence contract; it does not call the Windows API itself.
4. Epic Games, Unreal Engine 5.8, Memory Insights: https://dev.epicgames.com/documentation/unreal-engine/memory-insights-in-unreal-engine
   - UE records allocation/free events with callstacks and LLM tags, supports live-allocation and growth/decline queries, and includes leak-oriented investigation.
   - Applicability: Astral's process totals and trends remain materially narrower and cannot be called Memory Insights parity.
5. Epic Games, Unreal Engine 5.8, Low-Level Memory Tracker: https://dev.epicgames.com/documentation/unreal-engine/using-the-low-level-memory-tracker-in-unreal-engine
   - LLM tracks engine/OS allocation categories with scoped tags and can emit CSV data.
   - Applicability: allocator/tag attribution remains a separate Astral gap.
6. Unity 6.0 / 6000.0, Memory Profiler package 1.1.9: https://docs.unity3d.com/6000.0/Documentation/Manual/com.unity.memoryprofiler.html
   - Unity describes Memory Profiler as allocation-focused tooling for reducing memory usage across small and AAA projects.
   - Applicability: Astral's process-level CSV is only an initial measurable RAM layer.

No proprietary UE/Unity source is copied. No external library is imported. The implementation uses only Python standard-library modules already used by the repository tooling.

## Implementation contract

Add `Scripts/analyze_process_memory_capture.py` with these fail-closed behaviors:

- Accept only the exact Astral process-memory schema and fixed claim metadata emitted by `ProcessMemoryCapture`.
- Accept only the production Windows sampler label or the explicit caller-supplied contract-fixture label. The latter must never imply native evidence.
- Require canonical unsigned decimal rows, exact five-column header, at least one measured sample, bounded row count, valid saturation semantics and exact frame stride beginning at the declared warmup frame.
- Require `peak_working_set_bytes >= working_set_bytes`, nondecreasing lifetime peak, and nondecreasing page-fault count within one process capture.
- Re-verify the production benchmark manifest and release package through `benchmark_manifest`, then require the exact expected 40-hex candidate revision and 64-hex `AstralGame.exe` SHA-256.
- Require exactly one benchmark evidence entry with role `process_memory_csv`; reject path traversal, drive/backslash paths, symlinked roots/components, changed evidence bytes/hashes and duplicate/missing role binding.
- Produce descriptive min/mean/p50/p95/p99/max for working set, peak working set, private commit and page-fault count; first/last decile medians; last-minus-first decile median; and OLS slope per 1,000 frame indices. Page faults additionally retain first/last/delta.
- Do not fabricate time-rate evidence. The CSV has frame indices but no per-sample timestamps, so the analyzer must label its trend axis as `frame_index` and state that elapsed-time rate is unavailable.
- Refuse to overwrite an existing analysis receipt.
- Keep `ram_budget_verified`, `memory_leak_free_verified`, `vram_budget_verified`, `allocator_attribution_verified`, `comparative_parity_verified`, `instrumentation_overhead_verified`, and `independent_acceptance` hard-false.

## Verification contract

Regression coverage must include:

- known percentiles, first/last windows and exact frame-index slopes;
- missing, duplicate and unknown metadata;
- claim/source laundering attempts;
- noncanonical/negative/floating numeric fields;
- warmup/stride violations;
- current working set exceeding peak and decreasing lifetime peak;
- decreasing page-fault count;
- sample caps/saturation plus incomplete-but-descriptive captures;
- 250 seeded single-character mutations that either fail closed or fully revalidate all fixed claim boundaries;
- exclusive no-overwrite output;
- integration with the real repository release and benchmark manifest modules;
- rejection after raw evidence tampering or expected candidate/hash mismatch.

The existing profiling portability workflow must run the exact analyzer suite before the production-linked C++ Debug, optimized Release and Clang ASan+UBSan/leak capture contracts. Full Windows/package workflows remain separate existing gates and must stay green for the final candidate.

## External outputs and local handoff

A disposable parser fixture may run under `/tmp` and is not native Windows evidence. All repository builds remain outside the source tree. This coordinator does not operate the Company Runtime or Lucas's PCs.

For the future registered native benchmark, include the immutable process-memory CSV in the existing benchmark manifest as role `process_memory_csv`, then run this analyzer using the exact admitted package revision and independently retained executable hash. Retain the raw CSV, benchmark/release manifests, analysis JSON, exact package/source hashes, machine/OS/toolchain/CPU/RAM/GPU/driver identity, resolution/window/VSync settings, command/environment, UTC timestamps, logs, exit codes and required captured images. Run the matched capture-off control separately so instrumentation overhead is not silently ignored.

A passing analysis receipt is descriptive evidence only. It does not accept a RAM budget, leak-free state, VRAM budget, parity, clean-machine packaging, 24-hour soak or engine completion.

## Rollback and stop conditions

- Stop on the first deterministic parser/integration/CI failure until understood and repaired within these five paths.
- Never weaken a schema, provenance or acceptance guard just to make the lane green.
- If production-manifest integration fails in hosted CI, preserve the failure and do not substitute the partial fixture.
- Rollback is limited to this packet's commits/paths. Do not force-push, rewrite history or discard another worker's work.
- Keep PR #9 draft, issue #7 open, R0 uninvoked and all native/clean-machine/soak/independent-acceptance gates unresolved unless separate direct evidence exists.
