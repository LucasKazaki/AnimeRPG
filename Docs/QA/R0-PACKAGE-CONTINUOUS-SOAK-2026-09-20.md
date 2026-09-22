# R0 package continuous-soak QA, 2026-09-20

Status: **implementation/contract evidence only, native package soak not executed**.

Candidate baseline before this packet: PR #9 head `f984a649a30a08d4f5029040d4788c712f831d25`.

## Implemented boundary

`Scripts/run_package_continuous_soak.py` adds an external package-bound monitor for one continuously running `AstralGame.exe`. It verifies package bytes before launch and after shutdown, requires a fresh external runtime directory, requires a visible game window in production, records incremental process telemetry, requests normal `WM_CLOSE` shutdown, and retains strict claim guards.

The monitor samples Windows process working set/private usage, pagefile usage, total handles, GDI objects, and USER objects. It intentionally does not claim frame time, VRAM, allocator leak freedom, clean-machine compatibility, owned interactive desktop, independent acceptance, or final 24-hour-soak approval.

## Primary sources

Accessed 2026-09-20:

- Epic UE 5.8 Memory Insights: https://dev.epicgames.com/documentation/unreal-engine/memory-insights-in-unreal-engine
- Epic UE 5.8 Gauntlet overview: https://dev.epicgames.com/documentation/unreal-engine/gauntlet-automation-framework-overview-in-unreal-engine
- Unity 6.0 performance testing API 3.2.0: https://docs.unity3d.com/6000.0/Documentation/Manual/com.unity.test-framework.performance.html
- Microsoft GetProcessMemoryInfo: https://learn.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-getprocessmemoryinfo
- Microsoft GetProcessHandleCount: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocesshandlecount
- Microsoft GetGuiResources: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getguiresources

## Portable sandbox evidence before publication

Executed in an isolated Linux sandbox against the exact local candidate source:

```text
python -m py_compile run_package_continuous_soak.py test_package_continuous_soak.py
python test_package_continuous_soak.py
```

Result: Python compile **PASS**. Contract suite: **11 discovered, 10 passed, 1 expected Windows-only skip**, exit 0.

Covered contracts include exact revision/hash validation, unsafe runtime-root rejection, early process exit, telemetry failure and owned-process cleanup, missing-window failure, graceful-close failure, post-run package mutation invalidation, interruption with retained JSONL samples, summary calculations, and strict non-native claim guards. The Windows-only test starts a real child process and exercises the documented process-memory/handle/GUI-resource APIs when the suite runs on Windows.

Pre-publication SHA-256:

- `run_package_continuous_soak.py`: `bb449b929aad1b8ca00c559b07100e1e4428272e0954869641d25e8a47d0e85f`
- `test_package_continuous_soak.py`: `7d816d70f6996f3fc2bb48ade6a057c108b05e7f1969741b79462624c9e08e03`

## Pending hosted/native evidence

Hosted `windows-2022` must pass the exact published contract suite, including the real Windows telemetry API probe. That still will not launch the packaged GUI or establish native soak evidence.

The registered local executor must separately run the exact package on an exclusively owned supported Windows desktop after prerequisite and package-smoke gates. A useful final run is the repository-required 86,400 seconds with 10-second sampling. Retain the summary JSON, JSONL telemetry, `astral.log`, exact package/revision hashes, machine/toolchain/driver identity, command, and failure evidence.

No R0 invocation, merge, release, installer execution, or engine/game/CMake modification is part of this packet.
