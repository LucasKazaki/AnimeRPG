# Windows CI head and merge provenance QA — October 17, 2026

Packet: [ENGINE-CI-PROVENANCE-2026-10-17](../../Tasks/ENGINE-CI-PROVENANCE-2026-10-17.md).
Owner: `/root/candidate_review`, implementation role for this packet.
Worktree: `C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/astral-ci`.
Branch: `engine/ci-head-merge-20261017`.
Base: exact old PR #52 head `1b7a80715b33080372405ff357b1dafb3979d178`.

Status: bounded workflow repair implemented; the existing targeted source test
passes, local source/syntax inspections pass, and the coordinator-cleared complete
unchanged safety fixture suite passes. Independent review,
actual hosted head/merge execution and any later
current-main adoption remain pending. No build, native UI, historical R0, package
or whole-engine acceptance is claimed. The original PR owner's branch is intact.

## Scope and behavior

The existing full `build-and-test` job now expands `revision` through `fromJSON`:
PR events produce `head` and `merge`, other existing events produce one `commit`
job. `fail-fast: false` lets the other revision finish if one fails; no leg has
`continue-on-error` or a condition skipping its existing gates.

Checkout v6 receives the PR event head SHA only on the head leg, and event
`github.sha` on merge/non-PR legs. `fetch-depth: 2` makes the checked-out merge's
parents available. The mandatory provenance step independently selects the
expected SHA from event inputs, verifies `HEAD`, and requires exactly two ordered
merge parents matching the frozen event base/head. A wrong or missing identity
fails before the unchanged downstream jobs can claim success. Each successful
check writes the event/checkout/base/head/parent identity receipt to console and
the job summary. It does not compare against a moving branch ref or live PR API.

Every byte from `Set external build root` through the final clean-tree gate is
unchanged after normal newline decoding. This preserves the real R0 safety and
Windows Job Object probes, all prerequisite/runtime/bootstrap contracts, exact
configure and Debug/Release build/test commands, static verifiers and clean-tree
check. Workflow triggers, permissions, concurrency and runtime-smoke boundary
are unchanged. Runner/test source, dependency versions and policies are unchanged.

The behavior follows current primary GitHub documentation linked in the packet:
PR `GITHUB_SHA` identifies the synthetic merge, explicit PR head checkout selects
the head, `fromJSON` supplies matrix values, and fail-fast controls matrix
cancellation. The existing marker-based source test is necessary but is not
presented as proof that Actions created or executed both jobs.

## Exact observations and local commands

External evidence directory:
`C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/ci-evidence`.
Python: `C:/Users/lucas/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe`.
Commands below ran from the owned worktree unless otherwise stated.

| Command or inspection | Result | External evidence |
|---|---|---|
| `git rev-parse HEAD` before edits | `1b7a80715b33080372405ff357b1dafb3979d178` | session transcript |
| Absolute Python, `Scripts/test_r0_runner_safety.py R0RunnerSafetyTests.test_windows_ci_verifies_pr_head_and_merge_ref`, before edits | exit 1, 1 test/1 failure: missing head/merge matrix | `targeted-baseline.log` |
| Same targeted test after repair and after final source adjustment | exit 0, 1/1 each | `targeted-repaired.log`, `targeted-final.log` |
| Absolute Python, external `inspect-workflow.py` | exit 0; approved matrix JSON and exact checkout expression; unchanged event/control prefix and downstream suffix | `source-inspection.log` |
| PowerShell parser on extracted actual provenance script | exit 0, zero AST errors; script not executed | `powershell-syntax.log`, `provenance.ps1` |
| Absolute Python, `Scripts/test_r0_runner_safety.py`, after coordinator fixture-slot clearance | exit 0, 31 tests in 18.507 s, two POSIX-only skips; 29 executed pass | `full-suite.stdout.log`, `full-suite.stderr.log`, `full-suite-receipt.json` |
| `git diff --check` | exit 0 | session transcript |
| `git diff --name-only <base>` plus untracked-file inventory | only the three allowed paths | session transcript |

The external source inspector reads the actual workflow and its base with Git,
decodes the matrix's JSON arrays, checks the checkout expression and two-level
fetch, and compares preserved prefix/suffix text. It is not a generic YAML or
GitHub expression interpreter. Its extracted script was parsed by the installed
PowerShell AST parser, not executed. PyYAML and Node yaml/js-yaml were unavailable;
no parser or dependency was installed. Actual Actions parsing and execution are
separate hosted gates. No complete R0 safety suite or new process fixtures ran
under this implementation owner's initial clearance; the later coordinator
clearance and completed suite are recorded below.

## Coordinator-cleared local fixture suite

The coordinator granted the exclusive process-fixture slot after its native
tests completed. The unchanged full safety suite ran on `DESKTOP-AKPLL81`,
Windows 11 Home 10.0.26300 64-bit, Ryzen 9 7950X3D, Python 3.12.14,
Git 2.46.0.windows.1, PowerShell 7.6.5. Wrapper start/end UTC:
`2026-10-17T17:53:59.4431193Z` / `2026-10-17T17:54:19.5312629Z`.
The suite reported 18.507 seconds; the wrapper including environment/process
observations elapsed 20.089 seconds. Exit 0; no timeout. Source was the base
plus this frozen workflow patch; subsequent edits were task/QA only.

The wrapper `ci-evidence/run-safety-suite.ps1` launches only bundled Python with
`Scripts/test_r0_runner_safety.py`, redirects full stdout/stderr separately,
and samples descendants with PID plus creation-time identity. It observed 15
owned processes and zero surviving observed owned processes after completion.
Sampling can miss processes that end between observations; the unchanged suite's
timeout/interruption and exited-parent descendant checks also passed. No
unrelated process was terminated. The fixture slot was released immediately.

Expected mocked interruption checks print a blocked marker inside the test run;
the unittest result is successful. This is a disposable safety-fixture receipt,
not execution/admission of the historical R0 recovery/build/package cycle, actual
operator Ctrl+C evidence, native API failure injection or package acceptance.

## Refreshed PR #52 identity and evidence boundary

Read-only GitHub API observation while preparing this packet:

- Original head/ref: `1b7a80715b33080372405ff357b1dafb3979d178`,
  `repair/2026-09-24-r0-capture-bounded-drain`.
- PR metadata reported base/ref: `a32a729591bdf4dddf32494d08959c3e6f90d69e`, `main`.
- PR state: open, draft; `mergeable` was `null` during recomputation.
- Reported merge SHA: `3282eff4ede164aa6349dbb02483f310092e5855`.
  Its actual Git commit parents were **older base**
  `c644eec79752b9dc101aeca6b03141cf26854c96` followed by that same PR head.

Commands: `gh api repos/LucasKazaki/AnimeRPG/pulls/52` with a field-limited `--jq`
projection, then `gh api repos/LucasKazaki/AnimeRPG/git/commits/3282eff4ede164aa6349dbb02483f310092e5855`
projecting SHA/tree/parents. Both exited 0; receipts are
`pr52-current-identities.json` and `pr52-observed-merge.json`. A separate
`gh api repos/LucasKazaki/AnimeRPG/git/ref/heads/main --jq '.object.sha'` exited 0
and resolved actual remote main to `126e6cfafedc47d1a2266f1adf27d6b23293f329`
(`verified-main-ref.log`). Thus both the PR metadata base and advertised merge
were stale relative to the verified branch ref. No competing owner or source
change is inferred from that stale metadata. That advertised merge is not
accepted as current-main integration evidence. Historical failed run
`36026761079` remains a lead, not a result for this repair.

The coordinator will open a separate bounded draft against the preserved old
PR #52 branch, without pushing this owner's commit to that branch. The new PR's
final head/base/event merge identities and hosted run/job IDs do not exist yet
at this checkpoint. The immutable event identities in the resulting workflow
receipt must be reconciled with its actual checkout and parent output.

## Required next gates

1. Substantive independent review of the final three-path diff and commit.
2. Complete unchanged safety suite: **passed locally** under the cleared slot,
   with retained stdout/stderr and owned-process observations above. Bind this
   source/workflow tree to the final commit during independent review.
3. Hosted PR execution must show two distinct full Windows jobs on exact event
   head and merge SHAs, successful identity/parent receipts and every downstream
   gate on each revision. Retain actual logs and final tested SHAs. Separately
   exercise an existing non-PR event and confirm one `commit` job.
4. Keep issue #7 and historical R0 reuse gates open. Any adoption of old PR #52
   source into current main requires its own review, conflict resolution and
   applicable native/integration evidence. This workflow-only patch does not
   establish R0 safety, package launch, GPU, performance or soak acceptance.
