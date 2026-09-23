# E11 Editor Runtime Smoke evidence, 2026-09-23

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. This packet verifies the already-integrated Win32 `AstralEditor`; it does not authorize scene mutation/serialization, gizmos, Play-in-Editor, asset import, graphics/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed during this packet: `dece8fbb47b6a937b9bafafa3faea837a359fb4d`.
Current code candidate: `1cbef2242fb79edda068751dd71996b5548ec739`.
`CMakeLists.txt` blob: `ed6a7f44d87241560faf32a57465befd536b59f9`.
`Tests/EditorRuntimeSmoke.cpp` remains blob `3ca40bccd15950ebd8774ed36902ab15213035bb`.
Production editor source was not modified by this pass.

## Fresh review finding and repair

Codex review of exact candidate `7d38e2cd7ffc6b76952711212e7da78d66c7f1af` completed at `2026-09-23T00:29:04Z`. P2 comment `4077923601` found that the new synthetic zero-exit parent plus lingering-grandchild test manually invoked accounting primitives and did not execute the exact normal supervisor success branch. It could therefore remain green if that production acceptance branch were later bypassed or inverted. P2 comment `4077923604` also required the authoritative task, QA, and capability receipts to be refreshed for the new candidate and evidence.

Candidate `1cbef2242fb79edda068751dd71996b5548ec739` repairs the control-flow coverage gap. Normal supervisor handling now lives in `SupervisorRunNormalWorkerAcceptance(...)`, and both the real `argc == 3` supervisor path and the deterministic containment self-test call that same function.

For a worker that exits `0`, the shared function closes the direct worker handle, requires `SupervisorWaitForJobEmpty(...)` to observe `ActiveProcesses == 0`, and returns success only then. If a descendant remains, the exact same shared path reports `successful worker left contained process tree non-empty`, performs `TerminateJobObject` cleanup, requires the job to become empty, and returns failure. The self-test's `--contained-self-test-success-parent` creates that exact zero-exit-parent/lingering-grandchild state and requires the shared normal path to reject it for the expected reason. The prior live parent plus long-lived grandchild forced-cleanup test remains.

No editor production code and no `Tests/EditorRuntimeSmoke.cpp` code changed in this repair.

## Primary research, accessed 2026-09-23 UTC

- Microsoft Learn, Job Objects: https://learn.microsoft.com/en-us/windows/win32/procthread/job-objects
- Microsoft Learn, `AssignProcessToJobObject`: https://learn.microsoft.com/en-us/windows/win32/api/jobapi2/nf-jobapi2-assignprocesstojobobject
- Microsoft Learn, `TerminateJobObject`: https://learn.microsoft.com/en-us/windows/win32/api/jobapi2/nf-jobapi2-terminatejobobject
- Microsoft Learn, `QueryInformationJobObject`: https://learn.microsoft.com/en-us/windows/win32/api/jobapi2/nf-jobapi2-queryinformationjobobject
- Microsoft Learn, `JOBOBJECT_BASIC_ACCOUNTING_INFORMATION`: https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-jobobject_basic_accounting_information
- Microsoft Learn, `UpdateProcThreadAttribute`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-updateprocthreadattribute
- CMake, `TIMEOUT`: https://cmake.org/cmake/help/latest/prop_test/TIMEOUT.html
- Epic Games, UE 5.8 Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
- Epic Games, UE 5.8 Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
- Unity Technologies, Unity 6.0 Hierarchy: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html

Microsoft documents that processes created by a job-member process remain in the same Job Object by default unless breakaway is enabled; this E11 job enables neither breakaway flag. Job Objects therefore remain the packet's owned process-tree boundary. `TerminateJobObject` applies to all associated processes, while `JobObjectBasicAccountingInformation::ActiveProcesses` is the count used for the empty-job proof. These references are API/behavioral references only. No proprietary engine source was copied and no dependency was imported.

## Portable source-logic regression model

Shared-control-flow fixture SHA-256: `378637e40efe2a7ae255a4f232a29278b3ef0acf68b40269bee59e6ec7f35f26`.

```text
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror /tmp/e11_shared_acceptance_fixture.cpp -o /tmp/e11_shared_acceptance_fixture_gcc
/tmp/e11_shared_acceptance_fixture_gcc
PASS: shared normal-success acceptance fixture

clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /tmp/e11_shared_acceptance_fixture.cpp -o /tmp/e11_shared_acceptance_fixture_clang
ASAN_OPTIONS=detect_leaks=1 /tmp/e11_shared_acceptance_fixture_clang
PASS: shared normal-success acceptance fixture
```

This is a portable policy/control-flow model only. It is not Win32 Job Object or native GUI evidence.

## Exact hosted verification for `1cbef224...`

- Windows Server 2022 workflow `35802481221`, job `106995723688`: `completed/success` on exact SHA `1cbef2242fb79edda068751dd71996b5548ec739`, completed `2026-09-23T00:34:05Z`.
  - R0 parser and R0 safety contracts: PASS. The historical R0 runner itself was not invoked.
  - Release assertion and CTest safety contracts: PASS.
  - Visual Studio 2022 x64 configure: PASS.
  - Debug build: PASS.
  - deterministic Debug tests: PASS, including registered `EditorRuntimeSmokeContainmentTests` with the shared normal-success rejection path.
  - Release build: PASS.
  - deterministic Release tests: PASS, including the same containment target.
  - dependency/prerequisite policy checks, static verifiers, and clean tracked-tree verification: PASS.
- profiling capture portability `35802481230`: `completed/success`.
- release manifest integrity `35802481233`: `completed/success`.

The immediately preceding evidence-only head `20d19983fb54161177df37f6ade1bb1b9bce107a` also completed all three hosted workflows successfully, but its durable map still described code candidate `7d38e2c...`; it is retained only as historical evidence, not the current implementation receipt.

Hosted deterministic CTest intentionally excludes tests whose names end in `RuntimeSmoke`. The actual interactive editor `EditorRuntimeSmoke` therefore did not execute in hosted CI. The deterministic containment target proves the Windows recovery-harness behavior and shared acceptance route, not GUI correctness.

## Retained E11 editor acceptance surface

The unchanged GUI smoke still checks one stable visible/enabled process-owned top-level editor; original 12 direct child HWND/class identities; semantic Static-control binding; exact ordered Outliner and Assets rows; Outliner `LBS_NOTIFY`; exact Scene Root/Cube Inspector fixtures; post-notification Cube synchronization; disabled pending tools; bounded asynchronous resize with complete-state revalidation; bounded cross-process messaging; and clean worker-local shutdown/cleanup.

The CTest-facing supervisor additionally contains the worker/editor process tree in a kill-on-close Job Object and now has deterministic coverage that the same normal-success route refuses a zero-exit direct worker if a contained descendant remains.

## Acceptance state and limitations

`native_evidence` remains empty. No interactive Windows desktop execution, human-visible screenshots, actual GPU behavior, clean-machine packaging, measured comparative performance, stress/recovery beyond this bounded harness, or 24-hour soak was executed by this coordinator.

The fresh review of `7d38e2c...` found the shared-path coverage defect and is therefore not independent acceptance of `1cbef224...`. A new independent review of the repaired final evidence tree is required. Author review is not independent acceptance.

Issue #7 remains open, so the historical R0 runner remains blocked and was not executed.

Status: **the supervisor's normal-success process-tree acceptance route is now shared with and exercised by deterministic Windows Debug/Release containment tests on exact candidate `1cbef224...`; interactive native editor evidence and fresh independent acceptance remain pending, so E11 stays partial and no UE5/Unity parity claim is made**.

## Registered native handoff

After a clean independent review, run the exact final tree on one owned interactive Windows desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine and Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, normal and narrow-window screenshots, and a process inspection showing zero owned contained processes after any failure or interruption.

## Single next action

Obtain fresh independent review of code candidate `1cbef224...` plus the final evidence-only receipt head. If clean, execute the registered Debug/Release native GUI smoke and preserve the complete receipt set. Keep PR #13 draft and unmerged until native acceptance exists.
