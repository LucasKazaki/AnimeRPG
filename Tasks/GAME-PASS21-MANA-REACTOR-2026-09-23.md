# GAME pass 21: Mana Reactor expedition

Date: 2026-09-23
Owner: `animerpg-game-hourly`
Target: `main`
Baseline: `3290870e5cddb7f0ab012c9a29e471e57fcc2201`
Branch: `game/2026-09-23-mana-reactor-pass21`

## Bounded scope

Implement one original, game-domain Mana Reactor expedition slice using existing National Mall investigation progress and character progression. This packet does not change renderer, platform, editor, import, animation, audio, physics, CMake, workflows, dependencies, R0, networking, release, deployment, or another worker's PR.

Allowed production paths:
- `Engine/Scene/ManaReactorExpedition.h`
- `Engine/Scene/CharacterProgression.h`, only for the one-time Mana Reactor first-clear entitlement

Allowed verification and operating-record paths:
- `Tests/ManaReactorExpeditionPass21Tests.inc`
- `Tests/ThoughtCommandsTests.cpp`, only pass-21 include/call registration
- `Tasks/GAME-PASS21-MANA-REACTOR-2026-09-23.md`
- `Docs/Agents/animerpg-hourly/RUN-2026-09-23-PASS21.md`
- `Docs/Agents/animerpg-hourly/STATE.json`

This is game-domain logic. No rendered Mana Reactor scene, production geometry/enemies/VFX/audio, controller/menu UI, native interactive playtest, GPU/performance result, or cross-process save persistence may be claimed by this packet.

## Research mapping

Access date for all sources: 2026-09-23.

### GAME-102: gated staged expedition with visible progress
Reference: Zenless Zone Zero Version 2.6 update announcement, published 2026-02-05, documents commission progress percentages and distinct restart choices.
Source: https://www.hoyolab.com/article/43642066
Repository gap: the retained vision names a Mana Reactor dungeon, but current `main` has Shadow Crypt expedition logic and no Mana Reactor expedition module.
Original adaptation: three original Mana Reactor stages, Intake Bay, Cooling Lattice, and Core Chamber, gated by the existing completed Rift Investigation. Expose bounded authoritative progress including stage and percentage.
Acceptance: locked before the two-clue Rift Investigation completes; deterministic 0..100 progress; invalid mode values fail closed.

### GAME-103: heat/stability control loop
Reference: Genshin Impact Version 5.7's Forge Realm's Temper documents stage-specific rules that change effective combat strategy.
Source: https://www.hoyolab.com/article/39384127
Repository gap: no reactor-specific pressure/resource loop exists.
Original adaptation: each Mana Reactor stage has a distinct hazard and deterministic control actions that trade objective speed against heat and stability, both clamped to 0..100. Reaching heat 100 or stability 0 fails the current stage.
Acceptance: invalid controls do not mutate; bounds hold; riskier control advances faster but can fail; failure never silently advances a stage.

### GAME-104: optional stage challenge targets
Reference: Zenless Zone Zero `Snap! Hollow Realm Showdown`, published 2026-03-06, has six themed stages, main targets, additional challenge targets, and stage-specific effects.
Source: https://www.hoyolab.com/article/44074876
Repository gap: Shadow Crypt has a single optional cache, but no per-stage performance challenges in the proposed Mana Reactor.
Original adaptation: each reactor stage independently records one optional thermal/stability target on clear. Targets contribute to grade and score but never block main progression.
Acceptance: at most one optional target per stage; target state survives current-stage retry for already-cleared stages; failure cannot award it.

### GAME-105: consequence-free Calibration mode
References: ZZZ `Snap! Hollow Realm Showdown` unlocks Hyperfocus Shot with no challenge objectives after event-stage completion; Granblue Fantasy: Relink's current PlayStation accessibility listing documents a consequence-free Practice Mode.
Sources: https://www.hoyolab.com/article/44074876 ; https://www.playstation.com/en-us/games/granblue-fantasy-relink/
Repository gap: existing practice systems do not provide a Mana Reactor hazard-learning path.
Original adaptation: a Calibration mode runs the same bounded reactor controls and feedback, but cannot grant the expedition first-clear reward.
Acceptance: Calibration can complete and produce a summary, but reward entitlement and progression resources remain unchanged.

### GAME-106: completion grade, score, replay, and one-time reward
Reference: Genshin Impact Version 5.7 Forge Realm's Temper awards stage results/rewards from highest score, while the repository's existing Shadow Crypt establishes the local precedent for idempotent first-clear progression rewards.
Source: https://www.hoyolab.com/article/39384127
Repository gap: no Mana Reactor result contract or reward entitlement exists.
Original adaptation: deterministic Bronze/Silver/Gold result based on optional targets, retries, peak heat, and minimum stability; bounded nonnegative score; replay remains legal; one-time Expedition first-clear reward is owned by `CharacterProgression` so replay or a stale run object cannot regrant it.
Acceptance: score/grade deterministic; first Expedition clear grants once; repeated claims and later replays do not duplicate rewards; Calibration never consumes or grants the entitlement.

### QOL-022: retry current stage without replaying cleared stages
Community source: Genshin Impact Reddit, `Why can't the Devs make us able to retry the second half of an abyss chamber??`, published 2025-01-09, +583 at retrieval. The original post asks not to repeat a successful first half after failing the second. Replies include both support and a counterargument that completing both halves in one run is part of the challenge.
Source: https://www.reddit.com/r/Genshin_Impact/comments/1hx2ock
Corroborating/current-resolution context: ZZZ Version 2.6, published 2026-02-05, later added explicit `Start Over` and `Retry Challenge`, where retry restarts the current combat. This means the old player request is used as a historical usability lesson, not proof that every comparator still lacks it.
Source: https://www.hoyolab.com/article/43642066
Repository gap: Shadow Crypt has room-local defeat recovery and safe suspend, but no explicit user-facing semantic split between retrying the current challenge and starting the entire run over.
Original adaptation: `RetryCurrentStage()` restores only the current reactor stage baseline while retaining cleared-stage progress and previously earned optional targets; `StartOver()` restarts the run from Intake Bay while retaining the selected mode. Retry count contributes to grading.
Acceptance: retry cannot erase cleared stages or double-award targets; start-over resets run-local score/progress; both reject when no run exists; no reward is granted by retry/start-over.

## Verification contract

Register `Tests/ManaReactorExpeditionPass21Tests.inc` through the existing `ThoughtCommandsTests` shim. Cover:
- prerequisite gating and invalid enum rejection;
- deterministic stage/progress reporting;
- heat/stability bounds, risky control behavior, and failure without progression;
- optional targets and no duplicate target credit;
- Calibration completion with zero progression/reward mutation;
- Expedition grade/score and one-time first-clear idempotency across replay;
- exact current-stage retry vs full start-over state retention/reset;
- reward/progress state after retry and replay.

Required merge evidence remains separate: exact-head hosted Windows Debug/Release deterministic tests, release-integrity checks, clean full diff/scope inspection, and a fresh independent implementation review on the exact candidate. Native interactive/UI evidence is not supplied by hosted CI and is not claimed.
