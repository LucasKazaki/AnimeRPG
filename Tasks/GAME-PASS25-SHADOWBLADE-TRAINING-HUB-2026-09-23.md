# GAME Pass 25: Shadowblade Training Hub

Date: 2026-09-23
Owner: `animerpg-game-hourly`
Base main: `4c3308051c910b66dd0aebcf513956926beab505`
Target: `main`
Branch: `game/2026-09-23-training-hub-pass25`

## Scope

Pass 24 created a tested Shadowblade training coach, but independent review correctly required it to be labeled backend-only because no production game owner called it. This pass closes that specific game gap without editing engine-owned platform, renderer, editor, import, animation, audio, physics, build, workflow, dependency, networking, deployment, or release paths.

The integration point is the existing live `LandmarkEncounter` game owner. Completing the Lincoln Memorial encounter unlocks a bounded Shadowblade practice hub. The hub reuses the authoritative `CombatSandbox`, `ShadowbladeActions`, `DefensePracticeSession`, and pass-24 coach rather than creating a second combat framework. Native input/UI wiring remains deliberately out of scope because `Engine/Platform` belongs to the separate Astral Engine worker.

Allowed paths for this packet:
- `Engine/Scene/ShadowbladeTrainingHub.h`
- `Engine/Scene/LandmarkEncounter.h`
- `Engine/Scene/LandmarkEncounter.cpp`
- `Tests/ShadowbladeTrainingHubPass25Tests.inc`
- `Tests/ShadowbladeTrainingHubPass25ReviewTests.inc`
- `Tests/ThoughtCommandsTests.cpp` only for existing registered-test aggregation
- this task packet
- `Docs/Agents/animerpg-hourly/RUN-2026-09-23-PASS25.md`
- `Docs/Agents/animerpg-hourly/STATE.json`

## Research and feature map

Sources were read on 2026-09-23. Comparator mechanics are design references only. No comparator story, characters, maps, art, music, code, monetization, or party architecture is copied.

### GAME-122: story-gated training unlock

Reference: Zenless Zone Zero official `Combat Training - Triple Bounty` notice, published 2026-08-31: https://zenless.hoyoverse.com/m/en-us/news/165921 . It requires unlocking Combat Simulation through Main Story Chapter 1 before the activity is available.

Repository gap: pass-24 training had no production call site and no game-progress gate.

Original adaptation: the existing live Lincoln Memorial combat encounter becomes the authoritative unlock point. Before completion, configure/start fail closed. Completion changes the training hub from Locked to Ready exactly once without changing the encounter reward contract.

Acceptance:
- training is locked before encounter completion;
- only the existing completed encounter path unlocks it;
- repeated `Update` calls do not reset or duplicate training state;
- normal encounter completion reward remains idempotent.

### GAME-123: selectable drill, pace, and bounded attempt target

References:
- same 2026-08-31 ZZZ official notice: Combat Simulation lets players select an enemy card;
- Granblue Fantasy: Relink official PlayStation accessibility/gameplay listing: https://www.playstation.com/en-us/games/granblue-fantasy-relink/ , which documents Adjustable Difficulty;
- Genshin Impact official HoYoLAB Stygian Onslaught notice published 2026-01-19: https://www.hoyolab.com/article/43362737 , which documents selecting a difficulty before a multi-phase combat challenge.

Repository gap: pass-24 coach offered focus/pace mappings, but no live game owner stored a bounded player-selected training contract.

Original adaptation: the hub selects one existing coach focus, one existing practice pace, and 1 to 10 attempts. Invalid focus, pace, or attempt count cannot partially change the prior configuration.

Acceptance:
- all valid existing focus/pace combinations remain available;
- target attempts are bounded to 1..10;
- invalid enum values and out-of-range targets fail without mutation;
- reconfiguration is refused while a training run is active.

### GAME-124: consequence-free endless practice target on existing combat owners

Reference: Granblue Fantasy: Relink's official PlayStation listing describes Practice Mode as a consequence-free environment for practice.

Repository gap: the project already had `TrainingTargetMode::Endless`, but pass-24 coach was not attached to a live game owner using that target mode.

Original adaptation: starting training resets the existing combat target into Endless mode, clears only transient Shadowblade state while preserving loadout state, applies the selected pass-24 coach drill, binds the exact combat/actions pair, and queues the first existing enemy threat. No reward or new economy path is added.

Acceptance:
- the target cannot be killed during the training run;
- persistent loadout selection survives training start;
- the first threat is the selected coach sequence's first canonical pattern;
- unrelated combat/action objects cannot drive the bound run;
- encounter retry cannot reset an active practice owner pair.

### GAME-125: bounded multi-attempt run with debrief and guidance

References:
- Genshin Impact official Stygian Onslaught notice documents a bounded three-phase challenge after selecting difficulty;
- ZZZ's Combat Simulation selection gives a precedent for focused repeatable combat challenges;
- pass-24's already-reviewed coach provides authoritative debrief, challenge, recommendation, and timing-guide calculations.

Repository gap: pass-24 results could be produced only from test-created sessions. There was no production game-domain coordinator that started a finite practice run, auto-queued the next selected threat, and stopped at an exact target.

Original adaptation: an active hub run advances only its exact bound owners, queues the next selected threat after authoritative recovery, and transitions to Debrief at the configured resolved-attempt count. Feedback composes the existing coach debrief, challenges, recommendation, and timing guide.

Acceptance:
- no fifth threat is queued for a four-attempt run;
- debrief resolved count equals the selected target exactly;
- wrong-owner feedback fails closed rather than mixing provenance;
- coach scoring and defense timing continue to come from existing authoritative systems.

### GAME-126: pause/resume practice

Reference: Granblue Fantasy: Relink's official PlayStation listing documents Game Pausing during offline gameplay.

Repository gap: `DefensePracticeSession` already has pause semantics, but no integrated live game owner exposed them for the training flow.

Original adaptation: the training hub exposes pause/resume only while active. Its explicit training step owns both practice combat clocks, so a paused step does not advance combat time, threat time, damage, or practice progress. This method is intentionally not called from the current Win32 platform frame loop because that loop separately owns the same clocks.

Acceptance:
- pause freezes combat elapsed time, incoming-threat time, player health, and session progress;
- resume continues the same authoritative threat;
- non-active pause requests fail closed;
- no engine/platform code changes are required.

### QOL-026: bounded recent-damage feedback

Community source: original r/WutheringWaves discussion `God I wish Wuwa had a Training Dummy mode.`, posted 2026-03-14: https://www.reddit.com/r/WutheringWaves/comments/1rt5mnz/god_i_wish_wuwa_had_a_training_dummy_mode/ . The OP asks for a target that does not die too quickly while learning rotations. A 2026-03-18 reply specifically asks for total damage over roughly the last 10 to 20 seconds so players can compare rotation execution. Another reply wants a training room. Counterarguments/workarounds in the same discussion include resistant world enemies, bosses, other challenge modes, lower-level targets, and an NPC sparring option.

Evidence quality: this is corroborated player preference, not consensus. A fresh search of official Wuthering Waves material during this pass did not establish whether a later official training-dummy/damage-stat feature resolved the request, so this packet does not claim Wuthering Waves currently lacks it.

Repository gap: existing `TrainingDps()` is whole-session style telemetry and does not answer the requested recent-window comparison. The pass-24 coach also has no offensive rolling window.

Original adaptation: the live training hub records a fixed-capacity, read-only sample history and exposes damage plus hit count over a maximum 20-second recent window. Sampling from normal `LandmarkEncounter::Update` does not advance combat or Shadowblade clocks.

Acceptance:
- storage is fixed at 128 samples and sampling is rate-limited to at most 5 Hz;
- old damage falls out of the 20-second comparison window;
- combat/stat resets rebase the sample window instead of producing negative deltas;
- wrong combat owner cannot expose mixed-provenance telemetry;
- no persistence, renderer, or platform dependency is added.

## Verification plan

Registered regression coverage is added through the existing `ThoughtCommandsTests` aggregation target. Required hosted gates before merge:

1. exact-head Windows Debug and Release build/test workflow with registered gameplay regressions and existing static/scope checks;
2. exact-head Release-manifest integrity workflow;
3. fresh independent Codex review on the exact final head, with every material finding repaired and thread resolved;
4. full diff and ownership audit against the live `main` immediately before merge;
5. expected-head merge only if `main` has not moved incompatibly.

Additional boundary checks in the pass-25 test packet cover invalid enum/range inputs, exact owner binding, active-threat retry refusal, exact finite-attempt stop, pause/resume, damage-window expiry, and preserved loadout selection.

The independent review of candidate `e28f415b33461b367da27bec1d410e1ca585a35d` found two additional P2 edge cases. Both are merge-blocking until repaired and exact-head gates rerun:

- active timing guidance must validate the practice session's linked combat/action generation before describing a queued pattern, so an externally replaced combat-plan threat cannot be presented as the threat approaching the player;
- rolling damage telemetry must stop sampling after the run enters Debrief, so later encounter reuse or post-run combat mutations cannot rewrite the completed run's feedback.

The dedicated review-regression packet covers both cases. Timing feedback now uses `DefensePracticeSession::Cue` as the exact linked-threat witness while Active, and otherwise fails closed. Damage observation is restricted to Active state; the final terminal action is sampled before the transition to Debrief, after which the window remains frozen.

Local sandbox compile is not evidence for this pass because the container could not resolve `github.com` while attempting a clean branch clone. No local test success is claimed from that failed fetch.

## Evidence boundary

This pass creates a real production game-domain call site in `LandmarkEncounter::Update` for training unlock and read-only telemetry, and the existing live `LandmarkEncounter` owns the training APIs. However, current native Win32 input does not yet invoke configure/start/defend/advance/pause. Therefore this packet may count game-domain integration after tests/review, but native-player-playable verification remains zero. There is no rendered training UI, controller/keybinding route, production training scene, art/audio pass, GPU/performance capture, or cross-process training-state persistence in this packet.
