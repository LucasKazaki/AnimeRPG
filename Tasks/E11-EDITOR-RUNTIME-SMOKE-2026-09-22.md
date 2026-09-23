# E11 Editor Runtime Smoke task, 2026-09-23

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. This packet may harden the native Windows smoke, its recovery supervisor, deterministic containment tests, and evidence. It must not add scene mutation/serialization, gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, rebase, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently observed `main`: `747c6837afb6a8993286cc2139fb61931306645b`.
Pre-reconciliation branch head: `c4c5ba4ca4347592bdb04f07ae28d857970674d5`.
Containment-registration implementation commit: `78c4d82314739f045dd39f198601221fe2492221`.
Worker-local cleanup test implementation commit: `abdf334c23ce5dc2c4abbd6ade5bc2a3bccf14e0`.
Runtime-smoke source candidate: `81e7052f47cab06060ea69c9f9d4f25ec42145e4`.
`Tests/EditorRuntimeSmoke.cpp` blob: `117c101acc9d65e297c3e0f948a6c3724ff2416d`.
Current `CMakeLists.txt` blob: `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.

Allowed paths only: `CMakeLists.txt`, `Tests/EditorRuntimeSmoke.cpp`, this task, `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`, and `Docs/Research/ENGINE-CAPABILITIES.json`. One active writer only. Do not rebase, merge, force-push, or absorb unrelated work.

## Current review finding and bounded repair

Fresh independent review of exact head `c4c5ba4ca4347592bdb04f07ae28d857970674d5` opened P2 review thread `PRRT_kwDOTo2Ig86lCmgu` at `2026-09-23T06:34:21Z`. The implementation and QA/capability records had advanced to worker-local cleanup coverage, but this higher-priority task packet still described only the earlier containment-test rename and said the supervisor/containment logic was unchanged. That made the authoritative task contradict the code and evidence.

This pass is an evidence/control reconciliation only. Runtime code is unchanged. The authoritative packet now describes both completed E11 hardenings:

1. **Containment registration repair** at `78c4d823...`: deterministic test renamed from `EditorRuntimeSmokeContainmentTests` to `EditorContainmentTests`, so hosted `ctest ... -E "RuntimeSmoke"` still excludes the interactive GUI smoke but includes deterministic containment.
2. **Worker-local cleanup coverage** at `abdf334c...`: once the deterministic fixture observes a live two-process contained tree, it calls the real `CleanupProcess(child.process, ...)`, requires the production forced exit code `2`, proves at least one contained descendant remains, then calls `SupervisorTerminateJobAndVerifyEmpty` and requires zero active processes. The existing shared normal-success path still rejects a zero-exit worker that leaves a contained descendant.

No production editor source, `Tests/EditorRuntimeSmoke.cpp`, workflow file, dependency, graphics API, game content, or scheduler configuration changed in this reconciliation.

## Primary-source basis, rechecked 2026-09-23 UTC

- Microsoft Learn `TerminateProcess`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-terminateprocess
  - current applicability: terminating another process is asynchronous; callers that need confirmed termination must wait on the process handle.
- Microsoft Learn `TerminateJobObject`: https://learn.microsoft.com/en-us/windows/win32/api/jobapi2/nf-jobapi2-terminatejobobject
  - applicability: terminates all processes currently associated with the job.
- Microsoft Learn `QueryInformationJobObject`: https://learn.microsoft.com/en-us/windows/win32/api/jobapi2/nf-jobapi2-queryinformationjobobject
  - applicability: retrieves job state used for active-process accounting.
- Microsoft Learn Job Objects: https://learn.microsoft.com/en-us/windows/win32/procthread/job-objects
  - applicability: job objects provide process-group management/accounting and can be waited on for all-process termination.
- CMake `ctest(1)`: https://cmake.org/cmake/help/latest/manual/ctest.1.html
  - applicability: `-E` excludes tests whose names match the supplied regular expression.

Behavioral/API references only. No proprietary source was copied and no dependency was added.

## Hosted evidence retained

Implementation head `abdf334c23ce5dc2c4abbd6ade5bc2a3bccf14e0` passed Windows run `35826650901`, job `107069750614`, completed `success` at `2026-09-23T06:27:18Z`. Associated profiling `35826650939` and release-manifest `35826650785` also passed. The Windows job covered repository/R0 safety contracts, Release assertion/CTest safety, VS2022 x64 configuration, Debug and Release builds, deterministic suites including selected `EditorContainmentTests`, dependency/prerequisite/runtime-policy checks, static verifiers, and clean-tree verification.

Exact evidence head `c4c5ba4ca4347592bdb04f07ae28d857970674d5` passed Windows run `35827065028`, job `107071022769`, completed `success` at `2026-09-23T06:32:40Z`, plus profiling `35827065069` and release-manifest `35827065021`. These hosted results do not execute the interactive `EditorRuntimeSmoke`.

The P2 finding on `c4c5ba4...` is evidence/task traceability, not a new runtime-code defect. This reconciliation changes only durable records. A fresh independent review of the post-repair exact head is still required before independent acceptance.

## Retained native acceptance surface

`EditorRuntimeSmoke` still requires one stable visible/enabled process-owned top-level editor; original 12-child HWND/class continuity; semantic Static and toolbar Button identity; exact ordered Outliner/assets fixtures; `LBS_NOTIFY`; complete Inspector fixtures; selection synchronization; truthful disabled pending tools; bounded normal/narrow resizes; bounded cross-process operations; clean process-owned shutdown; worker-local cleanup; and supervisor-level zero-process-tree cleanup.

`native_evidence` remains empty. Issue #7 remains open, so the historical R0 runner is blocked and must not be invoked. E11 remains partial and is not UE5/Unity parity.

## Registered native handoff

After fresh independent review of the reconciled exact tree is clean, the registered Windows executor should run on one owned interactive desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, normal plus narrow-window screenshots for the GUI smoke, and process inspection proving zero owned contained processes after any failure or interruption.

## Rollback and stop conditions

Rollback only this evidence reconciliation if it misstates the existing implementation or receipts. Stop before production-runtime change, workflow edit outside packet authority, rebase, merge, R0 execution, scheduler operation, dependency addition, graphics/API change, or game-content work. Never weaken containment assertions or native acceptance checks to make a gate green.

## Single next useful action

Obtain fresh independent review of the post-reconciliation exact tree. If clean, hand that exact reviewed tree to the registered Windows executor for native Debug/Release `EditorContainmentTests` plus interactive `EditorRuntimeSmoke` acceptance.
