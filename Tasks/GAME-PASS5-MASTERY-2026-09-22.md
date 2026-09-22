# GAME-PASS5-MASTERY-2026-09-22

Owner: `animerpg-game-hourly`
Baseline: `7950687e3f9180787957e52394bf13cea49c35bf`
Target: `main`

## Scope

This packet is game-domain only. Allowed implementation paths are:
- `Engine/Scene/CombatSandbox.h`
- `Engine/Scene/CombatSandbox.cpp`
- `Tests/CombatSandboxTests.cpp`
- this task packet
- `Docs/Agents/animerpg-hourly/RUN-2026-09-22-PASS5.md`
- `Docs/Agents/animerpg-hourly/STATE.json`

Do not modify renderer, platform, editor, CMake, workflows, dependencies, R0/recovery code, or any engine-worker branch/PR.

## Research, accessed 2026-09-22

Primary/current references:
- Genshin Impact PlayStation listing: https://store.playstation.com/en-gb/concept/10000896 . The official description foregrounds elemental reactions as a combat system.
- Wuthering Waves PlayStation listing: https://store.playstation.com/en-us/concept/10010764/ . The official description highlights Extreme Evasion and Dodge Counter in fast combat.
- Granblue Fantasy: Relink PlayStation page: https://www.playstation.com/en-us/games/granblue-fantasy-relink/ . Accessibility metadata explicitly lists adjustable difficulty and consequence-free Practice Mode.
- Zenless Zone Zero official Combat Simulation event: https://zenless.hoyoverse.com/en-us/news/165277?catchSpider=1 . Players can select enemy cards for combat challenges.
- Zenless Zone Zero official current site/version material: https://zenless.hoyoverse.com/en-us/main?catchSpider=1 and the Version 3.2 official news/video index. Use ZZZ only as a category reference for enemy traits, stun/setup payoff, and training/challenge structure, not copied kits or content.

Community request:
- ZZZ Official Reddit thread, published 2026-09-11: https://www.reddit.com/r/ZZZ_Official/comments/1wdtzbh/training_mode/ . Multiple participants note that the normal training selection tops out at elite enemies and ask for boss-level training so boss-specific stun/rotation behavior can be practiced. This is current anecdotal community feedback, not a census or proof of an official roadmap.
- Corroborating historical ZZZ discussion, published 2026-02-14: https://www.reddit.com/r/ZZZ_Discussion/comments/1r4l7xn/why_cant_we_put_bosses_in_the_training_training/ . Treat as corroboration only.

## Five comparator-derived increments

1. `GAME-022` Attack-profile resistance. Vanguard and the new Boss profile gain different bounded direct-attack resistances so target selection changes which basic attack is efficient, while Standard and the already-shipped Bulwark damage behavior remain unchanged. Acceptance: resistance applies once, is visible in attack reports, and never increases damage.
2. `GAME-023` Stagger vulnerability. Direct attacks against an already-staggered target receive one bounded damage bonus. The hit that creates stagger does not retroactively receive the bonus. Acceptance: exact pre/post-stagger cases and defeat capping are deterministic.
3. `GAME-024` Technique-variety chaining. The existing technique chain now rewards alternating reaction/stagger/finisher techniques; repeating the same technique resets the live multiplier instead of farming it. Acceptance: distinct techniques extend, repeats reset to one, timeout still clears, best-chain history remains.
4. `GAME-025` Boss phase-shifting affinity weakness. A boss-grade training target changes from Solar weakness to Umbral weakness at half health, creating an original phase adaptation of elemental-reaction and boss-state design. Acceptance: threshold is exact, reset returns to phase one, and weakness checks use the current phase.
5. `GAME-026` Endless practice target. Add an opt-in consequence-free target mode that records damage/technique metrics without depleting target health, while Standard mode remains unchanged. Switching modes starts a clean attempt and preserves selected enemy/assist settings.

## Community increment

6. `QOL-006` Boss-grade training profile. Add a selectable Boss target with substantially higher health/posture and phase behavior so longer rotations can be practiced. This adapts the September 2026 request without copying ZZZ boss identities, chain-attack rules, or assets.

## Verification

Required before merge:
- regression coverage for all six increments in the existing compiled `CombatSandboxTests` target;
- existing pass-1 through pass-4 behavior must remain covered by the repository test suite;
- hosted Windows Debug/Release deterministic workflow on the exact final head;
- release-manifest integrity on the exact final head;
- independent Codex review of the exact final head with blocking findings repaired and checks rerun;
- full diff review against then-current `main`.

Native interactive/playable verification is not claimed by this domain-only packet. No UI, animation, audio, input, renderer, or editor changes are included.

## Stop conditions

Stop and leave the PR unmerged if a required hosted check fails, Codex has an unresolved material finding, `main` moves in a way that changes these files/contracts, or an engine worker begins owning any allowed implementation path.