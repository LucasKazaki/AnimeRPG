# E11 Editor Runtime Smoke task, 2026-09-23

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. It may harden the native Windows smoke, its recovery supervisor, deterministic containment tests, and evidence. It must not add scene mutation/serialization, gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed during this packet: `dece8fbb47b6a937b9bafafa3faea837a359fb4d`.
Current code candidate: `1cbef2242fb79edda068751dd71996b5548ec739`.
`CMakeLists.txt` blob: `ed6a7f44d87241560faf32a57465befd536b59f9`.
`Tests/EditorRuntimeSmoke.cpp` remains blob `3ca40bccd15950ebd8774ed36902ab15213035bb`.
Production editor source was not modified by this pass.

Allowed paths only: `CMakeLists.txt`, `Tests/EditorRuntimeSmoke.cpp`, this task, `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`, and `Docs/Research/ENGINE-CAPABILITIES.json`. One active writer only. Do not rebase, merge, force-push, or absorb unrelated work.

## Selected verification gap

The prior process-tree repair already required the CTest-facing supervisor to prove its Job Object had `ActiveProcesses == 0` before reporting normal success, and deterministic tests exercised forced cleanup of a live parent plus grandchild. Candidate `7d38e2cd7ffc6b76952711212e7da78d66c7f1af` added a synthetic parent that exits `0` while leaving a contained grandchild alive, but its first self-test version manually called the accounting primitives instead of the same normal-success control flow used by the production supervisor.

Independent Codex review of exact `7d38e2c...`, completed `2026-09-23T00:29:04Z`, reported P2 comment `4077923601`: if the production normal-success acceptance branch were removed, bypassed, or inverted, that manual self-test could still pass. Review comment `4077923604` also required the durable task/QA/capability receipts to be refreshed for the new candidate and checks.

## Implemented repair

Candidate `1cbef2242fb79edda068751dd71996b5548ec739` changes only packet-authorized `CMakeLists.txt`.

The supervisor's normal worker handling is now factored into `SupervisorRunNormalWorkerAcceptance(...)`. Both the real `argc == 3` supervisor route and `EditorRuntimeSmokeContainmentTests --self-test` call this same function. The shared path:

1. bounds the direct worker wait;
2. reads and validates the worker exit code;
3. treats nonzero worker exits as failure and performs whole-job cleanup;
4. after a zero worker exit, closes the retained worker handle and requires `SupervisorWaitForJobEmpty(...)` to prove `ActiveProcesses == 0`;
5. if a zero-exit worker left a descendant, fails with `successful worker left contained process tree non-empty`, performs controlled `TerminateJobObject` cleanup, and requires the job to become empty before returning failure;
6. reports success only after the worker exited zero and the complete contained job is empty.

The deterministic self-test now launches `--contained-self-test-success-parent`, which creates a long-lived contained grandchild and exits `0`, then passes that process/job pair through the exact shared normal acceptance function. The test passes only when the real normal-success path rejects the lingering descendant for the expected reason and controlled cleanup proves the job empty. The existing live parent+grandchild forced-termination test remains.

This closes the fresh review finding without weakening any acceptance criterion. The production editor and `Tests/EditorRuntimeSmoke.cpp` are unchanged.

## Primary-source basis, rechecked 2026-09-23 UTC

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

Microsoft documents that children created with `CreateProcess` remain in the same Job Object by default unless breakaway is enabled; this E11 job sets neither breakaway flag. `TerminateJobObject` terminates all processes associated with the job, and `JobObjectBasicAccountingInformation::ActiveProcesses` provides the contained-process count used for the empty-job proof. These references are behavioral/API references only. No proprietary engine source was copied and no dependency was added.

## Portable source-logic check

Shared-control-flow fixture SHA-256: `378637e40efe2a7ae255a4f232a29278b3ef0acf68b40269bee59e6ec7f35f26`.

```text
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror /tmp/e11_shared_acceptance_fixture.cpp -o /tmp/e11_shared_acceptance_fixture_gcc
/tmp/e11_shared_acceptance_fixture_gcc
PASS: shared normal-success acceptance fixture

clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /tmp/e11_shared_acceptance_fixture.cpp -o /tmp/e11_shared_acceptance_fixture_clang
ASAN_OPTIONS=detect_leaks=1 /tmp/e11_shared_acceptance_fixture_clang
PASS: shared normal-success acceptance fixture
```

This is a portable policy/control-flow model only, not Win32 Job Object or native GUI evidence.

## Exact hosted verification

Exact code candidate `1cbef2242fb79edda068751dd71996b5548ec739`:

- Windows Server 2022 build and deterministic tests: run `35802481221`, job `106995723688`, `completed/success`, completed `2026-09-23T00:34:05Z`. Repository/R0 safety contracts, Release assertion/CTest safety contracts, VS2022 x64 configure, Debug build and deterministic tests, Release build and deterministic tests, dependency/prerequisite checks, static verifiers, and clean-tree verification all passed. The registered containment self-test executes in both deterministic configurations and now exercises the shared normal-success rejection path.
- profiling capture portability: run `35802481230`, `completed/success`.
- release manifest integrity: run `35802481233`, `completed/success`.

Hosted deterministic CTest still excludes tests whose names end in `RuntimeSmoke`, so the actual interactive GUI `EditorRuntimeSmoke` was not executed by hosted CI. The containment target is recovery-harness evidence, not editor GUI acceptance.

## Retained acceptance surface

All established E11 shell checks remain required: one stable visible/enabled process-owned top-level editor; original 12-child HWND/class continuity; bound semantic controls; exact five ordered Outliner rows and four ordered Assets rows; Outliner `LBS_NOTIFY`; exact Scene Root/Cube Inspector fixtures; post-notification Cube synchronization; truthful disabled pending tools; bounded 800x600 and 420x260 resizes with complete-state and containment checks; bounded cross-process messages; clean process-owned normal shutdown; worker-local cleanup; and supervisor-level process-tree cleanup verification.

`native_evidence` remains empty. Independent acceptance of the new shared-path repair remains pending until a fresh review of the final evidence tree completes. Keep PR #13 draft and unmerged. Issue #7 remains open, so the historical R0 runner remains blocked and was not invoked.

## Registered native handoff

After a clean independent review, the registered Windows executor should run the exact final tree on one owned interactive desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, commands, full stdout/stderr, exit codes, UTC timestamps, normal plus narrow-window screenshots, and proof that any failure/interruption leaves zero owned contained processes.

## Single next useful action

Obtain fresh independent review of the shared normal-success acceptance repair and final evidence tree. If clean, run the registered Windows Debug/Release GUI smoke with the complete receipt set above. E11 remains partial and no UE5/Unity parity claim is authorized.
