# E11 Editor Runtime Smoke task, 2026-09-23

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. It may harden the native Windows smoke, its recovery supervisor, deterministic containment tests, and evidence. It must not add scene mutation/serialization, gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed during this packet: `dece8fbb47b6a937b9bafafa3faea837a359fb4d`.
Current code candidate: `1cbef2242fb79edda068751dd71996b5548ec739`.
`CMakeLists.txt` blob: `ed6a7f44d87241560faf32a57465befd536b59f9`.
`Tests/EditorRuntimeSmoke.cpp` blob: `3ca40bccd15950ebd8774ed36902ab15213035bb`.
Production editor source is unchanged by this pass.

Allowed paths only: `CMakeLists.txt`, `Tests/EditorRuntimeSmoke.cpp`, this task, `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`, and `Docs/Research/ENGINE-CAPABILITIES.json`. One active writer only. Do not rebase, merge, force-push, or absorb unrelated work.

## Selected verification gap and implemented repair

Independent review of candidate `7d38e2cd7ffc6b76952711212e7da78d66c7f1af` found that the synthetic zero-exit-parent/lingering-descendant containment test manually checked Job Object primitives rather than exercising the exact normal supervisor success path. Candidate `1cbef2242fb79edda068751dd71996b5548ec739` fixed that by factoring normal acceptance into `SupervisorRunNormalWorkerAcceptance(...)` and routing both the real supervisor and deterministic containment self-test through that same function.

The shared route bounds the direct worker wait, verifies its exit code, rejects nonzero exits, requires a zero-exit worker to leave `ActiveProcesses == 0`, and uses controlled `TerminateJobObject` cleanup plus an empty-job proof on failure. The synthetic success parent creates a long-lived contained grandchild and exits `0`; the registered self-test passes that state through the same normal acceptance function and succeeds only when the real path rejects the lingering descendant for the expected reason and cleanup proves the job empty. The existing live parent-plus-grandchild forced-cleanup test remains.

No runtime/editor code changed in the present evidence-reconciliation pass.

## Exact reviewed evidence checkpoint

The exact evidence tree `13a6d220125ede24c9eea1662ecb287837740763` was independently reviewed by Codex at `2026-09-23T00:41:26Z`. The review produced one P2 evidence-traceability finding, review comment `4077992187`: the authoritative receipts named code candidate `1cbef224...` but did not explicitly anchor the three subsequent evidence-only commits or their exact-head checks. The review did not identify a new runtime-smoke or containment-code defect at `13a6d220...`.

Exact hosted verification for `13a6d220125ede24c9eea1662ecb287837740763`:

- Windows Server 2022 run `35802790099`, job `106996725906`: `completed/success` at `2026-09-23T00:38:28Z`. Repository/R0 safety contracts, Release assertion/CTest safety checks, VS2022 x64 configure, Debug build and deterministic tests, Release build and deterministic tests, dependency/prerequisite checks, static verifiers, and clean tracked-tree verification passed.
- Profiling capture portability run `35802790114`: `completed/success`.
- Release manifest integrity run `35802790175`: `completed/success`.

These checks are evidence for the exact evidence tree. Hosted deterministic CTest still excludes tests whose names end in `RuntimeSmoke`, so the interactive GUI `EditorRuntimeSmoke` did not run in hosted CI.

### Receipt self-reference rule

A commit cannot generally contain its own not-yet-computed Git commit SHA because the commit ID is content-addressed from the tree and commit metadata. Therefore this receipt anchors the last independently reviewed and fully hosted-verified tree, `13a6d220...`. The exact post-receipt-repair head and its workflow results must be recorded in PR #13 metadata/commentary after the write, without creating another receipt-only commit solely to chase its own SHA. This avoids an infinite evidence-only commit loop while preserving exact traceability.

## Primary-source basis, rechecked 2026-09-23 UTC

- Microsoft Learn, Job Objects: https://learn.microsoft.com/en-us/windows/win32/procthread/job-objects
- Microsoft Learn, `TerminateJobObject`: https://learn.microsoft.com/en-us/windows/win32/api/jobapi2/nf-jobapi2-terminatejobobject
- Microsoft Learn, `QueryInformationJobObject`: https://learn.microsoft.com/en-us/windows/win32/api/jobapi2/nf-jobapi2-queryinformationjobobject
- Microsoft Learn, `JOBOBJECT_BASIC_ACCOUNTING_INFORMATION`: https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-jobobject_basic_accounting_information
- Microsoft Learn, `UpdateProcThreadAttribute`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-updateprocthreadattribute
- CMake, `TIMEOUT`: https://cmake.org/cmake/help/latest/prop_test/TIMEOUT.html
- Epic Games, UE 5.8 Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
- Epic Games, UE 5.8 Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
- Unity Technologies, Unity 6.0 Hierarchy: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html

Microsoft documents that child processes created by a job-member process remain in the same Job Object by default unless breakaway is enabled; E11 enables neither breakaway flag. `TerminateJobObject` terminates all currently associated processes, and `JobObjectBasicAccountingInformation::ActiveProcesses` supplies the contained-process count used for the empty-job proof. Epic UE 5.8 and Unity 6.0 remain workflow references only for hierarchy, selection, details/inspector, and asset/editor-surface expectations. No proprietary source was copied and no dependency was added.

## Retained acceptance surface

All established E11 checks remain required: one stable visible/enabled process-owned top-level editor; original 12-child HWND/class continuity; bound semantic controls; exact five ordered Outliner rows and four ordered Assets rows; Outliner `LBS_NOTIFY`; exact Scene Root/Cube Inspector fixtures; post-notification Cube synchronization; truthful disabled pending tools; bounded 800x600 and 420x260 resizes with complete-state and containment checks; bounded cross-process messages; clean process-owned normal shutdown; worker-local cleanup; and supervisor-level process-tree cleanup verification.

`native_evidence` remains empty. Issue #7 remains open, so the historical R0 runner is blocked and was not invoked. E11 remains partial and is not UE5/Unity parity.

## Registered native handoff

After the receipt repair receives clean independent re-review, the registered Windows executor should run the exact reviewed branch head on one owned interactive desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, commands, full stdout/stderr, exit codes, UTC timestamps, normal plus narrow-window screenshots, and proof that any failure/interruption leaves zero owned contained processes.

## Single next useful action

Re-review the evidence-only receipt repair while keeping the code candidate fixed. If clean, run the registered Windows Debug/Release GUI smoke and preserve the complete native receipt set. Do not create another documentation-only commit merely to embed that commit's own SHA.