# GAME Pass 1: combat and exploration quality slice

Date: 2026-09-22
Owner: `animerpg-game-hourly`
Baseline: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`
Branch: `game/2026-09-22-combat-exploration-pass1`
Target: `main`

## Purpose

Implement six small, testable gameplay increments for the existing Astral Engine prototype without taking over generic engine infrastructure:

1. `GAME-001` direction-aware normalized Shadowblade dash.
2. `GAME-006` deterministic attack-combo tracking inspired by fast basic/special attack flow.
3. `GAME-003` bounded posture, stagger, recovery, and one-shot stagger openings.
4. `GAME-004` contextual Fatal Strike follow-up that consumes a stagger opening for a reduced resource cost.
5. `GAME-005` ordered National Mall landmark objective progression while preserving free out-of-order discovery.
6. `QOL-002` resettable training session metrics, based on repeated player requests for a training target with damage history/statistics.

These adapt interaction patterns only. They do not copy another game's content, characters, assets, story, monetization, or code.

## Research map

Primary reference pages rechecked 2026-09-22:

- Genshin Impact PlayStation overview: https://www.playstation.com/en-us/games/genshin-impact/ . The current page describes open exploration, climbing/swimming/flying, ability variety, and combining elements for puzzles and attacks.
- Zenless Zone Zero App Store developer description: https://apps.apple.com/us/app/zenless-zone-zero/id1606356401 . It describes Basic/Special Attacks, Dodge/Parry, Stun, and Chain Attacks.
- Wuthering Waves PlayStation listing: https://store.playstation.com/en-us/concept/10010764/ . It describes high-mobility exploration plus Extreme Evasion and Dodge Counter.
- Granblue Fantasy: Relink PlayStation overview: https://www.playstation.com/en-us/games/granblue-fantasy-relink/ . It describes real-time combat, distinct styles, and Link Attacks/Chain Bursts.

New community item for this pass:

- Wuthering Waves Reddit, `Request to Kuro: ... DPS Testing tool`, published 2026-08-02: https://www.reddit.com/r/WuWaves/comments/1vd8ctk/ . The post asks for a training room/dummy with real-time total damage and combat statistics. It had 199 points at retrieval and comments both supporting the need and warning that dummy-only optimization does not represent interruption-heavy real combat.
- Older independent corroboration exists in Genshin discussions, including https://www.reddit.com/r/Genshin_Impact/comments/le0szq/ (2021-02-06) and https://www.reddit.com/r/Genshin_Impact/comments/17hqybl/ (2023-10-27), which ask for controllable training targets, damage history, and post-session statistics. These are community anecdotes, not proof of consensus or of any current comparator deficiency.

## Allowed paths

- `Engine/Scene/CombatSandbox.h`
- `Engine/Scene/CombatSandbox.cpp`
- `Engine/Scene/ShadowbladeActions.h`
- `Engine/Scene/ShadowbladeActions.cpp`
- `Engine/Scene/LandmarkInteraction.h`
- `Engine/Scene/LandmarkInteraction.cpp`
- `Engine/Platform/Win32Application.h`
- `Engine/Platform/Win32Application.cpp`
- `Tests/CombatSandboxTests.cpp`
- `Tests/ShadowbladeActionsTests.cpp`
- `Tests/LandmarkInteractionTests.cpp`
- `Docs/Agents/animerpg-hourly/BACKLOG.json`
- `Docs/Agents/animerpg-hourly/RUN-2026-09-22-PASS1.md`
- this task packet

No CMake, renderer, editor, asset pipeline, CI workflow, dependency, recovery runner, release, or deployment changes are admitted.

## Ownership check

At admission, the only open PR found was #13, `Add native Astral Editor runtime smoke coverage`, based on the same main revision. Its stated scope is the Astral Editor smoke and CMake wiring. This packet does not modify its editor smoke file or CMake. The Win32 application edits here are narrow gameplay-input/title integration and require hosted compilation plus native runtime acceptance before claiming full playable verification.

## Acceptance

### GAME-001
- cardinal and diagonal dash displacement has the same magnitude;
- invalid or neutral direction uses the existing deterministic forward fallback;
- cooldown/resource semantics remain unchanged;
- live `Q` dash uses current movement input when provided, while thought-command dash remains deterministic.

### GAME-006
- successful attacks inside the combo window increase the combo;
- timeout resets current combo without corrupting best combo;
- rejected/cooldown/out-of-range attempts do not increase combo.

### GAME-003
- light/heavy attacks apply bounded posture damage;
- threshold crossing creates one finite stagger window;
- defeat overrides stagger;
- posture recovers after an idle delay;
- consuming a stagger opening is idempotent.

### GAME-004
- Fatal Strike during stagger uses the reduced follow-up cost and consumes exactly one opening;
- ordinary Fatal Strike cost/damage/cooldown remain unchanged;
- range/resource/defeat rejection cannot consume the opening or spend resources.

### GAME-005
- objective guidance remains Lincoln -> Reflecting Pool -> Washington Monument;
- out-of-order discoveries remain recorded but do not skip missing prerequisite guidance;
- once the missing prefix is completed, previously discovered later landmarks count immediately;
- completion remains bounded to the existing three landmarks.

### QOL-002
- successful damage records total damage, hit count, peak hit, and best combo;
- `R` resets dummy health, posture, cooldown/session clocks, combo, and metrics without resetting the completed landmark encounter or granting rewards;
- invalid/zero damage does not inflate metrics.

## Verification plan

1. Compile and run the three affected deterministic domain test targets in Debug and Release-equivalent configurations where available.
2. Run the broader deterministic CTest suite through hosted Windows CI after push.
3. Run existing static milestone verifiers because the Win32 input path changes.
4. Native Windows runtime smokes remain a distinct gate. Existing M4/M7 smoke behavior must still compile, and the registered local executor should exercise directional `WASD+Q` plus `R` reset before this slice is called fully playable.
5. Independent PR review is required before merge. Author review is not independent.

## Stop conditions

Stop and leave the PR unmerged on any new test failure, unresolved independent review request, overlapping concurrent write, broken benchmark-input suppression, or evidence that a change requires engine architecture work beyond this packet.
