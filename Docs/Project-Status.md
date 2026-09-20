# Project direction and evidence, 2026-09-19

This is a repository audit snapshot, not a heartbeat or claim about a running PC.

## Repository roles and history

`LucasKazaki/AnimeRPG` is the custom C++17 Astral Engine project. ADR-0001 accepted
that direction on July 21, 2026. `LucasKazaki/AnimeRPG-UE5` is a separate, minimal
Unreal movement experiment, not a migration target or production dependency.

The original product backlog describes a supernatural National Mall action RPG:
Shadowblade, Arc Mage and Aegis; thought commands and a shadow summon; destruction;
Shadow Crypt and Mana Reactor dungeons; distinct enemy types; and male/female
character and environment art. These are product intentions, not delivered claims.

The July implementation reached a small Win32/GDI wireframe systems prototype:
movement/camera, a training target, bounded Shadowblade abilities, five key-driven
commands, three landmark proxies, discovery, and one training encounter.
August added repository recovery controls and hosted build checks. The August 14
release target is historical, not a current deadline or proof of delivery.

The September 17 director-context commit is
`a13c3dae28bf1745d4c13bd48269c07fd3318fc6`. At audit start it was one commit ahead of
`main` (`771b61ac116dfa4a70d81b53a390f672aaf3bf4e`) on
`codex/animerpg-director-context-recovery-20260917`, with no PR in the inspected
PR results. This audit branch preserves that commit rather than recreating or
rewriting its history. Its controls become part of main only after approved merge.

## Current direction on this branch

Read `../GAME_DEVELOPMENT_CONTROL.md` and `../GOAL_WORK.json` first.
Engine-first, local-only verification is the current director scope: 3D first,
genuine 2D support, and no restart of game content, combat tuning, encounters,
narrative, production assets, audio content, or game playtests before acceptance.
Procedural fixtures are for engine verification, not permission for content growth.

Do not treat the old "next M8" paragraph, August recovery deadline, old heartbeat,
or a highest-numbered task file as an active work assignment.

## Two numbering systems, not one completion percentage

| Label | Later implemented capability | Original backlog meaning |
|---|---|---|
| M8 | Five bounded Thought Commands | Broader command/slow-time/routing scope |
| M9 | Landmark interaction | Shadow summon, still separate |
| M10 | Landmark encounter | Enemy set, still separate |

"Implementation M10" does not mean ten of fifteen product milestones are done.
The current command controls are not evidence of a natural-language or LLM system.

## Repairs and remaining evidence gaps

The audit fixes disabled Release assertions, non-serialized GUI tests, and CI
failure propagation. See `QA/AUDIT-2026-09-19.md` for exact test boundaries.

The original R0 runner still needs a separate safety repair: reused-worktree/base
identity, overlapping output-path protection, bounded subprocess execution, fresh
dates, rerun-safe evidence handling, and actual runtime dependency packaging.
Do not invoke it merely because the old heartbeat recommends it.

The director document's mention of completed R0 is not a receipt. No fresh local
Windows process, full interactive test run, self-contained package, 3D/2D stress
result, frame-time/RAM/VRAM budget, 24-hour soak, or independent engine acceptance
was established by this connector audit. This means unverified here, not proof
that a local result does not exist.

## Dependency-ordered next work

First reconcile real local receipts and the registered worktree with the exact
revision under review. Obtain independent acceptance of the verification repairs.
Then admit one owned, bounded engine packet, following the existing research matrix:

1. Runtime, jobs, memory, and failure recovery.
2. Scene/data ownership, ECS decisions, and serialization.
3. GPU rendering, materials, lighting, and streaming.
4. Asset handling, animation, collision, physics, and navigation.
5. Audio engine, input/actions, save/replay, and genuine 2D support.
6. Profiling/tooling, packaging, measured stress, and the required soak.

Define measurable acceptance thresholds and test fixtures in each packet before
implementation. Do not substitute a broad engine rewrite for one evidenced gap.
Resume the retained game roadmap only after the stated engine acceptance and
Lucas's approval. This audit does not choose a GPU API or import a third-party engine.

## Source records

- `Architecture/ADR-0001-engine-choice.md`
- `Planning/MILESTONES.md`, `Decision-Log.md`, and existing `Tasks/` contracts
- `Agents/LOOP_STATUS_2026-08-11.md` and the R0 task/runner, as historical records
- September 17 director-control, goal-work, and engine-research files
- Sibling UE5 README, descriptor, module, target, input, and game-mode source
