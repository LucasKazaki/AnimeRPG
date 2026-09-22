# R0 package soak telemetry analysis, 2026-09-20

## Why this packet exists

PR #9 now has package-manifest integrity checks, prerequisite/runtime checks, package-bound runtime smoke, restart stress, and a continuous Windows soak monitor. The monitor retains OS-level process telemetry, but its raw JSONL evidence still needs an independent, bounded integrity and trend-analysis step before a reviewer can use it efficiently.

This packet adds a dependency-free post-soak analyzer. It does not modify Astral Engine behavior, native runtime behavior, the R0 runner, CMake, graphics, game content, or the Company Runtime. Baseline is PR #9 head `6be62ba27ef754db33d136a1160fa0bbb497e15b`, stacked on PR #6. Issue #7 remains open.

## Allowed paths

- `Scripts/analyze_package_soak_telemetry.py`
- `Scripts/test_package_soak_telemetry_analysis.py`
- `.github/workflows/release-manifest-validation.yml`
- `Tasks/R0-PACKAGE-SOAK-ANALYSIS-2026-09-20.md`
- `Docs/QA/R0-PACKAGE-SOAK-ANALYSIS-2026-09-20.md`

No `Engine/`, `Game/`, `Tests/`, CMake, R0-runner, scheduler/runtime database, dependency installation, package publication, merge, release, graphics API, or content change is authorized by this packet.

## Primary-source basis

Accessed September 20, 2026:

1. Epic Games, **Memory Insights**, Unreal Engine 5.8: https://dev.epicgames.com/documentation/unreal-engine/memory-insights-in-unreal-engine . UE can analyze allocation/deallocation events, growth versus decline, long-lived allocations, and leak-oriented queries. Astral's OS-level soak telemetry is much narrower and cannot identify allocator callsites.
2. Unity 6.0, **Memory Profiler** package 1.1.9: https://docs.unity3d.com/6000.0/Documentation/Manual/com.unity.memoryprofiler.html . Unity provides engine-level allocation profiling for target applications.
3. Unity 6.0, **MemoryProfiler runtime API**: https://docs.unity3d.com/6000.0/ScriptReference/Unity.Profiling.Memory.MemoryProfiler.html . Unity can capture memory snapshots from a running Player; Astral currently has no equivalent allocator snapshot facility.
4. Microsoft, **Process Working Set**: https://learn.microsoft.com/en-us/windows/win32/procthread/process-working-set . Working set is resident pages and includes shared/private data, so it must not be treated as a direct leak metric.
5. Microsoft, **PROCESS_MEMORY_COUNTERS_EX**: https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-process_memory_counters_ex . `PrivateUsage`/`PagefileUsage` represent process commit charge and are distinct from working set.

No proprietary source is copied and no third-party dependency is introduced.

## Acceptance contract

Add a post-soak analyzer that:

- reads only a regular, non-symlink soak summary JSON and telemetry JSONL under explicit size/sample bounds;
- requires schema-1 evidence with an exact 40-hex source revision, exact 64-hex `AstralGame.exe` SHA-256, exact telemetry SHA-256, and receipt/sample-count agreement;
- rejects malformed JSON, blank lines, non-finite/negative values, boolean values masquerading as integers, non-monotonic elapsed time, and summary/telemetry mismatches;
- rejects any source receipt that has prematurely upgraded soak, RAM/VRAM/frame-time, leak-free, clean-machine, desktop-ownership, or independent-acceptance claims;
- optionally re-binds analysis to operator-supplied expected commit and executable SHA-256;
- emits descriptive first/last/min/max/delta, first/last-decile medians, p50/p95/p99, OLS slope per hour, and sample-interval statistics for working set, private/commit usage, handle count, GDI objects, and USER objects;
- treats all trend statistics as descriptive evidence only, not approved thresholds or leak proof;
- sets only telemetry-integrity and receipt-binding claims true when validation succeeds;
- keeps required-soak acceptance, RAM/VRAM/frame-time budgets, leak-free status, clean-machine compatibility, desktop ownership, and independent acceptance false by design.

Hosted CI runs the contract suite only. It does not generate a 24-hour native soak, approve memory thresholds, or claim allocator-level evidence.

## Local evidence handoff

After a real package-bound continuous soak produces `<soak-summary.json>` and `continuous-soak-telemetry.jsonl`, independent QA can run:

```powershell
python Scripts/analyze_package_soak_telemetry.py `
  <soak-summary.json> `
  <continuous-soak-telemetry.jsonl> `
  --expected-commit <40-hex-admitted-revision> `
  --expected-executable-sha256 <independently-recorded-AstralGame-sha256> `
  --json <external-evidence>\package-soak-analysis.json
```

Retain the analyzer JSON beside the original soak receipt, telemetry JSONL, package/prerequisite receipts, machine/OS/toolchain/driver identity, logs, and exact command lines. A reviewer should use allocator-level or engine-level profiling when a resource trend needs root-cause attribution.

## Explicit limits

The analyzer does not collect telemetry itself. It does not measure VRAM or frame time, identify allocation callsites, distinguish intentional caches from leaks, define RAM/handle growth budgets, or establish clean-machine/desktop/independent acceptance. A positive OLS slope or decile delta is a diagnostic observation, not a failure threshold. A flat OS-level trend is not proof that the engine is leak-free.

## Stop and rollback

Stop on stale/mismatched hashes, malformed evidence, source-claim inconsistency, sample-order errors, summary mismatch, or analysis limits. Do not weaken validation to recover a receipt. Rollback is the commits for this packet on PR #9. Do not invoke R0 merely because this analyzer exists.
