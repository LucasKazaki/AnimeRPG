# AnimeRPG game pass 2: defensive timing

Date: 2026-09-22
Owner: `animerpg-game-hourly`
Base: PR #14 exact tested head `f445f156acf7ed2899f93a34191782aa357ad0d1`
Scope: game-domain logic and its existing Shadowblade tests only. No renderer, platform, editor, CMake, workflow, dependency, runtime-scheduler, release, or engine-worker changes.

## Research map

Current comparator descriptions were checked on 2026-09-22. These are design references only, not source or content to copy.

- `GAME-007` incoming attack telegraph: Zenless Zone Zero's developer description emphasizes reacting to opponents' counterattacks with dodge/parry, implying readable incoming threats. Source: https://apps.apple.com/sg/app/zenless-zone-zero/id1606356401
- `GAME-008` bounded guard integrity and guard break: Granblue Fantasy: Relink's guard/perfect-guard community discussion highlights blocking as a distinct defensive choice and the risk of guard break under pressure. Secondary source: https://steamcommunity.com/app/881020/discussions/0/565913463540536527/
- `GAME-009` perfect guard: Zenless Zone Zero explicitly describes parry, while Granblue discussion records perfect guard as a timing mechanic. Sources: https://apps.apple.com/sg/app/zenless-zone-zero/id1606356401 and https://steamcommunity.com/app/881020/discussions/0/4208119548512183204/
- `GAME-010` dodge window and perfect dodge: Wuthering Waves' current developer description explicitly names Extreme Evasion and Dodge Counter. Source: https://apps.apple.com/us/app/wuthering-waves/id6475033368
- `GAME-011` earned counter follow-up: Wuthering Waves names Dodge Counter, while ZZZ describes converting defensive responses and stun into offense. Adaptation is a single-protagonist, one-use Fatal Strike opportunity rather than character swapping. Sources: https://apps.apple.com/us/app/wuthering-waves/id6475033368 and https://apps.apple.com/sg/app/zenless-zone-zero/id1606356401
- `QOL-003` forgiving defense timing preset: a Granblue Fantasy: Relink Steam discussion dated 2024-02-20 says perfect guard is satisfying but the timing is very tight and asks for a way to make it easier. Another discussion dated 2024-02-03 argues the strict timing can feel under-rewarded, while respondents note later/endgame utility. This is historical community evidence, not consensus or proof the current game remains unchanged. Sources: https://steamcommunity.com/app/881020/discussions/0/7198511214976169414/ and https://steamcommunity.com/app/881020/discussions/0/4208119548512183204/

## Original AnimeRPG adaptation

The Shadowblade gains a deterministic incoming-attack state with explicit windup, damage, guard damage, and blockability. An early dodge does not erase the threat. A dodge inside the ordinary evasion window avoids the hit; a tighter perfect-dodge window grants one short counter opportunity. Guarding a blockable hit spends bounded guard integrity instead of health, while excessive guard damage breaks guard and applies one hit. Unblockable attacks bypass ordinary guard. A perfect guard spends no health or integrity and grants the same bounded counter opportunity. That counter can reduce the cost of one valid Fatal Strike, and is consumed only after all rejection checks succeed.

`QOL-003` adds an opt-in forgiving timing preset. The standard preset remains the default. The forgiving preset changes only the perfect-defense timing threshold, not damage, resource costs, dodge reach, or enemy timing.

## Acceptance

1. Valid threats queue once; malformed/nonfinite threats are rejected without mutating player health or guard state.
2. Early dodge, ordinary evade, perfect dodge, ordinary guard, perfect guard, guard break, unblockable hit, and automatic unresolved hit produce distinct deterministic results.
3. Perfect defense grants exactly one bounded counter; rejected Fatal Strikes preserve it, successful eligible Fatal Strike consumes it, and expiry removes it.
4. Guard integrity and player health stay within bounds; one attack cannot deal damage twice.
5. Standard and forgiving timing presets differ only at the intended perfect-defense threshold.
6. Equivalent frame splits produce the same attack-expiry and counter-expiry outcomes at exact timing boundaries.
7. Existing dash, Fatal Strike, stagger follow-up, combo, resource, and guard-conflict tests remain green.

## Verification and merge gate

Run the existing Shadowblade domain target through the repository's Windows deterministic workflow, plus release-manifest integrity. Exact-head independent Codex review is required before merge. This packet does not claim input/HUD wiring, native interactive playability, final combat balance, animation cues, audio cues, or engine acceptance. Those remain later coordinated integration work.
