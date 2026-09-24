# GAME pass 36: Encounter Special Offensive scorecard

Date: 2026-09-24  
Owner: `animerpg-game-hourly`  
Baseline: `1db755a0f06a936198a8a329d83ee2fd6a1052d3`  
Target: `main`

## Scope

Deepen the already-merged Lincoln Memorial encounter challenge instead of adding another combat framework. This packet stays in game-owned challenge/encounter paths and the already-registered encounter regression target. It does not change renderer/platform/editor/import/animation/audio/physics, CMake/workflows, networking, release/deployment, R0, or another worker's PR.

Allowed production/test paths:

- `Engine/Scene/EncounterChallenge.h`
- `Engine/Scene/LandmarkEncounter.h`
- `Engine/Scene/LandmarkEncounter.cpp`
- `Tests/LandmarkEncounterTests.cpp`
- this packet and `Docs/Agents/animerpg-hourly/` pass-36 records

## Research mapping

Sources accessed 2026-09-24.

Primary current reference: HoYoverse, **Shadow Chase Showdown** announcement, published 2026-09-14: https://zenless.hoyoverse.com/en-us/news/166073 . The event describes Alliance and Special Offensive VR stages, Special Offensive Tactical Buffs, damage and specific-technique score, Technique Combos from different techniques within a time window, combo score multipliers, and an in-stage score built from Damage Score, Technique Score, and a completion-time coefficient.

1. **GAME-177, Special Offensive encounter profile.** Repository gap: the existing `EncounterChallengeTracker` has Balanced and TechniqueFirst evaluation but no explicit score-attack profile. Adaptation: add an opt-in `SpecialOffensive` scoring mode to the existing tracker, retaining the single protagonist and existing encounter flow.
2. **GAME-178, active Technique Combo score.** Repository gap: `CombatSandbox` already has an authoritative mixed-technique chain with a real timeout, but encounter scoring did not expose that live chain. Adaptation: accepted reaction/stagger/finisher execution can contribute a bounded score-only combo bonus while the real chain is still active. No copied values or moves.
3. **GAME-179, selectable tactical score protocol.** Repository gap: tactical focus chooses which event is a side goal, but there was no separate challenge protocol. Adaptation: original `ShadowTempo`, `RiftPressure`, and `BalancedFlow` protocols change score only. They do not alter health, damage, enemies, or persistent progression.
4. **GAME-180, explicit score breakdown and time coefficient.** Repository gap: the challenge result previously exposed only an aggregate total. Adaptation: report activation-local Damage Score, Technique Score, Technique Combo bonus, tactical bonus, time coefficient, pre-difficulty score, and final score. Original coefficients are Gold 125%, Silver 100%, Bronze 75%.
5. **GAME-181, monotonic per-difficulty best score.** Current score-attack events emphasize repeated higher evaluations. Repository gap: the challenge tracker retained first-clear entitlements but no score-chase record. Adaptation: retain only a strictly higher positive Special Offensive score per difficulty across existing encounter retries.
6. **QOL-037, score coach.** Player discussions around ZZZ's score-oriented endgame have criticized systems that can reward unintuitive front-loaded or spammy play. A Jan. 8, 2026 discussion recirculating the then-new Shiyu scoring formula explicitly says players could be pushed to front-load damage and even ignore stun windows: https://www.resetera.com/threads/zenless-zone-zero-ot2-bangboo-for-your-buck.1208418/page-467 . These are community reactions, not consensus and not evidence that current ZZZ still has the same issue. The current Sept. 14 official event is materially more explicit about its components. Astral adaptation: a read-only coach names the weakest current score axis, Damage, Technique, Combo, Time, or None, from the accepted result instead of hiding the reason a run scored poorly.

Reference-game mechanics are design lessons only. No ZZZ characters, stages, story, art, audio, code, monetization, or party/gacha architecture are imported.

## Acceptance

- Existing Balanced and TechniqueFirst scoring remains backward-compatible when SpecialOffensive is not selected.
- SpecialOffensive uses activation-local raw damage and technique deltas, never pre-activation totals.
- Technique Combo reads the existing live `CombatSandbox::TechniqueChain()` and is bounded to four. Expired chains cannot score as active combos.
- Tactical protocols affect scoring only and cannot mutate combat damage, target health, progression, or rewards.
- Score arithmetic clamps negative evidence, saturates positive arithmetic, and fails closed on invalid configuration enums.
- Breakdown fields expose exact components and time coefficient deterministically.
- Personal best is per difficulty, replaces only on a strictly higher positive score, and survives the existing immediate retry path.
- Score coach is read-only and reports a deterministic weakest axis without changing the score or run.
- Existing challenge ranks, side goals, first-clear reward rules, encounter retry, and older regressions remain green.

## Verification required before merge

- `LandmarkEncounterTests` must compile and run in hosted Windows Debug and Release on the exact final candidate head.
- Existing deterministic hosted suite and Release-manifest integrity workflow must pass on that exact head.
- Independent Codex review must inspect that exact candidate. Repair material findings and rerun affected checks.
- Immediately before merge, reread current `main`, PR head, full diff, checks, reviews, and unresolved threads. Merge only with the expected-head guard.
- Native interactive/player-playable evidence is not claimed because this packet adds no renderer/menu/controller wiring.
