# GAME Pass 41: Shadow Crypt Pressure States

Owner: `animerpg-game-hourly`

Target: `main`

Baseline: `2958741188279a0b438dd489ad00cac546012c25`

Worker branch: `game/animerpg-hourly-pass41-status-pressure`

## Scope and authority

This packet is GAME work only. It preserves the custom C++17 Astral Engine and the existing modern-supernatural Washington DC game. It does not touch renderer, platform, editor, importer, animation, audio, physics, shared CMake/workflows, dependencies, networking, deployment, releases, R0, or another worker's PR. The existing `ShadowCryptSkirmish` game-domain module is extended in place rather than replaced by a parallel framework.

## Research mapping, accessed 2026-09-24

1. **GAME-202, Rift Mark:** Zenless Zone Zero 3.2's developer-announced Claret mechanics use target-side Gash stacking and Maim follow-up damage/buffs. Original adaptation: one source-tracked Rift Mark after a failed Rift Skirmisher read, with a bounded +6 surcharge on the next authoritative failed-defense telegraph and one-shot consumption. Source: https://www.gematsu.com/2026/08/zenless-zone-zero-version-3-2-update-their-secret-histories-launches-september-9, published 2026-08-28, quoting miHoYo.
2. **GAME-203, Veil Ward:** Granblue Fantasy: Relink - Endless Ragnarok Ver. 2.0.2 documents Shield effects, stackable status effects, and status-effect visibility. Original adaptation: a failed Veil Channeler interrupt creates a deterministic 14-point health-only ward on a living ally. Source: https://relink-ragnarok.granbluefantasy.com/en/updates/381/, published 2026-07-08 and updated 2026-09-04.
3. **GAME-204, Gravebound Guard Link:** the same Granblue update explicitly supports perfect guards during ongoing actions and preserves combo continuity. Original adaptation: a Bulwark-created one-attack protection link absorbs exactly 10 posture pressure on one ally rather than copying a playable-character guard system.
4. **GAME-205, pressure-aware target recommendation:** ZZZ's official `Combat Training - Triple Bounty` event lets players select enemy cards and rotates buffed enemies, reinforcing explicit enemy-state-aware planning. Original adaptation: a living enemy sourcing active pressure gets a bounded +60 recommendation priority, while manual lock remains authoritative. Source: https://zenless.hoyoverse.com/m/en-us/news/165921, published 2026-08-31.
5. **GAME-206, skillful pressure purge:** Wuthering Waves' current developer listing explicitly describes Extreme Evasion and Dodge Counter; Granblue 2.0.2 documents perfect-guard timing. Original adaptation: pressure can be purged through an existing precision semantic defense or a role-matched earned breach exploit, without weakening timing windows. Source: https://apps.apple.com/us/app/wuthering-waves/id6475033368, accessed 2026-09-24.
6. **QOL-042, read-only combat status ledger:** a ZZZ player post dated 2026-09-23 says important buff uptimes can be difficult to track when some effects are not surfaced in combat UI/effect details. A separate 2026-09-15 custom-HUD discussion includes a request for an optional high-information buff view. These are anecdotes, not consensus, and do not establish that current ZZZ lacks all status indicators. Original adaptation: a bounded presentation-agnostic status ledger exposes exact kind/source/target/magnitude without taking renderer/HUD ownership. Sources: https://www.reddit.com/r/ZZZ_Discussion/comments/1woiqg5/most_important_buffs_not_displayed_in_the_ui/ and https://www.reddit.com/r/ZenlessZoneZero/comments/1wgp9w0/my_custom_zzz_hud_now_tracks_wengine_buffs_claret/.

## Repository gaps and implementation

`ShadowCryptSkirmish` already has semantic threat reads, one-shot counters, stagger, breaches, Shadow Flow, target recommendation/manual lock, and room replay. It did not have persistent enemy-created combat pressure or an exact read-only status contract. This packet adds three role-specific, transient pressure states and two tactical interactions with those states, plus one status-introspection QoL surface.

- **GAME-202:** failed Rift Skirmisher read applies one Rift Mark. Existing 40-damage threat ceiling remains authoritative. The true marked damage is included in `CurrentThreat()` so downstream mission accounting cannot diverge from resolution.
- **GAME-203:** failed Veil Channeler read creates one 14-point ward on the deterministic lowest-health-ratio living ally. Health absorption is reported exactly and does not alter posture.
- **GAME-204:** failed Gravebound Bulwark read creates one guard link to the deterministic highest-priority living ally. The next attack against that ally loses exactly 10 nominal posture pressure, bounded at zero.
- **GAME-205:** `RecommendedTarget()` adds +60 only for a living source of currently active pressure. Existing stagger/counter/role priority and manual target lock remain intact.
- **GAME-206:** precision defense against a pressure source or role-matched breach exploitation purges that source's active pressure. Ordinary successful reads do not purge it.
- **QOL-042:** `StatusLedger()` returns at most five deterministic entries with exact kind/source/target/magnitude. Reading it does not mutate combat.

## Allowed paths

- `Engine/Scene/ShadowCryptSkirmish.h`
- `Tests/ShadowCryptSkirmishPass41Tests.inc`
- `Tests/ThoughtCommandsTests.cpp`
- `Tasks/GAME-PASS41-SHADOW-CRYPT-PRESSURE-STATES-2026-09-24.md`
- `Docs/Agents/animerpg-hourly/PASS41-BACKLOG.json`
- `Docs/Agents/animerpg-hourly/RUN-2026-09-24-PASS41.md`
- `Docs/Agents/animerpg-hourly/STATE.json`

## Acceptance and regression requirements

- Rift Mark: first failure retains base damage, next telegraph exposes the +6 surcharge, old mark is one-shot, cap remains 40.
- Veil Ward: deterministic target, durability exactly 14, health-only absorption, actual absorbed amount reported, no negative durability.
- Guard Link: deterministic target, exactly 10 posture protected once, link becomes inactive when its source is staggered/defeated.
- Pressure targeting: active source can outrank static priority; dead/inactive source does not; manual lock is unchanged.
- Purge: non-precision success preserves pressure; precision source read purges; matching earned breach purges.
- Ledger: deterministic bounded order, exact source/target/magnitude, repeated reads do not mutate, NaN timing rejection does not mutate, cancel resets all transient entries.
- Existing pass-38/pass-40 semantics and production `LandmarkInteraction` mission-damage synchronization must remain green.
- Debug and Release registered test targets must pass on the exact final head. Release assertions must remain meaningful. Existing release-manifest integrity gate must pass on that same head.
- Fresh independent review must inspect the exact final head. Self-review or CI is not a substitute.

## Verification boundary

A local Linux/C++17 smoke compile is only a syntax/domain sanity check. Hosted Windows deterministic CI and release-manifest integrity are separate. This packet changes no Win32/controller/menu/renderer path, so native interactive/player-playable verification remains unclaimed unless separate applicable evidence exists. No art/audio/VFX, GPU/performance, cross-process status persistence, or Genshin/ZZZ parity is claimed.
