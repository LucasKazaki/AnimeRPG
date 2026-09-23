# GAME pass 27: live Shadow Crypt mission

Date: 2026-09-24
Owner: `animerpg-game-hourly`
Baseline: `40311ba7dcf1035656b6b2d0dbeb589030071dc7`
Target: `main`
Branch: `game/2026-09-24-shadow-crypt-live-pass27`

## Scope and ownership

Attach the already-merged `ShadowCryptExpedition` domain to the production `LandmarkInteraction` game owner. Preserve Astral Engine, the National Mall setting, the persistent protagonist, and existing reward ownership. Do not change renderer, platform/input, editor, importer, animation, audio, physics, CMake, workflows, dependencies, R0, networking, release, deployment, or another worker's PR.

Allowed production paths:
- `Engine/Scene/ShadowCryptMission.h` (new game-owned coordinator)
- `Engine/Scene/LandmarkInteraction.h` (small live-owner API/member integration)

Allowed verification/records paths:
- `Tests/ShadowCryptMissionPass27Tests.inc`
- `Tests/ThoughtCommandsTests.cpp` only for the existing registered aggregation include/call
- this task packet
- `Docs/Agents/animerpg-hourly/PASS27-BACKLOG.json`
- `Docs/Agents/animerpg-hourly/RUN-2026-09-24-PASS27.md`
- `Docs/Agents/animerpg-hourly/STATE.json`

## Research map

Sources were read/revalidated on 2026-09-24. Reference games provide interaction lessons only. No proprietary story, characters, maps, art, audio, code, economy, or party architecture is copied.

1. **GAME-132, authoritative prerequisite-gated entry.** ZZZ's official `Combat Training - Triple Bounty` requires Combat Simulation to be unlocked in Main Story Chapter 1 before the event can be entered. Astral adapts that clarity by routing Shadow Crypt entry through the live `LandmarkInteraction` owner and its existing `ShadowCryptLead` evidence, not a test-created bypass. Source: https://zenless.hoyoverse.com/m/en-us/news/165921 (published 2026-08-31, accessed 2026-09-24).
2. **GAME-133, explicit room objective briefing.** ZZZ's developer-authored `Snap! Hollow Realm Showdown` describes six themed stages with explicit main and additional challenge targets. Astral keeps its four original rooms but exposes current room, objective, exact progress/requirement, cache state, damage/defeats, and safe-suspend state from the authoritative dungeon object. Source: https://www.hoyolab.com/article/44074876 (published 2026-03-06, accessed 2026-09-24).
3. **GAME-134, safe room-boundary suspend and one-shot resume.** Granblue Fantasy: Relink's current PlayStation accessibility listing documents pausing and consequence-free Practice Mode. Astral does not claim disk saving; it promotes the already-validated Shadow Crypt boundary checkpoint into the production owner, consumes it once on resume, and blocks a fresh run from silently overwriting a suspended run. Source: https://www.playstation.com/en-us/games/granblue-fantasy-relink/ (accessed 2026-09-24).
4. **GAME-135, monotonic best-clear record.** Current ZZZ combat challenge material uses bounded challenge targets and clear results, while the existing Shadow Crypt already grades Bronze/Silver/Gold. Astral adds an original deterministic score and keeps only the best completed expedition across replay. Sources: https://www.hoyolab.com/article/44074876 and https://zenless.hoyoverse.com/m/en-us/news/165921 (accessed 2026-09-24).
5. **GAME-136, one-time live first-clear reward plus immediate full replay.** ZZZ's current Combat Training event awards successful challenges, Genshin's official PlayStation overview frames domains as repeatable combat challenges with rewards, and the existing Astral progression object already owns a monotonic Shadow Crypt first-clear entitlement. The live owner now claims that existing entitlement and can restart a completed expedition without erasing the best record. Sources: https://zenless.hoyoverse.com/m/en-us/news/165921 and https://www.playstation.com/en-us/games/genshin-impact/ (accessed 2026-09-24).
6. **QOL-028, authoritative next-objective guidance.** In a Wuthering Waves player discussion dated 2025-04-19, a player specifically requested extending Rinascita's objective tracker to older regions so the map could show the next objective. A later 2026-07-01 discussion praised the added tracker/flying QOL in Huanglong while noting hidden objectives could still require external searching. This is player feedback and later corroboration, not proof of a current unresolved defect. Astral adapts the lesson into a compact Shadow Crypt next-action enum that surfaces the main room objective, the newly eligible Cooling Cache, resume, or replay directly from authoritative mission state. Community sources: https://www.reddit.com/r/WutheringWaves/comments/1k2pg14 and https://www.reddit.com/r/WutheringWaves/comments/1ukhiae/ (published 2025-04-19 and 2026-07-01; accessed 2026-09-24).

## Acceptance criteria

- **GAME-132:** a fresh live owner cannot enter Shadow Crypt; completing the existing National Mall objective plus Shadow Crypt dialogue evidence completes `ShadowCryptLead`; then exactly one active run can begin.
- **GAME-133:** briefing mirrors room/objective/progress/cache/damage/defeat/suspend state and never fabricates progress.
- **GAME-134:** mid-objective suspend fails without mutation; a valid room-boundary suspend creates one checkpoint, clears the live timeline, blocks fresh-run overwrite, resumes once, and rejects stale double-resume.
- **GAME-135:** a clean cache-clearing run records a deterministic Gold best; a later worse replay cannot replace it.
- **GAME-136:** the existing first-clear reward is granted once through the live progression owner; repeated claims and later replays cannot regrant it; replay is rejected while already active.
- **QOL-028:** guidance advances Entry Seal -> Archive -> cache discovery -> cache clear -> Rift stabilization -> Warden -> replay, and suspended state recommends resume.

## Verification contract

Before merge:
1. The existing registered `ThoughtCommandsTests` aggregation must compile and run pass-27 regressions in hosted Windows Debug and Release without changing shared CMake.
2. Existing repository deterministic tests and Release-manifest integrity must pass on the exact final head.
3. The pass-27 regression must exercise the real `LandmarkInteraction`/dialogue/world path, not only a teaching fixture, including invalid damage input, mid-objective suspend rejection, checkpoint single-consumption, record monotonicity, reward idempotency, and active-run replay rejection.
4. Obtain fresh independent Codex review on the exact final head and resolve every material thread.
5. Re-read `main`, PR head, full diff/checks/review threads immediately before expected-head merge. If main moved, reconcile and rerun affected gates.

Native dungeon rendering, controller/menu input, geometry/enemies/VFX/audio, GPU/performance evidence, disk persistence, cross-process resume, and hands-on native playtesting are outside this packet and must not be claimed.
