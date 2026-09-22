# GAME Pass 10: enemy patterns, punish windows, and phase practice

Date: 2026-09-22  
Owner: `animerpg-game-hourly`  
Base: `main` at `e782f595696c046c2c394b636622f700a18d0425`  
Working branch: `game/2026-09-22-enemy-patterns-pass10`

## Scope

This packet deepens the existing single-protagonist `CombatSandbox` with deterministic enemy attack planning, selectable aggression cadence, Boss pressure-phase attack variation, stagger interruption of queued enemy windups, a bounded perfect-defense punish opening, and an optional Boss phase-practice control. It stays inside existing game-domain combat code and does not create a parallel combat framework.

Allowed production/test paths:
- `Engine/Scene/CombatSandbox.h`
- `Engine/Scene/CombatSandbox.cpp`
- `Tests/CombatSandboxTests.cpp`

Allowed verification/records:
- this task
- `Docs/Agents/animerpg-hourly/RUN-2026-09-22-PASS10.md`
- `Docs/Agents/animerpg-hourly/STATE.json`

Explicitly excluded: shared CMake changes, renderer/platform/editor/import/animation/audio/physics systems, workflow/dependency changes, R0, networking, save formats, release/deployment, other workers' PRs, and native/local execution. Open PR #13 owns editor/runtime-smoke engine work and PR #23 owns starter-art work; neither has overlapping packet paths.

## Research mapping

Sources accessed 2026-09-22. Mechanics are reference points only; attack names, timing values, enemy profiles, punish rules, and practice-state behavior below are original AnimeRPG adaptations.

1. **GAME-047, deterministic enemy attack plans.** Wuthering Waves' current developer App Store description explicitly calls out fast combat with Extreme Evasion and Dodge Counter. Adaptation: existing training enemies expose bounded `QuickCut`, `GuardBreaker`, and `RiftBurst` plans with explicit windup, damage, guard damage, blockability, and recovery so later native telegraphs/defense integration can consume deterministic state.
   - https://apps.apple.com/us/app/wuthering-waves/id6475033368
   - Current App Store version observed: 3.6.4, dated 2026-09-11.

2. **GAME-048, selectable enemy aggression cadence.** Granblue Fantasy: Relink's current PlayStation accessibility description includes adjustable difficulty and Practice Mode. Adaptation: `Relaxed`, `Standard`, and `Aggressive` alter only future enemy recovery cadence, without silently inflating health or rewriting an attack already queued.
   - https://www.playstation.com/en-us/games/granblue-fantasy-relink/

3. **GAME-049, Boss pressure-phase attack rotation.** Genshin Impact's current PlayStation description frames system mastery as a battle advantage, while Granblue's current gameplay page uses enemy-specific stun opportunities and timing windows. Adaptation: the existing Boss changes from a two-step Normal rotation to a three-step Pressure rotation that introduces an unblockable RiftBurst, without copying any comparator boss.
   - https://store.playstation.com/en-us/concept/10000896/
   - https://relink.granbluefantasy.jp/en/gameplay

4. **GAME-050, stagger interruption of queued windups.** Granblue Relink's official gameplay page says filling an enemy stun gauge creates an attack opportunity and that Link Time slows enemy actions. Adaptation: crossing the existing Astral posture threshold cancels a queued enemy windup and blocks immediate requeue through the stagger window; no party Link Attack architecture is introduced.
   - https://relink.granbluefantasy.jp/en/gameplay

5. **GAME-051, defense-earned punish opening.** Granblue Fantasy: Relink - Endless Ragnarok Ver. 2.0.2 notes, updated 2026-09-04 JST, specifically improve perfect-guard/perfect-dodge feedback and foe warning-cue visibility. Wuthering Waves also advertises Dodge Counter. Adaptation: one resolved perfect defense grants one one-second 5/4 direct-attack punish window; misses/cooldown rejection preserve it and expiry removes it.
   - https://relink-ragnarok.granbluefantasy.com/en/updates/381/
   - https://apps.apple.com/us/app/wuthering-waves/id6475033368

6. **QOL-011, selectable Boss practice phase.** Original player discussion on r/WutheringWaves, published 2025-05-04, asks to choose a hologram boss phase directly so phase two can be practiced without repeatedly clearing phase one. This is anecdotal feedback, not consensus. The current Wuthering Waves App Store history shows Version 3.6 added a new `Tactical Hologram: Sparring` challenge, but the official material inspected here does not document the exact requested phase-selection control, so current resolution remains unverified.
   - https://www.reddit.com/r/WutheringWaves/comments/1keqncr/small_idea_to_make_learning_hologram_bosses_more/
   - https://apps.apple.com/us/app/wuthering-waves/id6475033368

## Player-observable acceptance

- **GAME-047:** Standard training queues `QuickCut` at 0.55 s windup, 18 damage, 20 guard damage, blockable, with Standard recovery. A second queue is rejected while one is pending and during recovery.
- **GAME-048:** Relaxed and Aggressive produce different future recovery durations. Changing the preset does not mutate an already pending plan. Invalid enum values fail closed.
- **GAME-049:** Boss Normal rotates `QuickCut` then `GuardBreaker`; Pressure rotates `QuickCut`, unblockable `RiftBurst`, then `GuardBreaker`.
- **GAME-050:** a pending Bulwark windup is removed by the hit that crosses its posture threshold, and requeue remains blocked until stagger ends.
- **GAME-051:** `PerfectDefense` arms exactly one punish opening. An out-of-range attempt does not consume it; a valid direct hit consumes it and applies the bounded multiplier; a later ordinary hit is unmodified; expiry removes the opening.
- **QOL-011:** selecting Boss Pressure practice starts at full Boss health with Pressure weakness/rotation, survives `ResetTrainingSession`, rejects non-Boss/invalid phases, and `ClearBossPracticePhase` returns to health-driven Normal/Pressure behavior.
- Invalid outcome/preset/phase enums fail closed and cannot mutate state.
- Nonfinite or nonpositive time deltas remain ignored by the existing combat clock.
- Training defeat/reset clears transient enemy attack and punish state without changing unrelated persistent progression or save formats.

## Verification commands and gates

The bounded implementation uses the existing registered `CombatSandboxTests` target through `astral_add_test`; shared CMake is not modified.

Disposable exact-source Linux fixture commands:

```bash
g++ -std=c++17 -Wall -Wextra -Werror -I<fixture-root> Engine/Scene/CombatSandbox.cpp pass10_test.cpp -o pass10_test && ./pass10_test
clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -I<fixture-root> Engine/Scene/CombatSandbox.cpp pass10_test.cpp -o pass10_test_asan && ./pass10_test_asan
```

Scope check before merge:

```bash
git diff --name-only e782f595696c046c2c394b636622f700a18d0425...HEAD
```

Every changed path must be one of the six allowed paths listed in this packet.

Required hosted exact-head gates before merge:
- `Windows build and deterministic tests`: success for the final PR head, covering repository Debug and Release builds/tests.
- `Release manifest integrity`: success for the final PR head.
- Independent Codex or other authorized implementation review on the exact final head, with no unresolved major finding.

No native interactive runtime command is admitted by this packet because it does not wire the planner into Win32 input/render/audio or add a rendered phase selector. Native playable status must remain false rather than substituting hosted/sandbox evidence.

## Risks and stop conditions

- The enemy planner currently exposes game-domain attack plans but does not itself drive rendered/audio telegraphs, player damage timing, or `ShadowbladeActions::BeginIncomingAttack`; those require a later coordinated native integration packet.
- QOL-011 is domain-level practice selection only, with no native phase-picker UI.
- New attack timings are prototype training values, not final balance claims.
- Do not merge if `main` changes incompatibly, any required exact-head hosted check fails/is missing, independent review has an unresolved finding, or the diff expands outside allowed paths.
- Do not invoke historical R0, start local jobs, modify other workers' branches, or claim Genshin/ZZZ parity.
