# E11 Editor Runtime Smoke task, 2026-09-23

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. This packet may harden the native Windows smoke, its recovery supervisor, deterministic containment tests, and evidence. It must not add scene mutation/serialization, gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, rebase, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed this pass: `977c5359491b24f729db6886af9b0fef35cea872`.
Repair parent: `9aed26fb23d9f88d4ed7c587d191445a3e628016`.
Runtime-smoke source candidate remains `81e7052f47cab06060ea69c9f9d4f25ec42145e4`.
`Tests/EditorRuntimeSmoke.cpp` remains blob `117c101acc9d65e297c3e0f948a6c3724ff2416d`.
Current CMake repair blob: `3f1f769a8920446907728ddb39e18bcfdc440360`.
Production editor source is unchanged by this pass.

Allowed paths only: `CMakeLists.txt`, `Tests/EditorRuntimeSmoke.cpp`, this task, `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`, and `Docs/Research/ENGINE-CAPABILITIES.json`. One active writer only. Do not rebase, merge, force-push, or absorb unrelated work.

## Selected reproducible verification defect

Fresh independent Codex review of exact head `9aed26fb23d9f88d4ed7c587d191445a3e628016`, submitted `2026-09-23T03:29:14Z`, found a P2 false-evidence path in the containment verification registration.

The Windows hosted workflow invokes deterministic Debug and Release tests with:

```text
ctest ... -E "RuntimeSmoke" --no-tests=error
```

Before this repair, the deterministic process-tree test was registered by `astral_add_test` as `EditorRuntimeSmokeContainmentTests`. `astral_add_test` gives the CTest test the target name. CTest `-E <regex>` excludes tests whose names match the expression, so the broad `RuntimeSmoke` filter excluded both the interactive `EditorRuntimeSmoke` and the supposedly deterministic `EditorRuntimeSmokeContainmentTests`.

Therefore the older green Windows workflows remain valid for the tests they actually executed, but they are **not evidence that the containment self-test ran**. Any durable record claiming hosted Debug/Release containment execution on those older heads is corrected by this packet.

## Bounded implementation

`CMakeLists.txt` now renames only the deterministic containment executable/test:

- old: `EditorRuntimeSmokeContainmentTests`
- new: `EditorContainmentTests`

The generated supervisor implementation, containment test logic, `Tests/EditorRuntimeSmoke.cpp`, and interactive GUI test name `EditorRuntimeSmoke` are unchanged. The hosted `-E "RuntimeSmoke"` filter therefore continues excluding the interactive desktop smoke while allowing `EditorContainmentTests` to participate in the ordinary deterministic Debug and Release suites.

No workflow file, production editor code, graphics API, dependency, game content, or scheduler configuration changed.

## Primary-source basis, rechecked 2026-09-23 UTC

- CMake 4.4/latest `ctest_test`: https://cmake.org/cmake/help/latest/command/ctest_test.html
  - applicability: `EXCLUDE <exclude-regex>` excludes tests whose names match the regular expression.
- CMake 4.4/latest `ctest(1)`: https://cmake.org/cmake/help/latest/manual/ctest.1.html
  - applicability: command-line `-E` is the test-name exclusion filter used by the hosted workflow; `--no-tests=error` fails only when the resulting selection is empty, not when one intended test was accidentally filtered out among many others.
- CMake 4.4/latest `add_test`: https://cmake.org/cmake/help/latest/command/add_test.html
  - applicability: `add_test(NAME <name> ...)` establishes the CTest test name; Astral's `astral_add_test(target)` supplies the target name as that `NAME`.

These are build/test API references only. No proprietary source was copied and no dependency was added.

## Reproduction and coordinator fixture

A disposable CMake selection fixture was run with CMake `3.31.6` in the coordinator sandbox. Fixture script SHA-256:

`65864f2af1d2d447e007baa2a660204e173a452818af0ae04c11fa62da05c6a3`

It created two minimal test registrations and exercised the exact exclusion form:

```text
BAD:  EditorRuntimeSmoke + EditorRuntimeSmokeContainmentTests
ctest -N -E RuntimeSmoke
=> Total Tests: 0

GOOD: EditorRuntimeSmoke + EditorContainmentTests
ctest -N -E RuntimeSmoke
=> EditorContainmentTests selected; Total Tests: 1

ctest --output-on-failure -E RuntimeSmoke --no-tests=error
=> EditorContainmentTests passed
```

The bad and good fixture `CMakeLists.txt` SHA-256 values were respectively `9a8585228569da4f711fa8561178cd213e5d283de6f2e3fb8d33db2e2b57fe1b` and `59e724e748dbde444d7dfddc141f59ff359586960d0148f0b471782f581e73c9`.

This is CTest-selection evidence only. It is not Windows Job Object execution or GUI evidence.

## Historical hosted evidence corrected

Exact pre-repair head `9aed26fb23d9f88d4ed7c587d191445a3e628016` completed these workflows successfully:

- Windows build and deterministic tests `35814098903`;
- profiling capture portability `35814099016`;
- release-manifest integrity `35814098909`.

The Windows workflow's overall success remains true, but both deterministic CTest phases used `-E "RuntimeSmoke"`, so `EditorRuntimeSmokeContainmentTests` was excluded. The earlier exact-head workflows for `727dcf7cef2a01c4be13931db0551247edf929bb` had the same selection behavior. Neither set may be cited as execution evidence for the containment self-test.

The first acceptable hosted containment evidence after this repair must come from a new exact-head Windows run where the renamed `EditorContainmentTests` is part of both deterministic Debug and Release selections. Do not infer that result from this source change alone.

## Retained E11 acceptance surface

The native editor smoke still requires one stable visible/enabled process-owned top-level editor; original 12-child HWND/class continuity; semantic Static and toolbar Button binding; exact five ordered Outliner rows and four ordered Assets rows; `LBS_NOTIFY`; exact Scene Root/Cube Inspector fixtures; post-notification Cube synchronization; truthful disabled pending tools; bounded 800x600 and 420x260 resizes with complete-state and containment checks; bounded cross-process messages; clean process-owned shutdown; worker-local cleanup; and supervisor-level process-tree cleanup verification.

`native_evidence` remains empty. Issue #7 remains open, so the historical R0 runner is blocked and was not invoked. E11 remains partial and is not UE5/Unity parity.

## Registered native handoff

After this exact repaired tree is hosted-green and independently re-reviewed, the registered Windows executor should run both the deterministic containment test and the interactive GUI smoke in each configuration on one owned interactive desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, commands, complete stdout/stderr, exit codes, UTC timestamps, normal plus narrow-window screenshots for the GUI smoke, and process inspection proving zero owned contained processes after any failure or interruption.

## Rollback and stop conditions

Rollback only the containment target rename and its evidence corrections if the renamed test does not build/register or changes the intended selection boundary. Stop before any production-runtime change, workflow edit outside packet authority, rebase, merge, R0 execution, scheduler operation, dependency addition, graphics/API change, or game-content work. Never weaken `-E "RuntimeSmoke"`, the containment assertions, or the native acceptance checks merely to turn CI green.

## Single next useful action

Verify the new exact repair head in hosted Windows Debug/Release and confirm `EditorContainmentTests` is actually selected/executed. Then obtain fresh independent review. If both are clean, proceed to the registered native Debug/Release containment plus GUI smoke handoff.