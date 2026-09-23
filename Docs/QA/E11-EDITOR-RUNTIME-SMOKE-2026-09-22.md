# E11 Editor Runtime Smoke evidence, 2026-09-23

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. Runtime code is unchanged in this pass. The pass reconciles the authoritative task/evidence records after independent review found that the task packet still described only the earlier containment-test rename even though the branch had already added positive worker-local `CleanupProcess` coverage.

Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently observed `main`: `747c6837afb6a8993286cc2139fb61931306645b`.
Pre-reconciliation evidence head: `c4c5ba4ca4347592bdb04f07ae28d857970674d5`.
Containment-registration implementation: `78c4d82314739f045dd39f198601221fe2492221`.
Worker-local cleanup-test implementation: `abdf334c23ce5dc2c4abbd6ade5bc2a3bccf14e0`.
Runtime-smoke source candidate: `81e7052f47cab06060ea69c9f9d4f25ec42145e4`.
`Tests/EditorRuntimeSmoke.cpp` blob: `117c101acc9d65e297c3e0f948a6c3724ff2416d`.
`CMakeLists.txt` blob: `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.
Production editor source is unchanged.

## Independent-review finding selected

Exact head `c4c5ba4...` received P2 review thread `PRRT_kwDOTo2Ig86lCmgu` at `2026-09-23T06:34:21Z`: the higher-priority task packet contradicted the implementation under review because it still claimed the bounded change was only a test rename and that containment logic was otherwise unchanged.

This pass repairs the durable control/evidence model, not runtime behavior. The task, QA receipt, and capability map now agree that E11 contains both the containment-registration repair and the later deterministic exercise of the real worker-local cleanup routine. Fresh independent review of the new exact evidence-only tree remains required.

## Current implementation described by the receipts

`EditorContainmentTests` is selected by hosted deterministic suites because its name no longer matches `-E "RuntimeSmoke"`. Its current self-test:

1. launches a contained parent that creates a long-lived descendant;
2. waits until Job Object accounting observes the two-process tree;
3. calls the real worker-local `CleanupProcess` on the direct child;
4. requires cleanup success and forced exit code `2`;
5. requires the job to remain non-empty, proving the descendant backstop is still exercised;
6. calls `SupervisorTerminateJobAndVerifyEmpty` and requires zero active processes;
7. runs the shared normal-success acceptance path against a zero-exit parent that leaves a descendant and requires rejection plus forced whole-job cleanup.

The interactive `EditorRuntimeSmoke` and production editor code were not changed by the worker-local cleanup-test hardening.

## Primary research, rechecked 2026-09-23 UTC

- Microsoft Learn `TerminateProcess`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-terminateprocess
  - cross-process termination is asynchronous; confirmed termination requires waiting on the process handle.
- Microsoft Learn `TerminateJobObject`: https://learn.microsoft.com/en-us/windows/win32/api/jobapi2/nf-jobapi2-terminatejobobject
  - terminates all processes currently associated with a job.
- Microsoft Learn `QueryInformationJobObject`: https://learn.microsoft.com/en-us/windows/win32/api/jobapi2/nf-jobapi2-queryinformationjobobject
  - retrieves job state for the active-process accounting proof.
- Microsoft Learn Job Objects: https://learn.microsoft.com/en-us/windows/win32/procthread/job-objects
  - documents process-group management/accounting and the signaled state after all processes terminate.
- CMake `ctest(1)`: https://cmake.org/cmake/help/latest/manual/ctest.1.html
  - `-E` excludes matching test names; the earlier registration repair remains applicable.

No proprietary source was copied and no dependency was imported.

## Hosted verification retained

Implementation head `abdf334c23ce5dc2c4abbd6ade5bc2a3bccf14e0`:
- Windows run `35826650901`, job `107069750614`, completed `success` at `2026-09-23T06:27:18Z`;
- profiling run `35826650939`, `completed/success`;
- release-manifest run `35826650785`, `completed/success`.

The Windows job passed repository/R0 safety contracts, Release assertion/CTest safety, VS2022 x64 configuration, Debug build and deterministic tests including `EditorContainmentTests`, Release build and deterministic tests including `EditorContainmentTests`, dependency/prerequisite/runtime-policy checks, static verifiers, and clean tracked-tree verification.

Exact pre-reconciliation evidence head `c4c5ba4ca4347592bdb04f07ae28d857970674d5`:
- Windows run `35827065028`, job `107071022769`, completed `success` at `2026-09-23T06:32:40Z`;
- profiling run `35827065069`, `completed/success`;
- release-manifest run `35827065021`, `completed/success`.

Hosted deterministic CTest intentionally excludes the interactive `EditorRuntimeSmoke`; none of these runs is native GUI evidence.

## Review state

The repository contained a contradictory statement that `c4c5ba4...` had a clean review even though the same exact tree has an unresolved P2 task-traceability thread created at `2026-09-23T06:34:21Z`. This receipt treats the unresolved thread as authoritative evidence requiring repair. The finding is addressed by this evidence-only reconciliation. Independent acceptance remains false until the post-repair exact tree receives a clean fresh review.

## Current verification state

Passed and evidenced:
- deterministic containment test is selected by hosted CI;
- real worker-local direct-process cleanup is exercised;
- direct cleanup must return exit code `2` and leave the descendant backstop active;
- supervisor whole-job cleanup must prove zero active processes;
- shared normal-success logic rejects a zero-exit worker that leaves a descendant;
- hosted MSVC Debug/Release deterministic suites and supporting workflows passed for the implementation and pre-reconciliation evidence head.

Still pending:
- fresh independent review of the post-reconciliation exact tree;
- registered interactive-Windows Debug and Release `EditorRuntimeSmoke`;
- local screenshots and GPU/driver receipts;
- clean-machine packaging, measured comparative performance/memory, broad stress/recovery, and the required 24-hour soak.

`native_evidence` remains empty. Issue #7 remains open and the historical R0 runner was not invoked. No UE5/Unity parity claim is made.

## Registered native handoff

After fresh independent review is clean, run on one owned interactive Windows desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact reviewed SHA, machine/Windows identity, MSVC/CMake and GPU/driver versions, exact commands, complete stdout/stderr, exit codes, UTC timestamps, normal and narrow screenshots, and process inspection showing zero owned contained processes after any failure or interruption.

## Single next action

Obtain fresh independent review of the reconciled exact tree. If clean, execute the native Debug/Release containment plus interactive GUI handoff on the registered Windows executor.
