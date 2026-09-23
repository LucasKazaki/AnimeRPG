# E11 Editor Runtime Smoke task, 2026-09-23

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. This packet may harden the native Windows smoke, its recovery supervisor, deterministic containment tests, and evidence. It must not add scene mutation/serialization, gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, rebase, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed for the verified PR integration run: `977c5359491b24f729db6886af9b0fef35cea872`.
Pre-repair parent: `9aed26fb23d9f88d4ed7c587d191445a3e628016`.
Containment-registration implementation commit: `78c4d82314739f045dd39f198601221fe2492221`.
Runtime-smoke source candidate remains `81e7052f47cab06060ea69c9f9d4f25ec42145e4`.
`Tests/EditorRuntimeSmoke.cpp` remains blob `117c101acc9d65e297c3e0f948a6c3724ff2416d`.
Repaired `CMakeLists.txt` blob: `3f1f769a8920446907728ddb39e18bcfdc440360`.
Production editor source is unchanged by this pass.

Allowed paths only: `CMakeLists.txt`, `Tests/EditorRuntimeSmoke.cpp`, this task, `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`, and `Docs/Research/ENGINE-CAPABILITIES.json`. One active writer only. Do not rebase, merge, force-push, or absorb unrelated work.

## Selected reproducible verification defect

Fresh independent Codex review of pre-repair head `9aed26fb23d9f88d4ed7c587d191445a3e628016`, submitted `2026-09-23T03:29:14Z`, found a P2 false-evidence path in containment verification. Hosted Debug and Release deterministic tests invoke:

```text
ctest ... -E "RuntimeSmoke" --no-tests=error
```

Before the repair, the deterministic process-tree test was registered by `astral_add_test` as `EditorRuntimeSmokeContainmentTests`. `astral_add_test` gives the CTest test the target name. CTest `-E <regex>` excludes tests whose names match the expression, so the broad `RuntimeSmoke` filter excluded both the interactive `EditorRuntimeSmoke` and the supposedly deterministic `EditorRuntimeSmokeContainmentTests`.

Therefore older green Windows workflows remain valid for the tests they actually executed, but they are **not evidence that the containment self-test ran**. Earlier durable hosted-containment pass claims are withdrawn.

## Bounded implementation

`CMakeLists.txt` renames only the deterministic containment executable/test:

- old: `EditorRuntimeSmokeContainmentTests`
- new: `EditorContainmentTests`

The generated supervisor implementation, containment logic, `Tests/EditorRuntimeSmoke.cpp`, and interactive GUI test name `EditorRuntimeSmoke` are unchanged. The hosted `-E "RuntimeSmoke"` filter therefore continues excluding the interactive desktop smoke while allowing `EditorContainmentTests` into the ordinary deterministic Debug and Release suites.

No workflow file, production editor code, graphics API, dependency, game content, or scheduler configuration changed.

## Primary-source basis, rechecked 2026-09-23 UTC

- CMake 4.4/latest `ctest_test`: https://cmake.org/cmake/help/latest/command/ctest_test.html
  - applicability: `EXCLUDE <exclude-regex>` excludes tests whose names match the regular expression.
- CMake 4.4/latest `ctest(1)`: https://cmake.org/cmake/help/latest/manual/ctest.1.html
  - applicability: command-line `-E` is the test-name exclusion filter used by hosted CI.
- CMake 4.4/latest `add_test`: https://cmake.org/cmake/help/latest/command/add_test.html
  - applicability: `add_test(NAME <name> ...)` establishes the CTest test name; Astral's `astral_add_test(target)` supplies the target name as that `NAME`.

These are build/test API references only. No proprietary source was copied and no dependency was added.

## Reproduction and coordinator fixture

A disposable CMake selection fixture ran with CMake `3.31.6` in the coordinator sandbox. Fixture script SHA-256:

`65864f2af1d2d447e007baa2a660204e173a452818af0ae04c11fa62da05c6a3`

It reproduced the old selection as zero tests and the repaired registration as one selected/passing `EditorContainmentTests` under the exact `-E RuntimeSmoke` form. Bad/good fixture `CMakeLists.txt` SHA-256 values: `9a8585228569da4f711fa8561178cd213e5d283de6f2e3fb8d33db2e2b57fe1b` and `59e724e748dbde444d7dfddc141f59ff359586960d0148f0b471782f581e73c9`.

This is CTest-selection evidence only, not Windows Job Object or GUI evidence.

## Hosted verification of the repair

GitHub Actions Windows run `35818360831`, job `107044690265`, completed successfully at `2026-09-23T04:28:58Z`. It was associated with PR source head `78c4d82314739f045dd39f198601221fe2492221`, but the checkout correctly used GitHub's synthetic PR merge commit `c49c88022656c81b8ef71e23d84459d474f42a49`, merging that source head into current base `977c5359491b24f729db6886af9b0fef35cea872`. Do not describe this as a raw-head checkout.

Runner/toolchain evidence from the retained log:

- runner image: Windows Server 2022 / `windows-2022`, OS `10.0.20348`, image `20260913.307.1`;
- CMake configure selected Windows SDK `10.0.26100.0`;
- MSVC compiler identification `19.44.35228.0`;
- Visual C++ Build Tools `14.44.35207`.

Positive containment execution evidence from the actual CTest output:

```text
Debug:   9/17 Test #9: EditorContainmentTests ... Passed 5.12 sec
Release: 9/17 Test #9: EditorContainmentTests ... Passed 5.10 sec
```

Both deterministic suites reported `100% tests passed, 0 tests failed out of 17`. Build Debug, Build Release, dependency/prerequisite checks, static milestone verifiers, and clean tracked tree also passed. The interactive `EditorRuntimeSmoke` did not run in this hosted suite, as intended.

Associated source-head workflows also completed successfully:

- profiling capture portability `35818360718`;
- release-manifest integrity `35818360753`.

This establishes hosted deterministic containment execution for the repaired registration on the PR merge result. It does **not** establish interactive native GUI acceptance, actual GPU behavior, clean-machine packaging, performance parity, stress/recovery acceptance, or soak.

## Historical hosted evidence corrected

Pre-repair source head `9aed26fb23d9f88d4ed7c587d191445a3e628016` had Windows `35814098903`, profiling `35814099016`, and release-manifest `35814098909` overall success. Reviewed predecessor `727dcf7cef2a01c4be13931db0551247edf929bb` had Windows `35810545499`, profiling `35810545487`, and release-manifest `35810545457` overall success. In both older Windows runs the containment test's old name matched `-E "RuntimeSmoke"`, so neither may be cited as containment-self-test execution evidence.

## Retained E11 acceptance surface

The native editor smoke still requires one stable visible/enabled process-owned top-level editor; original 12-child HWND/class continuity; semantic Static and toolbar Button binding; exact ordered Outliner/assets fixtures; `LBS_NOTIFY`; exact Inspector fixtures; selection synchronization; truthful disabled pending tools; bounded normal/narrow resizes; bounded cross-process messages; clean process-owned shutdown; worker-local cleanup; and supervisor-level zero-process-tree cleanup verification.

`native_evidence` remains empty. Issue #7 remains open, so the historical R0 runner is blocked and was not invoked. E11 remains partial and is not UE5/Unity parity.

## Registered native handoff

After fresh independent review of the repaired current tree is clean, the registered Windows executor should run both the deterministic containment test and interactive GUI smoke in each configuration on one owned interactive desktop:

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

Rollback only the containment target rename and its evidence corrections if the renamed test fails to build/register or changes the intended selection boundary. Stop before production-runtime change, workflow edit outside packet authority, rebase, merge, R0 execution, scheduler operation, dependency addition, graphics/API change, or game-content work. Never weaken the exclusion boundary, containment assertions, or native acceptance checks merely to make CI green.

## Single next useful action

Obtain fresh independent review of the repaired current tree. If clean, hand the exact reviewed tree to the registered Windows executor for native Debug/Release containment plus interactive `EditorRuntimeSmoke` acceptance.