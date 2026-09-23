# E11 Editor Runtime Smoke evidence, 2026-09-23

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. This pass hardened the existing deterministic containment test so it now executes the same worker-local `CleanupProcess` used by `EditorRuntimeSmoke`, verifies that direct-process cleanup succeeds with the expected forced exit code, proves a contained descendant remains for the supervisor backstop, and then verifies whole-job cleanup. It did not authorize scene mutation/serialization, gizmos, Play-in-Editor, asset import, graphics/API changes, dependencies, game content, scheduler operations, deployment, release, merge, rebase, or R0 execution.

Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` used by this pass's PR integration result: `dbb1121a0b562f5c5d11794e57e383bb47696db4`.
Previous independently reviewed evidence head: `4ae42123fc42df1e905f957c4e8fb9e7f0582035`.
Worker-local cleanup test implementation commit: `abdf334c23ce5dc2c4abbd6ade5bc2a3bccf14e0`.
Runtime-smoke source candidate remains `81e7052f47cab06060ea69c9f9d4f25ec42145e4`.
`Tests/EditorRuntimeSmoke.cpp` remains blob `117c101acc9d65e297c3e0f948a6c3724ff2416d`.
Current `CMakeLists.txt` blob: `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.
Production editor source remains unchanged.

## Selected verification gap

The real worker's failure path in `Tests/EditorRuntimeSmoke.cpp` uses `CleanupProcess`: it checks whether the owned editor process is already signaled, otherwise calls `TerminateProcess`, waits for process termination, and verifies a terminal exit code. The deterministic `EditorContainmentTests` previously exercised the outer Job Object supervisor and its zero-process-tree proof, but did not call this worker-local cleanup routine. That left an important recovery layer source-reviewed but not positively exercised by hosted deterministic Windows tests.

The selected packet is test hardening only. No production editor behavior was changed.

## Bounded implementation

Implementation commit `abdf334c23ce5dc2c4abbd6ade5bc2a3bccf14e0` changes only the generated containment self-test in `CMakeLists.txt`.

After the contained self-test parent has launched its long-lived descendant and the job reports at least two active processes, the deterministic test now:

1. calls the real worker-local `CleanupProcess(child.process, ...)`;
2. requires success and the production forced direct-process exit code `2`;
3. queries Job Object accounting and requires at least one active process to remain, proving the descendant backstop is still being exercised rather than accidentally accepting direct-process cleanup as whole-tree cleanup;
4. invokes `SupervisorTerminateJobAndVerifyEmpty` to terminate the remaining contained tree and prove the Job Object becomes empty;
5. retains the existing shared normal-success-path regression in which a worker exits `0` while leaving a descendant, which must be rejected and forcibly cleaned up.

The test PASS text now explicitly records worker-local direct cleanup, live-tree cleanup, and shared normal-success rejection. `EditorRuntimeSmoke` itself and production editor code are unchanged.

## Primary research, accessed 2026-09-23 UTC

- Microsoft Learn `TerminateProcess`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-terminateprocess
  - applicability: terminating another process is asynchronous; when termination must be confirmed, wait on the process handle. This is the behavior the worker-local cleanup routine now receives deterministic coverage for.
- Microsoft Learn process/thread functions, Job Object functions: https://learn.microsoft.com/en-us/windows/win32/procthread/process-and-thread-functions
  - applicability: `TerminateJobObject` terminates all processes currently associated with the job; `QueryInformationJobObject` retrieves job state. This supports the supervisor backstop remaining distinct from direct-process cleanup.
- Microsoft Learn `QueryInformationJobObject`: https://learn.microsoft.com/en-us/windows/win32/api/jobapi2/nf-jobapi2-queryinformationjobobject
  - applicability: retrieves Job Object state used by the active-process accounting check.
- Microsoft Learn Job Objects: https://learn.microsoft.com/en-us/windows/win32/procthread/job-objects
  - applicability: Job Objects provide process-tree management and accounting, including the `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE` semantics already used by the packet.
- CMake `ctest(1)` and `add_test` remain the registration basis for `EditorContainmentTests`; the earlier selection repair is unchanged.

These are behavioral/API references only. No proprietary source was copied and no new dependency was imported.

## Verification performed in this pass

The GitHub write was re-read through commit diff. Commit `abdf334c...` is exactly one commit ahead of the previous reviewed tree and changes only `CMakeLists.txt`: 43 additions and 3 deletions in the containment self-test. `Tests/EditorRuntimeSmoke.cpp`, production editor source, dependencies, workflow files, graphics API, game content, and scheduler state were not changed.

GitHub Actions Windows run `35826650901`, job `107069750614`, completed `success` at `2026-09-23T06:27:18Z` for source head `abdf334c23ce5dc2c4abbd6ade5bc2a3bccf14e0`. GitHub's current synthetic PR merge for that head is `08fa1bd6ac32ca661cbf21914f2c01570958b666`, merging the engine source head into `dbb1121a0b562f5c5d11794e57e383bb47696db4`; this is PR integration evidence rather than a raw-head checkout claim.

The Windows job positively passed:

- repository/R0 parser and safety contracts;
- Release assertion and CTest safety contracts;
- Visual Studio 2022 x64 configuration;
- Debug build;
- deterministic Debug tests, including the current selected `EditorContainmentTests`;
- Release build;
- dependency/prerequisite/runtime-policy checks;
- deterministic Release tests, including the current selected `EditorContainmentTests`;
- static milestone verifiers;
- clean tracked-tree verification.

No per-test timing is claimed for this run because the connector exposes step results but not the retained CTest log text. The previous registration repair already established that `EditorContainmentTests` is selected under the hosted `-E "RuntimeSmoke"` command, and that registration was unchanged by this pass.

Associated workflows for exact source head `abdf334c...` also completed successfully:

- profiling capture portability `35826650939`;
- release-manifest integrity `35826650785`.

The interactive `EditorRuntimeSmoke` remains intentionally excluded from hosted deterministic CTest and was not claimed as executed.

## Sandbox limitation

A coordinator checkout was attempted with:

```text
git clone --depth 1 --branch engine/2026-09-22-editor-runtime-smoke https://github.com/LucasKazaki/AnimeRPG.git /tmp/animerpg-e11
```

The sandbox could not resolve `github.com`, so no sandbox-native build was claimed. The real Windows hosted build/test evidence above is stronger for this Win32-only path.

## Historical containment registration repair retained

The earlier P2 defect, where `EditorRuntimeSmokeContainmentTests` matched hosted `ctest -E "RuntimeSmoke"` and was silently skipped, remains repaired by the name `EditorContainmentTests`. Positive prior execution evidence remains Windows run `35818360831`, job `107044690265`, where the renamed test passed in both Debug and Release. Older pre-rename workflow successes remain withdrawn as containment-test execution evidence.

## Current verification state

Passed and evidenced:

- worker-local `CleanupProcess` is now exercised by the real deterministic Windows containment test;
- direct-process cleanup must return the production forced exit code and leave the long-lived descendant for the supervisor backstop;
- supervisor whole-job termination must then prove the contained job empty;
- the shared normal-success path still rejects a zero-exit worker that leaves a descendant;
- hosted MSVC Debug and Release builds and deterministic suites passed for the new source head;
- profiling and release-manifest workflows passed for the same source head;
- previous exact tree `4ae42123...` had a clean independent Codex review before this additional test-only hardening.

Still pending:

- fresh independent review of the new cleanup-test implementation/evidence tree;
- registered interactive-Windows Debug and Release `EditorRuntimeSmoke` execution;
- local/native screenshots and GPU/driver receipts;
- clean-machine package launch, measured comparative performance/memory, broad stress/recovery, and 24-hour soak.

`native_evidence` remains empty. Issue #7 remains open; the historical R0 runner was not invoked.

Status: **Worker-local direct cleanup plus supervisor whole-tree recovery are now both positively exercised by hosted deterministic Windows tests. Native GUI and fresh independent acceptance remain pending. No UE5/Unity parity claim is made.**

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

Retain exact reviewed source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, normal and narrow-window screenshots for GUI checks, and process inspection showing zero owned contained processes after failure or interruption.

## Single next action

Obtain fresh independent review of this cleanup-test hardening. If clean, hand that exact reviewed tree to the registered Windows executor for native containment plus interactive GUI acceptance.