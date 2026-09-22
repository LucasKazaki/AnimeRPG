# E11 Editor Runtime Smoke evidence, 2026-09-22

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. This packet verifies the already-integrated Win32 `AstralEditor`; it does not authorize scene mutation/serialization, gizmos, Play-in-Editor, asset import, graphics/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed: `dece8fbb47b6a937b9bafafa3faea837a359fb4d`.
Current source head before this evidence edit: `a5315740458aaea68933384cf83d5555e646f42d`.
`Tests/EditorRuntimeSmoke.cpp` blob: `3ca40bccd15950ebd8774ed36902ab15213035bb`.
Integrated editor source fixture: `Tools/AstralEditorMain.cpp` blob `5142e632a79c89d0d0ce3efe87e752456f802185`; production editor source was not modified.

## Finding carried into this pass

Independent Codex review of exact evidence head `00099a8f7e6c7b935a7cad71e31a33f10dbf3e00`, submitted 2026-09-22T22:30:32Z, reported that the 135-second work deadline began only after `CreateProcessW` returned. That made process-launch latency invisible to the timeout budget and could consume the cleanup margin before CTest's external `TIMEOUT 180`.

Commit `a5315740458aaea68933384cf83d5555e646f42d` moved the deadline start immediately before `CreateProcessW` and now rejects an already-expired budget as soon as launch returns. The exact source head passed all hosted checks. This repairs accounting of elapsed launch time but does not bound the synchronous `CreateProcessW` call itself.

## Reproducible timing-model gap

Disposable coordinator-sandbox fixture: `/tmp/e11_launch_budget_fixture.cpp`.
SHA-256: `45ac2935e40181703e6f0f7d41bfd486cd8145930bdc17638d9a1d958c976f32`.

Commands executed:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror /tmp/e11_launch_budget_fixture.cpp -o /tmp/e11_launch_budget_fixture
/tmp/e11_launch_budget_fixture
clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /tmp/e11_launch_budget_fixture.cpp -o /tmp/e11_launch_budget_fixture_san
ASAN_OPTIONS=detect_leaks=1 /tmp/e11_launch_budget_fixture_san
sha256sum /tmp/e11_launch_budget_fixture.cpp
```

Results: GCC C++17 warning-clean compile/execution PASS. Clang C++17 ASan+UBSan warning-clean compile/execution PASS with no sanitizer finding. Sample model output was `0:fits`, `135000:fits`, `172999:fits`, `173000:overruns`, `179000:overruns`.

The fixture does not claim that `CreateProcessW` was observed stalling on Windows. It proves only that starting the deadline before launch does not, by itself, prove the cleanup margin is preserved when the launch API has no caller-supplied timeout. With a 180000 ms external timeout and a retained 5000 ms normal-close plus 2000 ms forced-cleanup allowance, a launch return at or after 173000 ms can consume the remaining modeled margin.

## Primary-source research, accessed 2026-09-22

- Microsoft Learn, `CreateProcessW`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw
- Microsoft Learn, `GetTickCount64`: https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-gettickcount64
- CMake, `TIMEOUT`: https://cmake.org/cmake/help/latest/prop_test/TIMEOUT.html
- Microsoft Learn, Job Objects: https://learn.microsoft.com/en-us/windows/win32/procthread/job-objects
- Microsoft Learn, `UpdateProcThreadAttribute`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-updateprocthreadattribute
- Microsoft Learn, `JOBOBJECT_BASIC_LIMIT_INFORMATION`: https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-jobobject_basic_limit_information
- Epic Games, UE 5.8 Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
- Epic Games, UE 5.8 Unreal Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
- Unity Technologies, Unity 6.0 Hierarchy: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html

Microsoft states that `CreateProcessW` creates a new process and returns before the child has completed initialization, but the API has no timeout parameter. CTest's `TIMEOUT` is an external wall-clock limit. Microsoft also documents that `PROC_THREAD_ATTRIBUTE_JOB_LIST` can assign job handles at child creation on Windows 10+/Windows Server 2016+, and that `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE` terminates associated processes when the last job handle closes. This makes a kill-on-close Job Object the leading source-backed repair direction for ensuring the owned child cannot survive external termination of the smoke while launch is in progress. No proprietary engine source was copied and no dependency was imported.

## Hosted verification

Exact source head `a5315740458aaea68933384cf83d5555e646f42d` completed all available hosted workflows successfully:

- Windows build and deterministic tests run `35796843081`, job `106978004858`: `completed/success`, exact head, completed 2026-09-22T23:21:22Z. Steps include R0 parser/safety contracts, Release assertion/CTest safety contracts, Visual Studio 2022 x64 configure, MSVC Debug build/tests, MSVC Release build/tests, dependency/prerequisite/runtime-policy checks, static milestone verifiers, and clean-tree verification.
- profiling capture portability run `35796843102`: `completed/success`.
- release manifest integrity run `35796843076`: `completed/success`.

Hosted deterministic CTest intentionally excludes every `RuntimeSmoke`. The workflow parsed and tested R0 safety contracts but did not invoke the historical R0 runner. Issue #7 remains open.

## Acceptance state

`native_evidence` remains empty. This coordinator did not access or claim a registered Windows interactive desktop. The exact current source has not yet received fresh independent acceptance after the `a531574...` launch-accounting change, and the launch call still lacks a proven fail-closed containment mechanism if CTest kills the smoke while `CreateProcessW` is blocked.

All prior E11 invariants remain required: stable process-owned top-level window, original 12 child HWND/class continuity, five semantic Static bindings, exact Outliner/Assets rows, Outliner `LBS_NOTIFY`, exact Inspector fixtures, Cube synchronization, truthful disabled pending tools, bounded resize/containment/full-state checks, bounded cross-process messages, normal clean exit, and verified owned-process cleanup on failure.

Do not accept native/runtime parity from hosted compilation. Do not weaken CTest's timeout or remove cleanup checks to obtain a green result.

## Next action

Implement launch-time owned-process containment, preferably with a kill-on-close Job Object assigned at process creation via `PROC_THREAD_ATTRIBUTE_JOB_LIST` after confirming the repository's supported Windows floor and hosted runner behavior. Then run a fresh independent review and hosted checks. Only after that source gate is clean should the registered Windows executor run Debug and Release `EditorRuntimeSmoke` with source SHA, Windows/machine identity, MSVC/CMake and GPU/driver versions, exact commands, complete stdout/stderr, exit codes, UTC timestamps, normal+narrow screenshots, and proof that a failed smoke leaves no owned editor process alive.

Status: **hosted compile/deterministic checks are green for `a531574...`, but the current launch-inclusive deadline is only a partial recovery-safety repair because synchronous `CreateProcessW` remains unbounded; native GUI evidence and independent acceptance remain pending**.
