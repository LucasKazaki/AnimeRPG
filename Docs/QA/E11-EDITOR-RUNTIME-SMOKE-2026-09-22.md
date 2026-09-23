# E11 Editor Runtime Smoke evidence, 2026-09-23

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. This pass repairs a deterministic-test registration defect in the existing editor runtime-smoke packet. It does not authorize scene mutation/serialization, gizmos, Play-in-Editor, asset import, graphics/API changes, dependencies, game content, scheduler operations, deployment, release, merge, rebase, or R0 execution.

Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed: `977c5359491b24f729db6886af9b0fef35cea872`.
Repair parent: `9aed26fb23d9f88d4ed7c587d191445a3e628016`.
Runtime-smoke source candidate remains `81e7052f47cab06060ea69c9f9d4f25ec42145e4`.
`Tests/EditorRuntimeSmoke.cpp` remains blob `117c101acc9d65e297c3e0f948a6c3724ff2416d`.
Repaired `CMakeLists.txt` blob: `3f1f769a8920446907728ddb39e18bcfdc440360`.
Production editor source remains unchanged.

## Independent-review finding reproduced

Fresh Codex review of exact head `9aed26fb23d9f88d4ed7c587d191445a3e628016`, submitted `2026-09-23T03:29:14Z`, reported one P2 verification-evidence defect. Both hosted deterministic test phases run:

```text
ctest ... -E "RuntimeSmoke" --no-tests=error
```

The deterministic containment target/test was named `EditorRuntimeSmokeContainmentTests`. Astral's `astral_add_test(target)` uses `add_test(NAME "${target}" ...)`, so its CTest name also contained `RuntimeSmoke`. CTest's `-E` exclusion regular expression therefore removed this test from both hosted deterministic suites.

This means the previous Windows workflows were green without executing the containment self-test. The earlier durable claims that the containment test had passed in hosted Debug/Release are withdrawn. This correction does not invalidate unrelated tests or other successful workflow steps.

## Implementation

The only implementation change in this pass is in `CMakeLists.txt`:

```text
EditorRuntimeSmokeContainmentTests -> EditorContainmentTests
```

The rename covers the executable target, include directories, compile definitions, link libraries, and `astral_add_test` registration. `EditorRuntimeSmoke` is intentionally unchanged so the existing hosted `-E "RuntimeSmoke"` boundary still excludes the interactive desktop smoke. The generated supervisor source and actual containment logic are unchanged.

No workflow file was edited because `.github/workflows/windows-ci.yml` is outside this packet's allowed paths and the problem can be repaired inside the owned CMake registration.

## Primary research, accessed 2026-09-23 UTC

- CMake 4.4/latest `ctest_test`: https://cmake.org/cmake/help/latest/command/ctest_test.html
  - `EXCLUDE <exclude-regex>` filters tests by test name.
- CMake 4.4/latest `ctest(1)`: https://cmake.org/cmake/help/latest/manual/ctest.1.html
  - the hosted command-line `-E` performs the same exclusion by regular expression.
- CMake 4.4/latest `add_test`: https://cmake.org/cmake/help/latest/command/add_test.html
  - `NAME` is the CTest test identity; Astral maps the target name directly to it.

These are build/test API references only. No proprietary engine source was copied and no new dependency was imported.

## Sandbox selection fixture

Coordinator environment: CMake `3.31.6`.
Disposable fixture script SHA-256: `65864f2af1d2d447e007baa2a660204e173a452818af0ae04c11fa62da05c6a3`.
Bad-registration CMake fixture SHA-256: `9a8585228569da4f711fa8561178cd213e5d283de6f2e3fb8d33db2e2b57fe1b`.
Good-registration CMake fixture SHA-256: `59e724e748dbde444d7dfddc141f59ff359586960d0148f0b471782f581e73c9`.

Observed results:

```text
# Before-equivalent naming
EditorRuntimeSmoke
EditorRuntimeSmokeContainmentTests
ctest -N -E RuntimeSmoke
=> Total Tests: 0

# Repaired-equivalent naming
EditorRuntimeSmoke
EditorContainmentTests
ctest -N -E RuntimeSmoke
=> Test #1: EditorContainmentTests
=> Total Tests: 1

ctest --output-on-failure -E RuntimeSmoke --no-tests=error
=> EditorContainmentTests ... Passed
=> 100% tests passed
```

This fixture proves the CTest selection behavior only. It does not exercise Windows Job Objects, the production generated supervisor, the GUI smoke, GPU behavior, or Lucas's workstation.

## Historical hosted evidence correction

Pre-repair exact head `9aed26fb23d9f88d4ed7c587d191445a3e628016`:

- Windows build and deterministic tests `35814098903`: `completed/success`;
- profiling capture portability `35814099016`: `completed/success`;
- release-manifest integrity `35814098909`: `completed/success`.

Earlier reviewed tree `727dcf7cef2a01c4be13931db0551247edf929bb`:

- Windows `35810545499` / job `107020923389`: `completed/success`;
- profiling `35810545487`: `completed/success`;
- release-manifest `35810545457`: `completed/success`.

For both Windows runs, the deterministic Debug and Release test commands used `-E "RuntimeSmoke"`. Because the old containment test name matched that expression, these workflow successes **must not** be cited as execution evidence for containment. They remain valid evidence for the other steps/tests that actually ran.

## Current verification state

Attempted and passed this pass:

- reproduced the old-name exclusion with a minimal CMake fixture;
- demonstrated that `EditorContainmentTests` survives the exact `-E RuntimeSmoke` filter;
- executed the selected repaired fixture successfully;
- preserved the interactive `EditorRuntimeSmoke` name and therefore its hosted exclusion boundary;
- kept the implementation inside the existing authorized CMake path.

Pending after this repository write:

- exact-head hosted Windows Debug and Release build/test workflows for the renamed target;
- positive evidence that `EditorContainmentTests` is selected/executed in those deterministic phases;
- fresh independent review of the repaired exact tree;
- registered interactive-Windows Debug and Release `EditorRuntimeSmoke` execution.

`native_evidence` remains empty. No Windows GUI screenshots, real GPU behavior, clean-machine packaging, comparative performance/memory evidence, broad stress/recovery, or 24-hour soak was executed by this coordinator. Issue #7 remains open; the historical R0 runner was not invoked.

Status: **CTest registration defect reproduced and repaired by renaming the deterministic containment test so hosted `-E RuntimeSmoke` no longer filters it. Historical hosted containment-pass claims are corrected. New exact-head hosted execution, independent review, and native GUI acceptance remain pending. No UE5/Unity parity claim is made.**

## Registered native handoff

After exact-head hosted verification and fresh independent review are clean, run the repaired head on one owned interactive Windows desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, normal and narrow-window screenshots for the GUI checks, and process inspection showing zero owned contained processes after failure or interruption.

## Single next action

Wait only for the newly written exact head's hosted workflows, verify the renamed containment test actually participates in Debug and Release, and request independent re-review. If those gates are clean, hand off the exact head for native containment plus GUI acceptance.