# Soak evidence preservation: local QA, October 28, 2026

## Result

The analyzer now creates `--json` reports exclusively. An existing output path produces a nonzero exit and a write diagnostic while leaving its bytes intact. This protects receipts, telemetry, hard-link aliases and earlier analysis reports on both successful and failed analysis. New success and failure reports continue to work, with the existing conservative acceptance claims.

Implementation and local tests are complete. Independent review accepted the exact source hashes with no blocking findings; see `Docs/Reviews/ENGINE-SOAK-EVIDENCE-PRESERVATION-2026-10-28.md` (SHA-256 `ae58de3af821021f0ea909c56da7199087dfcd6cf2b849e332d5b676e49ecd2a`). This is a local-only change, with no push, PR, merge, publication or deployment.

## Identity and baseline

- Repository: `https://github.com/LucasKazaki/AnimeRPG.git`, custom native C++17 Astral Engine. `AnimeRPG-UE5` is a separate sibling and is not the target.
- Current public main observed using GitHub's commit read on October 28: `09ab28f20a5736fba35179baa2d0d6a922c2f1d8`, PR110. Fetched that exact object without moving any existing local branch.
- Worktree: `C:/Users/lucas/Documents/Codex/2026-10-28/task-5/astral-soak-reliability`; branch `engine/soak-evidence-20261028`; baseline was clean.
- Owner: sprint task `01a18363-b6ef-72dd-8aef-db9f33091010`. Prior coordinator `01a14ae5-1646-7c80-b8a8-c76ba4ab5747`, Improve Astral Engine workflow, was not loaded; its last October 18 turn records shutdown. Lucas's October 28 sprint explicitly requests Astral progress. No existing scheduler/runtime is restarted here.
- Original `C:/AI/projects/AnimeRPG` remains at `771b61a`, with untracked `Docs/Research/`, `GAME_DEVELOPMENT_CONTROL.md`, `GOAL_WORK.json` preserved. Its stale local `main` must not be mistaken for current public main.
- Fresh pretest process check found no Astral/compiler/test processes. Available physical memory was 12,579,544 KiB of 32,673,900 KiB. Work stayed bounded and CPU-only.

## Exact commands and retained evidence

Evidence root: `C:/Users/lucas/Documents/Codex/2026-10-28/task-5/evidence`.

Installed Python: `C:/Users/lucas/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe`.

From the task workspace, set only this shell's `TEMP` and `TMP` to `C:/Users/lucas/Documents/Codex/2026-10-28/task-5/test-temp`; later related suites also set `PYTHONDONTWRITEBYTECODE=1`.

| Command (after the Python executable) | Exit | Observed result / raw log |
|---|---:|---|
| `-B astral-soak-reliability/Scripts/test_package_soak_telemetry_analysis.py` on original code | 1 | Initial sandbox default-temp fixture access failure; retained `baseline-soak-tests.log`. No product assertion result inferred. |
| Same baseline test, with workspace TEMP/TMP | 0 | 11/11 tests; `baseline-soak-workspace-temp.log`. |
| `-B astral-soak-reliability/Scripts/test_package_soak_telemetry_analysis.py SoakTelemetryAnalysisTests.test_cli_preserves_inputs_when_output_reuses_an_input_path SoakTelemetryAnalysisTests.test_cli_preserves_previous_report_on_success_and_failure SoakTelemetryAnalysisTests.test_cli_preserves_input_when_output_is_a_hard_link` against old analyzer | 1 | Six preservation subcase failures and one sandbox hard-link setup error; `old-analyzer-preservation-regressions.log`. |
| `-B evidence/reproduce-baseline.py` (approved unsandboxed bounded synthetic probe) | 0 | Four byte-replacement reproductions on the exact baseline blob; `old-evidence-probe.log`, `old-evidence-probe/reproduction.json`, all before/after fixtures and subprocess stdout/stderr retained. |
| `-B astral-soak-reliability/Scripts/test_package_soak_telemetry_analysis.py` after fix, sandboxed | 1 | 14 pass; hard-link setup still denied by sandbox; `fixed-soak-analysis-tests.log`. |
| Same fixed suite, approved unsandboxed fixture creation | 0 | 15/15 pass; `fixed-soak-analysis-unsandboxed.log`. No skips. |
| `-B astral-soak-reliability/Scripts/test_package_continuous_soak.py` | 0 | 11/11 pass; `continuous-soak-contract-tests.log`. Synthetic process fixtures plus an owned short-lived Python process sampler; no engine soak. |
| `-B astral-soak-reliability/Scripts/test_release_manifest.py` | 0 | 7/7 pass; `release-manifest-contract-tests.log`. No skips. |
| `git -C astral-soak-reliability diff --check` | 0 | No whitespace errors. |

The final three suites total 33 passing tests. Sandbox fixture failures were resolved by approved bounded test execution, without changing system security settings. The baseline probe retains SHA-256 `9bc4459857f64fea1ed02c28e91d9ca8b0dcd13a089203dd84dae7cdd41c8411` for the original analyzer.

Independent review compared the retained original and changed bytes for all four old-code cases, checked the raw logs and allowed scope, and matched both final script hashes: analyzer `51730bf0bf0d47b849e190faad92764f132cc71d7fa97d630542c90475378f53`; tests `bbba5b368fe9b7b080ffb1b161e993b1966761ab766907f2c28c0b07fa79b1ce`. No source edit or test rerun was made by the reviewer.

An additional read-only inventory check verified the existing demonstration package, without launching it. Command: `-B astral-soak-reliability/Scripts/release_manifest.py verify <delivery>/Package/release-manifest.json <delivery>/Package --expected-commit 4f3c6cef564e94980b333daf35d9870202be8dd5 --expected-executable-sha256 af55fa366d164985746b75114fa6fc01a193c243efdc7901245c3bd26309aa6e --executable-path AstralEnginePlayerWin32.exe --json evidence/retained-demo-package-verification.json`. Here `<delivery>` is `C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/outputs/astral-noon-integrated-20261018`. Exit 0: all 12 payload files verified; adding the manifest gives 13 package files. Original compile identity is retained. Raw JSON and log are external evidence; package bytes were not modified.

## Scope and demonstration

Changed implementation: `Scripts/analyze_package_soak_telemetry.py` (`_write_json` uses exclusive `x` creation; `--json` help describes the contract). Added four regression methods in `Scripts/test_package_soak_telemetry_analysis.py`; other changes are this QA record, the bounded task packet and the independent review artifact.

Demonstrate locally with the 15-test analyzer command above. Or use real retained evidence with the existing CLI and a **fresh** output path:

```powershell
& $taskPython -B astral-soak-reliability/Scripts/analyze_package_soak_telemetry.py `
  <soak-summary.json> <continuous-soak-telemetry.jsonl> `
  --expected-commit <exact-source-sha> --expected-executable-sha256 <exact-exe-sha256> `
  --json <new-analysis-path.json>
```

Reusing the output path now fails while preserving the previous file. Do not use real evidence to demonstrate the old truncation bug; its retained probe uses synthetic copies only.

The existing local visual demonstration and controls are documented in `<delivery>/README.md` and `WALKTHROUGH.md`. Its prepared CPU example is `<delivery>/Package/AstralEnginePlayerWin32.exe` with no arguments. Start it only under an owned desktop slot; this task performed the checksum check only.

## Native build status and remaining engine roadmap

No CMake configure, Debug/Release engine build, CTest native suite, GUI demonstration, hardware check or 24-hour soak was run for this Python-only fix. The current main handoff retains earlier bounded Debug/Release CPU, GPU and package workflow results with their original compile identities; those are historical evidence, not new October 28 passes.

The engine roadmap still lacks whole-engine acceptance, full GPU/parity/native controls, genuine 2D acceptance, audio acceptance, distinct F5 reset proof, clean-machine compatibility and the required immutable 24-hour soak. The separate Win32 input increment remains excluded. This task adds no gameplay feature and makes no parity, performance, leak-free or soak-acceptance claim.

Rollback: leave this unmerged branch/worktree unused. No existing source, frozen delivery, shared database, credentials, scheduler or user asset was edited. Preserve all retained evidence. The reviewed change is ready for a local checkpoint; merging and publishing remain outside this sprint task's authorization.
