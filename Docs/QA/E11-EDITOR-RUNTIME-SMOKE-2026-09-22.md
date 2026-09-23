# E11 Editor Runtime Smoke evidence, 2026-09-23

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. This packet verifies the already-integrated Win32 `AstralEditor`; it does not authorize scene mutation/serialization, gizmos, Play-in-Editor, asset import, graphics/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed during this packet: `dece8fbb47b6a937b9bafafa3faea837a359fb4d`.
Current code candidate: `1cbef2242fb79edda068751dd71996b5548ec739`.
`CMakeLists.txt` blob: `ed6a7f44d87241560faf32a57465befd536b59f9`.
`Tests/EditorRuntimeSmoke.cpp` blob: `3ca40bccd15950ebd8774ed36902ab15213035bb`.
Production editor source is unchanged by this evidence-only repair.

## Implemented containment repair retained

Candidate `1cbef2242fb79edda068751dd71996b5548ec739` factors normal worker handling into `SupervisorRunNormalWorkerAcceptance(...)`, used by both the real supervisor and the deterministic containment self-test. A worker that exits `0` is accepted only after its retained process handle is closed and Job Object accounting proves `ActiveProcesses == 0`. A zero-exit worker that leaves a contained descendant is rejected, the complete job is terminated with `TerminateJobObject`, and cleanup must prove the job becomes empty. The self-test creates that lingering-descendant state and routes it through the same shared normal-success function.

The earlier review finding on candidate `7d38e2cd7ffc6b76952711212e7da78d66c7f1af` is therefore repaired without weakening acceptance. No editor production source and no `Tests/EditorRuntimeSmoke.cpp` code changed in the present pass.

## Exact final evidence checkpoint reviewed

Exact evidence head: `13a6d220125ede24c9eea1662ecb287837740763`.

Codex independently reviewed that exact tree and completed at `2026-09-23T00:41:26Z`. It reported one P2 evidence-traceability finding, inline review comment `4077992187`: the durable receipts identified code candidate `1cbef224...` but did not explicitly record the three later evidence-only commits or the exact-head validation. The review did not report a new runtime-smoke, Job Object, or containment-code defect at `13a6d220...`.

Exact hosted checks for `13a6d220125ede24c9eea1662ecb287837740763`:

- Windows Server 2022 workflow `35802790099`, job `106996725906`: `completed/success`, completed `2026-09-23T00:38:28Z`.
  - repository/R0 safety contracts: PASS; historical R0 runner itself not invoked;
  - Release assertion and CTest safety contracts: PASS;
  - Visual Studio 2022 x64 configure: PASS;
  - Debug build and deterministic tests: PASS;
  - Release build and deterministic tests: PASS;
  - dependency/prerequisite checks: PASS;
  - static verifiers and clean tracked-tree verification: PASS.
- profiling capture portability `35802790114`: `completed/success`.
- release manifest integrity `35802790175`: `completed/success`.

Hosted deterministic CTest intentionally excludes tests whose names end in `RuntimeSmoke`. Therefore the actual interactive editor `EditorRuntimeSmoke` did not execute in hosted CI. The deterministic containment target is recovery-harness evidence, not editor GUI acceptance.

## Evidence self-reference policy

This receipt intentionally anchors the last independently reviewed and fully hosted-verified evidence tree, `13a6d220...`. A Git commit cannot generally embed its own final SHA because the commit ID is content-addressed from the tree and commit metadata. The exact head created by this receipt repair, plus its workflow results, must therefore be recorded in PR #13 metadata/commentary after the write instead of creating another receipt-only commit solely to self-reference. This policy preserves exact traceability without an infinite evidence-head rewrite loop.

## Primary research, rechecked 2026-09-23 UTC

- Microsoft Learn, Job Objects: https://learn.microsoft.com/en-us/windows/win32/procthread/job-objects
- Microsoft Learn, `TerminateJobObject`: https://learn.microsoft.com/en-us/windows/win32/api/jobapi2/nf-jobapi2-terminatejobobject
- Microsoft Learn, `QueryInformationJobObject`: https://learn.microsoft.com/en-us/windows/win32/api/jobapi2/nf-jobapi2-queryinformationjobobject
- Microsoft Learn, `JOBOBJECT_BASIC_ACCOUNTING_INFORMATION`: https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-jobobject_basic_accounting_information
- Microsoft Learn, `UpdateProcThreadAttribute`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-updateprocthreadattribute
- CMake, `TIMEOUT`: https://cmake.org/cmake/help/latest/prop_test/TIMEOUT.html
- Epic Games, UE 5.8 Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
- Epic Games, UE 5.8 Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
- Unity Technologies, Unity 6.0 Hierarchy: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html

Microsoft documents that child processes created by a process already in a Job Object remain associated with the same job by default unless breakaway is enabled. E11 enables neither breakaway flag. `TerminateJobObject` applies to all associated processes, and Job Object accounting supplies the `ActiveProcesses` count used for the empty-job acceptance proof. Epic UE 5.8 documents a Level Editor composed of cooperating Level Viewport, Outliner, Details, and Content Browser surfaces, with Outliner/viewport selection synchronization and Details following selection. Unity 6.0 documents the Hierarchy as the scene-object management surface. These are workflow/API references only; no proprietary source or dependency was imported.

## Retained E11 editor acceptance surface

The unchanged GUI smoke still requires one stable visible/enabled process-owned top-level editor; original 12 direct child HWND/class identities; semantic Static-control binding; exact five ordered Outliner rows and four ordered Assets rows; Outliner `LBS_NOTIFY`; exact Scene Root/Cube Inspector fixtures; post-notification Cube synchronization; disabled pending tools; bounded asynchronous resize with complete-state revalidation; bounded cross-process messages; clean worker-local shutdown/cleanup; and supervisor-level zero-contained-process verification.

## Acceptance state and limitations

`native_evidence` remains empty. No interactive Windows desktop execution, human-visible screenshots, actual GPU behavior, clean-machine packaging, measured comparative performance, broader stress/recovery, or 24-hour soak was executed by this coordinator.

The exact `13a6d220...` independent review found only the receipt-traceability P2 above. This evidence-only repair must receive a fresh independent re-review before E11 can treat the source/evidence review gate as clean. Author review is not independent acceptance.

Issue #7 remains open, so the historical R0 runner remains blocked and was not executed.

Status: **shared normal-success process-tree containment is implemented and hosted-tested; exact evidence head `13a6d220...` is independently reviewed and exact-head hosted-verified, but its receipt-traceability finding is being repaired here. Native GUI evidence and clean independent acceptance remain pending, so E11 stays partial and no UE5/Unity parity claim is made.**

## Registered native handoff

After a clean independent re-review of this receipt repair, run the exact reviewed branch head on one owned interactive Windows desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine and Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, normal and narrow-window screenshots, and a process inspection showing zero owned contained processes after any failure or interruption.

## Single next action

Obtain independent re-review of the evidence-only receipt repair while leaving the code candidate unchanged. If clean, execute the registered Debug/Release native GUI smoke and preserve the complete receipt set. Do not add another documentation-only commit merely to embed that commit's own SHA.