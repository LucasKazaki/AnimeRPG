# GAME pass 37: Shadow Crypt mission planning and records

Owner: `animerpg-game-hourly`  
Baseline: `02bca71b2da473626578f89b9876afb4045f0de0`  
Branch: `game/pass37-shadow-crypt-mission-planning`  
Target: `main`  
Access date: 2026-09-24

## Scope and ownership

Deepen the already-merged Shadow Crypt game-domain mission without changing Astral Engine infrastructure. `ShadowCryptMission` is an existing game-owned production module held by `LandmarkInteraction`; the live owner is changed only with thin Shadow Crypt planning wrappers required to make the new game behavior reachable. This packet does not change renderer, platform, editor, importer, animation, audio, physics, CMake, workflows, dependencies, networking, R0, release/deployment, or another worker's PR.

Allowed paths:
- `Engine/Scene/ShadowCryptMission.h`
- `Engine/Scene/LandmarkInteraction.h`, Shadow Crypt planning wrappers only
- `Tests/ShadowCryptMissionPlanningPass37Tests.inc`
- registration-only include/call additions in `Tests/ThoughtCommandsTests.cpp`
- this task and `Docs/Agents/animerpg-hourly/` pass-37 records

The packet does not add a Win32/menu/controller UI. New mission-planning APIs are game-domain behavior and remain non-native-player-playable until a separately coordinated UI/input path consumes them.

## Current reference research

Primary official reference: HoYoverse, Genshin Impact Version 7.1 update details, published 2026-09-23 and accessed 2026-09-24: https://genshin.hoyoverse.com/tr/news/detail/166383 . Relevant distinct mechanics in the current update include explicit quest unlock criteria, Focused Experience Mode on supported quests, guide-style direction into quest content, result presentation that includes a historical personal best, and an `All Records` surface for a combat activity. The Astral adaptations below copy none of Genshin's characters, story, maps, UI art, code, monetization, or exact content.

Community reference for `QOL-038`: Genshin Impact Reddit discussion `Domain QoL Improvements We Need`, original post/replies dated 2025-04-22 and accessed 2026-09-24: https://www.reddit.com/r/Genshin_Impact/comments/1k59yyr . One reply specifically requests checking domain enemies from the character-selection screen before committing. Other replies discuss different popup/timer tradeoffs, so this is an anecdotal preference, not consensus. Current 2026 Genshin resolution of that exact enemy-inspection request is unverified; this packet does not claim Genshin currently lacks pre-entry information.

## Five reference increments plus one community increment

### GAME-182, Focused Shadow Crypt guidance
Gap: Standard mission guidance currently prioritizes the optional Cooling Cache in Rift Nave before the main stabilization objective.  
Adaptation: reversible `Standard` and `Focused` guidance. Focused mode points at the main Rift-node objective while leaving the optional cache fully available. The live `LandmarkInteraction` owner exposes the mode change.  
Acceptance: changing guidance cannot delete cache state, auto-complete objectives, alter rewards, or accept invalid focus values.

### GAME-183, explicit entry prerequisite report
Gap: initial entry currently fails as a boolean with no game-domain explanation of the authoritative blocker.  
Adaptation: a read-only report exposes Shadow Crypt lead progress and distinguishes incomplete lead, missing protagonist ownership, active run, pending checkpoint, completed-run replay, and ready initial entry. The live owner adds the protagonist gate before declaring entry ready.  
Acceptance: reports match `BeginShadowCrypt`/replay authority and never create a second timeline.

### GAME-184, prerequisite-specific next action
Gap: callers would otherwise need to duplicate mission, field-guide, and protagonist-authority interpretation.  
Adaptation: derive `BindProtagonist`, `FindCryptSigil`, `AskAboutShadowCrypt`, `EnterShadowCrypt`, `ContinueRun`, `ResumeCheckpoint`, or `ReplayCompletedRun` from authoritative state.  
Acceptance: guidance must not mutate the player's currently tracked field operation and must not advertise an action the live owner would immediately reject for missing protagonist authority.

### GAME-185, latest-clear versus personal-best result
Gap: the mission stores a best record but does not expose a clear-result comparison.  
Adaptation: read-only latest clear, retained personal best, score gap, and whether the latest clear established a new best.  
Acceptance: a worse replay cannot replace the best, and comparison arithmetic remains non-negative.

### GAME-186, bounded All Records archive
Gap: only one best record is retained, so recent clear history cannot be inspected.  
Adaptation: retain the four newest accepted completion records in newest-first order while keeping the personal best separately monotonic.  
Acceptance: exactly one record is added per accepted clear, the archive never exceeds four, out-of-range reads fail closed, and dropping an old entry cannot erase a better personal best.

### QOL-038, pre-entry Shadow Crypt encounter intel
Gap: before beginning a run, the mission has no compact read-only description of its authored room/objective structure.  
Adaptation: preview the four room objective requirements, optional Cooling Cache availability point, and Rift Warden finale before commitment, without inventing enemy statistics.  
Acceptance: preview works through the live owner before `BeginShadowCrypt` and cannot mutate mission or field state.

## Verification requirements

Registered regression coverage must exercise all six increments through the existing `ThoughtCommandsTests` aggregate without changing shared CMake. Required cases include incomplete/complete lead state, production missing-protagonist rejection, no mutation of tracked field operation, active/checkpoint/completed entry blockers, live-owner focus selection, focus-mode state preservation and invalid enum rejection, first/worse replay comparisons, history capacity/ordering, and pre-entry preview availability.

Initial independent review on commit `49113d2` found two P2 integration gaps: Focused mode was not reachable through the production owner, and entry readiness ignored the live owner's required `CharacterProgression`. Both are in-scope game-owner repairs and require fresh exact-head CI and rereview after landing.

After the scoped PR is opened, require exact-head hosted Windows Debug/Release tests and release-manifest integrity. Require fresh independent Codex review on the exact accepted head. Repair material findings and rerun stale gates. Native interactive/player-playable verification is not claimable because there is no new UI/input path in this packet.

Before merge, re-read `main`, PR head, full diff, checks, reviews, threads, and dependency state. Merge only with the exact expected head SHA if all applicable gates are clean. Verify merge SHA and `main` ancestry after the write.
