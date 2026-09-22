# AnimeRPG / Astral Engine Agent Rules

This repository is a custom C++17 Windows action RPG prototype. Unreal Engine and Unity are not production dependencies. Work incrementally toward a playable supernatural Washington, DC National Mall vertical slice while preserving the existing Astral Engine architecture.

## Source-of-truth order
1. The active bounded task packet in `Tasks/`.
2. The latest dated loop-status file in `Docs/Agents/`.
3. `Docs/Decision-Log.md` and `Docs/Planning/MILESTONES.md`.
4. `README.md` and existing implementation contracts.

When sources conflict, stop and report the exact conflict. Do not infer an engine migration, silently renumber milestones, or broaden the product scope.

## Required behavior
- Read the active task packet before editing.
- Use one dedicated git worktree per implementation task. Never allow two coding agents to share a worktree.
- Modify only the packet's allowed files.
- Preserve the custom C++17, CMake, Win32, and GDI conventions already present unless Lucas explicitly approves an architecture change.
- Run the packet's exact configure, build, test, scope, and runtime commands before claiming completion.
- Record exact commands, exit results, files changed, test evidence, risks, and follow-ups.
- Produce an independent review and QA artifact before merge approval.

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

## September 22, 2026: separate hourly GAME worker authorization

Lucas explicitly requested an hourly AnimeRPG game worker, separate from the hourly Astral Engine worker, researching and implementing five reference-game improvements plus one new community-requested improvement per pass. He additionally instructed this cycle to push and merge into `main` in this repository.

For that worker only, [the operating contract](Docs/Agents/ANIMERPG-HOURLY.md) records standing push/merge approval and dependency-ready parallel game work. This supersedes older blanket game-content-pause and per-merge-human-approval wording within that scope. It does not waive independent implementation review, applicable tests, native acceptance, dependency/architecture approvals, or repository protections. An unresolved gate blocks dependent work, not unrelated independently verifiable game work. Documentation-only operating records are not product implementation and require content/structural validation and applicable checks, not fabricated gameplay acceptance.

Do not broaden this authorization to the engine worker, its unreviewed PR stack, unrelated repositories, local execution, deployment, or release. Keep the original rules above for all other work. Use the [game backlog](Docs/Agents/animerpg-hourly/BACKLOG.json) and a scoped task packet; verify live ownership before changing gameplay files that happen to reside under `Engine/Scene`.
