# GAME-PASS17: live Shadowblade loadout effects

Date: 2026-09-23
Owner: `animerpg-game-hourly`
Baseline: `dbb1121a0b562f5c5d11794e57e383bb47696db4`
Target: `main`

## Purpose

Wire the already-merged pass-16 `ShadowbladeLoadout` backend into the existing live `ShadowbladeActions` game-domain owner without touching Win32/platform, renderer/editor, import/animation/audio/physics, shared CMake/workflows, networking, R0, deployment, release, or the separate engine/art PR stack. Preserve existing default Shadowblade numbers when only the Training Blade is equipped, so this packet does not silently retune the current prototype before a player can select equipment.

## Research mapping, accessed 2026-09-23

- `GAME-082`, equipped build affects the combat kit. Current ZZZ Version 3.2 official content still ships character-specific W-Engines, and Granblue Fantasy: Relink officially exposes weapons/Sigils that aid battle. Original adaptation: make the protagonist's existing loadout profile feed its own Shadowblade action tuning rather than adding collectible characters. Sources: https://zenless.hoyoverse.com/en-us/news/166023 ; https://store.playstation.com/en-us/product/UP5460-PPSA06954_00-GBRELINKWPPWUP02
- `GAME-083`, offensive equipment changes attack output. Genshin Version 5.5 official update details describe weapons and artifact set effects that raise ATK or specific attack damage. Original adaptation: bounded net loadout Attack raises Fatal Strike damage only, while the current Training Blade remains the numerical baseline. Source: https://www.hoyolab.com/article/37843579
- `GAME-084`, movement-focused build effects. Wuthering Waves' current PlayStation description emphasizes high-mobility exploration, dashing, and fast combat. Original adaptation: bounded net Mobility extends Shadowblade dash distance while preserving normalization, resource cost, cooldown, and invalid-input behavior. Source: https://store.playstation.com/en-us/product/EB1238-PPSA24686_00-0625573803756286/
- `GAME-085`, equipment can affect resource flow. Genshin official material documents Energy Recharge and increasing it with weapons/artifacts. Original adaptation: bounded net Resource Recovery raises only Shadow resource regeneration, with the same 0..100 cap and finite-delta gate. Source: https://www.hoyolab.com/article/36476708
- `GAME-086`, set/family synergy has real gameplay consequences. Genshin Version 5.5 officially documents 2-piece and 4-piece artifact set effects. Original adaptation: the existing Cooling/Rift/Civic two-module resonance bonuses must change the effective Shadowblade action tuning through the same bounded profile bridge, rather than remaining a disconnected score. Source: https://www.hoyolab.com/article/37843579
- `QOL-018`, compare build effects without applying them. A Wuthering Waves community tool post dated 2026-07-05 describes the recurring player problem of deciding which build is stronger and the friction of setting up external comparisons; a 2024 Genshin UI/UX discussion independently asked for a comparison screen, particularly for weapons. These are community anecdotes, not consensus. Genshin later added custom artifact loadouts, so this packet does not claim the old request is unresolved. Original adaptation: provide a non-mutating preset-tuning preview that future game UI can render. Sources: https://www.reddit.com/r/WutheringWavesGuide/comments/1unym77/compare_and_evaluate_your_echoesbuilds_accurately/ ; https://www.reddit.com/r/Genshin_Impact/comments/1g6payz ; https://www.hoyolab.com/article/39120584

## Allowed paths

Production:
- `Engine/Scene/ShadowbladeActions.h`
- `Engine/Scene/ShadowbladeActions.cpp`
- `Engine/Scene/LandmarkEncounter.cpp`, narrowly admitted after independent review found that its existing retry reset discarded the newly persistent loadout state

Verification:
- `Tests/ShadowbladeLoadoutEffectsPass17Tests.inc`
- `Tests/ThoughtCommandsTests.cpp`, existing include/call registration only

Operating records:
- this packet
- `Docs/Agents/animerpg-hourly/STATE.json`
- `Docs/Agents/animerpg-hourly/RUN-2026-09-23-PASS17.md`

No other path is authorized by this packet.

## Acceptance

1. Default `ShadowbladeActions` owns/consults `ShadowbladeLoadout`, but Training-Blade-only behavior remains exactly 80 Fatal Strike damage, 6.0 dash distance, 15 resource/s regeneration, and zero guard-damage mitigation.
2. Net Attack above that baseline raises Fatal Strike damage with an explicit hard cap. Existing range, resource, cooldown, follow-up, defeat, and hit-registration semantics remain unchanged.
3. Net Mobility raises normalized dash distance with an explicit hard cap. Cost/cooldown and zero/nonfinite-direction fallback remain unchanged.
4. Net Resource Recovery raises finite positive-delta regeneration with an explicit hard cap, while resource remains capped at 100 and invalid deltas remain no-ops.
5. Net Guard reduces only blockable non-perfect guard-integrity damage, with an explicit hard cap. Unblockable hits, perfect defense, player-health damage on guard break, and invalid threat handling retain their existing semantics.
6. Crossing an existing two-module family-resonance threshold measurably changes effective action tuning through the live bridge, while one family module does not receive the set bonus.
7. Preset preview uses a copy, returns the real `LoadoutActionResult`, mutates neither current equipment nor the caller's output on failure, and reports the same tuning that applying the valid preset would produce.
8. All arithmetic remains bounded, deterministic, C++17, and baseline-neutral. No NaN/infinity path may create resource or movement. The profile-to-action transform must clamp extreme integer profiles without signed-overflow behavior.
9. Existing registered tests continue to pass. Add explicit pass-17 regressions for default compatibility, offense, mobility, regeneration, defense, resonance, preview success/failure atomicity, direct cap saturation, negative/extreme profile safety, nonfinite delta behavior, and persistent loadout survival through the transient retry-reset helper.
10. `LandmarkEncounter::Retry` must use the persistent-loadout transient-reset helper so the existing encounter retry path continues to reset resource/cooldowns/health/guard state and preserve the defense timing preference without erasing acquired/equipped loadout items or saved presets. Existing registered `LandmarkEncounterTests` remain the encounter-path regression for retry transient semantics.

## Independent-review repairs

Fresh Codex review of PR #36 head `f8c5daf26504930b890279a83867fc79a7ab02aa` found two material P2 issues:

1. `LandmarkEncounter::Retry` reset `ShadowbladeActions` to a fresh object, which would silently erase persistent loadout ownership/equipment/presets after a completed encounter. The repair adds `ShadowbladeActions::ResetTransientStatePreservingLoadout`, directly regression-tests its persistent/transient contract in the already registered Shadowblade test target, and makes `LandmarkEncounter::Retry` call it. Existing `LandmarkEncounterTests` already exercise the production retry reset and timing-preset semantics.
2. The initial complete-build test never exceeded most tuning clamps, so removal of clamp logic could still pass. The repair exposes the pure profile-to-action transform as a production domain helper, makes its clamp arithmetic overflow-safe with `std::clamp`, and tests `INT_MAX`/`INT_MIN` profiles against exact caps/baseline. This is not counted as an extra feature.

A repaired candidate initially failed Windows Debug link because the pass-17 test directly called `LandmarkEncounter` from the `ThoughtCommandsTests` target, which does not link `LandmarkEncounter.cpp`. The repair did not change CMake or shared CI. Instead, persistent-reset behavior is tested through the linked `ShadowbladeActions` helper while the existing dedicated `LandmarkEncounterTests` target continues to verify the retry path. Exact-final-head gates must pass after this correction.

These repairs do not authorize renderer/platform/editor/build-system work and do not change the five-plus-one feature count.

## Verification and merge gates

Use existing registered deterministic test paths. Do not alter CMake or workflows. Require exact-head hosted Windows Debug/Release tests plus release-manifest integrity, then a fresh independent Codex review against this packet. Native interactive equipment UI/playtesting is not available from this packet and must remain `not_run_not_claimed`. If a material review/check fails, repair only within allowed paths, rerun affected gates, and merge only the exact final tested/reviewed head after re-reading live `main` and PR head.

## Stop conditions

Stop this packet rather than expanding scope if implementation requires Win32/platform input/UI changes, renderer/editor work, shared build/CI changes, persistence-format changes, another worker's open PR, architecture changes, dependencies, local scheduler execution, release, or deployment.
