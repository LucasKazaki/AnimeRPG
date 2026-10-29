# Engine soak analysis: preserve source evidence and previous reports

- task_id: ENGINE-SOAK-EVIDENCE-PRESERVATION-2026-10-28
- title: Prevent soak-analysis output from replacing retained evidence
- goal: Make the existing soak analyzer create a fresh report exclusively, preserving input files and previous reports on both success and failure.
- risk_level: Low; dependency-free Python tooling only.

## Context and ownership

Repository: LucasKazaki/AnimeRPG. Current main observed through the GitHub connector on October 28, 2026: `09ab28f20a5736fba35179baa2d0d6a922c2f1d8` (PR110). The isolated worktree is `C:/Users/lucas/Documents/Codex/2026-10-28/task-5/astral-soak-reliability`, branch `engine/soak-evidence-20261028`, owned by sprint task `01a18363-b6ef-72dd-8aef-db9f33091010`.

The earlier coordinator was chat `01a14ae5-1646-7c80-b8a8-c76ba4ab5747` (Improve Astral Engine workflow), currently not loaded. Its final October 18 shutdown preserved source and delivery artifacts. Lucas explicitly requested renewed Astral progress in the October 28 sprint. This task does not restart that coordinator, Studio project, or any scheduler.

The current engine acceptance gap includes an unaccepted 24-hour soak. Its existing analyzer uses a truncating output write, allowing an accidentally reused `--json` path to replace a source receipt, telemetry, or previous analysis. Capture a regression before repairing it. Preserve all old evidence and existing worktrees.

## Allowed files

- `Scripts/analyze_package_soak_telemetry.py`
- `Scripts/test_package_soak_telemetry_analysis.py`
- `Tasks/ENGINE-SOAK-EVIDENCE-PRESERVATION-2026-10-28.md`
- `Docs/QA/ENGINE-SOAK-EVIDENCE-PRESERVATION-2026-10-28.md`
- `Docs/Reviews/ENGINE-SOAK-EVIDENCE-PRESERVATION-2026-10-28.md`

## Forbidden actions

No engine/game/native/CMake/workflow edits; no compiler, GUI, GPU, continuous-soak, model, or campaign execution; no dependency installation; no scheduler or shared-database changes; no publication, push, merge, deployment, cleanup of user files, or modification of other worktrees. No upgraded soak, performance, parity, or clean-machine acceptance claims.

## Allowed commands and test instructions

Use the already installed bundled Python at `C:/Users/lucas/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe`, with `-B`. Set this shell's `TEMP` and `TMP` to `C:/Users/lucas/Documents/Codex/2026-10-28/task-5/test-temp` because the managed shell's default package-temp directory rejected fixture writes. This changes only the test process environment.

From the task workspace:

```powershell
$taskPython = 'C:/Users/lucas/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$env:TEMP = $env:TMP = 'C:/Users/lucas/Documents/Codex/2026-10-28/task-5/test-temp'
& $taskPython -B astral-soak-reliability/Scripts/test_package_soak_telemetry_analysis.py
& $taskPython -B astral-soak-reliability/Scripts/test_package_continuous_soak.py
& $taskPython -B astral-soak-reliability/Scripts/test_release_manifest.py
git -C astral-soak-reliability diff --check
git -C astral-soak-reliability status --short
```

Also run the new targeted preservation cases against the original analyzer before fixing it, retaining the expected failing output. Read-only Git status, diff, history and worktree checks are allowed. No native configure/build/runtime command applies to this Python-only change; record them as not run, never as passed. The required full engine Debug/Release, 3D/2D, performance, stress and soak gates remain open.

## Acceptance criteria and expected output

1. Baseline existing analyzer tests pass with the workspace temporary directory.
2. New regressions demonstrate old source/report replacement using temporary synthetic evidence only.
3. Existing paths cannot be overwritten, including source input aliases and hard links; valid and invalid analysis retain original bytes.
4. A fresh output report still works for valid and invalid evidence; failures exit nonzero with useful diagnostics and conservative claims.
5. Exact commands, exit codes, changed-file scope, limitations and rollback instructions are retained outside source output directories. One independent reviewer checks the final diff and the actual evidence.

Stop at a deterministic unexpected failure. Do not broaden into unrelated analyzer or engine changes. Rollback: leave this unmerged branch/worktree unused; no existing source or delivery copy is modified. Do not delete evidence or revert shared history.
