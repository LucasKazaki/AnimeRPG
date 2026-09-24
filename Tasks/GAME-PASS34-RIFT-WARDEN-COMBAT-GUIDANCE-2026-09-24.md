# GAME pass 34: Rift Warden combat guidance

Owner: `animerpg-game-hourly`  
Target: `main`  
Baseline: `4c6cc97cacd191f8441b69a1d51d40914bdc88b2`  
Branch: `game/pass34-rift-warden-combat-guidance`

## Bounded scope

Deepen the already-merged Rift Warden game-domain combat loop without changing Astral Engine infrastructure, renderer/platform/editor/import/animation/audio/physics, shared CMake/workflows, networking, release/deployment, R0, or other workers' PRs.

Allowed production path:
- `Engine/Scene/RiftWardenTrial.h`

Registered regression path reused to avoid shared test-runner ownership changes:
- `Tests/ShadowbladeProgressionCombatPass33Tests.inc`

Operating records:
- this task packet
- `Docs/Agents/animerpg-hourly/PASS34-BACKLOG.json`
- `Docs/Agents/animerpg-hourly/RUN-2026-09-24-PASS34.md`
- `Docs/Agents/animerpg-hourly/STATE.json`

## Five plus one

- `GAME-167`: semantic Evade/Brace/Punish cue on every Warden telegraph, independent of color rendering.
- `GAME-168`: persistent-in-session learned-attack catalog driven only by accepted live/practice resolutions.
- `GAME-169`: bounded per-attack attempts, successes, and best success streak, retained across attempt resets.
- `GAME-170`: deterministic weakest-learned-attack practice recommendation using existing phase-practice capability.
- `GAME-171`: exact response-match and signed timing-margin feedback on every accepted Warden action.
- `QOL-035`: post-mistake coach that distinguishes wrong response, late response, and missed punish while reporting the expected action/cue/window.

## Research basis

Official material accessed 2026-09-24:
- Zenless Zone Zero current App Store developer description: Dodge/Parry counterplay, Stun into Chain Attacks, and opponent-specific traits. `https://apps.apple.com/us/app/zenless-zone-zero/id1606356401`
- Granblue Fantasy: Relink PlayStation page: adjustable assists, Control Reminders, Tutorial Reminders, Practice Mode, and pausing. `https://www.playstation.com/en-us/games/granblue-fantasy-relink/`

Community basis for `QOL-035`:
- ZZZ player discussion, 2026-06-19, asking how orange/red/gold combat prompts map to dodge/parry and where to relearn them. Replies explain the mappings and point to an existing tutorial. `https://www.reddit.com/r/ZenlessZoneZero/comments/1uag4q6/dodging/`
- Independent corroboration, 2026-06-28, requests better visual clarity because effects can obscure enemy tells in reaction-heavy combat. `https://www.reddit.com/r/ZZZ_Discussion/comments/1ui9kg4/visual_clarity_isnt_great/`

These are player anecdotes, not consensus. ZZZ already has tutorial/help surfaces according to the discussion, and this packet does not claim otherwise. Current official material inspected does not establish an exact post-mistake wrong-action-versus-late-timing breakdown, so current resolution of that narrower request remains unestablished.

## Acceptance

- Invalid enums and nonfinite/negative reaction times fail before training/coaching mutation.
- Cue semantics are deterministic for all four Warden attack states.
- Training counters saturate at explicit safe bounds and survive attempt resets without affecting ranked records/unlocks.
- Recommendation is deterministic and based only on actually learned attack evidence.
- Timing margin is exact at the inclusive boundary, positive early, negative late.
- Coach is updated only by accepted mistakes and preserves expected response, semantic cue, timing window, and observed reaction.
- Existing pass-32 boss timing, scoring, practice, saturation, owner, and record tests remain green.
- Hosted Windows Debug/Release and Release-manifest checks must pass on the exact final head.
- Fresh independent Codex review must be clean on the exact final head before merge.
- Native interactive/playable status remains unclaimed because no input/UI/rendered-scene path is changed.
