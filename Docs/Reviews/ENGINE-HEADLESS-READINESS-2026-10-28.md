# Independent review: headless clean-build readiness

Reviewer: `/root/soak_evidence_review`, October 28, 2026.

Verdict: **Accept this bounded local workflow/readiness change. No blocking implementation or evidence finding.** Native execution remains pending separate parent resource coordination. This is not engine build, runtime, hosted-CI, merge or release acceptance.

## Reviewed identity and source

- Owned worktree: `C:/Users/lucas/Documents/Codex/2026-10-28/task-5/astral-headless-readiness`.
- Base independently read through Git: `a618dbb15a794257559c79bde7688858da787bbf`, the local evidence-preservation checkpoint based on public-main observation `09ab28f20a5736fba35179baa2d0d6a922c2f1d8`. No publication of the local checkpoint is inferred.
- Read `AGENTS.md`, active `Tasks/ENGINE-HEADLESS-READINESS-2026-10-28.md`, changed workflow, new Python contract, portable CMake project, runtime/test helper modules, source catalog, QA record and retained evidence.
- Independently read Git status and `git diff --no-ext-diff --check`; approved read-only command exited 0. Status showed the workflow modification and new contract/task/QA files only. This review is the fifth allowed path. No implementation edit was made by the reviewer.

Exact reviewed SHA-256 identities:

| File | SHA-256 |
|---|---|
| `.github/workflows/engine-runtime.yml` | `1cdc8c578e5fda4fbbf96cb4f5af48fcdc241200a1ffe9bcbae464e96007e187` |
| `Scripts/test_portable_build_contract.py` | `66ff0cdf17ed24a48c62ea5ff0aa7571eebd79dd2a541f710d55d13240675f1f` |

## Correctness and coverage

The actual portable CMake project has six unique literal direct CMake/tool inputs: three CMake modules and three C++ tool sources (the two player targets share one source). The new test extracts those current direct inputs and verifies both event filters. Broadening the module filter to `cmake/**` covers the previously omitted `AstralRuntime.cmake` and `AstralTestSafety.cmake`; adding `Tools/AstralProject.cpp` covers the omitted tool. The assertion source is genuinely consumed by `AstralTestSafety.cmake`, and its explicit filter plus the new script's own filter complete the stated increment.

The new offline command appears after checkout and before configure in the existing `portable` matrix job, without a conditional. It therefore runs in each of the five existing matrix expansions. Matrix compiler/sanitizer settings, build commands, native target registration, timeouts, runtime checks and clean-tree step are unchanged by the patch.

For the current filter forms, slash-segment matching avoids Python whole-string `fnmatch` incorrectly allowing a single star to cross directories. Trailing `/**` uses the required directory prefix. Literal block-list extraction correctly finds the current PR and push path blocks. The source catalog and all 17 declared suite files exist according to retained inventory, and current Engine/Showcase/Sample/suite patterns cover their inspected locations.

The contract is intentionally a check of this workflow's current layout, not a general YAML, CMake or GitHub-glob interpreter. Future ordered negative filters, anchors/aliases, different job structure, variable-derived dependency forms or richer glob syntax require updating the contract. In particular, the current matcher does not implement ordered exclusions; this review does not claim it automatically rejects every unsupported future glob. No such unsupported filter exists in the reviewed workflow.

## Evidence inspected

Evidence root: `C:/Users/lucas/Documents/Codex/2026-10-28/task-5/evidence/headless-readiness`.

- `old-portable-ci-contract-corrected-inventory.log`: ten omitted-trigger subcase failures across PR/push and one absent-step failure. Four test methods ran; the catalog test passed. The earlier inventory-count setup error is retained separately and correctly disclosed in QA.
- `portable-ci-contract.log`: four passing contracts.
- `static-test-safety.log`: one passing static registration check; no compiler fixture ran.
- `windows-prerequisite-policy.log`: eight passing synthetic policy tests.
- Three `static-milestone*.log` files and `lightweight-checks.json`: all three scripts exited 0 from the documented source-root cwd.
- `cmake-source-inventory.log`, `inventory-source.cmake` and the exact recorded `cmake -P` argv: 67 grouped source entries (50/5/9/1/2) and 17 suite sources exist. Script mode only includes and checks the source catalog; it does not call `project`, configure a compiler, build or run engine code.
- The retained lightweight runner uses 30-second child bounds, stops on failure, records argv/cwd/exit/duration and parses Python with `ast.parse`. Its summary supports 52 Python syntax parses without script execution or bytecode generation.

Supported completed counts: **13 unit/contract tests, three static milestone gates, one CMake source inventory and 52 Python syntax parses.** I inspected these retained executions and did not duplicate the test suites or invoke native commands.

## Guarded native proposal

The QA proposal correctly separates a future MSVC portable-project check from the root Win32/runtime acceptance gate. It uses fresh external build roots, one compiler worker, serial headless CTest, explicit installed tool paths, no GPU or window launch, and a pre-run inventory requiring exactly the 17 owned engine-suite executables. Memory guards (8 GiB admission, 6 GiB abort), owned-process-tree reaping, source/cache identity, per-child timeouts and deadline stops remain requirements for the eventual runner; the displayed command list is expressly not an unattended runner.

The coordinator clarified the timing budget during review: grant Debug and Release separately, at most 40 minutes per configuration and 80 minutes combined, with a fresh Release grant only after Debug receipts are accepted. Configure 180 seconds + build 1,200 + inventory 30 + CTest 900 totals 38.5 minutes per configuration, fitting that cap. No command is admitted merely by appearing in the proposal. Parent coordination must still establish resource availability alongside EV/chess/Studio work and sufficient deadline budget.

Unrun counts remain two native configurations, two builds, 34 engine-suite invocations, two compiler-fixture safety methods, all GUI/GPU/demo runtime checks and long soaks. No hosted workflow has been triggered by this local change. Genuine 2D, audio, full GPU/native/parity, performance, clean-machine and immutable-soak acceptance remain open.

Rollback: leave this unmerged branch unused and preserve evidence, prior local checkpoint, original dirty copies and frozen delivery assets. This review authorizes no push, merge, publication, deployment, scheduler or security change.
