# GAME pass 14: field operations and exploration guide

Date: 2026-09-22 America/New_York
Owner: `animerpg-game-hourly`
Baseline: `1042339ee7052fcbff60b1b8bba6840b43019c4e`
Target: `main`
Branch: `game/2026-09-22-field-guide-pass14`

## Scope and ownership

Add one bounded game-domain exploration packet for the existing National Mall prototype. Preserve the custom C++17 Astral Engine, single persistent protagonist, existing landmark objective, dialogue, rewards, and all engine-worker boundaries. Do not alter renderer, platform, editor, import, animation, audio, physics, CMake, workflows, dependencies, R0, release, deployment, networking, or other workers' branches/PRs.

Allowed production paths:
- `Engine/Scene/ExplorationFieldGuide.h`
- `Engine/Scene/LandmarkInteraction.h`
- `Engine/Scene/LandmarkInteraction.cpp`

Allowed verification/records paths:
- `Tests/ExplorationFieldGuidePass14Tests.inc`
- `Tests/ThoughtCommandsTests.cpp` only as the smallest existing registered-test include/call shim while open engine PR #13 owns CMake
- this task packet
- `Docs/Agents/animerpg-hourly/STATE.json`
- `Docs/Agents/animerpg-hourly/RUN-2026-09-22-PASS14.md`

## Research map

Sources were accessed/revalidated 2026-09-23 UTC. Reference games provide design lessons only. No characters, maps, story, art, audio, code, economies, or monetization are copied.

1. `GAME-067` field-operation tracking. Zenless Zone Zero official event details for `Dangerous Fugitive's Leisurely Vacation`, event dates 2026-08-24 through 2026-09-07, describe daily-unlocked event commissions and completing those commissions for rewards: https://www.hoyolab.com/article/46388581 . Astral adapts only the idea of a small explicit operation board, with original National Mall objectives and no live-service cadence/reward schedule.
2. `GAME-068` free-order landmark survey chronology. Current Genshin Impact PlayStation material describes freely exploring an open world and uncovering its mysteries: https://www.playstation.com/en-us/games/genshin-impact/ . Current Wuthering Waves App Store material likewise emphasizes high-freedom overworld exploration and hidden truths: https://apps.apple.com/us/app/wuthering-waves/id6475033368 . Astral records the player's actual first-discovery order instead of forcing a party/character structure.
3. `GAME-069` evidence-driven investigation progress. The same official ZZZ 2026 fugitive event tells players to provide clues on the wanted criminal before/alongside event commissions. Astral maps authoritative existing dialogue clues into a bounded Rift Investigation and Shadow Crypt lead, without importing ZZZ narrative content.
4. `GAME-070` bounded field journal. Granblue Fantasy: Relink's current PlayStation page documents Lyria's journal for people, places, and history: https://www.playstation.com/en-us/games/granblue-fantasy-relink/ . Astral adds five original National Mall/evidence note flags driven by actual discoveries.
5. `GAME-071` context target recommendation. Genshin Impact Official's 2025-04-23 Developers Discussion documents Treasure Compass upgrades that locate Warrior's Challenges: https://www.hoyolab.com/article/38381823 . Astral adapts the discoverability lesson as a simple next-unvisited-site recommendation for its three-site survey, not a copied compass or map system.
6. `QOL-015` automatically reconcile a player-pinned field target when its site is discovered. A Genshin player thread dated 2026-05-02 asks for the official interactive map to sync with account exploration progress, with replies also preferring exploration help inside the game: https://www.reddit.com/r/Genshin_Impact/comments/1t1vjc0/interactive_map_sync/ . This is anecdotal player feedback, not consensus. HoYoLAB's existing Interactive Map guide documents manual synchronization of in-game pins to the web map, so the player request is specifically broader progress synchronization, not evidence that all pin sync is absent: https://www.hoyolab.com/article/17673509 . Astral implements only the local usability lesson: a temporary manual target clears itself when the matching site is actually discovered.

## Acceptance criteria

- `GAME-067`: three valid field operations expose bounded current/required progress; tracking a valid operation is explicit; invalid enum values fail closed without changing the tracked operation.
- `GAME-068`: first discoveries of Lincoln Memorial, Reflecting Pool, and Washington Monument are recorded once in actual discovery order; repeats do not grow the fixed ledger.
- `GAME-069`: existing `LandmarkDialogue` clue/lore truth is synchronized monotonically into investigation progress; a stale/lower snapshot cannot erase already observed evidence; Shadow Crypt completion requires both its clue and matching lore.
- `GAME-070`: first site observations unlock three site notes; Rift evidence and the Shadow Crypt lead unlock only from the corresponding evidence; journal capacity stays fixed at five and repeated inputs are idempotent.
- `GAME-071`: Mall survey tracking recommends the first unvisited site in the existing Lincoln -> Reflecting Pool -> Washington Monument route, skips visited sites, and exposes no phantom target after all three are observed.
- `QOL-015`: a player can pin an unvisited landmark as a temporary target; the pin overrides automatic recommendation; discovering it clears the pin exactly once; already visited and invalid landmarks cannot become unfinished pins.
- Production integration: first-time `LandmarkInteraction::TryInteract` discoveries feed the field guide, while dialogue-choice/outcome wrappers synchronize exact existing clue/lore state. Existing landmark objective/reward behavior remains unchanged.

## Verification contract

Before merge:
1. Registered `ThoughtCommandsTests` must compile/run the pass-14 regression include in both Debug and Release through existing hosted Windows CI. Existing `LandmarkInteractionTests` and full repository build/tests must remain green.
2. Release-manifest/integrity checks must pass on the exact final head if required by repository policy.
3. Check invalid operation/landmark enum handling, duplicate discovery idempotency, stale evidence monotonicity, completed-route termination, pin precedence/auto-clear, and existing landmark interaction regressions.
4. Obtain a fresh independent Codex review on the exact final head. Self-review and hosted CI are not independent approval. Resolve all material threads before merge.
5. Re-read `main` and PR head immediately before merge. If `main` moved, reconcile and rerun affected checks. Merge only with expected-head protection.

Native rendered map/quest UI, controller/menu wiring, save-file persistence, production art/audio, performance/GPU evidence, and hands-on playtesting are not part of this packet and must not be claimed.
