# R0 package continuous-soak evidence, 2026-09-20

## Why this packet exists

PR #9 now has exact package-manifest verification, one package-bound M10 runtime-smoke wrapper, and a bounded restart-stress sequence. Those checks repeatedly start and stop the application. They do not observe one `AstralGame.exe` process remaining alive for the repository's required 24-hour soak, and they do not retain operating-system RAM/resource telemetry during that continuous lifetime.

This packet adds a dependency-free continuous package-soak monitor. It is packaging/runtime QA only. It does not alter engine/game behavior, the native M10 smoke, the R0 runner, CMake, or the local scheduler. Baseline is PR #9 head `f984a649a30a08d4f5029040d4788c712f831d25`, stacked on PR #6. Issue #7 remains open.

## Allowed paths

- `Scripts/run_package_continuous_soak.py`
- `Scripts/test_package_continuous_soak.py`
- `.github/workflows/release-manifest-validation.yml`
- `Tasks/R0-PACKAGE-CONTINUOUS-SOAK-2026-09-20.md`
- `Docs/QA/R0-PACKAGE-CONTINUOUS-SOAK-2026-09-20.md`

No `Engine/`, `Game/`, `Tests/`, CMake, R0-runner, runtime-database, scheduler, dependency installation, package publication, merge, release, graphics API, or game-content change is authorized by this packet.

## Primary-source basis

Accessed September 20, 2026:

1. Epic Games, **Memory Insights**, Unreal Engine 5.8: https://dev.epicgames.com/documentation/unreal-engine/memory-insights-in-unreal-engine . UE records detailed allocation/deallocation information, live allocation counts, memory-growth/decline queries, and leak-oriented analysis. Astral's new OS-level working-set/private-memory sampler is intentionally much narrower and is not allocator-level leak proof.
2. Epic Games, **Gauntlet Automation Framework Overview**, Unreal Engine 5.8: https://dev.epicgames.com/documentation/unreal-engine/gauntlet-automation-framework-overview-in-unreal-engine . Gauntlet launches and monitors game sessions and preserves process/test results. Astral's monitor is a small single-process package QA primitive, not a Gauntlet equivalent.
3. Unity 6.0, **Performance testing API**, package 3.2.0: https://docs.unity3d.com/6000.0/Documentation/Manual/com.unity.test-framework.performance.html . Unity's performance-test package collects performance results and configuration metadata. Astral does not yet have comparable frame-time/VRAM instrumentation.
4. Microsoft, **GetProcessMemoryInfo**: https://learn.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-getprocessmemoryinfo . It reports process memory data through `PROCESS_MEMORY_COUNTERS[_EX]`.
5. Microsoft, **GetProcessHandleCount**: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocesshandlecount . It reports the number of open handles owned by a process.
6. Microsoft, **GetGuiResources**: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getguiresources . It reports GDI and USER object counts for a process in the current session.

No proprietary engine source is copied and no third-party dependency is introduced.

## Acceptance contract

Add a package-bound continuous-soak monitor that:

- requires an exact 40-hex admitted revision and exact SHA-256 for `AstralGame.exe` before creating its runtime output;
- verifies the package manifest before launch and after shutdown using the production `release_manifest.py` verifier;
- requires a fresh empty runtime root outside the immutable package;
- launches `AstralGame.exe` directly with no command-prefix indirection;
- on a native run requires Windows and a visible window belonging to the launched process within a bounded startup interval;
- supports native durations from 60 seconds through 90,000 seconds, including the repository-required 86,400-second soak, and sample intervals from 1 through 300 seconds;
- appends and flushes one JSONL telemetry record per sample so partial evidence survives interruption;
- samples Windows working set, peak working set, private usage, pagefile usage, open-handle count, GDI-object count, and USER-object count using documented OS APIs;
- fails if the process exits before the requested duration, process telemetry fails, the visible window never appears, the package changes, or a graceful `WM_CLOSE` shutdown cannot return exit code 0;
- terminates only the process tree it launched after failure, timeout, or interruption;
- records first/last/min/max/delta summaries without inventing a leak threshold or memory budget;
- permits `continuous_package_uptime_observed=true` only after a real Windows run using every production verifier/launcher/sampler/window/cleanup component;
- permits `required_24h_duration_observed=true` only when such a production observation actually lasts at least 86,400 seconds;
- keeps `required_24h_soak_verified`, RAM-budget, VRAM-budget, frame-time-budget, leak-free, clean-machine, owned-desktop, and independent-acceptance claims false by design pending separate QA evidence.

Synthetic/dependency-injected tests must never upgrade any native acceptance claim. Hosted CI executes the contract suite and a real Windows process-telemetry API probe only; it does not launch the packaged GUI or claim soak evidence.

## Local evidence handoff

Only after the exact package passes prerequisite/bootstrap checks and a package-bound M10 runtime smoke on the registered Windows executor should the continuous run be attempted on an exclusively owned supported desktop. Example:

```powershell
python Scripts/run_package_continuous_soak.py `
  <package>\MANIFEST.json <package> `
  --expected-commit <40-hex-admitted-revision> `
  --expected-executable-sha256 <independently-recorded-AstralGame-sha256> `
  --runtime-root <fresh-external-runtime-directory> `
  --duration-seconds 86400 `
  --sample-interval-seconds 10 `
  --window-wait-seconds 30 `
  --close-timeout-seconds 10 `
  --json <external-evidence>\package-continuous-soak.json
```

Retain the summary JSON, `continuous-soak-telemetry.jsonl`, `astral.log` if generated, package/prerequisite receipts, exact source and package hashes, machine/OS/toolchain/driver identity, command line, UTC/local timestamps, and any captured failure evidence. Independent QA must inspect the evidence before accepting the repository's soak gate.

## Explicit limits

This is not a frame-time profiler, VRAM profiler, allocator tracer, crash dump collector, or clean-machine installer test. Working-set growth alone cannot prove a memory leak. A 24-hour duration flag from this tool is only an observed runtime duration for the exact bound package. It does not self-approve the required soak or engine acceptance.

## Stop and rollback

Stop on malformed/mismatched revision or hash, unsafe paths, a nonempty runtime root, missing visible window, early exit, telemetry failure, package mutation, interruption, failed graceful close, or inability to preserve evidence. Never retry into the same runtime root. Rollback is the commit(s) for this packet on PR #9. Do not invoke R0 merely because this monitor exists.
