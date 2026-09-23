# E11 Editor Runtime Smoke evidence, 2026-09-23

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. Runtime code is unchanged in this pass. Fresh independent review found that the durable QA receipt was conflating a pull-request source SHA with the synthetic merge commit actually checked out by the hosted Windows workflow. This pass repairs that evidence provenance and preserves the existing native gate.

Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently observed `main`: `ee0c625712e214b2092a9e419b9e244fa84bae34`.
Pre-provenance-repair source head: `d80557ca89ec89b5c0f22eb3ec2ac86092230535`.
Containment-registration implementation: `78c4d82314739f045dd39f198601221fe2492221`.
Worker-local cleanup-test implementation: `abdf334c23ce5dc2c4abbd6ade5bc2a3bccf14e0`.
Runtime-smoke source candidate: `81e7052f47cab06060ea69c9f9d4f25ec42145e4`.
`Tests/EditorRuntimeSmoke.cpp` blob: `117c101acc9d65e297c3e0f948a6c3724ff2416d`.
`CMakeLists.txt` blob: `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.
Production editor source is unchanged.

## Independent-review finding selected

Fresh Codex review of source head `d80557ca89ec89b5c0f22eb3ec2ac86092230535` completed at `2026-09-23T07:25:58Z` and opened P2 thread `PRRT_kwDOTo2Ig86lDcQK` / comment `4080010085`: restore the tested PR merge identity.

The finding is correct. `.github/workflows/windows-ci.yml` triggers on `pull_request` and uses `actions/checkout@v6` without a `ref` override. GitHub documents that open mergeable pull-request workflows use `refs/pull/<number>/merge`; `GITHUB_SHA` is the merge-branch commit, and default `actions/checkout` checks out that merged result. Therefore a workflow's source association and the exact tested tree are different identities and must be recorded separately.

This pass changes evidence/control records only. No runtime behavior or test assertion is weakened.

## Corrected hosted evidence model

### Latest fully identified integration result before this repair

Associated PR source head:

`d80557ca89ec89b5c0f22eb3ec2ac86092230535`

Exact tested synthetic merge:

`f56157cc85122769324cc58ef5bd71b496fff6d7`

Exact tested base:

`747c6837afb6a8993286cc2139fb61931306645b`

The fetched synthetic commit has message:

```text
Merge d80557ca89ec89b5c0f22eb3ec2ac86092230535 into 747c6837afb6a8993286cc2139fb61931306645b
```

and exactly those two parents, base first and source head second. It was created at `2026-09-23T07:20:46Z`. Windows run `35831227820` was created two seconds later, is a `pull_request` run associated with source head `d80557...`, and job `107084165732` completed `success` at `2026-09-23T07:23:25Z` / run completion `2026-09-23T07:23:26Z`.

The job passed:

- repository/R0 parser and safety contracts;
- Release assertion and CTest safety contracts;
- Visual Studio 2022 x64 configuration;
- Visual C++ Build Tools version recording;
- Debug build;
- deterministic Debug tests;
- Release build;
- PE dependency and Windows prerequisite/runtime-policy checks;
- deterministic Release tests;
- static milestone verifiers;
- clean tracked-tree verification.

This is exact integration evidence for synthetic merge `f56157...`, associated with source head `d80557...`. It is not raw-head execution evidence for `d80557...`.

Associated source-head workflows also completed successfully:

- profiling capture portability `35831227853`;
- release-manifest integrity `35831227827`.

Those workflow IDs are retained as associated evidence. This receipt does not invent their checkout SHA where it has not been independently pinned.

### Implementation result with retained merge identity

Implementation source head `abdf334c23ce5dc2c4abbd6ade5bc2a3bccf14e0` was associated with Windows run `35826650901`, job `107069750614`, completed `success` at `2026-09-23T06:27:18Z`.

Its exact tested synthetic merge is `08fa1bd6ac32ca661cbf21914f2c01570958b666`, with source head `abdf334c...` merged into base `dbb1121a0b562f5c5d11794e57e383bb47696db4`. Associated profiling `35826650939` and release-manifest `35826650785` also passed.

### Historical `c4c5ba4...` correction

Source head `c4c5ba4ca4347592bdb04f07ae28d857970674d5` remains associated with Windows run `35827065028`, job `107071022769`, which completed successfully. The prior receipt incorrectly called that source tree an exact tested head while omitting the synthetic merge/base identity. Because that exact historical merge SHA is not retained in the current durable record, this run is no longer cited as proof that the raw `c4c5ba4...` tree itself executed or as exact integration evidence. The newer `f56157...` result above supplies a fully pinned integration receipt for the same unchanged runtime implementation plus the later evidence reconciliation.

Current `main` subsequently advanced independently to `ee0c625712e214b2092a9e419b9e244fa84bae34`. That later branch movement does not alter either historical tested base and was not merged or rebased into this engine branch by this pass.

## Current implementation described by the receipts

`EditorContainmentTests` is selected by hosted deterministic suites because its name no longer matches `-E "RuntimeSmoke"`. Its current self-test:

1. launches a contained parent that creates a long-lived descendant;
2. waits until Job Object accounting observes the two-process tree;
3. calls the real worker-local `CleanupProcess` on the direct child;
4. requires cleanup success and forced exit code `2`;
5. requires the job to remain non-empty, proving the descendant backstop is still exercised;
6. calls `SupervisorTerminateJobAndVerifyEmpty` and requires zero active processes;
7. runs the shared normal-success acceptance path against a zero-exit parent that leaves a descendant and requires rejection plus forced whole-job cleanup.

The interactive `EditorRuntimeSmoke` and production editor code were not changed by the worker-local cleanup-test hardening or this provenance repair.

## Primary research, rechecked 2026-09-23 UTC

- GitHub Docs, `Events that trigger workflows`: https://docs.github.com/en/actions/reference/workflows-and-actions/events-that-trigger-workflows
  - for open mergeable `pull_request` workflows, the event ref is `refs/pull/<number>/merge`, `GITHUB_SHA` is the merge-branch commit, and default `actions/checkout` checks that merged result. To test only the PR head, explicitly select `github.event.pull_request.head.sha`.
- `actions/checkout` documentation: https://github.com/actions/checkout
  - examples explicitly set `ref: ${{ github.event.pull_request.head.sha }}` when raw-head checkout is required.
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

## Review state

The earlier P2 on source head `c4c5ba4...` about the stale authoritative task packet was repaired by the evidence reconciliation leading to source head `d80557...`. The fresh review of `d80557...` then found the separate provenance defect described above. That finding is addressed by this bounded evidence-only repair, but independent acceptance remains false until the new exact source head receives a clean fresh review.

## Current verification state

Passed and evidenced:

- deterministic containment test is selected by hosted CI;
- real worker-local direct-process cleanup is exercised;
- direct cleanup must return exit code `2` and leave the descendant backstop active;
- supervisor whole-job cleanup must prove zero active processes;
- shared normal-success logic rejects a zero-exit worker that leaves a descendant;
- hosted MSVC Debug/Release deterministic suites passed on exact synthetic PR integration commit `f56157...`, with associated source head `d80557...` and tested base `747c6837...` explicitly distinguished;
- exact integration provenance is also retained for implementation head `abdf334c...` through synthetic merge `08fa1bd...` into base `dbb1121...`.

Still pending:

- fresh independent review of the provenance-repaired source tree;
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

Retain exact reviewed source SHA, machine/Windows identity, MSVC/CMake and GPU/driver versions, exact commands, complete stdout/stderr, exit codes, UTC timestamps, normal and narrow screenshots, and process inspection showing zero owned contained processes after any failure or interruption.

## Single next action

Obtain fresh independent review of the provenance-repaired source tree and verify the next PR integration run records source head, synthetic merge SHA, and tested base separately. If clean, execute the native Debug/Release containment plus interactive GUI handoff on the registered Windows executor.
