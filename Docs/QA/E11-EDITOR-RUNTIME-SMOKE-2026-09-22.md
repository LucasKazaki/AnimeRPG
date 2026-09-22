# E11 Editor Runtime Smoke evidence, 2026-09-22

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. This packet verifies the already-integrated Win32 `AstralEditor`; it does not authorize scene mutation/serialization, gizmos, Play-in-Editor, asset import, graphics/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed: `dece8fbb47b6a937b9bafafa3faea837a359fb4d`.
Implementation candidate: `6b608953b55be35d3ac4624f982739b283b8fa36`.
`CMakeLists.txt` blob: `d03ea8f6e967612ded6591bda9e7ae8d1fbfaf13`.
`Tests/EditorRuntimeSmoke.cpp` blob remains `3ca40bccd15950ebd8774ed36902ab15213035bb`.
Integrated editor source fixture remains `Tools/AstralEditorMain.cpp` blob `5142e632a79c89d0d0ce3efe87e752456f802185`; production editor source was not modified.

## Finding and bounded implementation

Independent Codex review comment `4077190261` identified a recovery-safety hole in the earlier timeout repair. Moving the 135-second work deadline before `CreateProcessW` accounts for launch time after that synchronous API returns, but `CreateProcessW` exposes no caller-supplied timeout. If the smoke process were externally terminated by CTest while blocked in launch, worker-local cleanup could still be bypassed.

A portable timing-model fixture, `/tmp/e11_launch_budget_fixture.cpp`, SHA-256 `45ac2935e40181703e6f0f7d41bfd486cd8145930bdc17638d9a1d958c976f32`, passed warning-clean GCC C++17 and Clang C++17 ASan+UBSan execution. It does not claim a Win32 stall was observed. It shows only that with a 180000 ms external timeout plus retained 5000 ms normal-close and 2000 ms forced-cleanup allowances, a launch return at or after 173000 ms can consume the modeled margin.

Candidate `6b608953b55be35d3ac4624f982739b283b8fa36` adds a CTest-facing containment supervisor in generated C++ from `CMakeLists.txt`, without changing `Tests/EditorRuntimeSmoke.cpp` or the production editor. The supervisor embeds the original smoke as a renamed worker entry point, creates a Win32 Job Object with `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE`, supplies that job in `PROC_THREAD_ATTRIBUTE_JOB_LIST`, and launches a child copy of itself in `--worker` mode using `EXTENDED_STARTUPINFO_PRESENT`. The worker then executes the existing editor smoke. The supervisor gives the worker 150 seconds, and on timeout closes the final job handle and verifies contained-process termination within five seconds. This leaves external CTest's existing 180-second timeout unchanged.

The same generated source also builds as a registered deterministic target, `EditorRuntimeSmokeContainmentTests`, whose `--self-test` child deliberately stays alive. The self-test proves the child is live before containment is exercised, closes the final kill-on-close job handle, then requires the child to signal termination within five seconds with a terminal exit state. The target does not end in `RuntimeSmoke`, so hosted deterministic Debug/Release CTest includes it. This gives real hosted Windows evidence for the recovery mechanism without pretending to be an interactive editor smoke.

The supervisor targets compile with `_WIN32_WINNT=0x0A00`. The repository README establishes Windows 10+ as the supported floor. Microsoft's `PROC_THREAD_ATTRIBUTE_JOB_LIST` documentation lists Windows 10+/Windows Server 2016+ support. No dependency was added and no production architecture or graphics API changed.

## Primary-source research, accessed 2026-09-22

- Microsoft Learn, `CreateProcessW`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw
- Microsoft Learn, Job Objects: https://learn.microsoft.com/en-us/windows/win32/procthread/job-objects
- Microsoft Learn, `UpdateProcThreadAttribute`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-updateprocthreadattribute
- Microsoft Learn, `JOBOBJECT_EXTENDED_LIMIT_INFORMATION`: https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-jobobject_extended_limit_information
- CMake, `TIMEOUT`: https://cmake.org/cmake/help/latest/prop_test/TIMEOUT.html
- Epic Games, UE 5.8 Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
- Unity Technologies, Unity 6.0 Hierarchy: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html

Microsoft documents that closing the last handle to a job configured with `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE` terminates associated processes, and that `PROC_THREAD_ATTRIBUTE_JOB_LIST` assigns listed jobs to a child at process creation. These references are used for behavior/API comparison only. No proprietary source was copied.

## Regression encountered and repaired

First containment attempt `cc44b29f11ad62f14f61c1555cb428dc8320866a` introduced an extra unregistered `EditorRuntimeSmokeWorker` executable. Windows run `35798176786`, job `106982219881`, failed exactly at `Verify Release assertion and CTest safety contracts`, before configure/build. This was a legitimate project-gate regression. It was not bypassed or weakened.

Commit `9e18effe009e690de97250ee2dba9339fec15752` replaced that layout with one registered smoke binary that self-reexecutes into worker mode. Exact-head Windows run `35798345772`, profiling run `35798345807`, and release-manifest run `35798345798` all completed successfully.

The final implementation candidate then added the registered deterministic containment self-test.

## Exact hosted verification for `6b608953...`

- Windows Server 2022 workflow `35798632904`, job `106983646947`: `completed/success` on exact head `6b608953b55be35d3ac4624f982739b283b8fa36`, completed `2026-09-22T23:43:39Z`.
  - R0 parser and safety contracts: PASS. The R0 runner itself was not invoked.
  - PE dependency, Windows prerequisite/runtime compatibility/bootstrap contracts: PASS.
  - Release assertion and CTest safety contracts: PASS.
  - Visual Studio 2022 x64 configure: PASS.
  - Debug build: PASS.
  - deterministic Debug tests: PASS, including the registered `EditorRuntimeSmokeContainmentTests` because it is not excluded by the `RuntimeSmoke` filter.
  - Release build: PASS.
  - deterministic Release tests: PASS, including the same containment test.
  - static milestone verifiers and clean tracked-tree verification: PASS.
- profiling capture portability `35798632844`: `completed/success`.
- release manifest integrity `35798632886`: `completed/success`.

Hosted deterministic CTest still intentionally excludes tests whose names end in `RuntimeSmoke`; therefore the actual interactive `EditorRuntimeSmoke` was not executed by this hosted workflow. The passing containment test proves the Windows Job Object mechanism exercised by the harness, not editor GUI correctness.

## Acceptance state

The existing smoke retains all established shell invariants: one stable visible/enabled process-owned top-level editor; original 12 direct child HWND/class identities; five bound semantic Static HWNDs; exact ordered Outliner and Assets rows; Outliner `LBS_NOTIFY`; exact Scene Root/Cube Inspector fixtures; bounded selection notification and post-notification synchronization; disabled pending tools; bounded asynchronous resize with containment and full-state revalidation; bounded cross-process reads; clean normal shutdown; and process-handle cleanup verification on worker-local failure.

The new supervisor adds a second containment layer so an external timeout of the CTest-facing process does not depend solely on the worker regaining control to clean up its process tree. This is specifically recovery-harness evidence.

`native_evidence` remains empty. This coordinator did not access or claim a registered interactive Windows desktop. The full GUI smoke still requires Debug and Release execution there, with exact source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, commands, complete stdout/stderr, exit codes, UTC timestamps, and normal plus narrow-window screenshots. Any failure/interruption must confirm no owned `AstralEditor` survives.

Fresh independent review of the final evidence tree is also pending. Author review is not independent acceptance. Issue #7 remains open, so the historical R0 runner remains blocked.

Status: **kill-on-close launch containment is implemented and exercised successfully by a real hosted Windows deterministic self-test on exact code candidate `6b608953...`; interactive native editor evidence and fresh independent acceptance are still pending, so E11 remains partial and no UE5/Unity parity claim is made**.

## Single next action

Request fresh independent review of the final evidence tree. If clean, run the registered Windows Debug and Release `EditorRuntimeSmoke` with the full receipt set and screenshots, leaving the PR draft and unmerged until native acceptance is complete.
