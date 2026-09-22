# GAME PASS 11: defense integration and practice feedback

Date: 2026-09-22
Owner: `animerpg-game-hourly`
Base: `main` at `b3a2b1bf8f2b0c356d5b352006c48cb86532426b`
Target: `main`

## Scope

Bridge the already-merged pass-10 enemy attack planner into the already-merged Shadowblade incoming-attack/defense state without changing renderer, platform, input, editor, CMake, workflows, dependencies, R0, networking, deployment, release, or another worker's branches. This remains game-domain integration and registered deterministic-test work. Native HUD/input/audio/animation integration is not claimed.

Allowed paths:
- `Engine/Scene/ShadowbladeActions.h`
- `Engine/Scene/ShadowbladeActions.cpp`
- `Engine/Scene/CombatDefenseTraining.h`
- `Tests/ShadowbladeActionsTests.cpp`
- `Tasks/GAME-PASS11-DEFENSE-INTEGRATION-2026-09-22.md`
- `Docs/Agents/animerpg-hourly/STATE.json`
- `Docs/Agents/animerpg-hourly/RUN-2026-09-22-PASS11.md`

`ShadowbladeActions.cpp` is admitted only for the independent-review repair that assigns a generation to each successfully queued incoming threat. No unrelated action/combat behavior may change there.

## Five reference-derived increments plus one community increment

### GAME-052: enemy-plan to Shadowblade threat bridge
Gap: pass 10 creates validated `EnemyAttackPlan` values, but they do not currently enter `ShadowbladeActions::BeginIncomingAttack`.
Adaptation: add a bounded game-domain coordinator that converts the currently queued enemy plan into the existing Shadowblade incoming threat contract exactly once.
Acceptance: queuing through the coordinator creates both one combat pending attack and one Shadowblade threat with matching windup/damage/guard/blockability; duplicate queueing is rejected.
Reference: Zenless Zone Zero's action-combat emphasis and Wuthering Waves' explicit enemy-attack evasion/counter loop.

### GAME-053: defense outcome round-trip
Gap: Shadowblade guard/dodge outcomes currently do not consume the pass-10 enemy planner state.
Adaptation: map terminal Shadowblade results back to existing `EnemyAttackOutcome` values. Too-early/no-threat inputs do not consume the plan. Perfect defense must preserve both systems' earned follow-up semantics.
Acceptance: Guarded/Evaded/PerfectDefense/Hit outcomes consume one planner event and start its existing recovery exactly once; too-early dodge preserves it.
Reference: Wuthering Waves documents Extreme Evasion and Dodge Counter; Granblue Relink documents stun/follow-up opportunities as explicit combat-state transitions.

### GAME-054: automatic impact synchronization
Gap: `ShadowbladeActions::AdvanceTime` can auto-resolve an expired threat, leaving the enemy planner pending unless an external caller manually reconciles it.
Adaptation: the coordinator advances both existing clocks and closes the matching enemy plan when the existing Shadowblade threat resolves automatically.
Acceptance: an unattended QuickCut damages the player once, closes the planner event once, records the hit once, and respects planner recovery.
Reference: fast action-RPG telegraph-to-impact combat timing from ZZZ/Wuthering Waves.

### GAME-055: interruption synchronization
Gap: pass-10 stagger can clear a queued enemy plan while the separate Shadowblade threat remains active.
Adaptation: a linked threat is canceled when its authoritative combat plan is interrupted/cleared, so a staggered or defeated enemy cannot land a stale delayed hit.
Acceptance: after the linked combat plan is interrupted, the corresponding Shadowblade threat is canceled before its clock can reach impact, including delayed reconciliation beyond the former remaining windup. The coordinator must identify the exact `ShadowbladeActions` instance and threat generation it owns so a replacement or unrelated standalone threat is never canceled or synchronized.
Reference: Granblue Relink documents stun gauges creating attack opportunities; this adapts interruption consistency to the single-protagonist system.

### GAME-056: defense drill telemetry and grade
Gap: training metrics cover outgoing damage but not defensive execution.
Adaptation: record linked attacks queued, defense inputs, perfect defenses, ordinary defenses, hits, interruptions, damage taken, current/best perfect streak, and a bounded None/Bronze/Silver/Gold drill grade after enough resolved attacks.
Acceptance: invalid/too-early inputs never fabricate successful defenses; counters are bounded; three clean perfect resolutions can earn Gold; damage lowers the outcome deterministically.
Reference: Granblue Relink exposes consequence-free Practice Mode and reminders, while ZZZ's current VR/challenge systems reward technique execution and graded performance.

### QOL-012: explicit defense timing cue metadata
Community source: Reddit r/ZenlessZoneZero, `Has anyone created a guide for control skill parry timings?`, published 2026-09-14, accessed 2026-09-22: https://www.reddit.com/r/ZenlessZoneZero/comments/1wg4jrg/has_anyone_created_a_guide_for_control_skill/
The post asks for visual input-window guidance and less-obvious cues for difficult parry sequences. Replies disagree about how universal the problem is and point out that some bosses already have assist-icon or animation cues. Treat this as anecdotal feedback, not consensus or proof of a current ZZZ defect.
Adaptation: expose deterministic `Approach`, `DodgeWindow`, and `PerfectWindow` cue stages plus seconds-to-impact, blockability, and attack pattern. This is metadata for later UI/audio work, not a copied indicator.
Acceptance: cue stage changes at the same tolerance-aware boundaries used by `TryDefend`, including ordinary float frame splits, reports unblockable plans, and returns `None` when no linked threat is active.

## Primary/reference sources, accessed 2026-09-22

- Wuthering Waves PlayStation overview: https://store.playstation.com/en-us/concept/10010764/ . Current official description explicitly names Extreme Evasion and Dodge Counter.
- Granblue Fantasy: Relink gameplay: https://relink.granbluefantasy.jp/en/gameplay . Official page documents stun gauges creating Link Attack opportunities.
- Granblue Fantasy: Relink PlayStation overview: https://www.playstation.com/en-us/games/granblue-fantasy-relink/ . Current accessibility metadata documents Practice Mode, control/tutorial reminders, and adjustable difficulty.
- Zenless Zone Zero `Combat Training - Triple Bounty`, published 2026-08-31: https://zenless.hoyoverse.com/m/en-us/news/165921 . Official event allows selecting enemy cards in Combat Simulation.
- Zenless Zone Zero `Virtual Shadow Hunt` event, published 2026-09-14: https://zenless.hoyoverse.com/m/zh-tw/news/166073 . Official event scores damage, techniques, technique chains, and time coefficient.

Reference games provide interaction lessons only. No proprietary code, characters, art, story, values, monetization, or party-switch architecture is copied.

## Implementation constraints

- Reuse `CombatSandbox`, `EnemyAttackPlan`, `ShadowbladeActions`, and their existing clocks/contracts. Do not create a second combat framework.
- Coordinator state must identify both the exact linked object instances and the exact successful incoming-threat generation, so it never cancels, advances as linked, or resolves an unrelated standalone replacement.
- If the authoritative combat plan disappears, cancel the owned Shadowblade threat before advancing its clock.
- New cancellation may clear only the current incoming threat. It must not heal/reset health/guard, create a counter, or alter unrelated cooldown/resource state.
- Cue classification must reuse the same half-ULP/timing tolerance semantics as the actual defense acceptance windows.
- Invalid/nonfinite/nonpositive deltas preserve current subsystem behavior.
- Counters must saturate or remain within ordinary integer limits; grade arithmetic uses widened values where multiplication is needed.
- Resetting drill telemetry must not mutate combat/player state.

## Registered regression coverage

Extend existing `ShadowbladeActionsTests`, already registered via `astral_add_test`, to cover:
1. exact enemy-plan mapping and duplicate rejection;
2. too-early dodge preservation then perfect-defense round-trip;
3. automatic impact closes both sides exactly once and records damage;
4. stagger interruption reconciled later than the remaining windup still cancels before player damage;
5. canceled/replaced threats and wrong `ShadowbladeActions` objects remain unrelated to the coordinator;
6. cue boundary stages use the same tolerance as immediate `TryDefend`, including the `0.43f` QuickCut boundary, plus unblockable metadata;
7. telemetry/streak accounting, grade thresholds, reset isolation, and invalid delta stability.

## Verification commands / gates

Hosted exact-head gates already present in the repository are required after push:
- Windows MSVC Debug and Release build plus deterministic CTest workflow;
- release-manifest integrity;
- any other automatically applicable existing workflow;
- independent Codex/project review on the exact final head with no unresolved material finding.

Sandbox/Linux checks are useful if exact production sources are available, but they are not substitutes for hosted Windows or native interactive evidence. Do not fabricate a local checkout when GitHub DNS is unavailable.

## Native evidence boundary

This pass does not add rendered telegraphs, new input bindings, audio cues, animation events, or a practice UI. Therefore native interactive/playable verification remains 0 unless independently obtained through the registered local executor. Do not start local execution from this worker.

## Stop conditions

Do not merge if `main` moves incompatibly, another active worker owns one of the admitted source paths, the final diff expands beyond allowed paths, exact-head hosted checks fail or are missing, independent review reports an unresolved material defect, or the coordinator can produce stale/double hits or mutate unrelated standalone threats in the registered tests. Reconcile and rerun affected gates rather than force-pushing or weakening acceptance.