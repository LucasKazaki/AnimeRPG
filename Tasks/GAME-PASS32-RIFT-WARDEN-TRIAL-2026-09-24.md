# GAME pass 32: Rift Warden mastery trial

Owner: `animerpg-game-hourly`  
Target: `main`  
Baseline: `1a575690df46ba5cff7e00b01a15f4256a844113`  
Branch: `game/2026-09-24-rift-warden-trial-pass32`

## Scope and ownership

This is a bounded GAME-domain packet. It deepens the already-merged Shadow Crypt by adding an original, deterministic post-clear Rift Warden mastery trial. It does not change Astral Engine renderer, platform, editor, importer, animation, audio, physics, shared CMake/workflows, dependencies, R0, networking, deployment, release, or another worker's PRs.

Allowed paths:
- `Engine/Scene/RiftWardenTrial.h`
- thin production-owner integration in `Engine/Scene/LandmarkInteraction.h`
- `Tests/RiftWardenTrialPass32Tests.inc`
- test registration only in `Tests/ThoughtCommandsTests.cpp`
- this task and `Docs/Agents/animerpg-hourly/` pass records

No proprietary mechanics, characters, bosses, animations, art, audio, maps, code, monetization, or party architecture are copied. The Rift Warden remains original Shadow Crypt content tied to the project's supernatural Washington DC setting.

## Research mapping, accessed 2026-09-24

Official/developer sources:
- Genshin Impact official Stygian Onslaught event details, published 2026-03-02: https://www.hoyolab.com/article/44016747. It requires selecting a difficulty, uses three time-limited combat phases, orders difficulty from Normal through Dire, and from Hard onward unlocks the next difficulty only after clearing the current one. Higher clears also grant lower-difficulty rewards.
- Zenless Zone Zero official `Combat Training - Triple Bounty`, published 2026-08-31: https://zenless.hoyoverse.com/m/en-us/news/165921. Combat Simulation is story-gated, lets players select enemy cards, and rewards completed challenges.
- Zenless Zone Zero App Store developer description, rechecked 2026-09-24: https://apps.apple.com/us/app/zenless-zone-zero/id1606356401. It describes Basic/Special attacks, Dodge and Parry responses, opponent Stun, and follow-up Chain Attacks. This is used only as a timing/stagger interaction lesson, not a party-switch requirement.
- Granblue Fantasy: Relink PlayStation overview, rechecked 2026-09-24: https://www.playstation.com/en-us/games/granblue-fantasy-relink/. It documents adjustable difficulty, individually activatable assists, control/tutorial reminders, consequence-free Practice Mode, and pausing.

Community practice request, original player discussion read 2026-09-24:
- ZZZ Official subreddit discussion dated 2026-03-02: https://www.reddit.com/r/ZZZ_Official/comments/1rikhmx/how_do_you_guys_like_to_test_your_teams/. The original poster says Free Combat is not sufficient for how they want to test teams. A reply specifically asks for more training options, more enemy varieties, and bosses in practice; another reply says every boss should be available to understand boss-specific stun/anomaly behavior. Other replies say they already use Free Training for stationary timing work or simply test in live challenge modes, so this is a player preference with counterexamples, not community consensus.

Current ZZZ official Combat Training material from 2026-08-31 confirms selectable enemy cards in Combat Simulation, but the inspected official source does not establish that every boss or arbitrary learned boss phase is available consequence-free. The exact current resolution of the March player request is therefore unestablished.

## Six bounded increments

### GAME-157: Shadow Crypt-gated Rift Warden mastery trial
Gap: Shadow Crypt currently ends after one abstract Warden objective step; there is no reusable post-clear boss-mastery challenge.
Adaptation: after the authoritative production Shadow Crypt is complete, the same persistent protagonist owner can start one original Rift Warden trial. Story difficulty is initially available; fresh owners and owner swaps fail closed.
Acceptance: no completed Shadow Crypt means no entry; no progression owner means no entry; one active run cannot be overwritten; protagonist-owner swaps cannot mutate the active run.

### GAME-158: deterministic three-phase boss attack pattern
Gap: the existing Warden objective has no attack sequencing or phase identity.
Adaptation: Opening, Fracture, and Overload each have a distinct deterministic order using original Rift Slash, Gravity Pulse, and Echo Burst attacks. A read-only telegraph reports the current attack and recommended response.
Acceptance: repeated telegraph reads do not advance state; phase patterns are deterministic; health thresholds change phase exactly once in the expected direction; invalid enum state fails closed where accepted from callers.

### GAME-159: difficulty- and phase-specific timed defense
Gap: the existing dungeon objective does not measure defensive execution against boss attacks.
Adaptation: Dodge or Guard must match the current telegraph within a bounded response window. Story, Standard, and Expert tighten timing, while later phases tighten it further. Late otherwise-correct input becomes an authoritative hit instead of being silently accepted.
Acceptance: negative/nonfinite timing is rejected without mutation; inside/outside boundary behavior is deterministic; incoming damage remains bounded; timing windows stay positive.

### GAME-160: posture break and one-shot punish opening
Gap: the Warden objective has no readable stagger/recovery cycle.
Adaptation: successful defenses build bounded Warden posture. Reaching the cap opens one finite strike-only punish opportunity; consuming or missing it resets posture before the next cycle.
Acceptance: posture never exceeds its cap; one opening cannot be punished twice; a wrong action during the opening deals no boss damage; defeat/phase transition cannot leave a stale stagger opening.

### GAME-161: progressive difficulty unlocks and per-difficulty mastery records
Gap: the dungeon has one best-clear summary but no dedicated boss mastery ladder.
Adaptation: Story clear unlocks Standard, Standard clear unlocks Expert, and each difficulty owns a monotonic Bronze/Silver/Gold best record using damage, defense execution, clear time, score, and streak witnesses.
Acceptance: locked difficulty rejects entry; clears only unlock the next bounded tier; records are separated by difficulty; lower-mastery/worse replay cannot roll back a better record.

### QOL-033: learned-phase boss practice
Gap: a recent ZZZ player discussion asks for more enemy variety and boss practice in Free Training so timing and team execution can be learned without repeatedly entering live challenge content.
Adaptation: after at least one ranked clear, the Rift Warden can be rehearsed consequence-free from Opening, Fracture, or Overload on an unlocked difficulty. Practice starts at bounded phase-appropriate health and is explicitly non-ranked.
Acceptance: practice is unavailable before a ranked clear; `Defeated` is never a practice start; practice cannot alter ranked records or difficulty unlocks; normal ranked runs remain unchanged.

## Verification requirements

The registered `ThoughtCommandsTests` aggregation must compile and execute pass-32 regressions in Debug and Release. Required coverage includes Shadow Crypt/progression ownership gates, deterministic/non-mutating telegraphs, invalid/nonfinite reaction timing, late-hit behavior, bounded posture, missed and successful punish windows, bounded completion, sequential difficulty unlocks, separate records, phase practice, practice non-ranking, and production-owner mutation blocking after a progression-pointer swap. All previously registered game regressions remain in the same aggregate.

Exact-final-head acceptance requires:
1. hosted Windows Debug/Release deterministic workflow success,
2. hosted Release-manifest integrity success,
3. fresh independent implementation review on that exact head with no unresolved material findings,
4. full diff/scope review and live `main`/PR-head reread immediately before expected-head merge.

Native rendered boss presentation, controller/menu wiring, boss animation/VFX/audio, physical collision, GPU/performance evidence, save persistence across processes, and hands-on native playtesting are not implemented by this packet and must not be claimed.
