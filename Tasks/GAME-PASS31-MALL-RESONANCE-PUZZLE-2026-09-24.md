# GAME pass 31: National Mall resonance puzzle

Owner: `animerpg-game-hourly`  
Target: `main`  
Baseline: `a32a729591bdf4dddf32494d08959c3e6f90d69e`  
Branch: `game/2026-09-24-mall-resonance-puzzle-pass31`

## Scope and ownership

This is a bounded GAME-domain packet. It adds one original National Mall environmental puzzle after the already-authoritative three-landmark objective. It does not change Astral Engine renderer, platform, editor, importer, animation, audio, physics, shared CMake/workflows, dependencies, R0, networking, deployment, release, or another worker's PRs.

Allowed paths:
- `Engine/Scene/MallResonancePuzzle.h`
- thin production-owner integration in `Engine/Scene/LandmarkInteraction.h`
- `Tests/MallResonancePuzzlePass31Tests.inc`
- test registration only in `Tests/ThoughtCommandsTests.cpp`
- this task and `Docs/Agents/animerpg-hourly/` pass records

The puzzle is original project content: three National Mall resonance anchors tied to the game's quantum-cooling/mana premise. Reference games supply interaction, difficulty, assist, record, retry, and recovery lessons only.

## Research mapping, accessed 2026-09-24

Official/developer sources:
- Genshin Impact Version 7.1 update details, published 2026-09-23: https://genshin.hoyoverse.com/tr/news/detail/166383. The current update documents a challenge mode with selectable stage parameters including difficulty, enemy HP and round duration, scoring based on selected parameters, and retained per-stage best scores.
- Zenless Zone Zero `Combat Training - Triple Bounty`, published 2026-08-31: https://zenless.hoyoverse.com/m/en-us/news/165921. It requires an earlier story unlock, lets players select challenge targets, and rewards successful challenge completion.
- Wuthering Waves Version 1.2 developer communication, published 2024-08-06: https://wutheringwaves.kurogames.com/kr/main/news/detail/1112. It added targeting-priority settings and an exploration shooting tool specifically to remove the need to rebuild a party around a pistol user for mechanisms.
- Granblue Fantasy: Relink PlayStation overview, rechecked 2026-09-24: https://www.playstation.com/en-us/games/granblue-fantasy-relink/. It documents adjustable difficulty, individually activatable assists, control/tutorial reminders, Practice Mode, and pausing.

Community recovery request, original player text read 2026-09-24:
- Genshin player request dated 2026-08-30: https://www.reddit.com/r/Genshin_Impact/comments/1w2s6by/great_now_what/. The player reports a statue wedged in a trapdoor and explicitly asks why there is no direct in-game puzzle reset.
- Related reports dated 2026-08-13, 2026-08-15, 2026-08-19, 2026-08-22, and 2026-09-01 independently describe stuck puzzle objects and recovery through relogging, teleporting, leaving the area, support, or other indirect resets: https://www.reddit.com/r/Genshin_Impact/comments/1vn7qrr/stuck_on_puzzle/ ; https://www.reddit.com/r/Genshin_Impact/comments/1vp3ulj/my_statue_fell_and_is_stuck_in_wall_cant_move_it/ ; https://www.reddit.com/r/Genshin_Impact/comments/1vt3268/snezhnaya_puzzle_broken/ ; https://www.reddit.com/r/Genshin_Impact/comments/1vv1po1/puzzles_requiring_relog/ ; https://www.reddit.com/r/Genshin_Impact/comments/1w4c71w/statue_stuck_in_open_royal_hall_door_puzzle/ .

These community reports are anecdotes, not consensus. Genshin Version 7.1 released on 2026-09-23, but the inspected official update material does not establish whether every cited puzzle/reset issue was fixed. Current comparator resolution is therefore recorded as unestablished, not as an ongoing defect claim.

## Six bounded increments

### GAME-152: objective-gated three-anchor resonance puzzle
Gap: the current three-landmark objective has tracking, rewards, dialogue, field-guide follow-up, and dungeons, but no reusable environmental manipulation puzzle.
Adaptation: after the authoritative National Mall objective is complete, a persistent-protagonist owner can start an original three-anchor phase puzzle using Lincoln Memorial, Reflecting Pool, and Washington Monument resonance anchors.
Acceptance: missing quest completion or progression ownership rejects entry; valid entry creates one active board; invalid anchor IDs fail closed; no engine/platform dependency is introduced.

### GAME-153: selectable bounded puzzle difficulty
Gap: there is no puzzle-specific challenge configuration.
Adaptation: `Survey`, `Standard`, and `Expert` use distinct target-phase profiles plus bounded par/time thresholds while keeping the same core rules.
Acceptance: invalid difficulty fails atomically; each valid profile has a deterministic solution and record slot; active runs cannot be silently overwritten by reconfiguration.

### GAME-154: optional class-neutral guidance assist
Gap: a puzzle can become opaque without forcing players to change combat class/loadout or consult external instructions.
Adaptation: optional `Guidance` identifies the first mismatched landmark and number of rotations still needed. It reads state only and never rotates or completes a node for the player.
Acceptance: assist-off exposes no guidance; assist-on points to the deterministic next mismatch; aligned boards switch guidance to a ready-to-stabilize state without mutation.

### GAME-155: per-difficulty mastery record
Gap: repeated puzzle clears have no mastery feedback.
Adaptation: completion produces Bronze/Silver/Gold plus deterministic score, moves and elapsed time. The best record is retained separately for each difficulty and cannot be replaced by a worse replay.
Acceptance: par/fast/no-failed-attempt clear reaches Gold; slower extra-move replay does not lower the best; untouched/invalid records fail closed.

### GAME-156: repeatable clear with one first-clear progression entitlement
Gap: environmental puzzle replay should support mastery without becoming an unlimited progression farm or allowing reward ownership to move between protagonists.
Adaptation: completed runs can start again while one bounded first-clear reward remains tied to the persistent `CharacterProgression` owner that first entered the production puzzle.
Acceptance: first clear exposes one entitlement; owner swap cannot claim or restart it; correct owner claims exactly once; replay preserves personal best and claimed history.

### QOL-032: explicit in-game active-puzzle reset
Gap: recent player reports from a comparator describe resorting to relog/teleport/area transitions when puzzle state becomes stuck.
Adaptation: one explicit recovery action restores the current active resonance board, timer, move count, and failed-stabilization count to its configured start while preserving difficulty, assist mode, personal bests, clears, and reward history.
Acceptance: reset is active-run-only, idempotently returns to a clean board, cannot erase records/rewards, and never requires process/session restart.

## Verification requirements

The registered `ThoughtCommandsTests` aggregation must compile and execute pass-31 production-owner regressions in Debug and Release. Required coverage includes quest/progression entry gates, all three solutions, invalid enum/time handling, assist read-only behavior, mismatch handling, personal-best monotonicity, owner-bound first-clear reward, replay, and explicit recovery reset. All previously registered game regressions remain in the same aggregate.

Exact-final-head acceptance requires:
1. hosted Windows Debug/Release deterministic workflow success,
2. hosted Release-manifest integrity success,
3. fresh independent implementation review on that exact head with no unresolved material findings,
4. full diff/scope review and live `main`/PR-head reread immediately before expected-head merge.

Native rendered puzzle UI, controller/menu interaction, puzzle geometry/VFX/audio, GPU/performance evidence, and cross-process puzzle persistence are not implemented by this packet and must not be claimed.
