# GAME Pass 7: single-character progression and build presets

Date: 2026-09-22  
Owner: `animerpg-game-hourly`  
Base: `main` at `1ac6bf8d54219968effadd17e575aafb4ebd3847`  
Working branch: `game/2026-09-22-progression-loadouts-pass7`

## Scope

This packet adds a bounded progression foundation for the one persistent customizable protagonist. It does not introduce collectible characters, gacha, party switching, engine architecture changes, renderer/editor/platform work, persistence-file formats, networking, dependencies, or live deployment.

Allowed production paths:
- `Engine/Scene/CharacterProgression.h`
- `Engine/Scene/LandmarkInteraction.h`
- `Engine/Scene/LandmarkInteraction.cpp`

Allowed verification/records:
- `Tests/LandmarkInteractionTests.cpp`
- this task
- `Docs/Agents/animerpg-hourly/RUN-2026-09-22-PASS7.md`
- `Docs/Agents/animerpg-hourly/STATE.json`

CMake is intentionally unchanged. `LandmarkInteractionTests` is already registered with `astral_add_test`, so the new header-only progression domain and its landmark integration run through an existing hosted Release-active test target.

## Research mapping

Research was read on 2026-09-22. Reference mechanics are adapted to the single-protagonist AnimeRPG design rather than copied.

1. **GAME-032, character XP and level ladder.** Genshin's PlayStation listing explicitly describes leveling characters to conquer harder enemies and domains. ZZZ producer Zhenyu Li's Version 2.5 PlayStation Blog post also references Agent promotion materials and ongoing combat-mechanic enhancement. Adaptation: a deterministic capped level ladder for the protagonist, with no roster or gacha dependency.
   - https://store.playstation.com/en-us/concept/10000896/
   - https://blog.playstation.com/2025/12/19/zenless-zone-zero-version-2-5-introduces-dual-form-void-hunter-on-december-30/

2. **GAME-033, Shadowblade mastery progression.** Cygames' official Granblue Fantasy: Relink product page includes Mastery Points among resources used to enhance characters, while Endless Ragnarok's official system page adds max-level Master Traits. Adaptation: lifetime mastery rank plus spendable mastery points for one protagonist.
   - https://relink.granbluefantasy.jp/en/products
   - https://relink-ragnarok.granbluefantasy.com/en/systems/

3. **GAME-034, rank-gated specialization tiers.** Endless Ragnarok describes three Master Trait play styles that modify skills and capabilities. Adaptation: three original Shadowblade paths, `ShadowStep`, `EclipseEdge`, and `BreakerFocus`, each with three bounded tiers and mastery-rank prerequisites.
   - https://relink-ragnarok.granbluefantasy.com/en/systems/

4. **GAME-035, weapon enhancement tiers.** Cygames says its Relink starter resources enhance characters and weapons; PlayStation's official Weapon Uncap pack documents weapon-upgrade materials. Adaptation: a single protagonist weapon tier using earned enhancement materials and level gates, with no loot rarity or monetization copied.
   - https://relink.granbluefantasy.jp/en/products
   - https://store.playstation.com/en-us/product/UP5460-PPSA06954_00-GBRELINKWPEXTD02/

5. **GAME-036, exploration-to-progression rewards.** Genshin's official PlayStation listing ties exploration, mechanisms, challenging domains, and rich rewards to character growth. Adaptation: completing the existing National Mall landmark objective once grants bounded XP, mastery points, and weapon materials while preserving the existing Shadowblade resource rewards and free discovery order.
   - https://store.playstation.com/en-us/concept/10000896/

6. **QOL-008, saved build presets.** A current ZZZ player thread on 2026-09-13 asks for a real loadout system instead of manually moving gear, with a counterpoint that some locked endgame modes reduce its value. Earlier March and June 2026 threads independently request preset slots and one-click swapping. Adaptation: three local preset slots save and restore the protagonist's equipped unlocked specialization talents. This is anecdotal community feedback, not a claim of consensus or a current ZZZ defect.
   - https://www.reddit.com/r/ZZZ_Official/comments/1wf0n2v/soooo_can_we_get_disc_loadouts_now_since_claret/
   - https://www.reddit.com/r/ZZZ_Official/comments/1rldm8a/request_drive_disc_presets/
   - https://www.reddit.com/r/ZZZ_Discussion/comments/1u4ruky/we_desperately_need_a_loadout_system_for_drive/

## Acceptance

- XP cannot exceed level 20 and extreme positive input terminates at the cap without overflow.
- Mastery rank derives from lifetime earned mastery while spending uses a separate bounded available balance.
- Talent upgrades enforce rank prerequisites, costs, maximum tier, valid IDs, and no duplicate equipped talent.
- Weapon upgrades enforce character-level gates, material costs, and maximum tier.
- National Mall objective completion grants progression once; repeat landmark interaction cannot farm it; callers without progression attached keep prior behavior.
- Three preset slots save/load equipped unlocked talents, while empty/out-of-range/duplicate operations fail closed.
- Existing registered `LandmarkInteractionTests` and repository deterministic checks pass on the exact final head.
- Independent Codex review has no unresolved major finding before merge.

## Explicit limits

This pass is game-domain progression state plus one existing exploration-loop integration. It does not save progression across process restarts, expose a progression/loadout UI, alter combat numbers from equipped talents, or prove native playability. Those are separate integration packets. Engine PR #13 and its CMake/editor runtime-smoke ownership remain untouched.
