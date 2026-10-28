# Headless clean-build readiness, October 28, 2026

## Result and source identity

The lightweight readiness gate passes. A reproducible portable-CI trigger gap is repaired locally: changes to the runtime/test CMake helpers, assertion guard, and AstralProject tool now match both PR and main-push filters. Each existing portable matrix job runs the new dependency-free source contract before configure. Engine code, compiler flags, sanitizer jobs, CTest registration/timeouts and existing native acceptance gates are unchanged.

Repository LucasKazaki/AnimeRPG; local base `a618dbb15a794257559c79bde7688858da787bbf`, preserving the accepted local evidence-preservation milestone based on public main `09ab28f20a5736fba35179baa2d0d6a922c2f1d8`. The local fix was not published. Worktree `C:/Users/lucas/Documents/Codex/2026-10-28/task-5/astral-headless-readiness`, branch `engine/headless-readiness-20261028`, owner sprint task `01a18363-b6ef-72dd-8aef-db9f33091010`.

Independent implementation review accepted the workflow/test repair with no blocking findings; see `Docs/Reviews/ENGINE-HEADLESS-READINESS-2026-10-28.md`, SHA-256 `e2813af21130edc221b31d2b12ddbc9bb36f616e34408922a70f8664a7560d64`. It matched workflow hash `1cdc8c578e5fda4fbbf96cb4f5af48fcdc241200a1ffe9bcbae464e96007e187` and contract-script hash `66ff0cdf17ed24a48c62ea5ff0aa7571eebd79dd2a541f710d55d13240675f1f`, inspected the retained evidence, and documented matcher/parser limits. No push, PR, merge, publication or deployment occurred. The following native build proposal is **pending parent resource coordination**, not executed, denied or uncertain.

## Fresh baseline / tool inventory

- No Astral/compiler/test processes observed before this pass. Free memory 12,421,636 KiB of 32,673,900 KiB (about 11.8 GiB free).
- CMake and CTest 3.31.6-msvc6 exist and their `--version` commands exit 0 at `C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/`. Neither is on this shell's PATH. Use exact installed paths; no installation/PATH/system changes were made.
- MSVC tool directory `C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.44.35207` exists. This does not establish that native configure or compilation will pass.
- CMake `-P` source inventory verifies 67 grouped source files (50 Runtime, 5 Showcase, 9 Gameplay, 1 Sample, 2 Editor), plus all 17 declared suite sources. No compiler, project configure or runtime is invoked by that inventory.
- All 52 Python scripts parse as syntax without execution or bytecode generation.

The later 17:00:20 UTC read-only inventory again observed no relevant processes and 12,270,444 KiB free RAM. It retained hashes of installed CMake, CTest and MSVC executables and the existing Windows SDK include directory `10.0.26100.0`, in external `current-resource-tool-inventory.json`. No compiler or native test was executed for that inventory.

## Reproduction and exact lightweight evidence

Evidence root `C:/Users/lucas/Documents/Codex/2026-10-28/task-5/evidence/headless-readiness`. Installed Python is `C:/Users/lucas/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe`; all commands use `-B`. Temporary fixtures use process-local TEMP/TMP `C:/Users/lucas/Documents/Codex/2026-10-28/task-5/test-temp` and `PYTHONDONTWRITEBYTECODE=1`.

`old-portable-ci-contract.log` retains the first expected red run plus an initial inventory-test count error (six unique direct inputs, since the two player targets share one source, rather than seven). After correcting that test expectation, `old-portable-ci-contract-corrected-inventory.log` records ten missing-trigger subcases across the two events and the absent contract step: 11 failures across four test methods, with source inventory passing. This is a source/workflow failure, not a failed native compile or engine test.

From the owned source root, `run-lightweight-checks.py` ran each command sequentially with a 30-second child timeout, stopping on failure. Exact argv, cwd, exit codes and durations are in `lightweight-checks.json`; raw stdout/stderr are in the corresponding `.log` files.

| Command after the installed Python executable | Exit | Result |
|---|---:|---|
| `-B Scripts/test_portable_build_contract.py` | 0 | 4 contract tests pass; both event filters cover current direct CMake/tool dependencies, assertion guard and contract script; job executes contract before configure; declared sources/suites exist. |
| `-B Scripts/test_test_safety.py TestSafety.test_all_native_test_targets_use_the_safety_helper` | 0 | 1 existing static safety test passes. The two compiler-fixture methods were deliberately not run. |
| `-B Scripts/verify_milestone1.py` | 0 | Static M1 source/marker presence gate passes. |
| `-B Scripts/verify_milestone2.py` | 0 | Static M2 source/fixture/marker gate passes. |
| `-B Scripts/verify_milestone3.py` | 0 | Static M3 source/marker gate passes, from documented source-root cwd. |
| `-B Scripts/test_windows_prerequisite_plan.py` | 0 | 8 synthetic prerequisite-policy tests pass; no installer, download or engine launch. |

Additional command: exact installed `cmake.exe -DTASK_SOURCE=C:/Users/lucas/Documents/Codex/2026-10-28/task-5/astral-headless-readiness -P C:/Users/lucas/Documents/Codex/2026-10-28/task-5/evidence/headless-readiness/inventory-source.cmake` exits 0. This is script-mode inventory only. `git diff --check` exits 0.

Current pass counts: **13 unit/contract tests; 3 static milestone gates; 1 CMake source inventory; 52 Python syntax parses**. Zero unexpected failures in the final lightweight gate. The old-filter failures and corrected test setup are retained, not relabeled as passes. No hosted workflow was triggered.

## Exact proposed native headless gate — UNRUN

Request a coordinated CPU slot before any command below. This is a clean portable-project MSVC check, not a full root Windows/Win32/runtime acceptance run. It will not execute the capture/player tools or create a window.

```powershell
$taskRoot = 'C:/Users/lucas/Documents/Codex/2026-10-28/task-5'
$source = "$taskRoot/astral-headless-readiness/Tests/EngineRuntime"
$cmake = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
$ctest = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/ctest.exe'
$debugBuild = "$taskRoot/builds/astral-portable-debug-20261028"
$releaseBuild = "$taskRoot/builds/astral-portable-release-20261028"
$env:TEMP = $env:TMP = "$taskRoot/test-temp"
$env:CMAKE_BUILD_PARALLEL_LEVEL = '1'

# Phase 1 only after its CPU slot is granted; check every exit before proceeding.
& $cmake -S $source -B $debugBuild -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON -DASTRAL_SANITIZE=OFF -DASTRAL_TSAN=OFF
& $cmake --build $debugBuild --config Debug --parallel 1 -- /nr:false
& $ctest --test-dir $debugBuild -C Debug --show-only=json-v1
& $ctest --test-dir $debugBuild -C Debug --output-on-failure --no-tests=error --parallel 1 --timeout 300

# Phase 2 only after Phase 1 receipts are accepted and its CPU slot is granted.
& $cmake -S $source -B $releaseBuild -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON -DASTRAL_SANITIZE=OFF -DASTRAL_TSAN=OFF
& $cmake --build $releaseBuild --config Release --parallel 1 -- /nr:false
& $ctest --test-dir $releaseBuild -C Release --show-only=json-v1
& $ctest --test-dir $releaseBuild -C Release --output-on-failure --no-tests=error --parallel 1 --timeout 300
```

The command block is a proposal, not a ready-made unattended runner: each child must be launched under the guards below, with its output retained and exit checked before the next command. Stop after the first unexpected failure; never execute Phase 2 just because the proposal lists it.

Resource/duration estimate (not measured yet): one compiler worker; roughly 2–4 GiB incremental peak RAM and 1–3 GiB combined disk; no GPU. Configure 30–90 seconds, fresh single-worker build roughly 5–15 minutes/configuration, headless suites roughly 1–5 minutes/configuration. Tests run one suite at a time, although concurrency tests can create their own internal worker threads. Grant Debug first; reassess real receipts before Release.

Proposed mandatory guards:

- Parent coordinates EV/chess/Studio owners and grants a CPU-only slot. No unrelated active compiler/CTest is preempted or stopped.
- Freeze the actual local source commit and clean tracked/untracked status after this task's local checkpoint. Preserve `a618dbb` and require build roots to be absent; do not reuse/delete an existing build or evidence directory.
- Re-probe at least 8 GiB free RAM before each configure/build/test phase; monitor every five seconds and abort the owned tree if free RAM drops below 6 GiB. No guard/security settings are changed.
- Bound configure to 180 seconds, build to 1,200 seconds, and each aggregate CTest invocation to 900 seconds; all child processes belong to an owned process tree/job and must be reaped. Never kill by global executable name.
- Inspect `--show-only=json-v1` before running tests: require exactly the 17 declared `Engine*Tests`, with executable paths under the owned build root and no RuntimeSmoke/native-window test.
- Retain exact source/tree identity, tool versions, cache/configure options, stdout/stderr, exit codes, elapsed time, memory snapshots, test inventory and final owned-process state. Stop on the first failure, memory guard, timeout, source mutation or missing test; do not weaken assertions or exclusions.
- Each separately granted configuration phase has an outer 40-minute cap (180-second configure, 1,200-second build, 30-second inventory and 900-second test caps total 38.5 minutes). Two phases require separate grants and have an 80-minute combined ceiling, not an unattended reservation. Each must finish before the October 29 16:00 UTC sprint deadline; do not start without its full phase budget remaining.

**Unrun:** 2 native configurations, 2 native builds, 34 engine-suite invocations (17/configuration), the 2 compiler-fixture safety methods, all native interactive/GPU/runtime demos and long soaks. No new Debug/Release/engine/runtime acceptance is claimed. Genuine 2D runtime, audio, full GPU/native controls/parity, clean-machine compatibility and the immutable 24-hour soak remain outstanding.

## Files, demonstration and rollback

Only the workflow, new `Scripts/test_portable_build_contract.py`, task packet, this QA report and independent review artifact are admitted. The contract understands this workflow's existing literal block-style path filters, segment `*` and trailing `/**`; unsupported structural YAML changes fail loudly rather than silently dropping checks.

Safe demonstration now: run the new four-test offline contract from the source root. The earlier retained CPU demo README is at `C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/outputs/astral-noon-integrated-20261018/README.md`; it was not launched in this pass.

Rollback: leave this unmerged branch/worktree unused. Previous `a618dbb` and the original dirty copies/assets/packages remain unchanged. Do not delete evidence, revert shared history, merge or publish.
