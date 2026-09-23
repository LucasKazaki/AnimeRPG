# GAME Pass 26: Live Mana Reactor mission

Date: 2026-09-24
Owner: `animerpg-game-hourly`
Base main: `43868c319e8253e146a29d9b3771d545e2c95f99`
Target: `main`
Branch: `game/2026-09-24-mana-reactor-live-pass26`

## Scope

Passes 21 and 22 created and deepened `ManaReactorExpedition`, but the subsystem remained test-created rather than owned by a production game object. This packet closes that specific gap by attaching the existing expedition to `LandmarkInteraction`, which already owns National Mall exploration evidence, dialogue, and persistent protagonist progression. It does not edit renderer, platform/input, editor, importer, animation, audio, physics, generic engine timing, CMake/workflows, dependencies, networking, deployment, release, or another worker's PR.

Allowed paths:
- `Engine/Scene/ManaReactorMission.h`
- `Engine/Scene/LandmarkInteraction.h`
- `Tests/ManaReactorMissionPass26Tests.inc`
- `Tests/ThoughtCommandsTests.cpp`, registration only
- this task packet
- `Docs/Agents/animerpg-hourly/PASS26-BACKLOG.json`
- `Docs/Agents/animerpg-hourly/RUN-2026-09-24-PASS26.md`
- `Docs/Agents/animerpg-hourly/STATE.json`

## Research and feature map

Sources accessed 2026-09-24. Reference games supply interaction lessons only. No story, characters, maps, art, music, code, monetization, or roster architecture is copied.

### GAME-127: production-owned evidence-gated Mana Reactor launch

Reference: Zenless Zone Zero official `Combat Training - Triple Bounty`, published 2026-08-31, https://zenless.hoyoverse.com/m/en-us/news/165921 . It requires the player to unlock Combat Simulation through Main Story Chapter 1 before entering the challenge.

Repository gap: `ManaReactorExpedition::TryBegin` already required completed `RiftInvestigation`, but only tests instantiated the expedition. No production game owner could start it from the actual field-guide evidence path.

Original adaptation: `LandmarkInteraction` owns a `ManaReactorMission` and starts it only with its own existing `ExplorationFieldGuide`. Real landmark visits plus existing dialogue evidence can satisfy the prerequisite. No second quest flag is created.

Acceptance: locked before evidence; real landmark/dialogue evidence completes the existing gate; valid production launch starts Intake Bay; invalid mode/difficulty/protocol remains fail-closed through the existing expedition validator.

### GAME-128: authoritative control briefing and recommendation

References: the same ZZZ notice lets players select an enemy card before a challenge; Granblue Fantasy: Relink's official PlayStation page documents Control Reminders and Tutorial Reminders, https://www.playstation.com/en-us/games/granblue-fantasy-relink/ .

Repository gap: pass 22 added exact `PreviewControl`, but no live owner presented all choices or a bounded recommendation without mutating the run.

Original adaptation: a read-only briefing exposes the three existing control previews and selects one deterministic safe recommendation using only projected objective progress, stability, heat, and stage-completion state. It also surfaces whether the existing emergency vent is worth considering at high heat.

Acceptance: all three previews are visible when active; reading briefing cannot mutate heat/stability/progress; failed projections are never recommended; recommendation is deterministic.

### GAME-129: explicit stage-failure recovery

References: Genshin Impact official Stygian Onslaught notice, published 2026-01-19, https://www.hoyolab.com/article/43362737 , describes a challenge made of three sequential phases; Granblue Fantasy: Relink documents consequence-free Practice Mode.

Repository gap: pass 21 had `RetryCurrentStage`, but a production game owner did not expose which recovery action applied after failure.

Original adaptation: failed reactor state reports `RetryStage`, then delegates the actual reset to the existing current-stage retry contract so cleared stages remain intact.

Acceptance: Core failure recommends stage retry; retry clears failure and stage-local state while retaining previously cleared stage count; completed runs never masquerade as failed-stage retries.

### GAME-130: per-configuration performance records

Reference: Zenless Zone Zero official `Virtual Shadow Chasing Front` event description, published 2026-09-14, https://zenless.hoyoverse.com/m/zh-tw/news/166073 . Its Special Assault stages use a score built from damage, technique execution and a time factor, and players can earn higher evaluations.

Repository gap: `ManaReactorExpedition` kept one global best Expedition score, which cannot tell the player how a specific difficulty/protocol configuration performed.

Original adaptation: the live mission retains a fixed 12-entry in-memory best-record table, one for each three-difficulty by four-protocol combination, using the existing authoritative completion score and grade. No online leaderboard or persistence claim is introduced.

Acceptance: completed run records score/grade in the correct bucket; another difficulty/protocol bucket remains independent; lower replay results cannot erase a better result; invalid enum keys fail closed.

### GAME-131: live one-time first-clear progression claim

Reference: ZZZ `Combat Training - Triple Bounty` grants challenge rewards; the 2026-09-14 ZZZ challenge notice also awards materials/currency for event challenge tasks.

Repository gap: pass 21's one-time Mana Reactor progression entitlement existed only on the isolated expedition API.

Original adaptation: `LandmarkInteraction` routes a completed live mission to the existing `CharacterProgression::ClaimManaReactorFirstClearReward`. No new currency or reward amount is added.

Acceptance: first Expedition clear grants the existing 350 XP / 35 mastery / 50 material package; repeat claim cannot duplicate it; null progression owner returns no reward; Calibration retains its existing no-reward rule.

### QOL-027: same-configuration replay from completion

Community sources: r/Genshin_Impact `Why does no one use the retry domain option?`, posted 2020-12-29, https://www.reddit.com/r/Genshin_Impact/comments/km30lp/ ; and `Please stop exiting the the domain only to que up for it again in co-op`, posted 2021-06-28, https://www.reddit.com/r/Genshin_Impact/comments/o9ufu0/ . Players discussed friction from leaving and re-entering Domains and noted that a retry option already existed but was easy to miss or awkward in co-op. This is historical UI-flow feedback, not evidence that current Genshin still has the same problem.

Repository gap: the reactor could `StartOver`, but a production completion flow had no explicit direct replay action or record-preservation contract.

Original adaptation: completed runs expose `ReplayRun` and restart the exact prior mode/difficulty/protocol in one action while preserving personal-best records and the protagonist's already-consumed first-clear entitlement. Replay is rejected while a run is active.

Acceptance: completion briefing offers replay; replay returns to Intake Bay at zero run progress with identical configuration; best record remains; repeated replay while active fails closed.

## Verification plan

Registered regression coverage is added through the existing `ThoughtCommandsTests` aggregation target. Before merge, require exact-head hosted Windows Debug/Release registered tests, exact-head Release-manifest integrity, fresh independent Codex review on the same head, full diff/ownership audit, live-main reconciliation, and expected-head merge.

Boundary checks include invalid enum record keys, locked entry, real evidence gating, non-mutating briefing, failure/retry retention, per-configuration record isolation, first-clear idempotency, and active-run replay rejection.

Local sandbox compilation is not claimed because the container cannot currently resolve `github.com` for a clean clone. Hosted CI is the product build evidence for this packet.

## Evidence boundary

This pass makes the Mana Reactor reachable through a production game-domain owner API, but current native Win32 input/UI still does not invoke these new methods. Therefore hosted/domain integration can be counted if green, while native-player-playable verification remains zero. No rendered reactor UI/scene, controller/keybinding route, production geometry/VFX/audio, cross-process mission persistence, native GPU/performance capture, or hands-on playtest is claimed.
