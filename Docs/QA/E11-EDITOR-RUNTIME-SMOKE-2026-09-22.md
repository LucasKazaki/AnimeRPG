# E11 Editor Runtime Smoke evidence, 2026-09-22

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. This packet verifies the already-integrated Win32 `AstralEditor`; it does not authorize scene mutation/serialization, gizmos, Play-in-Editor, asset import, graphics/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed: `dece8fbb47b6a937b9bafafa3faea837a359fb4d`.
Implementation candidate: `3c58d578dc48343427e7d157c26147f31287b49b`.
`CMakeLists.txt` blob: `0e605e9d693f8465b91f7d4f9e5aa1aeb5cc27b5`.
`Tests/EditorRuntimeSmoke.cpp` blob remains `3ca40bccd15950ebd8774ed36902ab15213035bb`.
Integrated editor source fixture remains `Tools/AstralEditorMain.cpp` blob `5142e632a79c89d0d0ce3efe87e752456f802185`; production editor source was not modified.

## Fresh independent finding

Codex reviewed evidence tree `6dc3aba3aa6a32bb935c42faaa3b03a5c2b8f0e2` and submitted P2 comment `4077723206` at `2026-09-22T23:52:53Z`. The prior supervisor closed its kill-on-close Job Object and waited only for the retained worker process handle. With both worker and `AstralEditor` alive, the worker could signal before its descendant reached a terminal state. That was not sufficient proof that the complete owned process tree was empty before the five-second cleanup deadline.

## Bounded repair

Candidate `3c58d578dc48343427e7d157c26147f31287b49b` changes only `CMakeLists.txt` within the packet's allowed paths.

Controlled supervisor cleanup now calls `TerminateJobObject`, retains the job handle, waits and closes the retained worker process handle, then queries `JobObjectBasicAccountingInformation` until `ActiveProcesses == 0` or the same five-second cleanup deadline expires. Only after zero active contained processes are proven does it close the job handle. Query, termination, wait, exit-code, accounting, and handle-close failures all fail the smoke rather than counting as cleanup success.

The normal worker-success path likewise requires the contained job to report `ActiveProcesses == 0` before supervisor PASS. This catches a worker that exits while leaving a descendant alive.

The deterministic containment test now launches a contained parent that creates a long-lived grandchild. It waits until job accounting reports at least two active contained processes, then terminates the job and requires the count to fall to zero. `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE` remains configured as fallback containment if the supervisor itself is externally terminated before controlled cleanup can run.

## Source research, accessed 2026-09-22

- Microsoft Learn, `TerminateJobObject`: https://learn.microsoft.com/en-us/windows/win32/api/jobapi2/nf-jobapi2-terminatejobobject
- Microsoft Learn, `QueryInformationJobObject`: https://learn.microsoft.com/en-us/windows/win32/api/jobapi2/nf-jobapi2-queryinformationjobobject
- Microsoft Learn, `JOBOBJECT_BASIC_ACCOUNTING_INFORMATION`: https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-jobobject_basic_accounting_information
- Microsoft Learn, `UpdateProcThreadAttribute`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-updateprocthreadattribute
- Microsoft Learn, Job Objects: https://learn.microsoft.com/en-us/windows/win32/procthread/job-objects
- CMake, `TIMEOUT`: https://cmake.org/cmake/help/latest/prop_test/TIMEOUT.html
- Epic Games, UE 5.8 Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
- Unity Technologies, Unity 6.0 Hierarchy: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html

Microsoft documents that `TerminateJobObject` terminates all processes associated with the job, and that `JOBOBJECT_BASIC_ACCOUNTING_INFORMATION::ActiveProcesses` is the number currently associated with the job and is decremented after terminated processes exit and process references are released. No proprietary source was copied and no dependency was imported.

## Portable regression model

Source: `/mnt/data/e11_job_accounting_fixture.cpp`
SHA-256: `6f12e0a2864c80d59b7e55ba979ba7befcfdea505270120f99da0bf8fc44f06e`

```text
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror /mnt/data/e11_job_accounting_fixture.cpp -o /mnt/data/e11_job_accounting_fixture_gcc
/mnt/data/e11_job_accounting_fixture_gcc
PASS: job-accounting cleanup model

clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /mnt/data/e11_job_accounting_fixture.cpp -o /mnt/data/e11_job_accounting_fixture_clang
ASAN_OPTIONS=detect_leaks=1 /mnt/data/e11_job_accounting_fixture_clang
PASS: job-accounting cleanup model
```

This fixture is a portable logic model. It is not Win32 Job Object or editor GUI evidence.

## Exact hosted verification for `3c58d578...`

- Windows Server 2022 workflow `35799946201`, job `106987769625`: `completed/success` on exact head `3c58d578dc48343427e7d157c26147f31287b49b`, completed `2026-09-23T00:00:55Z`.
  - R0 parser and safety contracts: PASS. The R0 runner itself was not invoked.
  - Release assertion and CTest safety contracts: PASS.
  - Visual Studio 2022 x64 configure: PASS.
  - Debug build: PASS.
  - deterministic Debug tests: PASS, including registered `EditorRuntimeSmokeContainmentTests` with the two-process contained tree.
  - Release build: PASS.
  - deterministic Release tests: PASS, including the same containment test.
  - PE dependency, runtime/prerequisite policy, static verifiers, and clean tracked-tree verification: PASS.
- profiling capture portability `35799946025`: `completed/success`.
- release manifest integrity `35799946070`: `completed/success`.

Hosted deterministic CTest still intentionally excludes tests whose names end in `RuntimeSmoke`. The actual interactive editor `EditorRuntimeSmoke` therefore did not execute in hosted CI. The deterministic containment target proves the real Windows recovery harness behavior, including a live contained descendant, but does not prove GUI correctness.

## Retained E11 acceptance surface

The existing smoke source is unchanged by this repair and still checks one stable visible/enabled process-owned top-level editor; original 12 direct child HWND/class identities; semantic Static-control binding; exact ordered Outliner and Assets rows; Outliner `LBS_NOTIFY`; exact Scene Root/Cube Inspector fixtures; post-notification Cube synchronization; disabled pending tools; bounded asynchronous resize with complete-state revalidation; bounded cross-process messaging; and clean worker-local shutdown/cleanup.

The supervisor now additionally requires complete contained-job emptiness on success and controlled failure paths.

## Acceptance state and limitations

`native_evidence` remains empty. No interactive Windows desktop, GPU behavior, clean-machine packaging, measured performance, stress/recovery beyond this bounded harness, or 24-hour soak was executed by this coordinator. Fresh independent review of the new candidate/evidence tree is required. Author review is not independent acceptance.

Issue #7 remains open, so the historical R0 runner remains blocked and was not executed.

Status: **full Job Object process-tree cleanup verification is implemented and exercised by hosted Windows deterministic Debug/Release tests on exact candidate `3c58d578...`; interactive native editor evidence and fresh independent acceptance are still pending, so E11 remains partial and no UE5/Unity parity claim is made**.

## Registered native handoff

On one owned interactive Windows desktop, build and run the exact candidate tree in both configurations:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain source SHA, machine and Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, normal and narrow-window screenshots, and a process inspection showing zero owned contained processes after any failure or interruption.

## Single next action

Obtain fresh independent review of the current process-tree cleanup repair and final evidence tree. If clean, execute the registered Debug/Release native GUI smoke and preserve the complete receipt set. Keep PR #13 draft and unmerged until native acceptance exists.
