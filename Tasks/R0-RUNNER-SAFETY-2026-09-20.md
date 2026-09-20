# R0 runner safety repair, 2026-09-20

## Why this packet exists

Issue #7 identified deterministic safety and provenance defects in the historical
R0 release runner. This packet repairs the runner without invoking it on Lucas's
Windows host and without changing Engine/, Game/, Tests/, CMakeLists.txt, engine
architecture, local schedulers, or paused game-content scope.

Baseline is PR #6 head `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`.
The original runner blob is `7f5c17fd6de0642b8f850583b6c0b51fb909f9b8`.
PR #8 is a separate stacked asset-loader candidate and is not a dependency for this
repair. Issue #7 remains open until native Windows verification and independent
review accept this candidate.

## Allowed paths

- `Scripts/invoke_r0_release_candidate.py`
- `Scripts/test_r0_runner_safety.py`
- `.github/workflows/windows-ci.yml`
- `Tasks/R0-RUNNER-SAFETY-2026-09-20.md`
- `Docs/QA/R0-RUNNER-SAFETY-2026-09-20.md`

No historical R0 execution, package publication, merge, release, SDK installation,
or modification of a registered local runtime is authorized by this packet.

## Primary-source research

Accessed September 20, 2026:

1. CPython 3.14.7 subprocess documentation:
   https://docs.python.org/3/library/subprocess.html
   `Popen.wait()` and `communicate()` support timeouts. A `communicate()` timeout
   does not kill the child automatically, so cleanup is the caller's responsibility.
   `CREATE_NEW_PROCESS_GROUP` is a supported Windows creation flag; POSIX
   `start_new_session=True` creates a new session for owned process-group cleanup.
   The same timeout/process-group APIs used here are present in Python 3.10, which
   is compatible with syntax already used by the repository script.
2. CPython 3.14.7 pathlib documentation:
   https://docs.python.org/3/library/pathlib.html
   `Path.resolve(strict=False)` makes a path absolute, removes `..`, and resolves
   symlinks as far as possible. The runner combines this with platform path-case
   normalization and `commonpath` checks before filesystem mutation.
3. Microsoft `taskkill` documentation, current page accessed September 20, 2026,
   page last updated November 1, 2024:
   https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/taskkill
   `/T` ends the selected process and child processes started by it. The repair
   uses the exact PID of a process created by the runner and never enumerates or
   kills unrelated processes.

These sources define process/path safety behavior only. They are not evidence that
native R0, packaging, or engine acceptance has run.

## Acceptance contract

Before any build/release/evidence directory can be archived or created, source,
worktree, build, release, and evidence roots must be pairwise disjoint after
symlink/junction-aware resolution. Equal, ancestor, and descendant overlaps fail.

The admitted revision is the exact resolved `base-ref` SHA. A pre-existing recovery
branch must point to that exact SHA. A pre-existing worktree must be the configured
branch, clean, and at that exact SHA. Stale clean branches fail before build or
report writes. R0 is one-shot: tracked evidence in a prior worktree is preserved and
blocks in-place reruns rather than being discarded.

Every Git/probe command and every long-running build/test/package command receives
a deadline. Timeout or interruption terminates only the runner-owned process tree,
retains partial command logs, records timeout/interruption state, and leaves an
honest blocked checkpoint. On Windows, owned descendant cleanup uses `taskkill`
with the exact child PID and `/T /F`; the portable test fixture uses a POSIX process
group so the same lifecycle contract can be exercised in the sandbox.

Generated reports use the actual run start time. Historical August filenames and
deadline are retained only as lineage and are explicitly labeled historical.
Command evidence records the admitted SHA, base ref, roots, deadlines, tool-version
commands, and exact command records. The package README explicitly leaves
clean-machine MSVC runtime/dependency acceptance pending.

## Test plan

The portable safety suite must compile/import the real runner and cover every pair
of configured roots for equality and nesting, symlink aliasing when available,
archive preservation, required-base rejection, stale and exact recovery branches,
dirty one-shot rerun rejection, short-command timeout, long-command timeout with
partial logs and descendant cleanup, interrupted-run descendant cleanup, and fresh
timestamps. Fixtures use only temporary repositories/directories.

Hosted Windows CI must run the safety suite before the normal MSVC build. It may
exercise Windows `taskkill`, path semantics, Git worktrees, and interruption logic,
but still does not execute the R0 release runner or GUI RuntimeSmoke tests.

## Stop conditions and rollback

Stop on any safety-test failure, CI failure, unexpected repository path change, or
revision/provenance ambiguity. Do not weaken a failing test. Rollback is limited to
reverting this candidate branch. Existing historical evidence and worktrees must
not be deleted or rewritten.
