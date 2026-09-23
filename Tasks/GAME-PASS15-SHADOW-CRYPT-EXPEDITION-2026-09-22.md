# GAME pass 15: Shadow Crypt expedition

Date: 2026-09-22 America/New_York
Owner: `animerpg-game-hourly`
Baseline: `977afadb2630bb3d25755d4407992bfab10018f8`
Target: `main`
Branch: `game/2026-09-22-shadow-crypt-pass15`

## Scope and ownership

Add one bounded game-domain Shadow Crypt expedition packet that consumes the already-merged exploration lead and progression systems. Preserve the custom C++17 Astral Engine, single persistent protagonist, current National Mall state, and all engine-worker boundaries. This packet does not alter renderer, platform, editor, import, animation, audio, physics, CMake, workflows, dependencies, R0, networking, release, deployment, or another worker's branches/PRs.

Allowed production path:
- `Engine/Scene/ShadowCryptExpedition.h`

Allowed verification/records paths:
- `Tests/ShadowCryptExpeditionPass15Tests.inc`
- `Tests/ThoughtCommandsTests.cpp` only as the smallest existing registered-test include/call shim while engine PR #13 owns shared CMake/editor verification work
- this task packet
- `Docs/Agents/animerpg-hourly/STATE.json`
- `Docs/Agents/animerpg-hourly/RUN-2026-09-22-PASS15.md`

## Research map

Sources were accessed/revalidated 2026-09-23 UTC. Comparator mechanics are design lessons only; no characters, maps, story, art, audio, code, live-service economy, or party architecture is copied.

1. `GAME-072` evidence-gated Shadow Crypt entry. The current Granblue Fantasy: Relink - Endless Ragnarok demo requires completion of three Quest Mode quests before its fourth quest unlocks, and current ZZZ content also uses explicit story/progression unlock conditions. Astral adapts the dependency lesson by requiring the already-merged `ShadowCryptLead` field operation to be complete before a run can start. Sources: https://store.playstation.com/en-us/product/UP5460-PPSA35115_00-GBRELINKERDEMO01 and https://zenless.hoyoverse.com/en-us/news/166000
2. `GAME-073` ordered room objectives with local retry. Granblue's current Quest Mode presents bounded individual quests, while ZZZ's official `Snap! Hollow Realm Showdown` event uses discrete themed combat stages and explicit main/additional targets. Astral adapts that clarity into four original rooms with room-specific objectives. A defeat resets only the active room's objective progress rather than erasing prior rooms. Sources: https://store.playstation.com/en-us/product/UP5460-PPSA35115_00-GBRELINKERDEMO01 and https://www.hoyolab.com/article/44074876
3. `GAME-074` optional Cooling Cache. ZZZ's current Version 3.2 update adds Hidden Space exploration commissions, container interactions, and valuables. Astral uses only the optional-exploration lesson, adding one original side cache that becomes available after two main rooms and is independently tracked/idempotent. Source: https://zenless.hoyoverse.com/en-us/news/166000
4. `GAME-075` bounded completion grade. ZZZ's official `Snap! Hollow Realm Showdown` distinguishes a main target from additional challenge targets, while established Shiyu Defense documentation uses tiered clear ratings. Astral uses an original Bronze/Silver/Gold summary driven only by optional-cache completion, defeats, and bounded damage; it does not copy ZZZ timings or rewards. Sources: https://www.hoyolab.com/article/44074876 and https://www.hoyolab.com/article/30690204
5. `GAME-076` replayable expedition plus one-time first-clear progression reward. ZZZ's official `Potential Hypothesis: New Chapter Reforged` permits repeated trial play while each stage reward can only be claimed once, and Granblue's current demo grants completion rewards for its quests. Astral allows Shadow Crypt replay while preserving one idempotent first-clear package through the existing `CharacterProgression` API. Sources: https://zenless.hoyoverse.com/en-us/news/161727 and https://store.playstation.com/en-us/product/UP5460-PPSA35115_00-GBRELINKERDEMO01
6. `QOL-016` explicit safe-suspend boundary and validated resume metadata. A 2026-02-19 Genshin player discussion shows substantial uncertainty about whether leaving a long quest will replay minutes of content; replies disagree on checkpoint frequency and note save indicators/checkpoints do exist. A 2026-03-31 ZZZ discussion similarly says story checkpoints exist but one commenter did not know of a clear checkpoint indication. This is anecdotal feedback, not consensus or proof either game currently lacks saving. Current ZZZ Version 3.2 also added an explicit early-retreat option while investigations are paused, and Granblue's 2026 demo documents automatic saving. Astral adapts only the usability lesson: expose whether the current room boundary is safe to suspend, create a versioned bounded resume point only at that boundary, and reject malformed/stale rewind attempts. This is in-memory domain state, not a claimed disk-save system. Community sources: https://www.reddit.com/r/GenshinImpactTips/comments/1r8nzba/will_it_restart_if_i_leave/ and https://www.reddit.com/r/ZenlessZoneZero/comments/1s8z8d2/can_i_quit_during_cutscenes/ . Current official context: https://zenless.hoyoverse.com/en-us/news/166000 and https://store.playstation.com/en-us/product/UP5460-PPSA35115_00-GBRELINKERDEMO01

## Acceptance criteria

- `GAME-072`: an incomplete field-guide lead cannot start Shadow Crypt; Crypt Sigil plus Shadow Crypt lore can; an active run cannot be started twice.
- `GAME-073`: four rooms advance only after exact room-specific progress requirements (2/3/2/1); defeat resets only the current room progress, preserves previously cleared rooms, and increments a bounded defeat counter.
- `GAME-074`: the Cooling Cache cannot be discovered before two rooms clear, can be discovered once, can be cleared once, and persists across a valid room-boundary resume point.
- `GAME-075`: completion is unranked before clear; Gold requires cache clear, zero defeats, and <=50 damage; Silver allows <=1 defeat and <=150 damage; other completed runs are Bronze; damage accounting ignores non-positive input and saturates safely.
- `GAME-076`: completion may be replayed; the first-clear reward calls existing `CharacterProgression::GrantRewards` once for 300 XP, 30 mastery points, and 40 enhancement materials; replay/resume cannot duplicate that reward.
- `QOL-016`: `SafeToSuspend()` is true only at active room boundaries; mid-objective snapshots are rejected; resume metadata is versioned and bounded; invalid schema/room/counters/cache relationships fail closed; an active expedition cannot be rewound over live state.

## Verification contract

Before merge:
1. Existing registered `ThoughtCommandsTests` must compile/run `ShadowCryptExpeditionPass15Tests.inc` in Debug and Release through hosted Windows CI. Existing full repository tests must remain green.
2. Release-manifest/integrity checks must pass on the exact final head if required by repository policy.
3. Cover incomplete-lead rejection, duplicate start rejection, each room requirement, local defeat retry, optional-cache gate/idempotency, Bronze/Silver/Gold boundaries, damage saturation, first-clear reward idempotency, replay behavior, checkpoint creation, replay reward preservation, malformed resume rejection, and stale active-run rewind rejection.
4. Obtain fresh independent Codex review on the exact final head. Self-review and hosted CI are separate evidence, not independent approval. Resolve every material thread before merge.
5. Re-read `main` and PR head immediately before merge. If `main` moved, reconcile and rerun affected checks. Merge only the exact reviewed/tested head.

Native dungeon rendering, room geometry, enemies, controller/menu wiring, animation, audio, GPU/performance evidence, persistent file serialization, cross-process load, and hands-on playtesting are outside this packet and must not be claimed.
