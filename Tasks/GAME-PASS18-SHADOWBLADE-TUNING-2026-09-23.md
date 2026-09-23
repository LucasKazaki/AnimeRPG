# GAME-PASS18 Shadowblade tuning workbench

Date: 2026-09-23
Owner: `animerpg-game-hourly`
Target: `main`
Baseline: `747c6837afb6a8993286cc2139fb61931306645b`
Branch: `game/2026-09-23-shadowblade-tuning-pass18`

## Objective

Deepen the already-live Shadowblade loadout from passes 16-17 with a bounded equipment-tuning loop. Keep all effects inside the existing one-protagonist loadout/action contracts. Do not add gacha, parties, online requirements, a parallel combat framework, renderer/platform work, or engine infrastructure.

## Five reference increments plus one community increment

1. `GAME-087` — module tuning ranks. Granblue Fantasy: Relink's official PlayStation weapon-upgrade packs include fully upgraded Sigils and upgrade materials. Astral adaptation: each owned resonance module has three bounded tuning ranks, with slot-specific stat growth that is consumed by the existing `ShadowbladeActions` tuning bridge. Sources: https://store.playstation.com/en-us/product/UP5460-PPSA06954_00-GBRELINKWPPWUP01/ and https://store.playstation.com/en-us/product/UP5460-PPSA06954_00-GBRELINKWPPWUP02 .
2. `GAME-088` — weapon calibration ranks. The same official Relink material explicitly frames weapon upgrade items as combat progression, while ZZZ Version 3.2 exposes W-Engine Modification stages and upgrade guidance. Astral adaptation: the three original Shadowblade weapon frames gain three calibration ranks, while ownership and existing level gates remain authoritative. Source: https://zenless.hoyoverse.com/en-us/news/166000 .
3. `GAME-089` — exact material planning. Genshin Impact Version 5.4's HoYoverse-authored PlayStation post says Level-Up Plans calculate required materials and show where to find them. Astral adaptation: owned weapons/modules expose exact remaining tuning-part cost to the rank cap, with no hidden currency. Source: https://blog.playstation.com/2025/01/26/20250127-gi/ .
4. `GAME-090` — deterministic next-upgrade guidance. Genshin Version 4.5's HoYoverse-authored PlayStation post documents the Training Guide's enhancement suggestions for levels, weapons, artifacts, and talents; ZZZ 3.2 also notes upgraded Combat Readiness guidance. Astral adaptation: recommend the cheapest next upgrade among currently equipped eligible gear, with stable tie order and explicit affordability. Sources: https://blog.playstation.com/2024/03/05/20230306-genshinimpact/ and https://zenless.hoyoverse.com/en-us/news/166000 .
5. `GAME-091` — rank-aware salvage recovery. ZZZ's official Version 2.5 shop update states that dismantling higher-rank A-rank W-Engines returns W-Engine Chips according to rank. Astral adaptation: salvaging an enhanced resonance module returns its base salvage value plus half of invested tuning parts, then clears its rank and remains one-shot. Source: https://zenless.hoyoverse.com/id-id/news/161722 .
6. `QOL-019` — explicit salvage protection. A Wuthering Waves player discussion dated 2024-10-07 asks for faster lock/discard controls, with another player warning that forgetting to lock gear can cause accidental deletion; a separate 2024-12-01 discussion requests easier lock/discard management. These are historical player anecdotes, not consensus or proof of a current comparator defect. Astral adaptation: an owned module can be explicitly protected, making salvage fail atomically until protection is removed. Sources: https://www.reddit.com/r/WutheringWaves/comments/1fy7egh and https://www.reddit.com/r/WutheringWaves/comments/1h4cmnk . Current resolution of the exact older request is not claimed.

Access/revalidation date for all sources: 2026-09-23.

## Allowed paths

Production:
- `Engine/Scene/ShadowbladeLoadout.h`

Verification:
- `Tests/ShadowbladeTuningPass18Tests.inc`
- `Tests/ThoughtCommandsTests.cpp`, include/call registration only

Operating records:
- this packet
- `Docs/Agents/animerpg-hourly/STATE.json`
- `Docs/Agents/animerpg-hourly/RUN-2026-09-23-PASS18.md`

No other path is authorized by this packet.

## Acceptance

- Module and weapon tune ranks are integers in `[0,3]` and cannot exceed the cap.
- Rank transitions spend exact tuning-part costs, reject insufficient funds without mutation, and preserve weapon ownership/level eligibility gates.
- Tuned Edge/Ward/Flow/Insight modules alter Attack/Guard/Resource Recovery/Mobility respectively through `ShadowbladeLoadout::Profile()`, so existing live `ShadowbladeActions::CurrentLoadoutTuning()` consumes the effects without a new combat path.
- Remaining-cost queries are exact, bounded, and return zero for invalid, unowned, or max-rank items.
- Recommendation considers currently equipped eligible items only, chooses the lowest next cost with stable weapon-then-slot traversal as the tie contract, and reports affordability from authoritative tuning parts.
- Enhanced salvage is one-shot, clears tune rank, preserves the global tuning-parts cap, and refunds only base salvage plus one-half of invested module tuning parts.
- Protected owned modules cannot be salvaged and protection/unprotection never changes equipment or tuning parts.
- Existing pass 16/17 loadout, preset, salvage, and live-action regressions remain green.
- Invalid enum IDs fail closed.

## Verification gates

Use the repository's existing registered deterministic test target and hosted Debug/Release workflow. No CMake/workflow changes are allowed. Require release-manifest integrity on the exact final head and a fresh independent Codex review of that exact head before merge. Native rendered equipment UI/playtesting remains not-run/not-claimed because this packet owns no platform or renderer path.

## Stop conditions

Stop rather than expanding scope if this slice requires Win32/platform input, renderer/editor/import/animation/audio/physics, shared build/CI, persistence-format work, another worker's open PR, architecture changes, dependencies, local scheduler execution, release, or deployment.
