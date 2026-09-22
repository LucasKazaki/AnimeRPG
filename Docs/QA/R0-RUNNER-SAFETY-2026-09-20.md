# R0 runner safety repair evidence, 2026-09-20

Status: sandbox safety suite passed. Hosted Windows CI and independent review are
pending until the GitHub candidate is published and checked. The historical R0
release runner was not executed by this pass.

## Reproduced code defects

At PR #6 head `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`, runner blob
`7f5c17fd6de0642b8f850583b6c0b51fb909f9b8` visibly had these behaviors:

- `archive_directory()` could rename user-selected output roots without a prior
  pairwise source/worktree/output overlap check.
- a reused recovery branch/worktree was checked for branch name and cleanliness,
  but not exact equality with the resolved requested revision.
- `capture()` had no timeout and `run_command()` streamed until the child exited,
  with no command deadline or owned descendant cleanup.
- generated reports hardcoded August 11, 2026 even on later runs.
- the failure handoff told operators to resume a preserved checkpoint even though
  subsequent `assert_clean()` would reject tracked evidence written by a prior run.
- packaging did not establish clean-machine MSVC runtime/dependency acceptance.

No unsafe path mutation or historical R0 invocation was used to demonstrate these
findings; they were established from the source and disposable fixtures.

## Candidate behavior

The replacement validates all five configured roots before `execute()` can mutate
outputs. Path comparison resolves symlinks/junctions as far as the host supports,
normalizes platform case rules, and rejects equality plus ancestor/descendant
relations. Archive targets are checked again before rename.

The exact resolved `base-ref` SHA becomes `admitted_revision`. Existing recovery
branches must equal it, and the linked worktree must be the expected branch, clean,
and at the same SHA. A dirty prior checkpoint is intentionally not resumed in
place. Operators must preserve/review it and use a new clean worktree/output set.

Short probes and long commands are isolated into owned process groups and have
configurable deadlines. Timeout/interruption cleanup targets the process created by
the runner and its descendants, not unrelated system processes. Long commands
stream to retained evidence logs, and command records include timeout/interruption
state. Tool-version probes are part of the command evidence.

Reports now include the actual `run_started_at`. August filenames/deadline remain
only as explicitly historical lineage. The package README no longer implies that
a clean Windows machine has been accepted; runtime dependency verification remains
an independent packaging gate.

## Sandbox execution

Environment: Linux sandbox, Python runtime supplied by this chat environment, Git
from PATH. This is not Lucas's Windows host. Commands executed from a disposable
candidate fixture:

```text
python -m py_compile Scripts/invoke_r0_release_candidate.py
python Scripts/test_r0_runner_safety.py
```

Final local result before publication: **13 tests passed, exit 0**, about 8.3 seconds.
The suite covers all 10 configured root pairs for equality and nesting, symlink
alias rejection where supported, archive preservation, missing required baseline,
stale branch rejection, exact branch/worktree acceptance, dirty one-shot rerun,
short capture timeout descendant cleanup, long command timeout with retained
partial log, interrupted command cleanup, and fresh generated timestamps.

The timeout fixtures create a descendant that would write a sentinel after two
seconds if it escaped. The parent is stopped after roughly 0.4 seconds, the suite
waits past the sentinel time, and verifies the sentinel is absent. This is bounded
process-lifecycle testing, not proof of Windows `taskkill` behavior; Windows CI is
required for that platform-specific implementation.

Candidate source SHA-256 before GitHub publication:

- `Scripts/invoke_r0_release_candidate.py`:
  `8440322230422e7f01010bd82cfc1a548e38a4a9e631c3902497ebbe5d3f714d`
- `Scripts/test_r0_runner_safety.py`:
  `c035828ae6a9f69a71ecca8728cbb2f6c39fd352837b6494aae2a2306b72f8a0`

## Required native/independent follow-up

Hosted `windows-2022` CI must pass the new safety suite and the existing full
Debug/Release deterministic gate for the exact candidate SHA. That hosted test is
useful Windows path/process evidence but still is not the registered local executor,
an interactive desktop run, package launch, or clean-machine dependency test.

After hosted success, the registered Company Runtime may admit a new local
verification task in a new clean worktree/output set. It must first run only the
safety suite and inspect exact SHA/toolchain receipts. Do not invoke the historical
R0 runner merely because this source repair exists. An independent reviewer must
accept the repair and decide whether a new R0 execution packet is appropriate.
Issue #7 should remain open until those native and independent gates are recorded.
