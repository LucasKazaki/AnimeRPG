# GAME PASS 3: reactions, finisher, scoring, exploration reward, and clean retry

Date: 2026-09-22  
Owner: `animerpg-game-hourly`  
Base: `main` at `84dc005e4e6259faf7209265def762339a6c0cf9`  
Branch: `game/2026-09-22-reactions-retry-pass3`

## Scope

This is a bounded GAME-domain packet. It does not edit renderer, platform, editor, generic engine facilities, CMake, workflows, dependencies, or the engine worker's PRs.

Allowed production paths:
- `Engine/Scene/CombatSandbox.h`
- `Engine/Scene/CombatSandbox.cpp`
- `Engine/Scene/LandmarkInteraction.h`
- `Engine/Scene/LandmarkInteraction.cpp`
- `Engine/Scene/LandmarkEncounter.h`
- `Engine/Scene/LandmarkEncounter.cpp`

Allowed verification/records:
- `Tests/LandmarkEncounterTests.cpp`
- this task
- `Docs/Agents/animerpg-hourly/RUN-2026-09-22-PASS3.md`
- `Docs/Agents/animerpg-hourly/STATE.json`

## Five reference features plus one community improvement

1. `GAME-012` Solar/Umbral -> Eclipse mana reaction. Reference: Genshin Impact's developer-described elemental reaction system. The names, rules, damage, and fiction are original to AnimeRPG.
2. `GAME-013` earned three-hit combo finisher. Reference: Granblue Fantasy: Relink's Link Attacks and Chain Bursts. This remains a single-protagonist follow-up and does not import party switching.
3. `GAME-014` optional Accessible combo-assist preset. Reference: Granblue Fantasy: Relink Assist Mode. Standard remains the default; the preset lowers only the finisher hit threshold.
4. `GAME-015` deterministic Lincoln training grade from completion time. Reference: Zenless Zone Zero's current Virtual Shadow Hunt event description, where tactical bonuses are used to obtain better scores. AnimeRPG uses a local Gold/Silver/Bronze training grade, not ZZZ scoring rules.
5. `GAME-016` one-time National Mall objective-completion resource reward. Reference: Genshin Impact's exploration of strange mechanisms and discoveries. It rewards completing the existing three-landmark chain while retaining free out-of-order exploration.
6. `QOL-004` clean encounter retry. Historical community input: a 2022 Genshin Impact discussion complained that retrying Spiral Abyss could retain skill cooldown state; a 2021 discussion similarly complained that a restart flow did not immediately restart a challenge. These are historical anecdotes, not claims about current Genshin behavior.

## Source locators, accessed 2026-09-22

- https://store.playstation.com/en-fi/concept/10000896
- https://www.playstation.com/en-us/games/granblue-fantasy-relink/
- https://store.playstation.com/en-us/product/UP5460-PPSA06954_00-GBRELINKERDGSTD1
- https://zenless.hoyoverse.com/fr-fr/news/166073
- https://www.reddit.com/r/GenshinImpact/comments/u0e71n/
- https://www.reddit.com/r/Genshin_Impact/comments/mau8t9/

## Acceptance

- Opposing Solar/Umbral affinity triggers one bounded Eclipse reaction, clears primed affinity, and reset clears transient affinity.
- Standard combo finisher requires three successful combo hits; invalid range preserves the opening; success consumes it once.
- Accessible assist is opt-in, requires two successful hits, and persists across training-session reset.
- Encounter completion reports Gold at <=3 s, Silver at <=6 s, otherwise Bronze, based on combat-domain elapsed time.
- Completing all three landmarks grants the objective completion reward once, even when discovery order is free; repeated interaction cannot farm it.
- Retry is unavailable before completion. After completion it immediately restores combat/Shadowblade transient attempt state, preserves the chosen defense timing preset, restarts the encounter, and can never duplicate the first-clear reward.

## Verification plan

1. Focused C++17 source fixture with warning-clean GCC.
2. Same focused packet with Clang ASan/UBSan where available.
3. Existing hosted Windows Debug/Release deterministic workflow on the exact PR head.
4. Existing release-manifest integrity workflow on the exact PR head.
5. Independent Codex review of the exact final head.
6. No native-playable claim unless a registered interactive Windows run is actually recorded.

The local fixture is intentionally limited: it compiles the exact modified game-domain sources with a minimal test-only interface for the unchanged Shadowblade dependency. Hosted repository CI is required to verify the real unchanged Shadowblade implementation and complete CMake target.
