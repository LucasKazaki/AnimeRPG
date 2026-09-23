# GAME pass 13: defense practice session depth

Date: 2026-09-22
Owner: `animerpg-game-hourly`
Target: `main`
Baseline: `dece8fbb47b6a937b9bafafa3faea837a359fb4d`
Branch: `game/2026-09-22-defense-session-pass13`

## Scope

Deepen the existing pass-11/pass-12 defense-practice gameplay domain without taking engine-worker ownership. This packet may add a thin game-specific practice-session coordinator around the existing `CombatDefenseTraining`, `CombatSandbox`, and `ShadowbladeActions` production domains. It may register regressions through an already registered gameplay test executable, but it must not edit CMake, workflows, renderer, platform, editor, import, animation, audio, physics, R0, release, deployment, or another worker's PR.

Allowed paths for this packet:
- `Engine/Scene/DefensePracticeSession.h`
- `Tests/DefensePracticePass13Tests.inc`
- `Tests/ThoughtCommandsTests.cpp` only as the smallest existing registered-test include/call shim because the open engine PR owns `CMakeLists.txt`
- `Tasks/GAME-PASS13-DEFENSE-SESSION-2026-09-22.md`
- `Docs/Agents/animerpg-hourly/STATE.json`
- `Docs/Agents/animerpg-hourly/RUN-2026-09-22-PASS13.md`

## Research map and feature IDs

All sources were accessed/revalidated on 2026-09-22.

### GAME-062: configurable practice pattern sequence
Reference: Zenless Zone Zero official HoYoLAB, `Data Bounty: Combat Simulation`, published 2026-07-20, https://www.hoyolab.com/article/45927809 . It explicitly describes selecting enemy cards in Combat Simulation.

Gap: pass 12 can directly request one known attack but cannot author and repeat a bounded multi-pattern drill.

Original adaptation: a maximum-six original Astral practice sequence using only existing QuickCut, GuardBreaker, and RiftBurst definitions. A successful queue advances the cursor; a rejected queue does not. The sequence loops deterministically and never advances the ordinary enemy rotation.

Acceptance: validate every pattern and length before mutation; queue order is deterministic; failed queue preserves cursor; normal combat values remain authoritative.

### GAME-063: practice pace presets without HP inflation
Reference: Granblue Fantasy: Relink PlayStation accessibility listing, https://www.playstation.com/en-us/games/granblue-fantasy-relink/ , documents adjustable difficulty and consequence-free Practice Mode.

Gap: the combat domain has recovery-aggression presets, but pass-12 practice does not expose a bounded training-oriented pace control.

Original adaptation: Learning/Standard/Expert practice pace maps to the existing Relaxed/Standard/Aggressive recovery cadence. It does not change enemy health, damage, guard damage, blockability, or windup definitions.

Acceptance: Learning and Expert alter only recovery cadence; canonical attack damage/blockability stay identical; invalid pace fails closed.

### GAME-064: alternating defense technique chain
Reference: Zenless Zone Zero combat challenge design uses technique-specific execution and combo timing as a scoring concept; the pass uses only the general lesson of rewarding varied execution, not copied attacks, content, or scoring values. Current event context was revalidated alongside official ZZZ combat-event material on 2026-09-22.

Gap: pass 12 counts guards and dodges independently but gives no bounded incentive to alternate defensive techniques.

Original adaptation: successful Guard/Dodge defenses build an alternating chain only when the authoritative defense result differs from the prior successful defense and arrives within 2.5 seconds of session-owned active practice time. Repeating the same defense, taking a hit, or an authoritative interruption resets the live chain while preserving the best chain. Successful defenses already resolved through `ShadowbladeActions` and reconciled on the next coordinator tick follow the same chain path as direct session input. Reconciliation timestamps an already-resolved defense at the pre-frame session time before the newly supplied coordinator delta is accumulated, so a long reconciliation frame cannot retroactively expire a defense that occurred before that frame.

Acceptance: Guard->Dodge->Guard reaches chain 3; Guard->Guard resets to 1; hit/interruption resets live chain; malformed raw input cannot fabricate a technique identity; pause cannot advance the timing clock; resetting the mutable combat clock cannot revive an expired chain; input-before-tick versus tick-after-input reconciliation produces the same successful chain accounting, including when the next coordinator frame exceeds the 2.5-second chain window.

### GAME-065: practice score with time coefficient
Reference: Zenless Zone Zero timed combat challenges combine execution objectives with time pressure; official current combat-event material was revalidated on 2026-09-22. The local values below are original and intentionally small.

Gap: pass 12 has a coarse grade but no score report that separates execution points from a time coefficient.

Original adaptation: perfect defenses, ordinary defenses, and best alternating chain contribute bounded base points. Average active session seconds per resolved attempt applies a 125%, 100%, or 75% coefficient. Hits grant no positive base points but still contribute to the timing denominator. The session accumulates only valid, unpaused time passed through its own coordinator, so a direct `CombatSandbox` clock reset cannot rewind an already-earned coefficient.

Acceptance: no resolutions score zero; hit-only reports still expose the defined timing coefficient while keeping zero points; clean fast practice gets the high coefficient; advancing valid unpaused time can lower only the coefficient; paused time does not change it; a direct combat clock reset cannot improve the coefficient; integer/time accumulation stays bounded.

### GAME-066: configurable drill target count
Reference: ZZZ official HoYoLAB `Snap! Hollow Realm Showdown`, published 2026-03-06, https://www.hoyolab.com/article/44074876 , documents main and additional challenge targets plus an objective-free mode after completion. A later official `Snap! Focus Showdown!` event on 2026-08-05 likewise documents six themed stages and objective-free Hyperfocus Shot mode, https://www.hoyolab.com/article/46150564 .

Gap: pass 12 hard-codes three successes for every non-free drill goal.

Original adaptation: preserve the existing goal categories but let practice choose 1 through 10 required successes. Free Practice remains objective-free.

Acceptance: target bounds are enforced; existing goal metrics remain authoritative; completion changes at the configured threshold; invalid targets do not mutate configuration.

### QOL-014: per-pattern practice analytics
Community sources: Wuthering Waves player discussion `This game really need a training room system`, posted 2024-09-11, https://www.reddit.com/r/WutheringWaves/comments/1feehnl , asks for training-room toggles and a DPS calculator; a separate 2025-01-27 survey discussion, https://www.reddit.com/r/WutheringWaves/comments/1ib0gu2 , asks for a training room and DPS meter; a 2026-08-02 discussion, https://www.reddit.com/r/WuWaves/comments/1vd8ctk , asks for detailed testing statistics and includes disagreement about dummy-only practice. These are player anecdotes, not consensus, and current full resolution is not established.

Gap: pass 12 exposes aggregate defense telemetry but cannot answer which attack pattern is causing failures.

Original adaptation: maintain bounded QuickCut/GuardBreaker/RiftBurst attempt, perfect, ordinary, hit, interruption, and damage-taken counters. This is a private single-player practice parser, not global competitive telemetry.

Acceptance: each queued pattern increments only its own attempts; terminal outcome updates exactly once; damage stays associated with the missed pattern; reset is rejected while an owned attack is live and clears metrics afterward without changing practice configuration.

## Verification

Required before merge:
1. Exact-head hosted Windows Debug and Release build/test workflow must pass with the new tests registered through the existing `ThoughtCommandsTests` target. This target already links `CombatSandbox.cpp`, `ShadowbladeActions.cpp`, and the scene headers needed by the session coordinator; CMake is intentionally untouched because open engine PR #13 owns that shared path.
2. Release-manifest/integrity workflow must pass on the same final head when triggered by repository policy.
3. Boundary checks must cover invalid sequence/preset/target values, rejected queues, pause behavior, reset idempotency, per-pattern isolation, deterministic scoring, hit-only coefficient reporting, combat-clock-reset invariance for score and chain timing, direct-`ShadowbladeActions` successful-defense reconciliation, and a pre-resolved Guard/Dodge reconciliation followed by a coordinator frame longer than the chain window.
4. Fresh independent Codex review must inspect the exact final candidate head. Material findings must be repaired and all applicable threads resolved before merge.
5. Re-read `main`, PR head, changed paths, checks, and review immediately before merge. Use expected-head protection and do not merge if the base moved without reconciliation.

Native rendered UI, audio cues, animation timing, controller/menu wiring, GPU behavior, performance, and hands-on playtesting are not part of this backend packet and must not be claimed.

## Stop conditions

Stop or leave the PR unmerged if the changes require CMake/shared-engine ownership, if exact-head required checks fail, if independent review remains unresolved, if current `main` introduces a conflicting gameplay owner, or if a native/engine dependency becomes necessary for correctness. Do not merge another worker's PR to clear this packet.
