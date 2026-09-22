# E11 Editor Runtime Smoke task, 2026-09-22

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. It may harden the native Windows smoke and its evidence, but it must not add scene mutation/serialization, gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed: `dece8fbb47b6a937b9bafafa3faea837a359fb4d`.
Current implementation candidate: `6b608953b55be35d3ac4624f982739b283b8fa36`.
`CMakeLists.txt` blob at that candidate: `d03ea8f6e967612ded6591bda9e7ae8d1fbfaf13`.
`Tests/EditorRuntimeSmoke.cpp` blob remains `3ca40bccd15950ebd8774ed36902ab15213035bb`.
Integrated editor source fixture remains `Tools/AstralEditorMain.cpp` blob `5142e632a79c89d0d0ce3efe87e752456f802185`; production editor source was not modified.

Allowed paths only: `CMakeLists.txt`, `Tests/EditorRuntimeSmoke.cpp`, this task, `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`, and `Docs/Research/ENGINE-CAPABILITIES.json`. One active writer only. Do not rebase, merge, force-push, or absorb unrelated game-worker work.

## Reproduced recovery-safety gap

Independent Codex review comment `4077190261` on evidence head `00099a8f7e6c7b935a7cad71e31a33f10dbf3e00` found that the earlier 135-second deadline began only after `CreateProcessW`. Commit `a5315740458aaea68933384cf83d5555e646f42d` moved the deadline start before launch, but the synchronous `CreateProcessW` API itself has no caller-supplied timeout, so the worker alone still could not prove it would regain control before CTest's external 180-second timeout.

Portable timing-model fixture `/tmp/e11_launch_budget_fixture.cpp`, SHA-256 `45ac2935e40181703e6f0f7d41bfd486cd8145930bdc17638d9a1d958c976f32`, passed warning-clean GCC C++17 and Clang C++17 ASan+UBSan execution. It demonstrates only the proof gap, not an observed Win32 stall: with the retained 5-second normal-close and 2-second forced-cleanup allowances, a launch return at or after 173000 ms can consume the modeled 180000 ms external-timeout margin.

## Implemented containment repair

Candidate `6b608953b55be35d3ac4624f982739b283b8fa36` changes only the packet-authorized `CMakeLists.txt` harness surface and leaves the existing worker smoke source unchanged. The CTest-facing `EditorRuntimeSmoke` executable now acts as a supervisor and embeds the original smoke source as a renamed worker entry point.

The supervisor:

1. creates a Win32 Job Object and sets `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE`;
2. creates a process-thread attribute list containing `PROC_THREAD_ATTRIBUTE_JOB_LIST`;
3. launches a child copy of itself in `--worker` mode with `EXTENDED_STARTUPINFO_PRESENT`, so the worker is assigned to the kill-on-close job at process creation;
4. the worker calls the existing `EditorRuntimeSmoke` logic, which launches `AstralEditor`; the editor remains in the contained process tree under normal job inheritance;
5. waits at most 150 seconds for the worker, then closes the job and requires bounded terminal-state verification on timeout/failure, retaining margin before CTest's 180-second external limit;
6. keeps the normal CTest invocation unchanged: `EditorRuntimeSmoke <AstralEditor> <working-directory>`.

The supervisor targets define `_WIN32_WINNT=0x0A00`. The repository README states Windows 10+ as the supported floor, matching Microsoft's documented availability for `PROC_THREAD_ATTRIBUTE_JOB_LIST` (Windows 10+/Windows Server 2016+). No third-party dependency or engine architecture/graphics API changed.

A deterministic registered `EditorRuntimeSmokeContainmentTests` target uses the same generated supervisor source in `--self-test` mode. It launches a child that deliberately stays alive, verifies the child is still running before containment is exercised, closes the final kill-on-close job handle, then requires that the contained child becomes signaled within five seconds with a terminal exit state. Because this test name does not end in `RuntimeSmoke`, it executes in the hosted deterministic Debug and Release test sets rather than the excluded GUI-smoke set.

An intermediate implementation `cc44b29f11ad62f14f61c1555cb428dc8320866a` added an unregistered worker executable and correctly failed the repository's Release assertion/CTest safety contract in Windows run `35798176786`, job `106982219881`. The repair did not weaken that verifier. Commit `9e18effe009e690de97250ee2dba9339fec15752` removed the extra target by self-reexecuting the single registered smoke binary; exact-head Windows run `35798345772`, profiling `35798345807`, and release-manifest `35798345798` then passed.

## Primary-source basis, rechecked 2026-09-22

- Microsoft Learn, `CreateProcessW`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw
- Microsoft Learn, Job Objects: https://learn.microsoft.com/en-us/windows/win32/procthread/job-objects
- Microsoft Learn, `UpdateProcThreadAttribute`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-updateprocthreadattribute
- Microsoft Learn, `JOBOBJECT_EXTENDED_LIMIT_INFORMATION`: https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-jobobject_extended_limit_information
- Microsoft Learn, `GetTickCount64`: https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-gettickcount64
- CMake, `TIMEOUT` test property: https://cmake.org/cmake/help/latest/prop_test/TIMEOUT.html
- Epic Games, UE 5.8 Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
- Unity Technologies, Unity 6.0 Hierarchy: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html

Microsoft documents that closing the last handle of a job carrying `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE` terminates associated processes, and that `PROC_THREAD_ATTRIBUTE_JOB_LIST` assigns listed jobs to the new child at process creation. Behavioral/API documentation only was used; no proprietary engine source was copied.

## Exact hosted verification for the implementation candidate

Exact candidate `6b608953b55be35d3ac4624f982739b283b8fa36` completed all available hosted workflows successfully:

- Windows Server 2022 build and deterministic tests: run `35798632904`, job `106983646947`, `completed/success`, completed `2026-09-22T23:43:39Z`. R0 parser/safety contracts, Release assertion/CTest safety contracts, VS2022 x64 configure, Debug build + deterministic tests, Release build + deterministic tests, dependency/prerequisite/runtime-policy checks, static verifiers, and clean-tree verification all passed. The registered `EditorRuntimeSmokeContainmentTests` belongs to those deterministic Debug/Release sets and therefore exercised the real Windows Job Object containment path in both configurations.
- profiling capture portability: run `35798632844`, `completed/success`.
- release manifest integrity: run `35798632886`, `completed/success`.

Hosted deterministic CTest still excludes tests whose names end in `RuntimeSmoke`, so the real GUI `EditorRuntimeSmoke` did not run on hosted CI. The containment self-test is recovery-harness evidence, not editor GUI acceptance. The historical R0 runner was not executed and issue #7 remains open.

## Acceptance contract and remaining gate

All established E11 shell checks remain required: one stable visible/enabled process-owned top-level window; original 12-child HWND/class continuity; bound semantic Static controls; exact five ordered Outliner rows and four ordered Assets rows; Outliner `LBS_NOTIFY`; exact Scene Root/Cube Inspector fixtures; post-notification Cube synchronization; truthful disabled pending tools; bounded 800x600 and 420x260 resizes with complete-state and containment checks; bounded cross-process messages; clean process-owned normal shutdown; and failure cleanup with terminal-state verification.

`native_evidence` remains empty and independent acceptance remains false. Do not count the hosted containment self-test as native GUI acceptance or engine parity. Keep this PR draft and unmerged.

After a fresh independent review of the final evidence tree, the registered Windows executor should run Debug and Release on one owned interactive desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, full stdout/stderr, exit codes, UTC timestamps, and normal plus narrow-window screenshots. If a smoke fails or is externally interrupted, verify no owned `AstralEditor` process survives.

## Single next useful action

Obtain fresh independent source review of the kill-on-close supervisor and its exact evidence tree. If clean, run the registered Windows Debug/Release GUI smoke with the receipt set above. E11 remains partial and no UE5/Unity parity claim is authorized.
