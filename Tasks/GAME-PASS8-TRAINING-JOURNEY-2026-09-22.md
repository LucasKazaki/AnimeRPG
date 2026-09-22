# GAME Pass 8: training plans, journey progression, and opt-in objectives

Date: 2026-09-22  
Owner: `animerpg-game-hourly`  
Base: `main` at `755faabfb5f04d5bc07324d91cbceb261cdc1060`  
Working branch: `game/2026-09-22-training-journey-pass8`

## Scope

This packet extends the persistent single-protagonist progression foundation and makes the National Mall objective optionally player-started. It does not introduce collectible characters, gacha, party switching, renderer/editor changes, persistence-file formats, networking, dependencies, or deployment.

Allowed production paths:
- `Engine/Scene/CharacterProgression.h`
- `Engine/Scene/LandmarkInteraction.h`
- `Engine/Scene/LandmarkInteraction.cpp`
- `Engine/Platform/Win32Application.cpp`, limited strictly to mapping the new `LandmarkInteractionResult::ObjectiveAdvanced` result to the existing player-visible interaction-status text. This one-line integration repair was admitted after independent review found that the new game-domain result otherwise displayed as `Unknown`. Open engine PR #13 was rechecked and does not modify this file. No other platform behavior is in scope.

Allowed verification/records:
- `Tests/LandmarkInteractionTests.cpp`
- this task
- `Docs/Agents/animerpg-hourly/RUN-2026-09-22-PASS8.md`
- `Docs/Agents/animerpg-hourly/STATE.json`

CMake is intentionally unchanged because engine PR #13 owns shared CMake/editor runtime-smoke paths. `LandmarkInteractionTests` is already a registered CTest target.

## Research mapping

Research was read on 2026-09-22. Reference mechanics are adapted to one persistent customizable protagonist and do not copy characters, terminology, economy, art, or monetization.

1. **GAME-037, Shadowblade skill proficiency ranks.** ZZZ separates Agent skills into upgradeable categories and gates higher skill levels by Agent progression. Genshin likewise uses separately leveled talents. Adaptation: Dash, Fatal Strike, and Defense each track bounded practice and deterministic ranks 1 through 5. This packet adds the domain contract; native input/combat hooks are a later integration step.
   - https://www.hoyolab.com/article/30592563
   - https://www.hoyolab.com/article/16305893

2. **GAME-038, progression training plan.** ZZZ Version 1.4 added Special Training Plan targets for Agents, skills, and equipment. Adaptation: one local plan records target protagonist level, weapon tier, all three specialization tiers, and all three Shadowblade skill ranks, then reports exact remaining progress.
   - https://www.hoyolab.com/article/35654082

3. **GAME-039, combat readiness score.** The same ZZZ update added Agent Combat Readiness with recommended upgrade paths. Adaptation: a bounded 0 to 100 readiness score summarizes permanent protagonist progression only. It is advisory and does not alter combat or gate content.
   - https://www.hoyolab.com/article/35654082

4. **GAME-040, Journey milestone claims.** ZZZ Version 1.4's Journey system exposes progress nodes and claimable chapter rewards. Adaptation: level 5, 10, 15, and 20 protagonist milestones each expose a one-time mastery/material reward with explicit claim state.
   - https://www.hoyolab.com/article/35654082

5. **GAME-041, weapon awakening.** Cygames and PlayStation document Relink weapon enhancement and weapon-uncap resources. Adaptation: after the existing weapon reaches tier 5, two late-progression awakening ranks require protagonist level gates and earned enhancement materials. No loot rarity or paid item path is copied.
   - https://relink.granbluefantasy.jp/en/updates
   - https://store.playstation.com/en-us/product/UP5460-PPSA06954_00-GBRELINKWPEXTD02/

6. **QOL-009, opt-in landmark objective activation.** A Genshin player discussion from 2026-01-10 asks world quests not to auto-trigger merely from proximity and proposes a start confirmation. Independent August 12-13, 2026 discussions similarly ask for explicit interaction before quests begin. These posts are community feedback, not proof of universal consensus or a current official defect. Adaptation: the National Mall objective can be configured to `ManualStart`; ordinary exploration and discoveries still work before activation, and already discovered landmarks can be revisited to advance the explicitly started objective without duplicating discovery rewards.
   - https://www.reddit.com/r/Genshin_Impact/comments/1q8skhj/its_2026_genshin_should_stop_auto_triggering/

## Acceptance

- Skill practice is independent per skill, ignores invalid/nonpositive input, saturates at rank 5, and cannot overflow on extreme input.
- Training plans validate every target against current caps, report eight independent targets, and become complete only when all are met.
- Combat readiness remains within 0 to 100 and is derived only from bounded permanent progression state.
- Journey milestone rewards are level-gated, explicit, and idempotent.
- Weapon awakening requires max weapon tier, level 15 then 20, bounded material costs, and caps at rank 2.
- Auto-start landmark behavior remains backward compatible.
- Manual-start mode permits free discovery without quest progress, begins only after explicit activation, allows a discovered landmark to advance the objective on revisit, and cannot duplicate exploration/objective rewards or reset a progressed objective.
- Reapplying the current objective activation mode is idempotent and cannot cancel an already-started manual objective.
- A player-visible manual objective revisit reports `Objective Advanced`, not `Unknown`, through the existing runtime title mapper.
- The existing registered `LandmarkInteractionTests` target covers the game-domain behavior and repository deterministic checks pass on the exact final head.
- Independent Codex review has no unresolved major finding before merge.

## Explicit limits

The skill-proficiency domain does not yet receive automatic practice events from native input/combat actions, training plans/readiness have no UI, and progression still lacks a cross-process save format. Manual quest activation is domain behavior, not a rendered confirmation dialog. Native playability is not claimed in this packet. The one-line runtime label mapping is an integration repair only and does not claim native interactive verification.
