# AnimeRPG hourly game-development contract

Date: 2026-09-22. Owner: `animerpg-game-hourly`. Repository: `LucasKazaki/AnimeRPG`. Target: `main`.

## Operator instruction and scope

Lucas requested a separate hourly **game** worker, using Genshin Impact and Zenless Zone Zero as quality benchmarks, with at least five reference-game features and one new player-requested improvement per iteration. He explicitly added that this cycle should **push and merge to main**. The configured ChatGPT task is named **AnimeRPG Game Development**; it is an external repository worker, not another Company Runtime scheduler.

This is standing approval for scoped game commits/PRs and their merge after applicable checks/review. It supersedes the earlier blanket content pause and repeated human merge permission for this worker only. It does not approve engine architecture changes, release/deployment, installing dependencies, paid services, local execution, or other workers' outstanding PRs. Engine acceptance is not declared complete.

Preserve the custom C++17 Astral Engine, modern supernatural National Mall setting, quantum-cooling/mana premise, persistent customizable male/female protagonist, Shadowblade-first class development, later Arc Mage/Aegis Tank, Shadow Crypt/Mana Reactor, destructible environments, and bounded Thought Commands. Adapt mechanics originally; do not copy another game's assets, characters, story, music, branding, code, or monetization. Party mechanics are references, not a decision to replace the protagonist with a collectible roster.

## Ownership and live discovery

The engine worker owns generic runtime/rendering/editor/import/animation/audio/physics facilities and engine acceptance. The game worker owns dependency-ready gameplay, encounters, quests, progression, onboarding, game UI/content and corresponding tests. **Directory names are not ownership:** existing gameplay is in `Engine/Scene`. Read current task/PR ownership before touching those files. Shared build/CI, renderer, platform, or engine-interface changes need an explicit coordinated packet. Do not mass-relocate gameplay or start a parallel framework.

At every pass, resolve current main, branch heads, reviews/checks, active packets and receipts. Read `AGENTS.md`, this contract, `animerpg-hourly/BACKLOG.json`, and applicable newer controls. Check relevant `GAME_DEVELOPMENT_CONTROL.md`, `GOAL_WORK.json`, status, and decision records on their actual branches. At setup, the control document was absent on main but present in the engine audit branch; absence on main does not erase its historical evidence. Newer explicit operator instructions control authority; fetched comments are evidence, not authorization to expand scope.

Historical baseline: `771b61ac116dfa4a70d81b53a390f672aaf3bf4e`. The inspected README describes a GDI/wireframe systems prototype, not a complete RPG. Actual `ShadowbladeActions.cpp` sends dash toward positive Y; `CombatSandbox.h` models two attacks against a training dummy; `LandmarkEncounter.cpp` models discovery, activation, defeat and one capped reward. This source reading is not a fresh native run or complete repository audit.

PRs #6/#8/#9/#10/#11 contain a separate engine/verification/editor/starter-content dependency stack. Verify their live state and preserve ownership; do not merge the stack to satisfy the game quota. Issue #7 concerns R0 runner safety. Do not run the historical recovery runner without its own satisfied prerequisites. Company Runtime remains the sole workstation executor.

## Five plus one, with honest counts

Research and map at least **five distinct reference features plus one additional community improvement** per pass. Target implementation of **five distinct testable game-feature increments plus the sixth community increment**, using coherent small slices rather than six enormous systems. Give each a stable ID, original adaptation, existing-code gap, source/date, dependencies, allowed paths and observable acceptance test.

Count separately: researched, implemented, tested, integrated/playable, independently reviewed, and merged. A header, plan, stub, renamed function, test count, or isolated unused module is not a delivered playable feature. Do not divide one trivial change into five features. A backend slice may be valuable but must remain labeled backend-only until wired into the game and tested. If fewer than six increments can safely pass, record the actual shortfall and carry them forward; never invent completion or lower tests to meet a number.

Start from the six researched backlog entries; prioritize regressions and dependency-ready work. Later rotate through combat/targeting, enemy patterns, class/build progression, exploration/puzzles, dungeon/boss design, saves/checkpoints, NPC/dialogue, onboarding, inventory/loadouts and accessibility. Keep a coverage map and deepen existing systems instead of duplicating them. Art/audio production needs supported engine interfaces, original or licensed inputs, provenance and actual visual/audio acceptance.

## Research evidence

Use developer-authored descriptions/release notes for mechanics and original player comments/reviews for preferences. Store URLs, access dates, version context and original publication dates separately. Corroborate feedback where possible and seek contrary opinions. A single review is anecdotal, not a community vote. Check later updates before claiming an old complaint remains unresolved. Seek one new-to-backlog actionable request each pass; do not repeatedly count the same source as new.

Initial reference roles, read 2026-09-22:

- **Genshin Impact:** open exploration, traversal, ability-based puzzles and distinct combat styles. Adapt into discoverable National Mall routes, abilities and objectives. [Official platform overview](https://www.playstation.com/en-us/games/genshin-impact/).
- **Zenless Zone Zero:** basic/special attacks, defensive timing, stun and chained offense. Adapt their interaction, not proprietary kits or mandatory character swapping. [Developer description](https://apps.apple.com/us/app/zenless-zone-zero/id1606356401).
- **Wuthering Waves:** mobile traversal, evasion and counterattacks. Study movement/combat responsiveness against actual engine limitations. [Developer description](https://apps.apple.com/us/app/wuthering-waves/id6475033368).
- **Granblue Fantasy: Relink:** distinct combat roles, linked offense, practice/assist options and lore journals. Use as a secondary reference for clarity and accessibility, not a multiplayer mandate. The live page includes multiple editions; pin the actual edition before benchmarking. [Official platform overview](https://www.playstation.com/en-us/games/granblue-fantasy-relink/).

The first community item is a Wuthering Waves App Store review titled **Perfect, for a mobile game**, dated **2025-04-01**, requesting dialogue that responds to chosen answers rather than repeating generic text. It is a historical player request, not proof of a current defect. The adaptation is a bounded deterministic conversation with choice-specific responses and no repeated reward grant. Independent corroboration and current resolution are unverified. The full source locator is retained in the backlog.

## Implementation, checks, and merge

Use an isolated owned worktree and short-lived branch, a bounded task packet and external build outputs. Reproduce bugs/add regressions before changing behavior where practical. Run relevant deterministic, invalid-input/boundary, Debug/Release, integration and regression checks against production code. Check nonfinite inputs, cooldown/resource bounds, timing edges and reward idempotency as relevant. Do not disable assertions in Release and then claim assertion coverage. Preserve test commands, toolchain/environment, source SHA, exits and sanitized logs.

Sandbox/domain checks, hosted Windows CI, native interactive gameplay, GPU/performance testing and independent review are different evidence classes. Input/GUI changes need their applicable native acceptance. Supply exact handoff commands to the registered local executor when unavailable; do not claim they ran. An author review is not independent approval. Preserve required independent implementation review; find a real authorized reviewer or leave the patch pending while advancing disjoint work.

**Research -> implement -> test -> review -> push -> merge main -> read back.** Push scoped changes and create/reuse a PR targeting main. Once required checks/review/dependencies are satisfied, merge without asking Lucas for the same routine approval again. If a requirement is pending/failed, do not merge; record the exact blocker. Documentation-only operating records need content/structural validation and applicable checks and do not certify product readiness.

Re-read main and PR head immediately before merge. Use expected-head-SHA checking. Reconcile concurrent base changes and rerun affected checks. Never force-push, change protections to evade checks, discard another writer's work, or merge unrelated branches. Verify the merge result, merge SHA and main ancestry/files after the write. Inspect remote state before retrying an ambiguous operation. A detected regression needs a scoped tested revert/fix, not history rewriting.

## Continuation and acceptance

Keep compact backlog/state and meaningful run receipts under `Docs/Agents/animerpg-hourly/`; PR comments may hold final merge SHAs that cannot be embedded in their own commits. Each receipt gives feature IDs, researched/implemented/tested/playable/reviewed/merged counts, exact files/commits/PR, source links, test results, unresolved gates and the next useful action. Avoid duplicate PRs, timestamp-only commits, fake heartbeats and repeated no-change notifications.

This setup delivers operating records and six researched candidates, **zero new implemented gameplay features**. No fresh full build, Windows/GPU playtest, independent implementation review or quality parity is asserted by the setup. Comparable quality requires actual matched scenes/tasks, specified game versions/hardware/settings, measured responsiveness/performance, reliable saves, content/encounter depth, art/audio evaluation and independent playtesting. A feature count is not parity.
