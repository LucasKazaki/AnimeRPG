# GAME PASS 6: Encounter Challenge Rules

Date: 2026-09-22
Owner: animerpg-game-hourly
Base: `f68e0917b38a0b4780d94a409874bb94df818163`
Target: `main`

## Scope

Game-domain-only packet. Allowed paths are `Engine/Scene/EncounterChallenge.h`, `Engine/Scene/LandmarkEncounter.*`, `Engine/Scene/CombatSandbox.h`, `Engine/Scene/CombatSandbox.cpp`, the already-registered `Tests/LandmarkInteractionTests.cpp`, the already-registered `Tests/LandmarkEncounterTests.cpp`, this task, and the hourly game state/receipt. `Tests/LandmarkEncounterTests.cpp` is explicitly included for production-path integration coverage of encounter activation, retry, reward caps, and activation-local challenge metrics. `CombatSandbox.*` is explicitly included only for the smallest challenge-boundary repair needed to clear transient technique-chain carryover at configured encounter activation without resetting accumulated training statistics. Do not touch renderer, platform, editor, CMake, workflows, dependencies, R0, or engine-worker files. PR #13 currently owns CMake/editor runtime-smoke work, so this packet deliberately avoids CMake edits.

## Research mapping

All sources accessed 2026-09-22.

1. GAME-027, selectable tactical focus. ZZZ's 2026-09-14 Virtual Shadow challenge describes stage-specific buffs and tactical bonuses that players can use for higher evaluations: https://zenless.hoyoverse.com/m/zh-tw/news/166073 . Adaptation: choose Balanced, Reaction, Stagger, or Finisher focus for one protagonist. No character/card copying.
2. GAME-028, encounter side goals. Granblue Fantasy: Relink centers repeatable quests, distinct combat objectives and rewards, while its demo and product descriptions separate story, quest and tutorial play: https://www.playstation.com/en-us/games/granblue-fantasy-relink/ and https://relink.granbluefantasy.jp/en/products . Adaptation: original flawless, technique-variety, speed, and tactical-focus side goals.
3. GAME-029, composite challenge score. ZZZ's 2026-09-14 Special Ops rules combine damage score, technique score and a time coefficient, with technique-combo multipliers: https://zenless.hoyoverse.com/m/zh-tw/news/166073 . Adaptation: existing Astral damage/technique score plus side-goal and tactical-event points, with existing encounter time grade feeding challenge evaluation.
4. GAME-030, difficulty/rank tiers. The same ZZZ event separates themed Alliance stages and harder Special Ops stages with distinct challenge mechanics. Granblue Relink supports adjustable difficulty: https://www.playstation.com/en-us/games/granblue-fantasy-relink/ . Adaptation: Standard, Expert and Apex challenge tiers with progressively stricter rank thresholds, rather than raw HP inflation.
5. GAME-031, first-clear rewards. Current ZZZ Combat Simulation allows selection of enemy cards and grants challenge rewards, and ZZZ trial/challenge events use clear rewards: https://zenless.hoyoverse.com/m/en-us/news/165921 . Adaptation: one bounded resource reward per challenge difficulty, preserved across retries and capped by the existing Shadowblade resource limit.
6. QOL-007, technique-first scoring. A 2026-04-18 r/ZZZ_Official discussion asks for less reliance on rising HP and more emphasis on boss skill/mechanics: https://www.reddit.com/r/ZZZ_Official/comments/1solaxz/what_changes_would_you_make_to_endgame/ . A 2026-02-19 discussion independently criticizes increased HP pools/scoring friction: https://www.reddit.com/r/ZZZ_Official/comments/1r8ts9k/endgame_increasing_enemy_hp_pools/ . This is player feedback, not consensus or evidence of current developer intent. Adaptation: optional TechniqueFirst scoring halves raw base-score weight and raises technique/side-goal value without changing enemy HP.

Genshin remains a broader combat/exploration reference: its official PlayStation description emphasizes elemental reactions whose mastery matters in both battle and exploration: https://store.playstation.com/en-fi/concept/10000896 . This packet does not copy Genshin content or monetization.

## Acceptance

- Challenge rules are disabled by default and cannot change existing encounter rewards or grades until explicitly configured.
- Tactical focus can independently target Reaction, Stagger, Finisher, or all three.
- Four side goals are deterministic: flawless, technique variety, speed, and selected tactical focus.
- Standard, Expert, and Apex produce progressively stricter rank thresholds. Apex specifically uses 2 goals = None, 3 = Bronze, 4 = Silver unless Gold time, and 4 + Gold time = Gold.
- Challenge scoring uses saturating 64-bit arithmetic and fails closed for negative counts/base scores.
- Challenge technique counters and base combat score are measured from activation-local deltas, so pre-activation training actions cannot satisfy encounter goals or inflate encounter score.
- Configured challenge activation clears only the transient technique-chain carryover used for score multipliers. It must not reset accumulated training statistics, target state, rewards, or unrelated combat preferences. Unconfigured encounters keep existing technique-chain behavior.
- TechniqueFirst weighting materially rewards techniques and side goals without increasing target health.
- First-clear resource rewards are one-shot per difficulty, persist across encounter retries, and are applied only through the existing resource cap.
- Existing unconfigured LandmarkEncounter behavior remains backward-compatible.
- Every challenge acceptance case is exercised by a test executable that the existing hosted CTest configuration actually registers and runs.

## Verification plan

1. Use focused GCC C++17 `-Wall -Wextra -Werror -pedantic` and Clang ASan/UBSan checks while developing challenge rules when the execution environment permits; do not reuse an earlier-head result as final-head proof after production changes.
2. Keep pure challenge-rule boundary cases in the already-registered `LandmarkInteractionTests` target and production encounter integration cases in the already-registered `LandmarkEncounterTests` target, so hosted CTest compiles and executes both without modifying CMake owned by engine PR #13.
3. `LandmarkEncounterTests` must cover first-clear persistence through `Retry`, capped reward application, exclusion of pre-activation damage/techniques from challenge scoring and side goals, and exclusion of pre-activation technique-chain multipliers from the first in-encounter technique.
4. Hosted Windows Debug/Release deterministic tests must pass on the exact final head after all review repairs.
5. Release-manifest integrity and independent Codex review must pass on that same final head.
6. Merge only after material findings are repaired and a final current-main/diff recheck is clean.

Native input/UI/playable verification is not claimed by this domain-only packet.
