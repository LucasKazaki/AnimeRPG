# Execute current-main Windows CI on exact PR head and merge

Owner `/root`; independent reviewer `/root/containment_implementation`.
Dedicated worktree `work/astral-current-ci`, branch
`engine/current-ci-provenance-20261017`, deliberate current-main base
`126e6cfafedc47d1a2266f1adf27d6b23293f329`. Lucas's October 17 engine-only
implementation/push/merge authority applies, with independent review and
applicable gates retained. No historical recovery invocation is authorized by
this packet.

## Problem and source evidence

Current main's Windows workflow has one conventional pull-request checkout,
so its successful build verifies a synthetic merge without independently
executing the submitted head. Reviewed PR83 repaired this behavior against
historical PR52 at1b7a807; that branch contains unrelated recovery changes and
is not an appropriate wholesale current-main adoption. Its actual three hosted
head/merge/manual jobs passed (39055999596 and39055998947), with independent
source/provenance review. Current main still has the old single checkout.

## Scope and approach

Own only `.github/workflows/windows-ci.yml`, this task and
`Docs/QA/ENGINE-CURRENT-CI-PROVENANCE-2026-10-17.md`.
Port only the reviewed6a1ddc workflow prefix: event-specific head/merge versus
single-commit matrix, fail-fast false, exact SHA checkout with depth2, and
PowerShell SHA/ordered-parent verification plus job receipts. Preserve current
main byte-for-byte from `Set external build root` through workflow end. Do not
copy historical PR52's downstream Job Object/recovery probes, change build/test
steps or assertions, or edit any runtime/editor/game/policy file.

The matrix builds/tests both actual revisions; a string-presence marker is
insufficient acceptance. Treat event metadata as environment values, require
40-hex identities before comparisons, and fail when a merge's two ordered
parents disagree with event base/head. Push/manual runs have one commit leg.

## Acceptance and exact checks

Before edits retain original prefix and compare it to reviewed PR83. After
porting, parse the actual PowerShell step with the installed PowerShell AST;
verify its prefix equals reviewed6a1ddc and current-main downstream bytes remain
identical. Obtain independent actual diff review, then publish a scoped draft.
Run an actual PR head/merge pair and a workflow_dispatch branch job; inspect
every job/step exit and download logs with actual checkout/provenance receipts.
Do not infer head execution from a merge-only CI badge. No local compiler/native
slot is needed for AST/diff review; repository/native tests remain serialized
and unchanged downstream hosted checks must execute successfully.

Commands, with session-absolute output files in `work/current-ci-evidence`:

```powershell
git diff --check
git diff --name-only
git diff 126e6cf -- .github/workflows/windows-ci.yml
# Extract run text between the named provenance step and Set external build root.
$parseErrors = $null; $tokens = $null
[System.Management.Automation.Language.Parser]::ParseInput($provenanceScript, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw 'PowerShell provenance parse failed' }
# After independently reviewed commit and push:
gh pr create --repo LucasKazaki/AnimeRPG --base main --head engine/current-ci-provenance-20261017 --draft --body-file <session-work-body>
gh workflow run windows-ci.yml --repo LucasKazaki/AnimeRPG --ref engine/current-ci-provenance-20261017
gh run view <run-id> --repo LucasKazaki/AnimeRPG --json headSha,jobs,conclusion
gh api --allow-escape-sequences repos/LucasKazaki/AnimeRPG/actions/jobs/<job-id>/logs > <owned-log>
```

Bound unchanged hosted execution through the existing30-minute job timeout.
Do not invoke actual historical R0 or claim its present admission; its fixture
checks remain ordinary unchanged workflow steps. After successful hosted gates
and final independent binding, coordinator may ready/merge this reviewed packet
using expected-head protection, re-fetch and verify actual integrated main.

Risk: concurrent main movement can regenerate synthetic merge identities. Keep
SHA-named receipts and refresh actual event parents instead of force updating old
snapshot refs. Two PR jobs use more hosted work but do not compete with local
compiler/GPU ownership. Rollback is an ordinary reviewed revert, no history edit.
