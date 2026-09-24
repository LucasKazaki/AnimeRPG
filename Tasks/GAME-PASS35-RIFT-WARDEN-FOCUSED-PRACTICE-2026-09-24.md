# GAME pass 35: Rift Warden focused practice

Date: 2026-09-24  
Owner: `animerpg-game-hourly`  
Baseline: `3fcc9a003799dc23dddf8c7cadac4c857f826428`  
Target: `main`

## Scope

Deepen the already-merged Rift Warden boss-training loop with bounded practice controls. This packet does not change Astral Engine renderer/platform/editor/import/animation/audio/physics, shared CMake/workflows, networking, release/deployment, R0, or other workers' PRs. It preserves the single persistent protagonist and existing Shadow Crypt -> Rift Warden progression authority.

Allowed production/test paths:

- `Engine/Scene/RiftWardenTrial.h`
- `Engine/Scene/LandmarkInteraction.h`
- `Tests/RiftWardenFocusedPracticePass35Tests.inc`
- registration-only additions to `Tests/ThoughtCommandsTests.cpp`
- this packet and `Docs/Agents/animerpg-hourly/` pass-35 operating records

## Research mapping

Accessed 2026-09-24.

1. **GAME-172, focused learned boss-attack drill.** ZZZ's official `Combat Training - Triple Bounty` event description (2026-08-31) says Combat Simulation lets players select an enemy card before challenging it: https://zenless.hoyoverse.com/m/en-us/news/165921 . Adaptation: after the player has actually encountered a Rift Warden attack in a ranked clear, allow a one-attempt practice drill for that exact original attack. Do not import ZZZ enemies or roster systems.
2. **GAME-173, practice pace presets.** PlayStation's current Granblue Fantasy: Relink accessibility page documents adjustable difficulty and individually selectable assists: https://www.playstation.com/en-us/games/granblue-fantasy-relink/ . Adaptation: Guided/Standard/Expert modify only practice response windows by fixed bounded multipliers; ranked Warden timing remains unchanged.
3. **GAME-174, control reminder.** The same Relink page documents Control Reminders. Adaptation: expose the current Warden attack, semantic cue, and expected response as a read-only practice reminder derived from the authoritative telegraph.
4. **GAME-175, learned attack guide.** The same Relink page documents Tutorial Reminders. Adaptation: once an original Warden move is learned, expose its last-seen phase, observed attempt/success evidence, semantic cue, expected response, and timing window without mutating the fight.
5. **GAME-176, practice pause/resume.** The same Relink page documents Practice Mode and offline Game Pausing. Adaptation: practice-only pause freezes the Warden practice clock and rejects action resolution until resumed; ranked challenge behavior is unchanged.
6. **QOL-036, looping boss-move rehearsal.** ZZZ players repeatedly requested boss access in free training so they could practice parry windows, stun rotations, and specific boss mechanics. Original discussions: 2026-02-14 https://www.reddit.com/r/ZZZ_Discussion/comments/1r4l7xn/why_cant_we_put_bosses_in_the_training_training/ ; 2026-03-02 https://www.reddit.com/r/ZZZ_Official/comments/1rikhmx/how_do_you_guys_like_to_test_your_teams/ ; 2026-08-01 https://www.reddit.com/r/ZZZ_Discussion/comments/1vcwt03/i_wish_we_could_practice_against_bosses_in_free/ ; and a 2026-09-22 weekly thread still reports that the VR training room does not include bosses: https://www.reddit.com/r/ZZZ_Official/comments/1wnlvf6/weekly_question_discussion_megathread_september/ . These are corroborating player anecdotes, not consensus. The current official Combat Training event page confirms selectable enemy cards but does not establish universal boss availability. Adaptation: looping focused practice repeats an already-learned original Rift Warden move indefinitely without consuming boss health, advancing phase, or touching ranked records.

Genshin's 2026-03-02 Stygian Onslaught description was also rechecked for the existing project pattern of selectable difficulty, sequential unlocks, and timed multi-phase combat; those capabilities already exist in the Warden trial and are not recounted as new pass-35 work: https://www.hoyolab.com/article/44016747 .

## Acceptance

- Focused practice rejects entry before a ranked clear, for invalid enums, locked difficulty, or an attack that has never been learned.
- A one-shot focused drill emits only the selected learned attack, records one accepted training sample, completes after that attempt, and cannot alter ranked best records or difficulty unlocks.
- Guided practice uses exactly `1.25x` the standard practice window; Expert uses `0.80x`; Standard is unchanged. Ranked windows are unchanged.
- Control reminders and attack guides are read-only and derive from the authoritative telegraph/training evidence.
- Practice pause freezes elapsed time and rejects action resolution without mutating training evidence; resume restores normal practice behavior.
- Looping focused practice repeatedly emits the selected learned attack, keeps boss health/phase stable, keeps ranked records stable, and can be disabled so the next accepted attempt completes the focused drill.
- Focused stagger-opening practice may report simulated punish damage but never consumes boss health while looping.
- The same protagonist that completed Shadow Crypt and the ranked Warden trial can use the new practice controls through `LandmarkInteraction`; swapping the progression owner blocks all practice mutation.
- Invalid/nonfinite action input and existing pass-32/pass-34 Warden regressions remain green.

## Required verification before merge

- Registered `ThoughtCommandsTests` must compile and execute the pass-35 regression include in Debug and Release on the exact candidate head.
- Existing hosted Windows deterministic suite and Release-manifest integrity workflow must pass on that exact candidate head.
- Independent Codex review must inspect the exact final candidate; material findings must be repaired and re-run before merge.
- Read the full diff, current `main`, expected PR head, review threads, and checks immediately before merge. Use expected-head guarding and never merge if required evidence is missing or stale.
- Native interactive playability remains a separate gate because this packet adds no renderer/menu/controller wiring. Do not claim it from hosted domain tests.
