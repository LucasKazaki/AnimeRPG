# Bounded Development Loop

1. Producer creates a task packet in `Tasks/` with allowed files, forbidden actions, commands, acceptance criteria, risk, and test instructions.
2. Coordinator creates one dedicated worktree under `C:/AI/worktrees/AnimeRPG/<task-id>` from the current branch. No two coding agents share a worktree.
3. One implementation agent edits only its packet's allowed files and returns a diff plus exact commands.
4. Build agent runs configure/build/tests in that worktree. If a prerequisite is missing, record the exact command and blocker; do not fake runtime evidence.
5. Independent review agent reads the diff and packet without editing. Review must identify scope violations, test gaps, and merge recommendation.
6. QA agent runs acceptance tests and records pass/fail evidence, including manual playtest steps when applicable.
7. Coordinator writes a decision-log entry and queues the next packet only after the previous gate is resolved or explicitly marked blocked.
8. Lucas approves merges to protected branches and architecture changes.

## Stop conditions
- Stop when the packet is complete, blocked by an external prerequisite, or the time/budget limit is reached.
- Stop immediately for destructive changes, secrets, license acceptance, public/network exposure, or out-of-scope edits.
- Emergency stop: do not launch persistent agents; terminate their tracked process/worktree task and record the reason in the log.

## Artifacts
Task packets: `Tasks/*.md`; reviews: `Docs/Reviews/`; QA: `Docs/QA/`; decisions: `Docs/Decision-Log.md`; inventory: `Docs/Inventory/`; raw agent transcripts remain in Hermes delegation cache and are referenced by delegation ID in the log.
