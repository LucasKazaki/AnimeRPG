# Independent review: soak evidence preservation

Reviewer: `/root/soak_evidence_review`, October 28, 2026.

Verdict: **Accept this bounded local tooling change. No blocking implementation or QA finding.** This review does not authorize merge, publication, deployment, a native runtime replay, or broader engine acceptance.

## Reviewed identity and scope

- Baseline independently read from the owned worktree: `09ab28f20a5736fba35179baa2d0d6a922c2f1d8`.
- Worktree: `C:/Users/lucas/Documents/Codex/2026-10-28/task-5/astral-soak-reliability`.
- Active packet: `Tasks/ENGINE-SOAK-EVIDENCE-PRESERVATION-2026-10-28.md`.
- Read the worktree's `AGENTS.md`, complete changed CLI output path, added tests, frozen source patch, packet and QA record.
- Read-only Git status showed two modified implementation/test paths and the new task/QA records. `git diff --no-ext-diff --check` passed. This review artifact is the fifth allowed path. No engine, CMake, workflow, native runtime, asset, scheduler or shared-database change was found in the inspected scope.

Exact reviewed SHA-256 identities:

| Artifact | SHA-256 |
|---|---|
| `Scripts/analyze_package_soak_telemetry.py` | `51730bf0bf0d47b849e190faad92764f132cc71d7fa97d630542c90475378f53` |
| `Scripts/test_package_soak_telemetry_analysis.py` | `bbba5b368fe9b7b080ffb1b161e993b1966761ab766907f2c28c0b07fa79b1ce` |
| External `evidence/review-source.patch` | `8ef75f8c67e59556b932ec6b94033ddc448892ac73b20089ebe9c44043b2d64b` |
| Exact baseline analyzer retained under `evidence/old-evidence-probe/` | `9bc4459857f64fea1ed02c28e91d9ca8b0dcd13a089203dd84dae7cdd41c8411` |

## Correctness assessment

Replacing the truncating text write with `Path.open("x")` makes creation exclusive at the filesystem operation. Existing files are refused without a separate existence-check/write race. This covers ordinary existing paths, source input aliases and hard links; the Windows hard-link test ran successfully in the retained approved test run. The Python standard-library API introduces no platform or dependency change relative to the project's Python syntax requirements.

Both success and analysis-failure branches already catch output `OSError`, produce a useful stderr diagnostic and return nonzero. Fresh success and failure reports remain supported. The CLI help now states that output paths must be new. The patch leaves evidence validation, statistical calculations and engine-acceptance flags unchanged.

The four new test methods exercise both receipt and telemetry aliases with valid and invalid analysis, earlier report preservation with valid and invalid analysis, a real hard-link alias, and fresh nested failure-output creation with all acceptance claims false. The original success-output and validation tests remain present.

## Independently inspected evidence

Evidence root: `C:/Users/lucas/Documents/Codex/2026-10-28/task-5/evidence`.

- `fixed-soak-analysis-unsandboxed.log`: 15 tests, OK, no skips.
- `continuous-soak-contract-tests.log`: 11 tests, OK, including the owned short-lived Python process sampler; this is not an engine soak.
- `release-manifest-contract-tests.log`: 7 tests, OK, no skips.
- Total supported tooling test count: 33 passing tests. These are the implementer's retained executions; I inspected them and did not duplicate the suites.
- `old-evidence-probe/reproduction.json` and actual synthetic `.before`/current files: independently compared SHA-256 pairs and confirmed the receipt overwrite, invalid telemetry overwrite, hard-link receipt/report overwrite and invalid previous-report overwrite. The unrelated fixture bytes remained unchanged in those cases.
- The old-code failure log and final sandbox hard-link setup error are explicitly retained in QA, rather than being relabeled as passes. Final approved execution resolves the fixture limitation.
- Read-only worktree status/baseline/diff checks succeeded in an approved elevated review command. A normal-sandbox Git attempt first rejected administrator-owned worktree metadata as dubious ownership. No global `safe.directory`, ownership, security or Git configuration change was made.

## Practical limits and residual risk

Repeated analysis must choose a new report path; automatic replacement is intentionally no longer supported. Callers must honor the nonzero exit and stderr diagnostic if report creation fails. A storage failure after a fresh file is created may leave a partial new report; the patch promises preservation of existing files, not crash-atomic report publication.

This review is limited to the two frozen source files, task/QA content and the evidence listed above. It does not independently re-establish current public-remote identity, the older coordinator's shutdown, physical-memory inventory or historical native package acceptance. Those remain attributed coordinator observations in QA. A later retained-demo-package inventory addendum, if appended by the coordinator, is outside this code review's evidence set.

No configure, C++ Debug/Release build, CTest native suite, GUI, GPU test or 24-hour soak was run by this reviewer. Whole-engine, genuine 2D, audio, full native/GPU/parity, performance, clean-machine and immutable-soak acceptance remain open.

Rollback remains the packet's non-destructive option: leave the unmerged worktree/branch unused and preserve evidence. No existing source or delivery copy needs to be reverted for this task.
