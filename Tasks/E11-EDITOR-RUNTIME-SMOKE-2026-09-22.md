# E11 Editor Runtime Smoke task, 2026-09-22

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. It may harden the native Windows smoke and its evidence, but it must not add scene mutation/serialization, gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed: `dece8fbb47b6a937b9bafafa3faea837a359fb4d`.
Current source head: `a5315740458aaea68933384cf83d5555e646f42d`.
Current smoke blob: `3ca40bccd15950ebd8774ed36902ab15213035bb`.
Integrated editor source fixture: `Tools/AstralEditorMain.cpp` blob `5142e632a79c89d0d0ce3efe87e752456f802185`; production editor source was not modified.

Allowed paths only: `CMakeLists.txt`, `Tests/EditorRuntimeSmoke.cpp`, this task, `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`, and `Docs/Research/ENGINE-CAPABILITIES.json`. One active writer only. Do not rebase, merge, force-push, or absorb unrelated game-worker work.

## Current finding

Independent Codex review of exact evidence head `00099a8f7e6c7b935a7cad71e31a33f10dbf3e00`, submitted 2026-09-22T22:30:32Z, found that the earlier 135-second work deadline began only after `CreateProcessW` returned. That allowed process creation itself to consume the 38-second cleanup margin before CTest's registered `TIMEOUT 180`.

Commit `a5315740458aaea68933384cf83d5555e646f42d` moved the deadline start to immediately before `CreateProcessW` and accounts for elapsed launch time after the call returns. This is a useful partial repair, and all hosted checks for that exact head passed. It does not, however, establish a hard upper bound on the synchronous `CreateProcessW` call itself.

Microsoft documents `CreateProcessW` as a synchronous API that creates a process and returns before the new process has finished initialization. The API exposes no timeout parameter. Therefore the current source cannot prove that a pathological or externally delayed `CreateProcessW` call will return early enough for the smoke's 5-second normal-close allowance plus 2-second forced-cleanup allowance to complete before CTest kills the smoke process.

Portable timing fixture `/tmp/e11_launch_budget_fixture.cpp`, SHA-256 `45ac2935e40181703e6f0f7d41bfd486cd8145930bdc17638d9a1d958c976f32`, demonstrates the remaining proof gap without claiming a Win32 stall was observed. With the deadline starting before launch, a launch return at 172999 ms still leaves time for the 7-second close/cleanup allowance, while returns at 173000 ms or 179000 ms can overrun the 180000 ms external limit. GCC C++17 warning-clean execution passed; Clang C++17 ASan+UBSan warning-clean execution passed with no sanitizer finding.

## Research basis, rechecked 2026-09-22

Primary references:

- Microsoft Learn, `CreateProcessW`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw
- Microsoft Learn, `GetTickCount64`: https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-gettickcount64
- CMake, `TIMEOUT` test property: https://cmake.org/cmake/help/latest/prop_test/TIMEOUT.html
- Microsoft Learn, Job Objects: https://learn.microsoft.com/en-us/windows/win32/procthread/job-objects
- Microsoft Learn, `UpdateProcThreadAttribute`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-updateprocthreadattribute
- Microsoft Learn, `JOBOBJECT_BASIC_LIMIT_INFORMATION`: https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-jobobject_basic_limit_information
- Epic Games, UE 5.8 Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
- Epic Games, UE 5.8 Unreal Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
- Unity Technologies, Unity 6.0 Hierarchy: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html

Microsoft documents `PROC_THREAD_ATTRIBUTE_JOB_LIST` for assigning job handles to a child at process creation on Windows 10+/Windows Server 2016+, and `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE` for terminating associated processes when the last job handle closes. Those APIs are the leading bounded repair direction because they can make external termination of the smoke fail closed with respect to its owned child process, even if launch itself cannot be given a timeout. This is a proposed next implementation, not a completed claim. No proprietary source was copied and no dependency was imported.

## Hosted verification

Exact source head `a5315740458aaea68933384cf83d5555e646f42d` completed all available hosted workflows successfully:

- Windows build and deterministic tests run `35796843081`, job `106978004858`: `completed/success` on exact head, including R0 parser/safety contracts, Release assertion/CTest safety contracts, VS2022 x64 configure, MSVC Debug build/tests, MSVC Release build/tests, dependency/prerequisite/runtime-policy checks, static milestone verifiers, and clean-tree verification.
- profiling capture portability run `35796843102`: `completed/success`.
- release manifest integrity run `35796843076`: `completed/success`.

Hosted deterministic CTest intentionally excludes every `RuntimeSmoke`, so this is compile/non-runtime regression evidence only. The historical R0 runner was not executed. Issue #7 remains open.

## Acceptance contract and stop condition

All established E11 shell checks remain required: one stable visible/enabled process-owned top-level window; original 12-child HWND/class continuity; bound semantic Static controls; exact five ordered Outliner rows and four ordered Assets rows; Outliner `LBS_NOTIFY`; exact Scene Root/Cube Inspector fixtures; post-notification Cube synchronization; truthful disabled pending tools; bounded 800x600 and 420x260 resizes with complete-state and containment checks; bounded cross-process messages; clean process-owned normal shutdown; and failure cleanup that reaches a verified terminal state or records an explicit cleanup failure.

Do not count the current launch-inclusive deadline as a complete proof against CTest killing the smoke during a blocked `CreateProcessW`. Do not weaken the external timeout or acceptance checks. Do not start dependent editor features from hosted compilation alone.

## Registered-local handoff

After the launch-lifetime safety gap receives an independently reviewed source repair, run Debug and Release on one owned interactive Windows desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, full stdout/stderr, exit codes, UTC timestamps, and normal plus narrow-window screenshots. If a smoke fails, verify that no owned `AstralEditor` process survives.

## Next useful action

Implement and independently review launch-time owned-process containment so an external kill of the smoke cannot orphan the launched editor. The preferred source-backed direction is a kill-on-close Job Object assigned at process creation via `PROC_THREAD_ATTRIBUTE_JOB_LIST`, subject to confirming compatibility with the supported Windows floor and hosted runner. Then rerun hosted checks and only afterward execute the native Debug/Release smoke. E11 remains partial; `native_evidence` is empty and independent acceptance is false.
