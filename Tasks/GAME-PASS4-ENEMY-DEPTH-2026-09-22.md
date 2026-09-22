# GAME PASS 4: Enemy depth, technique scoring, and training variety

Date: 2026-09-22
Owner: animerpg-game-hourly
Dependency baseline: pass-3 head `77c37a4483de9169bdecbd922866f9937e5ca07f`, merged to `main` as `71e5eeb9c10bdd2b0860acb4d761334bd2336de3`.
Target: `main`

## Scope

This packet is game-domain only. Allowed implementation paths are `Engine/Scene/CombatSandbox.*`, `Engine/Scene/LandmarkInteraction.*`, `Tests/LandmarkEncounterTests.cpp`, and this worker's task/receipt/state files. It does not modify renderer, platform, editor, CMake, workflows, dependencies, the R0 runner, or engine-worker PRs.

## Five comparator-derived increments

1. `GAME-017` Weakness-aware training enemies. Add original Standard, Vanguard, and Bulwark profiles with bounded health/posture differences and optional Solar/Umbral weaknesses. Category references: ZZZ currently advises exploiting opponent traits/weaknesses; Granblue Relink emphasizes varied enemies and combat roles.
2. `GAME-018` Weakness-sensitive Eclipse. When the second opposing affinity matches the selected enemy weakness, Eclipse receives one bounded bonus rather than multiplying arbitrary damage. Category reference: Genshin elemental interaction and ZZZ weakness exploitation.
3. `GAME-019` Eclipse finisher opening. A successful nonlethal Eclipse arms one original single-protagonist follow-up. A valid combo finisher receives a bounded bonus and consumes it; invalid/out-of-range attempts preserve it. Category references: ZZZ stun-to-Chain-Attack flow and Granblue Link Attacks/Chain Bursts.
4. `GAME-020` Technique-chain challenge scoring. Reaction, stagger, and finisher techniques build a capped short-window chain. Score combines damage, technique points, and a bounded completion-time coefficient. The current ZZZ September 2026 Virtual Shadow Hunt scoring format was used only as a design category reference; exact proprietary values are not copied.
5. `GAME-021` Ordered landmark resonance. Free exploration remains valid, but discovering Lincoln Memorial, Reflecting Pool, then Washington Monument in that intended order grants one extra capped resource reward. Category reference: Genshin exploration mechanisms and reward loops.

## New community-requested increment

`QOL-005` Training enemy variant selector. Two independent ZZZ community posts requested more enemy types/variants in free training so players can practice distinct movesets and bosses. The implementation exposes deterministic profile switching at the game-domain layer. Selecting the active profile is idempotent; selecting a new profile resets transient training state while preserving the player's combat-assist preference. This is community feedback, not evidence of consensus or proof that current ZZZ still lacks the requested coverage.

Community sources:
- https://www.reddit.com/r/ZenlessZoneZero/comments/1k2qi6u/ , published 2025-04-19, accessed 2026-09-22.
- https://www.reddit.com/r/ZenlessZoneZero/comments/1hru9ss/ , published 2025-01-02, accessed 2026-09-22.

Current/reference sources accessed 2026-09-22:
- https://apps.apple.com/us/app/zenless-zone-zero/id1606356401
- https://www.playstation.com/en-us/games/genshin-impact/
- https://www.playstation.com/en-us/games/wuthering-waves/
- https://www.playstation.com/en-us/games/granblue-fantasy-relink/
- https://zenless.hoyoverse.com/fr-fr/news/166073 (official event URL recorded from prior source discovery; direct fetch was unavailable in this environment during this pass, so current scoring details were cross-checked against dated coverage that attributes them to the 2026-09-14 HoYoverse notice rather than treated as independently measured behavior)

## Acceptance

- Profile switching is bounded and idempotent, uses original profile data, and cannot carry stale affinity/combo/score state across a changed training opponent.
- Weakness bonus occurs only when the reacting affinity matches the selected profile weakness.
- Eclipse opening is one-use, nonlethal-only, and survives invalid finisher attempts.
- Technique chain has a fixed cap and timeout; score freezes its time coefficient at target defeat so post-clear idling cannot change earned score.
- Ordered exploration bonus is optional, one-use, capped through existing resource rules, and never blocks or penalizes free out-of-order discovery.
- Existing pass-3 behavior and tests remain green.

## Merge gates

Hosted Windows Debug/Release deterministic tests and release-manifest integrity must pass on the exact final PR head. Exact-head independent Codex review must finish with no unresolved findings. Native interactive/input/UI acceptance is not claimed by this domain-only packet. Do not merge on stale CI or review evidence.