# GAME-PASS22-REACTOR-STRATEGY-2026-09-23

Owner: `animerpg-game-hourly`
Target: `main`
Baseline: `3c3babd46c4539d474b7ea78ab01d5ed71dde7ac`
Branch: `game/2026-09-23-reactor-strategy-pass22`

## Scope

Deepen the already-merged `ManaReactorExpedition` rather than creating another framework. Preserve pass-21 Standard/Baseline behavior and the same first-clear reward entitlement. No renderer, platform, editor, importer, animation, audio, physics, build-system, workflow, dependency, R0, release, deployment, networking, or unrelated-repository work is admitted.

## Research mapping

Sources accessed 2026-09-23. Comparator mechanics are design references only; no comparator story, characters, art, audio, code, maps, economies, or monetization are copied.

1. **GAME-107 - selectable reactor pressure presets.** Genshin Impact's official Stygian Onslaught event description, published 2026-08-17, requires selecting a difficulty before a three-phase challenge and applies more restrictive rules at higher difficulties. Original adaptation: Guided, Standard, and Critical modify only Mana Reactor hazard pressure; the objective graph and one-time reward entitlement stay unchanged. Source: https://www.hoyolab.com/article/46329018
2. **GAME-108 - opt-in reactor protocols.** Genshin Impact's official Reminiscent Regimen: Thrill description, published 2024-10-15, lets players select a stage buff before each stage. Original adaptation: Baseline, Thermal Sink, Stability Mesh, and Surge Harness are single-protagonist reactor-control protocols chosen before a run. Source: https://www.hoyolab.com/article/34259331
3. **GAME-109 - bounded emergency vent.** Zenless Zone Zero's official Snap! Focus Showdown! description, published 2026-08-05, adds a stage-specific skill used at the right moment to obtain a stage buff. Original adaptation: a once-per-stage emergency vent is a manual tactical recovery action that cools the reactor at a stability/score cost; it does not advance objectives. Source: https://www.hoyolab.com/article/46150564
4. **GAME-110 - optional performance amplifies the selected protocol.** ZZZ's official Snap! Hollow Realm Showdown description, published 2026-03-06, says additional challenge targets permanently enhance the stage's Filter Effect. Original adaptation: meeting the existing thermal/stability optional target raises the selected reactor protocol one bounded rank for later stages in the same run. Source: https://www.hoyolab.com/article/44074876
5. **GAME-111 - precision control chains.** ZZZ's official Simulated Sequence Showdown description, published 2025-12-05, awards Technique Combos and score multipliers for varied techniques inside a timing window. Original adaptation: alternating valid reactor controls builds a bounded precision chain; at three or more, a small pressure-relief pulse is applied and counted in the deterministic score. No real-time combo timer is introduced. Source: https://www.hoyolab.com/article/42631745
6. **QOL-023 - compact, exact control forecast.** A Genshin player discussion published 2024-12-22 praised its event but called unit information inaccurate/lacking and asked for more detailed descriptions, while another June 10, 2024 discussion objected to pages of instructions that did not help with meaningful choices. A Wuthering Waves discussion on 2025-03-13 likewise criticized overlong skill descriptions, with a reply noting the game already offered a shorter details view with current/projected values. These are player anecdotes with counterexamples, not consensus. The cited events are historical and the Wuthering Waves thread itself documents a partial existing solution, so this packet does not claim an unresolved current defect. Original adaptation: `PreviewControl()` exposes only projected objective progress, heat, stability, strategy application, chain state, and result, using the exact same evaluator as commit. Sources: https://www.reddit.com/r/Genshin_Impact/comments/1hjsv5r ; https://www.reddit.com/r/Genshin_Impact/comments/1dc9cby ; https://www.reddit.com/r/WutheringWaves/comments/1jackhp

## Acceptance contracts

### GAME-107
- Invalid difficulty fails closed before run mutation.
- Guided, Standard, and Critical keep the same three-stage objective graph.
- A first Intake `Balance` from baseline state yields deterministic heat/stability: Guided 22/90, Standard 26/90, Critical 32/86.
- Heat/stability remain clamped to 0..100.

### GAME-108
- Invalid protocol fails closed.
- `Baseline` preserves pass-21 control tuning.
- Rank-one `ThermalSink` changes a first Standard `Balance` result from 26 heat to 23 without changing stability.
- Rank-one `StabilityMesh` changes that result from 90 stability to 93 without changing heat.
- `SurgeHarness` affects only Overdrive and becomes more productive after earned amplification.

### GAME-109
- Vent is available at most once per stage and never advances objective progress.
- It removes exactly 25 heat, costs exactly 5 stability, breaks the live precision chain, and is rejected when unavailable/unsafe.
- Duplicate use is non-mutating.
- Stage transition/retry refreshes stage availability, while run-level vent-use count remains bounded and score-accounted; Start Over resets run-local use count.

### GAME-110
- A completed existing optional thermal/stability target raises a non-Baseline protocol exactly one rank, capped at 3.
- Missed optional target does not fabricate a rank.
- Earned rank survives stage transition/retry but Start Over resets it to 1 while retaining the selected difficulty/protocol.
- Rank-two Surge Harness can supply the third required Cooling objective unit from one Overdrive, with its declared additional heat risk.

### GAME-111
- Precision chains exist only for non-Baseline protocols.
- Repeating the same control resets the live chain to 1; alternating controls raises it, capped at 4.
- Chain 3+ applies a fixed -4 heat/+2 stability precision pulse through the same control evaluation path.
- Stage transition and emergency vent reset only the live chain; run best and successful pulse count remain bounded for scoring.
- The registered all-target Thermal Sink route completes Gold with rank 3, best chain 3, two pulses, and deterministic score 1446.

### QOL-023
- Preview is non-mutating for all inputs.
- Invalid control preview returns rejected/invalid without mutation.
- Valid preview exposes projected objective progress, heat, stability, chain, protocol application, precision-pulse flag, and terminal/nonterminal result.
- Committing a nonterminal preview lands on the exact projected authoritative state because preview and commit share `EvaluateControl`.

## Allowed paths

- `Engine/Scene/ManaReactorExpedition.h`
- `Tests/ManaReactorStrategyPass22Tests.inc`
- `Tests/ThoughtCommandsTests.cpp` only for pass-22 include/call registration
- `Tasks/GAME-PASS22-REACTOR-STRATEGY-2026-09-23.md`
- `Docs/Agents/animerpg-hourly/STATE.json`
- `Docs/Agents/animerpg-hourly/RUN-2026-09-23-PASS22.md`

## Verification required before merge

- Existing hosted Windows Debug and Release deterministic build/tests on the exact final head.
- Existing release-manifest integrity workflow on the exact final head.
- Registered pass-21 regressions remain green, including the exact legacy Gold score of 1310 for Standard/Baseline.
- New pass-22 regressions exercise invalid enums, pressure bounds, protocol effects, vent idempotency/reset, amplification reset/cap behavior, precision scoring, and preview/commit equivalence.
- Fresh independent implementation review on the exact final head, with every material finding repaired and thread resolved.
- Full diff/scope audit and reread of `main` plus PR head immediately before merge.

Native rendered reactor UI/scene, controller/menu wiring, GPU/performance evidence, production art/VFX/audio, cross-process persistence, release, deployment, and hands-on native gameplay are explicitly not claimed by this packet.
