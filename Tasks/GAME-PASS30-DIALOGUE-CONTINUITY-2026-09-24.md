# GAME pass 30: dialogue continuity and interruption recovery

Owner: `animerpg-game-hourly`  
Target: `main`  
Baseline: `332290bd9ba5f019aff182d74bf2d26eda7b681c`  
Branch: `game/2026-09-24-dialogue-continuity-pass30`

## Scope and ownership

This is a bounded GAME-domain packet. It deepens the existing `LandmarkDialogue` / `LandmarkInteraction` production path without changing Astral Engine renderer, platform, editor, importer, animation, audio, physics, shared CMake/workflows, dependencies, R0, networking, deployment, release, or another worker's PRs.

Allowed paths:
- `Engine/Scene/LandmarkDialogueContinuity.h`
- thin integration in `Engine/Scene/LandmarkInteraction.h`
- `Tests/LandmarkDialogueContinuityPass30Tests.inc`
- test registration only in `Tests/ThoughtCommandsTests.cpp`
- this task and `Docs/Agents/animerpg-hourly/` pass records

## Research mapping, accessed 2026-09-24

Official/developer sources:
- Genshin Impact Version 1.4 update notice, published 2021-03-15: https://www.hoyolab.com/article/244378. It introduced permanent Hangout Events behind adventure/prerequisite gates.
- Zenless Zone Zero Version 1.4 update details, published 2024-12-17: https://www.hoyolab.com/article/35654082. It documents Trust Events and Quality Time, including Trust prerequisites and first-completion Trust/reward progression.
- Granblue Fantasy: Relink developer interview, published 2024-01-30: https://blog.playstation.com/2024/01/30/granblue-fantasy-relink-devs-discuss-crafting-an-immersive-rpg-world-for-ps5-ps4-out-feb-1/. Cygames describes multi-part Fate Episodes that reveal backstory and deepen character relationships.
- Granblue Fantasy: Relink PlayStation overview: https://www.playstation.com/en-us/games/granblue-fantasy-relink/. It documents Lyria's journal and Fate Episodes as lore/backstory surfaces.

Supporting player-authored Genshin Hangout documentation, used only for the branch/revisit lesson rather than as developer authority:
- https://www.hoyolab.com/article/274907, published 2021-03-30.
- https://www.hoyolab.com/article/279894, published 2021-04-03.

Community improvement source:
- ZZZ player report, published 2026-01-02: https://www.reddit.com/r/ZenlessZoneZero/comments/1q1qhmg/is_it_just_me_or_does_anyone_has_experience_their/. The author reports completed dialogue replaying when returning near its trigger; replies independently report similar repetition in current and earlier story content.
- Independent related interruption complaint, published 2025-07-17: https://www.reddit.com/r/ZenlessZoneZero/comments/1m2i08y. The author reports exploration/combat progression cutting off dialogue unless the player stops moving.
- Additional high-engagement corroboration, published 2026-01-17: https://www.reddit.com/r/ZenlessZoneZero/comments/1qfocdw/please_no_more_story_voicelines_during_combat/.

These player posts are anecdotes and corroborated preferences, not consensus. Inspected later official update material did not establish a specific fix for repeated/retriggered or interrupted exploration dialogue, so current comparator resolution is recorded as unestablished rather than claimed broken.

## Six bounded increments

### GAME-147: relationship-tier readout
Gap: existing trust is numeric and bounded but has no stable relationship-state surface for later authored content.
Adaptation: derive `Wary`, `Acquainted`, or `Confidant` directly from authoritative `LandmarkDialogue::Trust()` without a second mutable trust counter.
Acceptance: negative, neutral/low-positive, and high-positive trust map deterministically; no trust mutation occurs while reading the tier.

### GAME-148: per-topic substantive checkpoint memory
Gap: the existing bounded history is chronological but does not expose a stable last substantive branch point for each topic.
Adaptation: record the newest accepted, non-preview, non-redundant substantive beat per topic with a monotonic sequence witness.
Acceptance: first substantive beat is retained; preview, close, and exhausted-repeat beats cannot overwrite it; invalid topic queries fail closed.

### GAME-149: idempotent ending archive
Gap: `CommitOutcome()` is authoritative and idempotent but there is no separate read-only archive surface for completed story outcomes.
Adaptation: archive only the exact committed authoritative outcome, one time per outcome identity.
Acceptance: `None`, mismatched, and duplicate commits cannot create archive entries; repeated production commits do not duplicate counts.

### GAME-150: authored story-episode prerequisites
Gap: clues, lore, trust, and outcomes exist independently but later character-story slices have no bounded combined prerequisite surface.
Adaptation: original `MallWitness`, `RiftConfidant`, and `CryptPartner` episode flags derive monotonically from existing lore/clue/trust/outcome evidence.
Acceptance: episodes do not unlock without their authoritative evidence; qualifying evidence unlocks once; invalid episode IDs fail closed.

### GAME-151: non-mutating checkpoint revisit
Gap: players cannot inspect a prior substantive dialogue branch without invoking `Choose()` and risking story mutation.
Adaptation: expose the saved beat as a read-only revisit with its original sequence and explicit `mutatesStory=false` contract.
Acceptance: revisiting does not change trust, discussed topics, lore, clues, outcome, or dialogue history.

### QOL-031: interruption-safe latest-beat recovery
Gap: the current dialogue domain can retain history, but the game owner has no explicit handoff for a beat that was accepted and then visually/audio-interrupted before the player consumed it.
Adaptation: capture the latest accepted beat once, preserve the original sequence through repeated interruption calls, and require an explicit exact-once acknowledgement to clear it.
Acceptance: no beat means no capture; a pending recovery cannot be silently overwritten; acknowledgement clears once; the recovery path never calls `Choose()` and therefore cannot replay story mutation.

## Verification requirements

The registered `ThoughtCommandsTests` aggregation must compile and execute pass-30 production-owner regressions in Debug and Release. Required coverage includes production `LandmarkInteraction` wrappers, checkpoint overwrite boundaries, read-only revisit, exact-once interruption recovery, authoritative ending archive behavior, relationship thresholds, episode prerequisites, invalid enum handling, and compatibility with all previously registered pass tests.

Exact-final-head acceptance requires:
1. hosted Windows Debug/Release deterministic workflow success,
2. hosted Release-manifest integrity success,
3. fresh independent implementation review on that exact head with no unresolved material findings,
4. full diff/scope review and live `main`/PR-head reread immediately before expected-head merge.

Native rendered dialogue UI, controller/menu input, voice/audio timing, GPU/performance evidence, and cross-process narrative persistence are not implemented by this packet and must not be claimed.
