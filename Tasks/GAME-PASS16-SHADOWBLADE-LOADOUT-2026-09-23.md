# GAME pass 16: Shadowblade loadout and equipment management

Date: 2026-09-23 America/New_York
Owner: `animerpg-game-hourly`
Baseline: `977c5359491b24f729db6886af9b0fef35cea872`
Target: `main`
Branch: `game/2026-09-23-shadowblade-loadout-pass16`

## Scope and ownership

Add one bounded game-domain loadout packet for the persistent Shadowblade protagonist. Preserve the custom C++17 Astral Engine, existing progression/combat/exploration contracts, and the separate engine/art workers. This packet does not change renderer, platform, editor, import, animation, audio, physics, CMake, workflows, dependencies, R0, networking, release, deployment, or another worker's PR.

Allowed production path:
- `Engine/Scene/ShadowbladeLoadout.h`

Allowed verification/records paths:
- `Tests/ShadowbladeLoadoutPass16Tests.inc`
- `Tests/ThoughtCommandsTests.cpp` only as the smallest existing registered-test include/call shim
- this task packet
- `Docs/Agents/animerpg-hourly/STATE.json`
- `Docs/Agents/animerpg-hourly/RUN-2026-09-23-PASS16.md`

No shared engine path is admitted by this packet.

## Research map

Sources were accessed/revalidated 2026-09-23. Comparator mechanics are design lessons only. No characters, proprietary item names, art, code, economy, monetization, or collectible-character architecture are copied.

1. `GAME-077` equippable weapon frames with progression eligibility. ZZZ Version 1.7 explicitly added new W-Engines and treats them as Agent equipment; Granblue Fantasy: Relink's official updates also document weapon upgrade/uncap item flows. Astral adapts only the readable equipment-choice lesson into three original Shadowblade weapon frames, one equipped at a time, with ownership and protagonist-level gates. Sources: https://www.hoyolab.com/article/38380449 and https://relink.granbluefantasy.jp/en/updates
2. `GAME-078` four typed resonance-module slots. ZZZ's official update notes repeatedly treat Drive Discs as equipment with filtering/display management. Astral uses four original protagonist module slots (`Edge`, `Ward`, `Flow`, `Insight`) with strict slot compatibility and ownership rather than copying ZZZ's six-slot layout. Sources: https://www.hoyolab.com/article/36519625 and https://www.hoyolab.com/article/38380449
3. `GAME-079` bounded equipment-family resonance bonuses. Genshin Version 5.5 officially documents 2-piece/4-piece Artifact set effects. Astral adapts the set-synergy lesson into original two-or-more-module family bonuses for Cooling, Rift, and Civic equipment, with a deterministic bounded profile. Source: https://www.hoyolab.com/article/37843579
4. `GAME-080` saved loadout switching. Genshin's official June 3, 2025 Developers Discussion states that Version 5.7 Fast Equip can save and switch Custom Loadouts. Astral adds three protagonist-local saved loadout slots that snapshot the currently equipped weapon and modules and apply only after full validation. Source: https://www.hoyolab.com/article/39120584
5. `GAME-081` explicit salvage of unused modules into bounded tuning parts. ZZZ Version 1.7 officially expanded Drive Disc discard handling, and Genshin Version 5.5 documents Artifact Salvage quick-selection behavior. Astral adapts only the inventory-cleanup lesson: an owned unequipped resonance module can be salvaged once for bounded tuning parts, while equipped gear is protected. Sources: https://www.hoyolab.com/article/38380449 and https://www.hoyolab.com/article/37843579
6. `QOL-017` exact-item preset fidelity with atomic failure. A March 5, 2026 ZZZ community request asks for Drive Disc presets, optionally paired with Engines, so players can toggle exact builds instead of manually moving shared gear. Independent commenters describe the same desire for multiple builds on one character. Current official resolution of that exact request was not established in this pass. Astral stores exact original equipment IDs in each saved preset and, if a saved item is later salvaged/missing or level-locked, refuses the switch without partially mutating the current loadout or substituting another item. Community source: https://www.reddit.com/r/ZZZ_Official/comments/1rldm8a/request_drive_disc_presets/

## Acceptance criteria

- `GAME-077`: Training Blade starts owned/equipped; unowned or malformed weapon IDs fail closed; Riftsteel Sabre and Cryo Edge require ownership plus levels 8 and 14 respectively; failed equips do not replace the current weapon.
- `GAME-078`: modules require ownership and the exact compatible slot; invalid slot/module IDs fail closed; replacement affects only the target slot; unequip is explicit.
- `GAME-079`: two or more equipped modules from a family activate that family's original bounded bonus exactly once; mixed singletons do not; readiness is deterministic and clamped to 0..100.
- `GAME-080`: three preset slots can save and restore exact equipped state; empty/out-of-range presets fail without mutation; a valid apply is atomic.
- `GAME-081`: equipped modules cannot be salvaged; unequipped owned modules can be salvaged once; duplicate salvage grants nothing; tuning parts stay within the bounded maximum.
- `QOL-017`: presets retain exact equipment identity. If a saved module is salvaged after the snapshot, applying that preset returns `MissingPresetItem` and leaves weapon, module occupancy, module IDs, tuning parts, and derived profile unchanged. No fallback module is selected.

## Verification contract

Before merge:
1. Existing registered `ThoughtCommandsTests` must compile/run `ShadowbladeLoadoutPass16Tests.inc` in hosted Windows Debug and Release, and the existing repository suite must remain green.
2. Release-manifest/integrity checks must pass on the exact final head if required by repository policy.
3. Cover ownership/level gates, malformed enum values, wrong-slot rejection, family-bonus thresholds, profile bounds, valid save/switch, empty/out-of-range presets, equipped-salvage rejection, one-time salvage, and stale exact-item preset atomicity.
4. Obtain fresh independent Codex review on the exact final head. Self-review and hosted CI are separate evidence. Resolve material findings before merge.
5. Re-read `main` and PR head immediately before merge. If `main` moved, reconcile and rerun affected checks. Merge only the exact reviewed/tested head.

Native equipment UI, inventory menus, rendered icons/models, controller/menu wiring, combat-stat application, disk-save persistence, cross-process load, GPU/performance evidence, art/audio, and hands-on native playtesting are outside this packet and must not be claimed.
