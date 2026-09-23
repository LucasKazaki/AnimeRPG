# E11 Editor Runtime Smoke task, 2026-09-23

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. This packet may harden the native Windows smoke, its recovery supervisor, deterministic containment tests, and evidence. It must not add scene mutation/serialization, gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, rebase, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently observed `main`: `ee0c625712e214b2092a9e419b9e244fa84bae34`.
Pre-provenance-repair branch head: `d80557ca89ec89b5c0f22eb3ec2ac86092230535`.
Containment-registration implementation commit: `78c4d82314739f045dd39f198601221fe2492221`.
Worker-local cleanup test implementation commit: `abdf334c23ce5dc2c4abbd6ade5bc2a3bccf14e0`.
Runtime-smoke source candidate: `81e7052f47cab06060ea69c9f9d4f25ec42145e4`.
`Tests/EditorRuntimeSmoke.cpp` blob: `117c101acc9d65e297c3e0f948a6c3724ff2416d`.
Current `CMakeLists.txt` blob: `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.

Allowed paths only: `CMakeLists.txt`, `Tests/EditorRuntimeSmoke.cpp`, this task, `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`, and `Docs/Research/ENGINE-CAPABILITIES.json`. One active writer only. Do not rebase, merge, force-push, or absorb unrelated work.

## Current review finding and bounded repair

Fresh independent review of source head `d80557ca89ec89b5c0f22eb3ec2ac86092230535` completed on `2026-09-23T07:25:58Z` and opened P2 review thread `PRRT_kwDOTo2Ig86lDcQK`. The defect is evidence provenance, not runtime behavior: this packet's QA record called source head `c4c5ba4...` an exact tested evidence tree even though the Windows workflow is triggered by `pull_request` and uses `actions/checkout@v6` without a `ref` override. GitHub's documented behavior is that such a run checks out `refs/pull/<n>/merge`; `GITHUB_SHA` is the synthetic merge commit, not the raw source-head SHA.

This pass repairs the evidence model only. Runtime code is unchanged. Hosted evidence must distinguish:

1. **PR source head**: the branch revision associated with the workflow run.
2. **Tested integration commit**: the synthetic `refs/pull/13/merge` commit actually checked out by default `actions/checkout` behavior.
3. **Tested base**: the first parent of that synthetic merge at run creation.

For the latest fully identified hosted Windows result before this repair:

- associated source head: `d80557ca89ec89b5c0f22eb3ec2ac86092230535`;
- tested PR merge: `f56157cc85122769324cc58ef5bd71b496fff6d7`;
- tested base: `747c6837afb6a8993286cc2139fb61931306645b`;
- the fetched synthetic commit message is `Merge d80557ca89ec89b5c0f22eb3ec2ac86092230535 into 747c6837afb6a8993286cc2139fb61931306645b`, and its two parents are exactly that base and source head;
- Windows run `35831227820`, job `107084165732`, completed `success` at `2026-09-23T07:23:26Z`.

This is exact PR-integration evidence for `f56157...`, associated with source head `d80557...`. It is **not** a claim that the raw `d80557...` tree itself was what the Windows runner checked out. Current `main` later advanced independently to `ee0c625...`; that does not rewrite the historical tested base and is not absorbed into this branch.

Historical source head `c4c5ba4ca4347592bdb04f07ae28d857970674d5` remains associated with Windows run `35827065028`, but its synthetic merge identity was not preserved in the current durable record. Therefore this packet no longer treats that run as exact raw-head or exact-merge evidence. The implementation head `abdf334c23ce5dc2c4abbd6ade5bc2a3bccf14e0` does retain exact integration provenance: synthetic merge `08fa1bd6ac32ca661cbf21914f2c01570958b666` into base `dbb1121a0b562f5c5d11794e57e383bb47696db4`.

## Implemented E11 verification hardening retained

1. **Containment registration repair** at `78c4d823...`: deterministic test renamed from `EditorRuntimeSmokeContainmentTests` to `EditorContainmentTests`, so hosted `ctest ... -E "RuntimeSmoke"` still excludes the interactive GUI smoke but includes deterministic containment.
2. **Worker-local cleanup coverage** at `abdf334c...`: once the deterministic fixture observes a live two-process contained tree, it calls the real `CleanupProcess(child.process, ...)`, requires the production forced exit code `2`, proves at least one contained descendant remains, then calls `SupervisorTerminateJobAndVerifyEmpty` and requires zero active processes. The shared normal-success path still rejects a zero-exit worker that leaves a contained descendant.

No production editor source, `Tests/EditorRuntimeSmoke.cpp`, workflow file, dependency, graphics API, game content, or scheduler configuration changed in this provenance repair.

## Primary-source basis, rechecked 2026-09-23 UTC

- GitHub Docs, `Events that trigger workflows`: https://docs.github.com/en/actions/reference/workflows-and-actions/events-that-trigger-workflows
  - applicability: for open mergeable `pull_request` workflows, `GITHUB_REF` is `refs/pull/<number>/merge`, `GITHUB_SHA` is the merge-branch commit, and default `actions/checkout` checks out that merge result; checking only the PR head requires explicitly selecting `github.event.pull_request.head.sha`.
- `actions/checkout` documentation: https://github.com/actions/checkout
  - applicability: examples explicitly pass `ref: ${{ github.event.pull_request.head.sha }}` when the desired subject is the raw pull-request head rather than the default event ref.
- Microsoft Learn `TerminateProcess`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-terminateprocess
  - applicability: terminating another process is asynchronous; callers that need confirmed termination must wait on the process handle.
- Microsoft Learn `TerminateJobObject`: https://learn.microsoft.com/en-us/windows/win32/api/jobapi2/nf-jobapi2-terminatejobobject
  - applicability: terminates all processes currently associated with the job.
- Microsoft Learn `QueryInformationJobObject`: https://learn.microsoft.com/en-us/windows/win32/api/jobapi2/nf-jobapi2-queryinformationjobobject
  - applicability: retrieves job state used for active-process accounting.
- Microsoft Learn Job Objects: https://learn.microsoft.com/en-us/windows/win32/procthread/job-objects
  - applicability: job objects provide process-group management/accounting and can be waited on for all-process termination.
- CMake `ctest(1)`: https://cmake.org/cmake/help/latest/manual/ctest.1.html
  - applicability: `-E` excludes tests whose names match the supplied regular expression.

Behavioral/API/evidence references only. No proprietary source was copied and no dependency was added.

## Hosted evidence retained

Implementation source head `abdf334c23ce5dc2c4abbd6ade5bc2a3bccf14e0` was associated with Windows run `35826650901`, job `107069750614`, completed `success` at `2026-09-23T06:27:18Z`. Its exact tested synthetic merge was `08fa1bd6ac32ca661cbf21914f2c01570958b666` into base `dbb1121a0b562f5c5d11794e57e383bb47696db4`. Associated profiling `35826650939` and release-manifest `35826650785` also passed.

Source head `d80557ca89ec89b5c0f22eb3ec2ac86092230535` was associated with Windows run `35831227820`, job `107084165732`. The exact tested synthetic merge was `f56157cc85122769324cc58ef5bd71b496fff6d7` into base `747c6837afb6a8993286cc2139fb61931306645b`. The Windows job passed repository/R0 safety contracts, Release assertion/CTest safety, VS2022 x64 configuration, Debug and Release builds, deterministic suites, dependency/prerequisite/runtime-policy checks, static verifiers, and clean-tree verification. Associated profiling `35831227853` and release-manifest `35831227827` completed successfully.

Hosted deterministic CTest intentionally excludes interactive `EditorRuntimeSmoke`; none of these results is native GUI evidence. A fresh independent review of the post-repair exact source tree is required before independent acceptance.

## Retained native acceptance surface

`EditorRuntimeSmoke` still requires one stable visible/enabled process-owned top-level editor; original 12-child HWND/class continuity; semantic Static and toolbar Button identity; exact ordered Outliner/assets fixtures; `LBS_NOTIFY`; complete Inspector fixtures; selection synchronization; truthful disabled pending tools; bounded normal/narrow resizes; bounded cross-process operations; clean process-owned shutdown; worker-local cleanup; and supervisor-level zero-process-tree cleanup.

`native_evidence` remains empty. Issue #7 remains open, so the historical R0 runner is blocked and must not be invoked. E11 remains partial and is not UE5/Unity parity.

## Registered native handoff

After fresh independent review of the provenance-repaired exact source tree is clean, the registered Windows executor should run on one owned interactive desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact reviewed **source** SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, normal plus narrow-window screenshots for the GUI smoke, and process inspection proving zero owned contained processes after any failure or interruption. Native evidence must name the source SHA directly because it is not a GitHub synthetic-merge run.

## Rollback and stop conditions

Rollback only this evidence reconciliation if it misstates the existing implementation or receipts. Stop before production-runtime change, workflow edit outside packet authority, rebase, merge, R0 execution, scheduler operation, dependency addition, graphics/API change, or game-content work. Never weaken containment assertions or native acceptance checks to make a gate green.

## Single next useful action

Obtain fresh independent review of the post-provenance-repair source tree and verify the new PR integration run records its source head, tested synthetic merge, and tested base separately. If clean, hand that reviewed source tree to the registered Windows executor for native Debug/Release `EditorContainmentTests` plus interactive `EditorRuntimeSmoke` acceptance.
