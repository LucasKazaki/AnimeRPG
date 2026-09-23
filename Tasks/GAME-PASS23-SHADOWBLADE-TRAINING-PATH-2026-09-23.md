# GAME pass 23: Shadowblade training path

Date: 2026-09-23
Owner: `animerpg-game-hourly`
Base: `7dfaeeb340e57d1024a8bc818c65c82cd391d4ae`
Target: `main`
Branch: `game/2026-09-23-training-path-pass23`

## Scope

Deepen the already-merged Shadowblade defense-practice gameplay instead of creating a second combat framework. Preserve the custom C++17 Astral Engine, existing `CombatSandbox`, `ShadowbladeActions`, `CombatDefenseTraining`, and `DefensePracticeSession` authority. This packet adds only game-owned curriculum/progression policy and registered regressions.

No renderer, platform, editor, importer, animation, audio, physics, generic engine tooling, CMake, workflows, dependencies, R0, Company Runtime scheduling, release, deployment, networking, or unrelated repository work is admitted. No local executor job is started by this packet.

Allowed production path:
- `Engine/Scene/ShadowbladeTrainingPath.h`

Allowed verification and operating-record paths:
- `Tests/ShadowbladeTrainingPathPass23Tests.inc`
- `Tests/ThoughtCommandsTests.cpp`, only pass-23 header/include/call registration
- this task packet
- `Docs/Agents/animerpg-hourly/RUN-2026-09-23-PASS23.md`
- `Docs/Agents/animerpg-hourly/STATE.json`

## Five reference features plus one community improvement

Access date for all sources below: 2026-09-23.

1. **GAME-112, sequential Shadowblade lesson curriculum.** The Granblue Fantasy: Relink demo describes a Tutorial Mode for learning character controls and game basics, while Genshin Impact's January 19, 2026 Stygian Onslaught description uses three sequential challenge phases. Original adaptation: five small Shadowblade lessons, Guard Fundamentals, Dodge Fundamentals, Mixed Timing, Pressure Handling, and Boss Rehearsal, unlock in order. This is not copied story/content. Sources: https://store.playstation.com/en-my/product/HP0177-CUSA44775_00-GBRELINKDEMO0001 and https://www.hoyolab.com/article/43362737 .

2. **GAME-113, atomic authored lesson setup.** Zenless Zone Zero's July 20, 2026 `Data Bounty: Combat Simulation` description says players enter Combat Simulation and select enemy cards for challenges. Original adaptation: each lesson atomically applies a bounded existing `DefensePracticeSession` sequence, pace, goal, and target, rejecting active-threat setup without partial mutation. Source: https://www.hoyolab.com/article/45927809 .

3. **GAME-114, lesson mastery medals.** ZZZ's March 6, 2026 `Snap! Hollow Realm Showdown` uses main and additional challenge targets, with additional targets strengthening a stage effect. Original adaptation: Bronze/Silver/Gold mastery comes only from authoritative practice results, with no currency or copied reward structure. Source: https://www.hoyolab.com/article/44074876 .

4. **GAME-115, opt-in forgiving defense timing.** PlayStation's Granblue Fantasy: Relink accessibility description lists adjustable difficulty and individually activatable assists. Original adaptation: a training-only option selects the already-existing Forgiving perfect-defense timing preset. It does not inflate player health, lower incoming damage, or alter enemy patterns. Source: https://www.playstation.com/en-my/games/granblue-fantasy-relink/ .

5. **GAME-116, validated curriculum checkpoint.** The PlayStation Store page for Granblue Fantasy: Relink - Endless Ragnarok Demo, released June 18, 2026, explicitly says demo progress automatically saves. Original adaptation: a versioned in-memory Shadowblade training-progress checkpoint with strict validation and monotonic no-rollback semantics. This does not implement disk/cloud persistence and makes no cross-process save claim. Source: https://store.playstation.com/en-us/product/UP5460-PPSA35115_00-GBRELINKERDEMO01 .

6. **QOL-024, boss-style three-threat rehearsal.** A September 11, 2026 player thread in `r/ZZZ_Official` requests a training environment that can reproduce boss-style three-chain rotations; replies note the ordinary VR room offers elites and suggest alternatives such as Annihilation Simulacrum. A separate January 2, 2025 thread also requests bosses in free training for learning moves. Original adaptation: the final Shadowblade lesson rehearses the game's existing QuickCut -> GuardBreaker -> RiftBurst threat sequence in one bounded loop. This is a player-request-inspired rehearsal, not a copied ZZZ boss, chain-attack system, or claim of community consensus. Sources: https://www.reddit.com/r/ZZZ_Official/comments/1wdtzbh/training_mode/ and https://www.reddit.com/r/ZenlessZoneZero/comments/1hru9ss/ . Current resolution: not established from an authoritative September 2026 source; alternatives exist, so the request is treated as preference evidence rather than proof of a current product defect.

## Acceptance

### GAME-112
- Only Guard Fundamentals is initially unlocked.
- Completing and committing a lesson advances to exactly the next lesson.
- Stale metrics cannot double-complete the next lesson.

### GAME-113
- A lesson configures only existing practice sequence/pace/goal/target mechanics.
- Setup is all-or-nothing through a candidate copy.
- Reconfiguration during an owned live threat fails without erasing or changing that attempt.

### GAME-114
- An incomplete lesson has no medal.
- Clean ordinary completion earns Silver; all-perfect eligible completion can earn Gold; completed runs with misses can only earn Bronze.
- Stored mastery only moves upward.

### GAME-115
- Forgiving assistance uses `DefenseTimingPreset::Forgiving` and standard mode restores `DefenseTimingPreset::Standard`.
- Toggling is rejected while an incoming threat is active.
- No enemy plan, damage, health, resource, or reward constant is modified by this feature.

### GAME-116
- Checkpoint schema, lesson enum, prefix completion mask, medal values, and current-lesson consistency are validated before mutation.
- Older progress or lower medals cannot roll local progress backward.
- Malformed checkpoints fail closed.
- This pass claims only an in-memory contract, not durable save integration.

### QOL-024
- Boss Rehearsal is locked behind the prior four lessons.
- It deterministically queues QuickCut, GuardBreaker, and RiftBurst using existing production threat definitions.
- Completion requires at least one resolved attempt for each threat; interruptions alone cannot complete it.
- Per-pattern practice telemetry demonstrates all three threats were actually exercised.

## Verification requirements

Use the existing registered `ThoughtCommandsTests` target rather than a mock-only test executable. Required exact-head evidence before merge:
- hosted Windows Debug build and tests;
- hosted Windows Release build and tests with the repository's active assertion/test contract;
- applicable static/prerequisite checks already in the workflow;
- release-manifest integrity workflow if triggered for this PR;
- full diff and allowed-path scope audit;
- fresh independent Codex review of the exact final head with no unresolved material findings.

Native rendered/player-facing verification is separate. No UI/input-screen/controller wiring is changed here, so this slice remains game-domain integrated rather than native-playable evidence. Do not claim GPU, art/audio, performance, cross-process persistence, or Genshin/ZZZ parity.

## Stop condition

Stop and leave the PR unmerged if exact-head required CI fails, independent review raises an unresolved material issue, `main` moves incompatibly, or the diff exceeds the admitted paths. Repair only scoped findings, then rerun the affected exact-head gates. Merge only the exact tested/reviewed candidate using expected-head checking.
