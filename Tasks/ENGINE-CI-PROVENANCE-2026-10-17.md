# Windows CI head and merge provenance repair — October 17, 2026

## Ownership and authority

Owner: `/root/candidate_review`, sole implementation owner for
`engine/ci-head-merge-20261017` in
`C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/astral-ci`.
Deliberate base: PR #52 actual head
`1b7a80715b33080372405ff357b1dafb3979d178`, not current main.
Lucas's October 17 engine-only assignment authorizes this bounded repair and
supersedes older per-change/local-only approval wording within that scope.
Review, process-safety, native, provenance and merge gates remain required.

Only these source-controlled paths are allowed:

- `.github/workflows/windows-ci.yml`
- This task packet
- `Docs/QA/ENGINE-CI-PROVENANCE-2026-10-17.md`

Do not edit the original PR #52 branch, runner, tests, policies or protections;
do not invoke historical R0, install dependencies, build, run GUI tests, push or
merge. The coordinator owns those separately admitted operations. Issue #7
remains open and R0 reuse is not accepted by this packet.

## Reproduced problem and research

At the exact base, Windows CI has one full Debug/Release job and explicitly
checks out only the PR head. Its existing
`R0RunnerSafetyTests.test_windows_ci_verifies_pr_head_and_merge_ref` fails
locally: one test, one failure, exit 1, before any build. External evidence:
`../ci-evidence/targeted-baseline.log`. The failure agrees with actual workflow
behavior, not merely a missing string. No runner entry point was invoked.

Primary GitHub sources retrieved October 17, 2026:

- [PR workflow events](https://docs.github.com/en/actions/reference/workflows-and-actions/events-that-trigger-workflows#pull_request):
  open mergeable PRs supply the synthetic merge SHA through `GITHUB_SHA` and
  the merge ref through `GITHUB_REF`; the PR payload separately identifies head.
- [Checkout v6 README](https://raw.githubusercontent.com/actions/checkout/v6/README.md):
  explicit `github.event.pull_request.head.sha` selects the exact head;
  `fetch-depth` controls available history. Preserve existing checkout v6.
- [Expressions and fromJSON](https://docs.github.com/en/actions/reference/workflows-and-actions/expressions#fromjson):
  JSON arrays can provide evaluated matrix values.
- [Matrix jobs](https://docs.github.com/en/actions/how-tos/write-workflows/choose-what-workflows-do/run-job-variations):
  matrix values generate separate jobs, and `fail-fast: false` avoids cancelling
  the other revision merely because one leg fails.
- [Safety issue #7](https://github.com/LucasKazaki/AnimeRPG/issues/7):
  hosted source checks do not replace registered-local process cleanup,
  interruption, package or broader engine acceptance evidence.

Firecrawl credits are exhausted; the built-in web tool retrieved primary GitHub
documentation. The v6 tree page returned a retrieval error; retrieving its raw
official README succeeded and supplied the action-specific contract.

## Implementation contract

1. Add a real matrix to the existing complete Windows job: `head` and `merge`
   for `pull_request`, one `commit` leg for push/manual events. Disable fail-fast.
2. Checkout the event head SHA only for the PR head leg; otherwise checkout
   immutable event `github.sha`. Fetch two levels so merge parents are present.
3. A mandatory pre-build step verifies actual `HEAD` against the selected event
   SHA. For the PR merge leg require exactly two parents, first equal to event
   base SHA and second equal to event head SHA. Fail closed on missing or wrong
   identities; record revision, event and checkout SHA, event base/head and parents
   in job output and its step summary.
4. Leave every existing downstream safety/Job Object probe, configure,
   Debug/Release build/test, dependency/prerequisite/bootstrap and clean-tree gate
   in the same full job. Do not substitute markers, skip failed legs, or weaken
   runner assertions. Preserve contents-read permissions and event types.

## Local validation and remaining acceptance

Use existing Python:
`C:/Users/lucas/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe`.
From the owned worktree run the targeted source-contract regression:

```text
python Scripts/test_r0_runner_safety.py R0RunnerSafetyTests.test_windows_ci_verifies_pr_head_and_merge_ref
git diff --check
git diff --name-only 1b7a80715b33080372405ff357b1dafb3979d178
```

Also inspect the YAML structure, matrix/checkout expressions and PowerShell syntax
without building or running R0. Retain exact commands, outputs, SHA and limits in
the QA record. The source-contract test is necessary but cannot establish actual
GitHub matrix expansion, checkout or downstream execution. Full safety-suite
process fixtures require the coordinator's execution slot and explicit clearance.

Before adoption: independent review of the final diff/commit; a coordinator-owned
draft PR against the old PR #52 branch; refreshed event head/base/merge identities;
hosted logs proving two distinct complete Windows jobs and both exact checkouts,
ordered merge parents and downstream gates. A non-PR run must show one leg.
Retain failures and provenance rather than inferring success from job names.
Current-main integration is a later decision; neither a passing workflow nor this
repair accepts the entire old PR stack, historical R0, native package, GPU or soak.

## Bounded stop

Commit only the allowed three paths, then hand the SHA and evidence to the
coordinator. Stop at a deterministic blocker and retain its log. No source-wide
rebase, original-owner branch change, R0 execution or unrelated engine work.
