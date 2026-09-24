# GAME Pass 38: Shadow Crypt skirmish combat

Date: 2026-09-24
Owner: `animerpg-game-hourly`
Baseline: `cc491c87253ccaf8affc3b91055036a7af143e10`
Target: `main`
Working branch: `game/pass38-shadow-crypt-skirmish`

## Scope and ownership

This is dependency-ready GAME work only. It adds room-level Shadow Crypt enemy combat and production-owner integration without changing Astral Engine renderer/platform/editor/import/animation/audio/physics facilities, CMake/workflows, R0, networking, deployment, or unrelated PRs. Native UI/input presentation remains a separate dependency and is not claimed here.

Allowed paths:
- `Engine/Scene/ShadowCryptSkirmish.h`
- `Engine/Scene/LandmarkInteraction.h`
- `Tests/ShadowCryptSkirmishPass38Tests.inc`
- `Tests/ThoughtCommandsTests.cpp`
- this task packet
- `Docs/Agents/animerpg-hourly/PASS38-BACKLOG.json`
- `Docs/Agents/animerpg-hourly/RUN-2026-09-24-PASS38.md`
- `Docs/Agents/animerpg-hourly/STATE.json`

## Research mapping

Sources were accessed 2026-09-24. Comparator mechanics are design references only. No comparator characters, enemy designs, names, art, audio, code, story, maps, monetization, or party architecture are copied.

1. **GAME-187, distinct enemy roles.** Zenless Zone Zero Version 3.2 update announcement, published 2026-09-09, adds multiple named enemy classes and variants. Adaptation: original Shadow Crypt `RiftSkirmisher`, `VeilChanneler`, and `GraveboundBulwark` roles with bounded local stats.
2. **GAME-188, room-scaled formations.** Genshin Impact Stygian Onslaught event details, published 2026-03-02, describe selectable escalating difficulty with tougher enemies and more restrictive rules. Adaptation: Entry Seal, Archive Gallery, and Rift Nave formations increase role variety and bounded durability while staying inside one authored dungeon.
3. **GAME-189, semantic counter cues.** ZZZ Version 3.2 explicitly differentiates attack flashes by whether Defensive Assist can counter the skill. Adaptation: non-color-only `Evade`, `Interrupt`, and `Brace` cues expose the expected response and counterability in data.
4. **GAME-190, interruptible control attack.** ZZZ Version 3.2 adds a response where the first hit of certain enemy Control Skills can be countered with Defensive Assist. Adaptation: Veil Channeler exposes a bounded interrupt window that opens one earned counter opportunity and cannot be consumed twice.
5. **GAME-191, posture break and recovery.** ZZZ Version 3.2 Shiyu Defense continues to use Stun DMG Multiplier and stun-recovery modifiers. Adaptation: Gravebound posture reaches zero once, opens one staggered punish, then restores posture when the punish is consumed rather than allowing stun lock.
6. **QOL-039, sticky manual target focus.** A Genshin player discussion dated 2026-03-30 asks for manual target lock because automatic targeting can select an unintended enemy. Older ZZZ discussions independently describe target switching during multi-enemy fights, while other replies point out that ZZZ already has a manual lock option. Adaptation: a manual Shadow Crypt target stays selected until explicitly cleared or defeated, then deterministic threat recommendation resumes. This is a player preference, not a claim that current comparator games lack target controls.

Sources:
- https://zenless.hoyoverse.com/en-us/news/166000?catchSpider=1&lang=en-us&page=news
- https://www.hoyolab.com/article/44016747
- https://zenless.hoyoverse.com/m/en-us/news/165921
- https://www.reddit.com/r/Genshin_Impact/comments/1s7gon1/i_wish_hoyo_adds_a_targetlock_enemy_feature/
- https://www.reddit.com/r/ZZZ_Official/comments/1ewmn2i

## Acceptance

- Invalid tiers, enum values, defeated targets, and nonfinite/negative reaction times fail closed without gameplay mutation.
- Three original enemy roles have distinct bounded health/posture and room-dependent formations.
- Telegraphs expose semantic cue, expected response, response window, failure damage, and counterability.
- Channel interrupt at the exact boundary succeeds and creates one counter only; duplicate counter attempts fail.
- Bulwark posture is bounded, stagger opens at zero, one follow-up consumes it, and posture recovers.
- Manual target focus is sticky across other enemy actions and automatically releases only when its target is defeated or the player clears it.
- Through production `LandmarkInteraction`, skirmish entry derives from the active authoritative Shadow Crypt room and protagonist owner, failed defense contributes mission damage, and a completed skirmish advances exactly one objective step.
- Existing registered `ThoughtCommandsTests` aggregation includes the pass-38 regression. Hosted Windows Debug/Release and release-manifest checks must pass on the exact accepted head.
- Independent Codex review must complete on the exact accepted head with no unresolved material findings before merge.

## Verification classes

Sandbox C++17 compile/run is supporting domain evidence only. Hosted Windows CI, release-manifest integrity, native interactive play, GPU/performance evidence, and independent review remain distinct. This packet does not modify UI/input/rendering and therefore does not claim native-player-playable verification.
