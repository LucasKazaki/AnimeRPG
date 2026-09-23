# E11 Editor Runtime Smoke evidence, 2026-09-23

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. This pass repaired a deterministic-test registration defect in the existing editor runtime-smoke packet. It did not authorize scene mutation/serialization, gizmos, Play-in-Editor, asset import, graphics/API changes, dependencies, game content, scheduler operations, deployment, release, merge, rebase, or R0 execution.

Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest base used by the verified PR integration run: `977c5359491b24f729db6886af9b0fef35cea872`.
Pre-repair parent: `9aed26fb23d9f88d4ed7c587d191445a3e628016`.
Containment-registration implementation commit: `78c4d82314739f045dd39f198601221fe2492221`.
Runtime-smoke source candidate remains `81e7052f47cab06060ea69c9f9d4f25ec42145e4`.
`Tests/EditorRuntimeSmoke.cpp` remains blob `117c101acc9d65e297c3e0f948a6c3724ff2416d`.
Repaired `CMakeLists.txt` blob: `3f1f769a8920446907728ddb39e18bcfdc440360`.
Production editor source remains unchanged.

## Independent-review finding reproduced

Fresh Codex review of source head `9aed26fb23d9f88d4ed7c587d191445a3e628016`, submitted `2026-09-23T03:29:14Z`, reported one P2 verification-evidence defect. Hosted deterministic test phases use:

```text
ctest ... -E "RuntimeSmoke" --no-tests=error
```

The deterministic containment target/test was named `EditorRuntimeSmokeContainmentTests`. Astral's `astral_add_test(target)` uses that target name as the CTest test name, so CTest's exclusion regular expression removed the containment self-test from both hosted deterministic suites.

Previous Windows workflows were therefore green without executing the containment self-test. Prior hosted containment-pass claims are withdrawn, without invalidating unrelated tests or workflow steps that did execute.

## Implementation

`CMakeLists.txt` renames only the deterministic target/test:

```text
EditorRuntimeSmokeContainmentTests -> EditorContainmentTests
```

The rename covers the executable target, include directories, compile definitions, link libraries, and `astral_add_test` registration. `EditorRuntimeSmoke` is intentionally unchanged, so hosted `-E "RuntimeSmoke"` still excludes the interactive desktop smoke. The generated supervisor source and actual containment logic are unchanged.

No workflow file was edited because `.github/workflows/windows-ci.yml` is outside this packet's allowed paths and the defect is repaired inside the owned CMake registration.

## Primary research, accessed 2026-09-23 UTC

- CMake 4.4/latest `ctest_test`: https://cmake.org/cmake/help/latest/command/ctest_test.html
  - `EXCLUDE <exclude-regex>` filters tests by test name.
- CMake 4.4/latest `ctest(1)`: https://cmake.org/cmake/help/latest/manual/ctest.1.html
  - command-line `-E` excludes matching test names.
- CMake 4.4/latest `add_test`: https://cmake.org/cmake/help/latest/command/add_test.html
  - `NAME` establishes the CTest test identity; Astral maps the target name directly to it.

These are build/test API references only. No proprietary source was copied and no new dependency was imported.

## Sandbox selection fixture

Coordinator environment: CMake `3.31.6`.
Fixture script SHA-256: `65864f2af1d2d447e007baa2a660204e173a452818af0ae04c11fa62da05c6a3`.
Bad-registration fixture SHA-256: `9a8585228569da4f711fa8561178cd213e5d283de6f2e3fb8d33db2e2b57fe1b`.
Good-registration fixture SHA-256: `59e724e748dbde444d7dfddc141f59ff359586960d0148f0b471782f581e73c9`.

Observed selection behavior:

```text
Before-equivalent names + ctest -N -E RuntimeSmoke => Total Tests: 0
Repaired-equivalent names + same exclusion       => EditorContainmentTests selected; Total Tests: 1
Repaired execution with --no-tests=error         => EditorContainmentTests passed
```

This fixture proves CTest selection behavior only. It does not exercise Windows Job Objects, the production generated supervisor, the GUI smoke, GPU behavior, or Lucas's workstation.

## Hosted Windows verification

GitHub Actions run `35818360831`, job `107044690265`, completed `success` at `2026-09-23T04:28:58Z`.

Important provenance detail: the run is associated with PR source head `78c4d82314739f045dd39f198601221fe2492221`, while Actions checked out synthetic PR merge commit `c49c88022656c81b8ef71e23d84459d474f42a49`, which merged source head `78c4d823...` into current base `977c5359491b24f729db6886af9b0fef35cea872`. This receipt must be described as a PR integration result, not a raw-head checkout.

Retained runner/toolchain evidence:

```text
OS: Microsoft Windows Server 2022, 10.0.20348
Runner image: windows-2022, image version 20260913.307.1
Windows SDK selected: 10.0.26100.0
MSVC compiler identification: 19.44.35228.0
Visual C++ Build Tools: 14.44.35207
```

The actual CTest output positively proves the renamed containment test ran and passed in both configurations under the same `-E "RuntimeSmoke"` filter:

```text
Debug:
Start 9: EditorContainmentTests
9/17 Test #9: EditorContainmentTests ... Passed 5.12 sec
100% tests passed, 0 tests failed out of 17

Release:
Start 9: EditorContainmentTests
9/17 Test #9: EditorContainmentTests ... Passed 5.10 sec
100% tests passed, 0 tests failed out of 17
```

The same job also passed VS2022 x64 configure, Debug build, Release build, PE dependency inspection, Windows prerequisite/runtime-policy checks, static milestone verifiers, and clean tracked-tree verification. The real interactive `EditorRuntimeSmoke` remained excluded from hosted deterministic CTest as intended.

Associated workflows for source head `78c4d823...`:

- profiling capture portability `35818360718`: `completed/success`;
- release-manifest integrity `35818360753`: `completed/success`.

## Historical hosted evidence correction

Pre-repair source head `9aed26fb23d9f88d4ed7c587d191445a3e628016` had Windows `35814098903`, profiling `35814099016`, and release-manifest `35814098909` overall success. Earlier reviewed tree `727dcf7cef2a01c4be13931db0551247edf929bb` had Windows `35810545499`, profiling `35810545487`, and release-manifest `35810545457` overall success. Their Windows deterministic phases used the same broad exclusion while the containment test still carried `RuntimeSmoke` in its name, so neither older run counts as containment-self-test execution evidence.

## Current verification state

Passed and evidenced:

- reproduced the selection defect in a minimal CMake fixture;
- repaired the registration without weakening the hosted GUI-smoke exclusion;
- built `EditorContainmentTests` in hosted MSVC Debug and Release;
- executed and passed `EditorContainmentTests` in hosted Debug and Release under the real deterministic-suite command;
- passed the rest of the Windows job, profiling workflow, and release-manifest workflow associated with the repaired source head;
- preserved `Tests/EditorRuntimeSmoke.cpp` and production editor source unchanged.

Still pending:

- fresh independent review of the final repaired evidence tree;
- registered interactive-Windows Debug and Release `EditorRuntimeSmoke` execution;
- local/native screenshots and GPU/driver receipts;
- clean-machine package launch, measured comparative performance/memory, broad stress/recovery, and 24-hour soak.

`native_evidence` remains empty. Issue #7 remains open; the historical R0 runner was not invoked.

Status: **Containment CTest selection defect reproduced, repaired, and positively executed in hosted Debug and Release on the PR integration merge. Native GUI and independent acceptance remain pending. No UE5/Unity parity claim is made.**

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

Obtain fresh independent review of the repaired evidence tree. If clean, hand that exact reviewed tree to the registered Windows executor for native containment plus interactive GUI acceptance.