# GAME-PASS19 Shadowblade combat-flow packet

Date: 2026-09-23
Owner: `animerpg-game-hourly`
Target: `main`
Baseline: `ee0c625712e214b2092a9e419b9e244fa84bae34`
Branch: `game/2026-09-23-shadowblade-combat-flow-pass19`

## Objective

Deepen the existing live `ShadowbladeActions` combat loop rather than add another disconnected backend. Preserve one persistent protagonist, the existing timed defense/counter system, pass-16 through pass-18 loadout/tuning behavior, and the custom Astral Engine. This packet owns only game-domain combat behavior, its registered regressions, and the hourly game-worker records. It does not own renderer/platform/input/editor/import/animation/audio/physics, shared build/CI, persistence formats, R0, release, deployment, or the separate engine/art PRs.

## Five reference increments plus one community increment

1. `GAME-092` — bounded Shadow Momentum from perfect defense. The current Zenless Zone Zero developer description presents Dodge/Parry as active counterplay and Stun as the opening for stronger chained offense; Wuthering Waves' current developer App Store description explicitly calls out Extreme Evasion and Dodge Counter. Astral adaptation: a perfect guard or perfect dodge grants one stack of a local Shadow Momentum resource, capped at three, without replacing the existing defense-counter window. Sources: https://apps.apple.com/us/app/zenless-zone-zero/id1606356401 and https://apps.apple.com/us/app/wuthering-waves/id6475033368 .
2. `GAME-093` — one-stack Momentum Fatal Strike payoff. ZZZ's current developer description links defensive execution to a stronger offensive sequence after control openings, while Genshin Impact's current PlayStation description emphasizes combat payoffs created by combining mechanics rather than isolated attacks. Astral adaptation: an otherwise-valid Fatal Strike consumes at most one Shadow Momentum stack for a bounded damage bonus. Failed, blocked, out-of-range, cooldown, or defeated-target attempts cannot spend momentum. Sources: https://apps.apple.com/us/app/zenless-zone-zero/id1606356401 and https://www.playstation.com/en-us/games/genshin-impact/ .
3. `GAME-094` — Riftsteel follow-up efficiency. Granblue Fantasy: Relink's current PlayStation page explicitly differentiates combatants by unique weapons, skills, and combat styles. Astral adaptation: the already-owned Riftsteel Sabre gets one narrow identity perk: earned stagger/defense-counter Fatal Strikes cost five less Shadow resource, without changing normal Fatal Strike cost or level/ownership gates. Source: https://www.playstation.com/en-us/games/granblue-fantasy-relink/ .
4. `GAME-095` — CryoEdge perfect-guard recovery. Genshin's current official PlayStation description emphasizes distinct abilities/combat styles and using mechanics together for combat advantage; ZZZ explicitly distinguishes Dodge and Parry responses. Astral adaptation: CryoEdge restores a bounded 20 Guard Integrity only on Perfect Guard, never on ordinary guard, perfect dodge, or unblockable impact. Sources: https://www.playstation.com/en-us/games/genshin-impact/ and https://apps.apple.com/us/app/zenless-zone-zero/id1606356401 .
5. `GAME-096` — Training Blade perfect-dodge mobility recovery. Wuthering Waves' current developer description pairs high-mobility traversal with Extreme Evasion and Dodge Counter. Astral adaptation: with Training Blade equipped, Perfect Dodge removes up to 0.5 seconds from the existing dash cooldown; ordinary evasion does not. This is a bounded cooldown reduction, not unlimited dodge/dash chaining. Source: https://apps.apple.com/us/app/wuthering-waves/id6475033368 .
6. `QOL-020` — explicit action-readiness telemetry. A ZZZ mobile-player discussion dated 2024-08-07 reports that dodge cooldown and assist-point indicators can be obscured by the player's thumb; replies include independent agreement that the dodge-cooldown indicator is hard to see, while other replies say they did not experience the problem. This is historical anecdotal feedback, not consensus. Later ZZZ updates changed mobile/control layouts, but the research in this pass did not establish that the specific cooldown-visibility complaint is currently resolved. Astral adaptation: expose a deterministic non-rendered readiness snapshot for Shadow resource, dash/Fatal cooldown remaining and normalized cooldown, base resource affordability, guard state, and current Shadow Momentum so a later owned UI can present the information without scraping private state. Source: https://www.reddit.com/r/ZZZ_Official/comments/1emgtux/the_main_negative_of_playing_in_mobile/ .

Access/revalidation date for all sources: 2026-09-23.

## Repository gap

Passes 17-18 made loadout/tuning stats affect live `ShadowbladeActions`, but all three weapon frames still share the same action rules. Timed perfect defense grants the existing short defense-counter window but no bounded persistent combat-flow resource. Fatal Strike has no earned stack payoff. The action owner also has no single structured readiness snapshot for a future game UI, forcing any future display to query and reinterpret several fields independently.

## Allowed paths

Production:
- `Engine/Scene/ShadowbladeActions.h`
- `Engine/Scene/ShadowbladeActions.cpp`

Verification:
- `Tests/ShadowbladeCombatFlowPass19Tests.inc`
- `Tests/ThoughtCommandsTests.cpp`, include/call registration only

Operating records:
- this packet
- `Docs/Agents/animerpg-hourly/STATE.json`
- `Docs/Agents/animerpg-hourly/RUN-2026-09-23-PASS19.md`

No other path is authorized by this packet.

## Acceptance

- Shadow Momentum is an integer in `[0,3]`; only Perfect Guard/Perfect Dodge grant it, repeated perfect defenses saturate rather than wrap, ordinary guard/evade/hit/invalid defense do not grant it, and defense reset clears it.
- A valid Fatal Strike with momentum consumes exactly one stack and adds exactly 12 bounded damage before target-health clamping. Any Fatal Strike rejection leaves momentum unchanged.
- Riftsteel Sabre reduces only earned stagger or defense-counter Fatal Strike resource cost by exactly 5, never below zero, while ordinary Fatal Strike stays at the existing cost.
- CryoEdge Perfect Guard restores at most 20 Guard Integrity and never exceeds `MaximumGuardIntegrity`; ordinary Guard, Perfect Dodge, and unblockable hits do not trigger the recovery.
- Training Blade Perfect Dodge reduces the current dash cooldown by at most 0.5 seconds, clamped at zero; ordinary Evade does not reduce it beyond ordinary time advancement.
- `CurrentActionReadiness()` reports finite/clamped cooldown values and normalized fractions in `[0,1]`, authoritative resource, base affordability, guard state, and Shadow Momentum without mutating combat state.
- `ResetTransientStatePreservingLoadout()` continues to preserve loadout/tuning while clearing transient momentum/action state.
- Existing Shadowblade, combat, defense, practice, loadout, tuning, Thought Command, and Release regressions remain green.

## Verification gates

Use the repository's existing registered deterministic tests and hosted Windows Debug/Release workflow. No CMake/workflow change is allowed. Require the release-manifest integrity workflow and fresh independent Codex review on the exact final head before merge. The production changes are game-domain logic only, so native rendered UI/controller evidence is not claimed by this packet. Native interactive play remains a separate evidence class.

## Stop conditions

Stop rather than expanding scope if implementation requires Win32/platform input, renderer/editor/import/animation/audio/physics, shared build/CI, persistence-format work, another worker's open PR, architecture/dependency changes, Company Runtime invocation, release, deployment, or weakening an existing acceptance check.
