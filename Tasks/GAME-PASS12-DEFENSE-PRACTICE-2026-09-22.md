# Game Pass 12: Defense Practice and Accessibility

Baseline: `012ca98f6aa1ec49bbec830666830de5ab7d1874` on `main`.
Owner: AnimeRPG hourly GAME worker only.

## Bounded objective

Deepen the pass-11 defense bridge into a more useful offline practice loop without changing renderer/editor/platform/build infrastructure or taking over Astral Engine worker scope.

## Allowed paths

- `Engine/Scene/CombatSandbox.h`
- `Engine/Scene/CombatDefenseTraining.h`
- `Engine/Scene/ShadowbladeActions.h`
- `Tests/ShadowbladeActionsTests.cpp`
- `Tests/DefensePracticePass12Tests.inc`
- this task packet
- `Docs/Agents/animerpg-hourly/STATE.json`
- `Docs/Agents/animerpg-hourly/RUN-2026-09-22-PASS12.md`

No CMake, workflow, renderer, editor, platform, asset pipeline, dependency, R0, release, deployment, or unrelated-worker edits.

## Research map

Access date for all sources: 2026-09-22.

### GAME-057: selectable attack-pattern practice
Gap: pass 11 can practice the current enemy rotation, but cannot request a specific known attack pattern without replaying the rotation.
Adaptation: allow deterministic QuickCut, GuardBreaker, or RiftBurst practice while preserving normal sequence order and existing readiness/ownership gates.
Reference: Zenless Zone Zero Combat Simulation lets players select enemy cards for training/challenges. Current Version 3.1 developer announcement also retains Combat Simulation and a training event. Sources: https://www.hoyolab.com/article/46037106 and https://zenless.gg/combat-training-triple-bounty-event-details/ .
Acceptance: selected attacks copy the canonical pattern values, respect recovery/stagger/pending/death gates, receive new generations, and do not advance the ordinary enemy rotation.

### GAME-058: drill goal presets
Gap: telemetry and grades exist, but practice has no explicit technique objective.
Adaptation: bounded Perfect Defense, Guard Discipline, and Dodge Discipline goals, each requiring three successful matching resolutions, plus Free Practice.
Reference: ZZZ's September 2026 Shadow Chase Showdown VR stages use themed stage mechanics, tactical buffs, technique scoring, technique combos, and ratings. Source: https://zenless.gg/shadow-chase-showdown-event-details/ (published 2026-09-14, source credited to HoYoverse).
Acceptance: goals report deterministic current/target/completion state and distinguish guard from dodge success without changing damage or grade thresholds.

### GAME-059: control reminders
Gap: practice exposes timing and blockability, but not the recommended defensive control.
Adaptation: expose a semantic Guard or Dodge reminder derived from the actual linked attack, with unblockable attacks always recommending Dodge and Dodge Discipline overriding blockable guidance.
Reference: Granblue Fantasy: Relink lists Control Reminders as an accessibility feature. Source: https://www.playstation.com/en-ca/games/granblue-fantasy-relink/ .
Acceptance: reminder is None with no owned threat, Guard for ordinary blockable practice, Dodge for unblockable threats, and Dodge for Dodge Discipline.

### GAME-060: tutorial reminder escalation
Gap: repeated defensive mistakes do not surface actionable training guidance.
Adaptation: after one consecutive hit suggest reading the telegraph; after repeated hits retain the missed attack's blockability so the recovery-time hint can say Guard blockable or Dodge unblockable; after three consecutive successful non-perfect defenses suggest tighter timing even if the session previously contained a perfect defense.
Reference: Granblue Fantasy: Relink lists Tutorial Reminders as an accessibility feature. Same PlayStation source above.
Acceptance: hints are bounded, deterministic, retain actionable post-hit context during recovery, reset hit streaks on successful defense, track the ordinary-defense streak separately from lifetime perfects, and never mutate combat state.

### GAME-061: pause/resume defense drill
Gap: the practice coordinator cannot freeze an active drill without letting combat/action clocks advance.
Adaptation: a drill-local pause freezes both linked clocks, suppresses queueing/input mutation, preserves the linked generations and cue, and resumes from the exact remaining windup.
Reference: Granblue Fantasy: Relink lists Game Pausing and consequence-free Practice Mode. Same PlayStation source above.
Acceptance: large paused deltas cause no damage, recovery, resource/cooldown, timing, telemetry, or generation changes; wrong-object calls cannot mutate or impersonate the owned link; resume continues exactly once.

### QOL-013: non-color-only defense cue symbols
Community request: Wuthering Waves players with color-vision deficiencies asked for important information to use shapes/symbols instead of relying only on color. Original discussions include May 25, 2024 and March 25, 2025. Sources: https://www.reddit.com/r/WutheringWaves/comments/1d0301u and https://www.reddit.com/r/WutheringWaves/comments/1jjw8ih . These are player anecdotes with corroborating replies, not proof of consensus. A 2026-09-22 search of Kuro's indexed official news did not establish whether later Wuthering Waves updates fully resolved the request, so this is used only as a design lesson.
Adaptation: attach semantic non-color cue symbols for blockable, unblockable, and perfect-timing states to the existing defense cue metadata. No claim of rendered accessibility compliance is made.
Acceptance: active cue symbols derive from authoritative blockability/timing state and remain usable independently of future color choices.

## Regression contract

Extend the already registered `ShadowbladeActionsTests` target, without modifying CMake, to cover:
1. selected-pattern canonical values, invalid pattern rejection, generation behavior, pending/recovery/stagger/death gates, and non-perturbation of normal rotation;
2. goal progress and separation of guard/dodge/perfect counts;
3. control reminders for blockable/unblockable and goal override;
4. mistake-driven tutorial hints during recovery, retained blockability, hit-streak reset, and three consecutive ordinary defenses after an earlier perfect;
5. pause/input/clock/generation preservation, wrong-object isolation, and exact resume;
6. semantic cue symbols across blockable, unblockable, perfect, and no-threat states.

`Tests/DefensePracticePass12Tests.inc` is included by the already registered `Tests/ShadowbladeActionsTests.cpp` translation unit so these regressions run in the existing Debug and Release test target without touching shared CMake.

Also retain all prior pass-11 ownership, stale-threat, invalid-delta, split-frame, telemetry, and grade regressions.

## Verification and merge gates

- Hosted Windows Debug and Release registered tests on the exact candidate SHA.
- Existing repository static/integrity checks on the exact candidate SHA.
- Full diff/path review against this packet and current `main` immediately before merge.
- Fresh independent Codex review on the exact final candidate SHA, with all substantive threads resolved.
- Merge only if required checks/review are green and `main` has not moved incompatibly. If `main` moves, reconcile and rerun affected gates.

Native interactive UI/audio/GPU evidence is not claimed by this domain-only packet. The new metadata is a dependency-ready gameplay contract for a later engine/UI integration slice.
