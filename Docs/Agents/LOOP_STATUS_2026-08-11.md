# AnimeRPG Loop Status — 2026-08-11

Assessment: **paused, recovery required**

Scope note: this assessment is based on repository-visible state. It does not claim that a local Windows process is alive. A live Agent Studio loop must prove liveness by updating the heartbeat artifact defined below.

## Current verified baseline

`main` points to `181298f69b42899db83040d2d5b6c3e67804e242`, `merge M10 landmark encounter loop`, dated 2026-07-31.

The repository currently contains a custom C++17 Win32/GDI prototype with:

- native window, game loop, timing, input, and logging;
- static scene, camera, player movement, and National Mall wireframe blockout;
- bounded combat sandbox and Shadowblade actions;
- Thought Commands;
- landmark selection and discovery;
- one Lincoln Memorial training encounter with a capped resource reward;
- focused domain tests and native runtime-smoke executables through implementation task M10.

The M10 task record states that Visual Studio 2022 x64 configure, Debug and Release builds, and 14/14 CTest runs passed on 2026-07-30. That historical evidence is useful, but it is not a fresh August 11 health check.

## Why the loop is not considered operational

- No repository commit has landed since 2026-07-31.
- Before this recovery branch, only `main` existed remotely.
- There was no open pull request, open issue, commit status, or CI workflow acting as a heartbeat.
- `AGENTS.md` incorrectly described the repository as an Unreal Engine 5 project even though the accepted architecture and implementation are custom C++.
- `Docs/Blockers/BLOCKER-0001-native-toolchain.md` still described the toolchain as blocking even though later milestones built and ran.
- `Docs/Decision-Log.md` stops at M7, while implementation tasks M8, M9, and M10 are merged.
- `Docs/Planning/MILESTONES.md` still says M8 is next and its original M9/M10 product definitions do not match the later implementation-task labels. This numbering drift must be reconciled before selecting another feature.
- There are no standalone QA reports for implementation tasks M8, M9, and M10.

The code should not be rolled back solely because the audit trail is incomplete. The correct response is a bounded recovery and fresh verification pass.

## Delivery target for this week

By **Friday, August 14, 2026**, the game loop should deliver a release-candidate package of the current M10 playable prototype, not an unverified feature expansion. The package must include:

- the freshly built `AstralGame.exe` and required runtime assets;
- fresh Debug and Release build logs;
- complete local CTest evidence, including native runtime smokes;
- QA reports for implementation tasks M8, M9, and M10 or one clearly indexed recovery QA report that covers them;
- a concise controls and known-limitations README;
- a reconciled milestone map and one bounded next-feature packet;
- a commit, branch, or pull request that makes the result externally visible.

A new gameplay feature may begin only after the release-candidate gate passes and the milestone numbering conflict is resolved.

## Recovery topology

Use the smallest useful team:

1. **Coordinator:** owns the packet, worktree, heartbeat, stop conditions, and final decision.
2. **Build/release worker:** runs the exact Windows configure, build, CTest, packaging, and artifact checks. It does not redesign the game.
3. **Independent reviewer/QA:** checks scope, logs, package contents, and claims without editing.

Do not run multiple implementation agents in parallel during recovery. When a deterministic test failure appears, stop the release pass and create one narrow fix packet for that failure.

## Required heartbeat

The local coordinator must maintain `Docs/Agents/LOOP_HEARTBEAT.json` in its active branch or worktree with these fields:

```json
{
  "loop": "game-dev",
  "task": "R0-loop-recovery-release-candidate",
  "status": "running | blocked | review | complete",
  "updated_at": "ISO-8601 timestamp with offset",
  "worktree": "absolute local worktree path",
  "branch": "branch name",
  "head": "git commit SHA",
  "current_command": "exact command or null",
  "last_result": "concise factual result",
  "blocker": "exact blocker or null",
  "next_action": "one concrete next action"
}
```

Liveness rule: a `running` heartbeat older than 30 minutes is stale unless the current command is a known long build or test and the tracked process still exists. A stale loop must be marked `blocked` or restarted from the last clean gate. Never repeatedly call a model merely to refresh the timestamp.

## Token and time policy

- Supply workers only the active packet, relevant contracts, and failing log excerpt.
- Use local utility models for extraction and classification, not repeated architecture deliberation.
- Do not retry an identical failed command more than once without a material change.
- Cache summaries and command outputs in files so agents do not reread full transcripts.
- Prefer deterministic scripts, tests, and git state over model judgment.
- Stop after the release-candidate acceptance gate. Do not spend remaining budget inventing additional scope.
