# Engine headless clean-build readiness

- task_id: ENGINE-HEADLESS-READINESS-2026-10-28
- title: Establish current source/build readiness and prevent skipped portable CI
- goal: Inventory lightweight deterministic readiness, repair a reproducible CI dependency-trigger gap, and return a guarded native build proposal without starting it.
- risk_level: Low; offline source and workflow contracts only.

## Context and ownership

Repository LucasKazaki/AnimeRPG, retained local base `a618dbb15a794257559c79bde7688858da787bbf` (accepted evidence-preservation fix based on public main `09ab28f`, not published). Owner sprint task `01a18363-b6ef-72dd-8aef-db9f33091010`. Dedicated worktree `C:/Users/lucas/Documents/Codex/2026-10-28/task-5/astral-headless-readiness`, branch `engine/headless-readiness-20261028`. Preserve the prior clean worktree, old dirty canonical checkout, all packages, assets and shared runtime state.

Current process/memory probe: no Astral/compiler/test process observed, 12,421,636 KiB free physical memory of 32,673,900 KiB. Installed Visual Studio CMake/CTest 3.31.6 are present but absent from the shell PATH. MSVC tool directory 14.44.35207 exists. This is inventory, not a successful compile.

The portable project includes `cmake/AstralRuntime.cmake` and `cmake/AstralTestSafety.cmake`, builds `Tools/AstralProject.cpp`, and consumes `Tests/AssertionsEnabled.cpp`. The existing workflow path filters omit these inputs. First retain failing contract tests showing changes can skip portable CI, then repair those filters and run the contract in each portable job. Do not change engine behavior, warnings, assertions, target registration, test timeouts, or sanitizer settings.

## Allowed files

- `.github/workflows/engine-runtime.yml`
- `Scripts/test_portable_build_contract.py`
- `Tasks/ENGINE-HEADLESS-READINESS-2026-10-28.md`
- `Docs/QA/ENGINE-HEADLESS-READINESS-2026-10-28.md`
- `Docs/Reviews/ENGINE-HEADLESS-READINESS-2026-10-28.md`

## Forbidden actions

No engine/native/CMake source edits, compiler configuration or build, GPU/runtime demo/capture, long soak, model/campaign job, installation, shared database/scheduler/security change, publication/push/merge/deployment, or user-file deletion. Do not start a potentially heavy native command without first returning its exact command, expected resources/duration and guards for parent coordination.

## Allowed commands and test instructions

Use installed bundled Python `C:/Users/lucas/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe` with `-B`. Fixture tests use only task-owned `test-temp` via process-local TEMP/TMP and `PYTHONDONTWRITEBYTECODE=1`. Allowed: read-only Git/source/toolchain inventory, CMake/CTest `--version`, CMake `-P` source inventory with no project configure/compiler, Python syntax parsing and the following source contracts:

```powershell
& $taskPython -B Scripts/test_portable_build_contract.py
& $taskPython -B Scripts/test_test_safety.py TestSafety.test_all_native_test_targets_use_the_safety_helper
& $taskPython -B Scripts/verify_milestone1.py
& $taskPython -B Scripts/verify_milestone2.py
& $taskPython -B Scripts/verify_milestone3.py
& $taskPython -B Scripts/test_windows_prerequisite_plan.py
git diff --check
git status --short
```

Run static milestone scripts from the repository root as documented. Retain the expected old-filter failure before the fix. Stop on an unexpected deterministic failure and report it; do not relax checks. Source graph and syntax validation are static readiness checks, never native test passes. A new independent review checks the final diff, exact evidence and build proposal.

## Acceptance criteria / expected output

1. Preserve base commit and all pre-existing copies; record exact source identity and tools.
2. New contract derives direct portable CMake/tool dependencies and verifies their coverage under both pull_request and main-push path filters; source catalog and test-suite inputs must exist.
3. Old-filter omission is reproducible; repaired contract and existing lightweight checks pass.
4. Future CI runs the new offline contract before configure; no CI execution is claimed here.
5. Retain raw commands/logs, pass/fail/unrun counts, limitations, one independent review and a local patch/commit.
6. Return explicit proposed external build roots, single-job or bounded two-job CPU use, memory/time/deadline guards and headless-only test commands for coordination. Debug/Release/native/runtime gates stay unrun until actually executed.

Rollback: leave this unmerged task branch unused. No original branch/history, user assets or shared state is changed. No merge/push/deploy is authorized.
