# E11 Editor Runtime Smoke task, 2026-09-22

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. It may harden the native Windows smoke, its recovery supervisor, and its evidence. It must not add scene mutation/serialization, gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed: `dece8fbb47b6a937b9bafafa3faea837a359fb4d`.
Current implementation candidate: `3c58d578dc48343427e7d157c26147f31287b49b`.
`CMakeLists.txt` blob: `0e605e9d693f8465b91f7d4f9e5aa1aeb5cc27b5`.
`Tests/EditorRuntimeSmoke.cpp` remains blob `3ca40bccd15950ebd8774ed36902ab15213035bb`.
Integrated editor source fixture remains `Tools/AstralEditorMain.cpp` blob `5142e632a79c89d0d0ce3efe87e752456f802185`; production editor source was not modified.

Allowed paths only: `CMakeLists.txt`, `Tests/EditorRuntimeSmoke.cpp`, this task, `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`, and `Docs/Research/ENGINE-CAPABILITIES.json`. One active writer only. Do not rebase, merge, force-push, or absorb unrelated game-worker work.

## Reproduced recovery-safety gap

Fresh independent Codex review of evidence head `6dc3aba3aa6a32bb935c42faaa3b03a5c2b8f0e2`, comment `4077723206`, found a false cleanup proof in the CTest-facing Job Object supervisor. On supervisor timeout, the previous implementation closed the kill-on-close job and waited only for the retained worker process handle. If both the worker and its `AstralEditor` descendant were alive, the worker could become signaled before the descendant reached a terminal state. The supervisor could therefore return without proving that the entire owned process tree was empty within the five-second cleanup window.

A portable source-logic fixture at `/mnt/data/e11_job_accounting_fixture.cpp`, SHA-256 `6f12e0a2864c80d59b7e55ba979ba7befcfdea505270120f99da0bf8fc44f06e`, demonstrates this distinction: a worker-only terminal check can pass while a descendant remains active, while the repaired acceptance requires both retained-worker termination and job `ActiveProcesses == 0`. This is a portable model, not Win32 runtime evidence.

Fixture commands and results:

```text
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror /mnt/data/e11_job_accounting_fixture.cpp -o /mnt/data/e11_job_accounting_fixture_gcc
/mnt/data/e11_job_accounting_fixture_gcc
PASS: job-accounting cleanup model

clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /mnt/data/e11_job_accounting_fixture.cpp -o /mnt/data/e11_job_accounting_fixture_clang
ASAN_OPTIONS=detect_leaks=1 /mnt/data/e11_job_accounting_fixture_clang
PASS: job-accounting cleanup model
```

## Implemented process-tree cleanup repair

Candidate `3c58d578dc48343427e7d157c26147f31287b49b` changes only packet-authorized `CMakeLists.txt`. The existing editor and `Tests/EditorRuntimeSmoke.cpp` are unchanged.

The supervisor now:

1. retains the existing `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE` containment and creation-time `PROC_THREAD_ATTRIBUTE_JOB_LIST` assignment;
2. uses `TerminateJobObject` for controlled timeout/failure cleanup while keeping the job handle open;
3. waits the retained worker handle when present, obtains a terminal exit code, and closes that process handle before job accounting acceptance;
4. polls `QueryInformationJobObject(..., JobObjectBasicAccountingInformation, ...)` until `ActiveProcesses == 0` within the same five-second cleanup deadline;
5. closes the job handle only after the accounting query proves the job empty;
6. requires the same empty-job proof on the normal worker-success path before reporting supervisor PASS;
7. fails closed if querying accounting, terminating the job, waiting the worker, or proving job emptiness fails.

The deterministic `EditorRuntimeSmokeContainmentTests --self-test` is also strengthened. Its contained test parent now creates a long-lived grandchild. The supervisor waits until job accounting observes at least two active contained processes, terminates the job, and then requires `ActiveProcesses == 0`. This directly exercises a process tree rather than only the direct child.

The kill-on-close limit remains as the fallback if the supervisor itself is externally terminated and cannot execute controlled cleanup. The controlled path now keeps the job queryable until process-tree emptiness is proven.

## Primary-source basis, rechecked 2026-09-22

- Microsoft Learn, `TerminateJobObject`: https://learn.microsoft.com/en-us/windows/win32/api/jobapi2/nf-jobapi2-terminatejobobject
- Microsoft Learn, `QueryInformationJobObject`: https://learn.microsoft.com/en-us/windows/win32/api/jobapi2/nf-jobapi2-queryinformationjobobject
- Microsoft Learn, `JOBOBJECT_BASIC_ACCOUNTING_INFORMATION`: https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-jobobject_basic_accounting_information
- Microsoft Learn, `UpdateProcThreadAttribute`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-updateprocthreadattribute
- Microsoft Learn, Job Objects: https://learn.microsoft.com/en-us/windows/win32/procthread/job-objects
- CMake, `TIMEOUT`: https://cmake.org/cmake/help/latest/prop_test/TIMEOUT.html
- Epic Games, UE 5.8 Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
- Unity Technologies, Unity 6.0 Hierarchy: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html

Microsoft documents that `TerminateJobObject` terminates all processes currently associated with a job, including child jobs in a nested hierarchy. `QueryInformationJobObject` exposes `JobObjectBasicAccountingInformation`, whose `ActiveProcesses` field is the number of processes currently associated with the job and is decremented after terminated processes exit and their process references are released. These references are used for API behavior only. No proprietary engine source was copied and no dependency was added.

## Verification for exact implementation candidate

Exact candidate `3c58d578dc48343427e7d157c26147f31287b49b`:

- Windows Server 2022 build and deterministic tests: run `35799946201`, job `106987769625`, `completed/success`, completed `2026-09-23T00:00:55Z`. R0 parser/safety contracts, Release assertion/CTest safety contracts, VS2022 x64 configure, Debug build + deterministic tests, Release build + deterministic tests, runtime dependency/prerequisite checks, static verifiers, and clean tracked-tree verification all passed. The registered `EditorRuntimeSmokeContainmentTests` is in both deterministic sets and therefore exercised the real Windows two-process Job Object containment/accounting path in Debug and Release.
- profiling capture portability: run `35799946025`, `completed/success`.
- release manifest integrity: run `35799946070`, `completed/success`.

Hosted deterministic CTest still excludes tests whose names end in `RuntimeSmoke`, so the actual interactive GUI `EditorRuntimeSmoke` was not executed by hosted CI. The containment test is recovery-harness evidence, not editor GUI acceptance.

Earlier containment candidate `cc44b29f11ad62f14f61c1555cb428dc8320866a` was correctly rejected by the repository test-safety verifier for introducing an unregistered worker executable. Candidate `9e18effe009e690de97250ee2dba9339fec15752` repaired that without weakening the verifier. Candidate `6b608953b55be35d3ac4624f982739b283b8fa36` added creation-time job containment but its worker-only terminal proof led to the current review finding. The present candidate repairs that exact finding.

## Acceptance contract and remaining gate

All established E11 shell checks remain required: one stable visible/enabled process-owned top-level window; original 12-child HWND/class continuity; bound semantic Static controls; exact five ordered Outliner rows and four ordered Assets rows; Outliner `LBS_NOTIFY`; exact Scene Root/Cube Inspector fixtures; post-notification Cube synchronization; truthful disabled pending tools; bounded 800x600 and 420x260 resizes with complete-state and containment checks; bounded cross-process messages; clean process-owned normal shutdown; worker-local cleanup; and supervisor-level process-tree cleanup verification.

`native_evidence` remains empty and independent acceptance remains false. Do not count hosted containment testing as native GUI acceptance or engine parity. Keep this PR draft and unmerged.

After fresh independent review of the final evidence tree, the registered Windows executor should run Debug and Release on one owned interactive desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, commands, full stdout/stderr, exit codes, UTC timestamps, and normal plus narrow-window screenshots. Any failure or interruption must confirm that the supervisor/job cleanup leaves zero owned contained processes.

Issue #7 remains open, so the historical R0 runner remains blocked and must not be invoked.

## Single next useful action

Obtain fresh independent source review of the process-tree accounting repair and its final evidence tree. If clean, run the registered Windows Debug/Release GUI smoke with the receipt set above. E11 remains partial and no UE5/Unity parity claim is authorized.
