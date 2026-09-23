# GAME Pass 24: Shadowblade Training Coach

Date: 2026-09-23  
Owner: `animerpg-game-hourly`  
Target: `main`  
Baseline: `3eaeb72d65c98f887e827999a78eabb636120dfe`  
Branch: `game/2026-09-23-training-coach-pass24`

## Scope and ownership

This is a bounded GAME-only follow-up to the merged Shadowblade defense-practice and training curriculum. It preserves the custom C++17 Astral Engine and reuses `DefensePracticeSession`, `CombatSandbox`, and `ShadowbladeActions` as authoritative gameplay owners. It does not change renderer/platform/editor/import/animation/audio/physics, CMake/workflows/dependencies, R0, networking, deployment, release, or another worker's PR.

Allowed paths for this packet:
- `Engine/Scene/ShadowbladeTrainingCoach.h`
- `Tests/ShadowbladeTrainingCoachPass24Tests.inc`
- `Tests/ThoughtCommandsTests.cpp` only for registering this pass's production header/test function
- `Tasks/GAME-PASS24-SHADOWBLADE-TRAINING-COACH-2026-09-23.md`
- `Docs/Agents/animerpg-hourly/RUN-2026-09-23-PASS24.md`
- `Docs/Agents/animerpg-hourly/STATE.json`

Native rendered UI, input-device wiring, persistence, rewards, engine facilities, and production art/audio are outside this packet.

## Research provenance and adaptation

Accessed 2026-09-23. References supply design lessons only.

1. **ZZZ Combat Training: Triple Bounty**, HoYoverse, published 2026-08-31: Combat Simulation lets players select an enemy card and complete a challenge.  
   https://zenless.hoyoverse.com/m/en-us/news/165921
2. **ZZZ Shadow Chase Showdown**, HoYoverse event text published 2026-09-14 and mirrored with source attribution: Special Offensive scoring combines damage, technique execution, and a time coefficient; technique chains reward varied execution.  
   https://zenless.gg/shadow-chase-showdown-event-details/
3. **ZZZ Snap! Hollow Realm Showdown**, HoYoLAB, published 2026-03-06: stages contain main and additional challenge targets, with a free-practice-like Hyperfocus mode after completion.  
   https://www.hoyolab.com/article/44074876
4. **Granblue Fantasy: Relink**, PlayStation: adjustable difficulty, control reminders, tutorial reminders, consequence-free Practice Mode, and pausing are documented accessibility/gameplay features.  
   https://www.playstation.com/en-us/games/granblue-fantasy-relink/
5. Existing Astral game code already has authoritative per-pattern stats, scoring, grades, configurable practice sequences, pace, goals, timing presets, and enemy definitions. This pass exposes and composes those owners rather than creating a parallel simulator.

Community request, new to the backlog this pass:
- **"Has anyone created a guide for control skill parry timings?"**, r/ZenlessZoneZero, published 2026-09-14, accessed 2026-09-23. The author asks for a visual/input-window guide and cues for boss parry timing. Replies include both support for training-mode practice and counterpoints that existing assist icons or learned telegraphs can be sufficient. Treat this as anecdotal player feedback, not consensus or proof of a current ZZZ defect.  
  https://www.reddit.com/r/ZenlessZoneZero/comments/1wg4jrg/has_anyone_created_a_guide_for_control_skill/

## Five reference increments plus one community increment

### GAME-117: Selectable focused threat drills
Gap: the curriculum is sequential, but players cannot select a bounded single-pattern or mixed drill from one game-domain policy.  
Adaptation: expose QuickCut, GuardBreaker, RiftBurst, Mixed Defense, and Boss Cycle plans using only the existing three authoritative patterns and existing pace/goal controls.  
Acceptance: exact sequence/goal/pace mapping; invalid focus/pace fail closed; reconfiguration during an active threat makes no mutation.

### GAME-118: Scored practice debrief
Gap: score, grade, pattern stats, accuracy inputs, streak, and time coefficient exist separately.  
Adaptation: provide one read-only debrief assembled from authoritative session state.  
Acceptance: resolved/success/perfect/hit/damage metrics, percentages, streak/alternation, base score, coefficient, final score, and grade agree with production owners; no alternate score formula mutates gameplay.

### GAME-119: Optional mastery challenge status
Gap: practice supports goals, but not independent mastery checks suitable for optional training objectives.  
Adaptation: bounded Perfect Streak 3, No-Hit 5, and Perfect-on-All-Three-Patterns checks.  
Acceptance: each challenge derives only from authoritative metrics; invalid enum is inert; challenge state grants no rewards and mutates nothing.

### GAME-120: Deterministic post-session coaching
Gap: existing tutorial hints are live/local but there is no post-session recommendation based on measured pattern outcomes.  
Adaptation: recommend the canonical pattern with the worst observed hit ratio; if there are no pattern failures, recommend better perfect timing when ordinary defenses exceed perfect defenses; otherwise recommend maintaining form.  
Acceptance: deterministic, bounded, read-only, ratio comparison avoids floating-point/overflow hazards for normal bounded counters.

### GAME-121: Quick drill retry
Gap: players can reset metrics through a lower-level session call, but the new training policy should expose explicit same-drill retry semantics.  
Adaptation: clear session metrics and restart sequence cursor while retaining selected sequence, pace, goal, and target.  
Acceptance: active threat rejects retry atomically; successful retry preserves configuration and clears results.

### QOL-025: Authoritative timing guide
Gap: exact attack windup/blockability and defense-window constants are split across production owners.  
Community adaptation: expose a non-mutating per-pattern timing guide with windup, damage, guard damage, blockability, recommended Guard/Dodge input, selected perfect-defense window, both standard/forgiving perfect windows, and dodge window.  
Acceptance: values are read from production `CombatSandbox` threat definitions and `ShadowbladeActions` constants; invalid pattern/preset fails closed without overwriting the caller's previous guide.

## Verification requirements

Registered regression coverage must execute through the existing `ThoughtCommandsTests` target and exercise production code, including invalid enums, active-threat atomicity, actual queued threat definitions, real perfect/ordinary/hit resolution, challenge completion, recommendation behavior, retry state, and timing-guide values.

Required before merge:
- exact-head hosted Windows Debug build/tests;
- exact-head hosted Windows Release build/tests with registered assertions active;
- existing static/scope/prerequisite checks;
- exact-head Release-manifest integrity when the repository workflow applies;
- fresh independent Codex review of the exact final candidate with all material findings repaired/resolved;
- full diff and ownership review immediately before merge;
- re-read `main` and PR head and merge only the exact tested/reviewed head.

A container clone attempt earlier in this run could not resolve `github.com`; that is an external sandbox DNS limitation, not test evidence or a product failure. Do not claim sandbox compilation from it. Native Windows interactive/rendered gameplay, GPU/performance evidence, controller/UI integration, production animation/audio, cross-process persistence, and parity remain unverified.
