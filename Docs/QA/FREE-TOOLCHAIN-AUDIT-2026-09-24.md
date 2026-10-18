# FT-001 author audit receipt

This is a research and documentation audit, not an implementation test report.
Baseline: `2958741188279a0b438dd489ad00cac546012c25` in LucasKazaki/AnimeRPG.
Dedicated source-slice worktree: `free_toolchain_audit_work`, branch
`research/free-toolchain-progress-20260924`. No native workstation was accessed.

## Collection

Connected GitHub reads inspected the main ref, capability register, renderer
excerpt, StaticMesh interface, editor excerpt, project-status record, creative
plan excerpt, PRs #13/#52/#71 and hosted workflow/job/log data. The main workflow
36015583984 passed; PR #52 current head 1b7a807 failed workflow36026761079.
Its failing test is test_windows_ci_verifies_pr_head_and_merge_ref. This audit
reports that result; it did not execute or repair the runner.

Primary upstream project, license and model documentation supports the tool
selection. Links are in the toolchain report. Where access failed, the report
retains the limitation rather than inventing clearance. In particular, VRoid
is optional/pending for release-use review, not a necessary free baseline tool.
Blender's official source COPYING and indexed official feature pages were used
because the main license page failed retrieval. No pricing/billing settings,
installed-software inventory or full transitive dependency lockfile was audited.

## Executed local validation

Python 3.13.5, Git 2.47.3, Linux. An attempted git ls-remote failed DNS with exit128;
the worktree contains only five newly authored report files, not a full repository
checkout. Content verification is separate from upstream engine testing.

Commands:
```
python /mnt/data/validate_free_toolchain_audit.py /mnt/data/free_toolchain_audit_work
git -C /mnt/data/free_toolchain_audit_work add Tasks Docs
git -C /mnt/data/free_toolchain_audit_work diff --cached --check
```
The validator checks the exact five-file scope, JSON types, all 18 unique IDs,
weights summing to100, score9.75 rounded to10, zero recorded native acceptance,
explicit unverified-install/license-inventory boundaries, local Markdown links
and trailing whitespace. This is a content check, not a new engine test suite.
The validator/output are retained in the conversation artifacts.

## Limits

All maturity scores and weights are author judgments, not measured completion or
remaining effort. The5-15% range is not a statistical confidence interval. The
existing E00-E17 catalogue is incomplete. No new native benchmark, soak, voice
sample, installation, generation call, runtime integration, billing lock, review
approval or merge was performed. No unchanged source or previously reported test
was rerun here. Independent content review remains pending.

The publication adds only the task, free-toolchain report, progress report,
calculation JSON and this receipt. It preserves existing files and does not
change another worker's PR. The free-only direction is recorded for adoption;
this documentation does not enforce runtime dependency or account billing policy.
