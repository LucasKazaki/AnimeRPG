# AnimeRPG / Astral Engine Agent Rules

This repository is a custom C++17 Windows engine and action-RPG prototype.
Unreal Engine and Unity are not production dependencies. Preserve Astral Engine.
The September 17 director scope is engine-first and local-only; game-content work
remains paused until engine acceptance. The supernatural National Mall game
remains the retained product vision, not the currently admitted feature queue.

## Source-of-truth order
1. `GAME_DEVELOPMENT_CONTROL.md`: current operator scope and acceptance boundaries.
2. The active bounded task packet in `Tasks/`, within that scope.
3. `Docs/Project-Status.md`: dated repository reconciliation, not a live heartbeat.
4. Fresh local evidence and dated loop-status records, with their observation limits.
5. `Docs/Decision-Log.md`, `Docs/Planning/MILESTONES.md`, README, and code contracts.

Historical deadlines, old next-task paragraphs, and heartbeat snapshots do not
override current scope. A task cannot unpause content or change architecture.
When sources conflict, report the exact conflict. Do not infer an engine migration,
silently renumber milestones, or broaden the product scope.

## Required behavior
- Read the active task packet before editing.
- Use one dedicated git worktree per implementation task. Never allow two coding agents to share a worktree.
- Modify only the packet's allowed files.
- Preserve the custom C++17, CMake, Win32, and GDI conventions already present unless Lucas explicitly approves an architecture change.
- Run the packet's exact configure, build, test, scope, and runtime commands before claiming completion.
- Record exact commands, exit results, files changed, test evidence, risks, and follow-ups.
- Produce an independent review and QA artifact before merge approval.
- Register native tests through `astral_add_test` so Release assertions stay active.
- Use one owned interactive desktop for runtime smokes. RUN_SERIAL covers one CTest process, not concurrent invocations.

## Efficiency rules
- Use one coordinator, one implementation agent, and one independent reviewer/QA pass for a bounded packet. Do not create duplicate agents for the same role.
- Give each worker only the task packet, relevant contracts, and the smallest necessary code context.
- Do not repeat an identical failed command more than once without changing a material condition.
- Stop at the first deterministic blocker, preserve logs, and report the smallest next action.
- Use local utility models for bounded extraction, log classification, and checklist work. Reserve stronger models for planning, implementation, and review.
- Do not start a new feature while the current recovery or QA gate is unresolved.

## Forbidden by default
No deletion, dependency or plugin installation, external API calls, networking changes, asset-wide refactors, credential access, public deployment, history rewriting, source-root CMake output, or unrelated edits without Lucas's explicit approval.

## Review and merge
All implementation occurs in a dedicated worktree and branch. Lucas approves merges and architecture changes. Codex or another independent reviewer reviews diffs without editing unless explicitly assigned a separate fix packet.
A connector audit, source-only check, or hosted build cannot stand in for a local
process inspection, native interactive test, package launch, or the required soak.

## September 22, 2026: separate hourly GAME worker authorization

Lucas explicitly requested an hourly AnimeRPG game worker, separate from the hourly Astral Engine worker, researching and implementing five reference-game improvements plus one new community-requested improvement per pass. He additionally instructed this cycle to push and merge into `main` in this repository.

For that worker only, [the operating contract](Docs/Agents/ANIMERPG-HOURLY.md) records standing push/merge approval and dependency-ready parallel game work. This supersedes older blanket game-content-pause and per-merge-human-approval wording within that scope. It does not waive independent implementation review, applicable tests, native acceptance, dependency/architecture approvals, or repository protections. An unresolved gate blocks dependent work, not unrelated independently verifiable game work. Documentation-only operating records are not product implementation and require content/structural validation and applicable checks, not fabricated gameplay acceptance.

Do not broaden this authorization to the engine worker, its unreviewed PR stack, unrelated repositories, local execution, deployment, or release. Keep the original rules above for all other work. Use the [game backlog](Docs/Agents/animerpg-hourly/BACKLOG.json) and a scoped task packet; verify live ownership before changing gameplay files that happen to reside under `Engine/Scene`.

## September 22, 2026: separate hourly ART worker authorization

Lucas explicitly requested an hourly AnimeRPG art worker, separate from both the
GAME and Astral Engine workers, for bounded art direction, creative-tool planning,
original source/default asset production, and creative verification. For that
worker only, this supersedes the older blanket art/assets pause for work that is
independent of missing engine capabilities.

Use `GAME_DEVELOPMENT_CONTROL.md`, a scoped art task, and the durable records under
`Docs/Agents/art-hourly/`. The art worker may create and verify original source
assets and starter/default content, but must keep unsupported outputs explicitly
source-only. It does not gain renderer/editor/gameplay ownership, automatic merge
authority, architecture approval, dependency-install authority, spending authority,
local Company Runtime control, deployment, release, or permission to clear native,
performance, provenance, independent-review, or engine-acceptance gates.
