# GAME pass 40: Shadow Crypt mastery flow and immediate room replay

Date: 2026-09-24  
Owner: `animerpg-game-hourly`  
Baseline: `43051845a6f663cdd2942ce4d321f2c9583e1dcb`  
Target: `main`  
Branch: `game/pass40-shadow-crypt-flow-replay`

## Authority and ownership

This is a bounded GAME packet under Lucas's September 22 standing game-worker authorization in `GAME_DEVELOPMENT_CONTROL.md`, `AGENTS.md`, and `Docs/Agents/ANIMERPG-HOURLY.md`. It deepens the already-merged Shadow Crypt skirmish domain without taking Astral Engine ownership. The custom C++17 engine, modern supernatural Washington DC setting, persistent customizable protagonist, Shadowblade-first development, later Arc Mage/Aegis Tank direction, Shadow Crypt/Mana Reactor content, destructibility direction, and bounded Thought Commands remain unchanged.

Live discovery at admission found `main` at `43051845a6f663cdd2942ce4d321f2c9583e1dcb`. Open engine/art/editor PRs remain owned by their existing workers and are not merged or modified by this packet. No renderer, platform, editor, importer, animation, audio, physics, shared CMake/workflow, dependency, R0, networking, release, deployment, or local scheduler path is in scope.

Allowed production path:
- `Engine/Scene/ShadowCryptSkirmish.h`

Allowed verification/record paths:
- `Tests/ShadowCryptSkirmishPass40Tests.inc`
- `Tests/ThoughtCommandsTests.cpp`, registration only
- this task packet
- `Docs/Agents/animerpg-hourly/PASS40-BACKLOG.json`
- `Docs/Agents/animerpg-hourly/RUN-2026-09-24-PASS40.md`
- `Docs/Agents/animerpg-hourly/STATE.json`

`LandmarkInteraction` remains unchanged. GAME-197 through GAME-201 execute through the already-live `ResolveThreat` / `AttackTarget` skirmish used by the production owner. QOL-041 is deliberately a core game-domain replay API in this packet; it is tested but not claimed as menu/controller/player-playable until an owned production command path calls it.

## Research map

Sources accessed 2026-09-24. Comparator mechanics are design lessons only. No proprietary characters, attacks, assets, story, audio, maps, code, monetization, or collectible-character architecture is copied.

1. **GAME-197, role weakness intel.** The current Zenless Zone Zero developer listing describes Dodge/Parry combat and advises using different opponents' traits and weaknesses. Astral exposes original Shadow Crypt role intel with a deterministic recommended attack style. Source: https://apps.apple.com/us/app/zenless-zone-zero/id1606356401, current version history shows 3.2.0 dated 2026-09-08.
2. **GAME-198, defense-created breach opening.** Wuthering Waves' current developer listing names Extreme Evasion and Dodge Counter as linked defensive/offensive combat tools. Astral adapts that interaction into one source-specific breach opening created by a correct semantic defense. Source: https://apps.apple.com/us/app/wuthering-waves/id6475033368.
3. **GAME-199, weakness exploit payoff.** ZZZ's opponent-trait/weakness guidance and Granblue Fantasy: Relink's distinct weapons/skills/combat styles support readable matchup decisions. Astral gives the next accepted attack against an opened enemy a bounded +10 health / +8 posture bonus only when it matches that original role's recommended style. Sources: ZZZ listing above and https://www.playstation.com/en-us/games/granblue-fantasy-relink/.
4. **GAME-200, bounded Shadow Flow.** ZZZ's Dodge/Parry/Stun loop and Wuthering Waves' evasion-counter cadence are used as references for rewarding consecutive correct reads. Astral adds a single-protagonist three-step Shadow Flow meter, capped at 3; accepted failed defense resets it, while rejected invalid timing cannot mutate it. Sources: ZZZ and Wuthering Waves listings above.
5. **GAME-201, earned Flow Break.** Granblue Fantasy: Relink documents Link Attacks and Chain Bursts, while ZZZ documents Stun into stronger follow-up attacks. Astral does not add party switching. Full Shadow Flow instead empowers only the next accepted Heavy attack with a bounded +12 health / +10 posture bonus, then consumes the meter exactly once. Sources: Granblue and ZZZ listings above.
6. **QOL-041, immediate completed-room replay.** A ZZZ player post dated 2026-09-18 asks for direct replay after finishing a Shiyu boss instead of returning through the main menu, and replies explicitly describe value for score chasing. The same thread notes Deadly Assault already has a direct retry path. An older 2024-08-03 player thread independently describes in-fight battle restart as useful. These are player anecdotes, not consensus or an official current UI contract. Astral adds a direct same-tier room replay primitive after a completed skirmish. Sources: https://www.reddit.com/r/ZZZ_Official/comments/1wjfjm3/a_very_nitpick_about_endgame_mode/ and https://www.reddit.com/r/ZenlessZoneZero/comments/1ej6nzi/you_should_know_retrying_individual_battles_in/ .

## Acceptance criteria

- `GAME-197`: every live room enemy exposes bounded role intel and a deterministic recommended attack; out-of-range intel fails closed; reading intel does not advance threats or mutate combat.
- `GAME-198`: a successful semantic defense opens a breach only for the source enemy; attacks against other enemies do not consume it; the next accepted attack against that source consumes it once; a failed read against that source closes it.
- `GAME-199`: a consumed breach grants +10 health and +8 nominal posture only when the accepted attack matches the role recommendation; nonmatching attacks consume the opening without the bonus; consumed openings cannot be farmed.
- `GAME-200`: consecutive successful semantic defenses increment Shadow Flow from 0 to a hard cap of 3; rejected/nonfinite timing does not mutate it; an accepted failed defense resets it to zero.
- `GAME-201`: only an accepted Heavy attack at full Shadow Flow consumes the meter and adds +12 health / +10 nominal posture; Light/Counter do not spend the meter; a second Heavy has no bonus unless flow is rebuilt.
- `QOL-041`: replay is rejected while active or before completion; after completion it restarts the same tier with fresh enemies, zero transient damage, zero Shadow Flow, cleared breach/counter/target state, and an explicit replay-mode marker. This packet does not claim a production menu/input command for replay.

Pass-39 regression coverage remains registered and must stay green, including formations, deterministic variants, low-health escalation, exact-boundary and nonfinite timing, precision counter values, Bulwark brace posture truthfulness, stagger recovery/lethal suppression, sticky targeting, threat queue behavior, and production mission damage/objective lifecycle.

## Verification and merge contract

Author-side portable C++17 smoke compiled and ran successfully against the proposed production header with:

```text
g++ -std=c++17 -Wall -Wextra -Werror -pedantic -I/tmp /tmp/pass40_test.cpp -o /tmp/pass40_test
/tmp/pass40_test
# PASS
```

This is source-level Linux evidence only. Before merge, the exact final PR head must pass the repository's hosted Windows Debug/Release deterministic workflow and release-manifest integrity workflow, including the registered `ThoughtCommandsTests`. A fresh independent Codex review must bind to that exact head with no unresolved material finding. Re-read `main`, PR head, diff, checks, and review threads immediately before an expected-head merge; if `main` moves, reconcile and rerun affected gates.

Native Win32/controller/menu presentation, production VFX/audio, hands-on combat feel, GPU/performance evidence, cross-process persistence, release/deployment, and Genshin/ZZZ quality parity remain unclaimed.
