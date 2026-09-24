# GAME pass 33: Shadowblade progression combat integration

Date: 2026-09-24  
Owner: `animerpg-game-hourly`  
Baseline main: `23a0fd258b170864893cf953b25ddf2c7e99491a`  
Branch: `game/pass33-shadowblade-progression-combat`

## Scope

Deepen the existing persistent single-protagonist Shadowblade progression by making already-earned/equipped core talents and action-family skill ranks affect the existing production combat economy. Preserve all existing base action APIs and the custom C++17 Astral Engine. Do not touch renderer/platform/editor/import/animation/audio/physics, shared CMake/workflows, networking, R0, release/deployment, other workers' PRs, or native input/UI ownership.

## Research mapping, accessed 2026-09-24

1. `GAME-162`, equipped specialization authority. The current Genshin PlayStation overview describes characters with different abilities/combat styles. Astral keeps one persistent protagonist and makes only actually equipped Shadowblade talents affect combat, instead of copying a roster/swap model. Source: https://www.playstation.com/en-us/games/genshin-impact/
2. `GAME-163`, ShadowStep dash efficiency. Current ZZZ developer text explicitly makes Dodge part of fast-paced combat. Astral adapts movement specialization into a bounded resource refund on the existing dash, without changing its distance/cooldown or engine input. Source: https://apps.apple.com/us/app/zenless-zone-zero/id1606356401
3. `GAME-164`, EclipseEdge counter efficiency. Current ZZZ developer text explicitly pairs Dodge/Parry with neutralizing counterattacks. Astral makes an equipped talent reduce only the existing earned perfect-defense counter follow-up cost. Source: https://apps.apple.com/us/app/zenless-zone-zero/id1606356401
4. `GAME-165`, BreakerFocus stagger follow-up efficiency. Current ZZZ developer text says Stun opens powerful Chain Attacks. Astral keeps its single-protagonist stagger opening and makes the equipped BreakerFocus talent reduce only the existing stagger follow-up cost. Source: https://apps.apple.com/us/app/zenless-zone-zero/id1606356401
5. `GAME-166`, action-family mastery has live effects. Granblue Fantasy: Relink's current PlayStation page describes unique weapons/skills/combat styles plus reviewable practice/tutorial support. Astral converts the already-persistent Dash/FatalStrike/Defense practice ranks into small bounded live action-economy effects, not a second progression currency. Source: https://www.playstation.com/en-us/games/granblue-fantasy-relink/
6. `QOL-034`, exact build-impact briefing. An original ZZZ player post dated 2026-08-04 asks for build explanation and says builds can be confusing; a separate 2026-08-24 returning-player post asks whether to use personal or suggested builds and notes that the game already has suggestions/readiness grades. Astral therefore exposes exact read-only talent/rank deltas and effective costs instead of claiming ZZZ lacks recommendations. Sources: https://www.reddit.com/r/ZZZ_Official/comments/1vf2z4k/builds/ and https://www.reddit.com/r/ZZZ_Official/comments/1vx3xoy/so_hey_long_time_players_when_yall_do_the_builds/

Community evidence is anecdotal/corroborating, not consensus. Current exact-delta resolution in ZZZ remains unestablished; the official Version 3.2 listing does not establish whether this exact explanation request was addressed.

## Allowed paths

Production:
- `Engine/Scene/ShadowbladeActions.h`

Registered game test aggregation:
- `Tests/ShadowbladeProgressionCombatPass33Tests.inc`
- `Tests/ThoughtCommandsTests.cpp`

Operating records:
- `Tasks/GAME-PASS33-SHADOWBLADE-PROGRESSION-COMBAT-2026-09-24.md`
- `Docs/Agents/animerpg-hourly/PASS33-BACKLOG.json`
- `Docs/Agents/animerpg-hourly/RUN-2026-09-24-PASS33.md`
- `Docs/Agents/animerpg-hourly/STATE.json`

## Acceptance

- Existing base `TryDash`, `TryFatalStrike`, and `TryDefend` semantics remain unchanged when progression-aware APIs are not used.
- Upgraded but unequipped talents provide no combat bonus.
- ShadowStep applies only to dash; EclipseEdge applies only to a defense-counter follow-up; BreakerFocus applies only to a stagger follow-up.
- Dash/FatalStrike/Defense skill ranks produce bounded exact effects and resource always remains in `[0,100]` through the existing `RestoreResource` cap.
- Rejected actions receive no refund; ordinary Fatal Strike does not receive counter/stagger talent refund.
- The player-facing progression briefing is read-only and reports exact talent tiers, ranks, refunds and effective costs.
- Registered regression tests exercise standard action paths plus equipped/unequipped, counter, stagger, perfect-defense and briefing behavior.
- Hosted Windows Debug/Release and Release-manifest gates must pass on the exact final head.
- Fresh independent review must be clean on the exact final head before merge.
- Native interactive/playable acceptance is not claimed because this packet does not own or change Win32/controller/menu UI wiring.

## Stop conditions

Stop and preserve the PR if production compilation/tests fail, shared-engine changes become necessary, an independent reviewer finds an unresolved material defect, exact-head hosted checks are absent/failed, main moves incompatibly, or native ownership would have to be bypassed. Do not weaken a regression or broaden into UI/engine work to meet the quota.
