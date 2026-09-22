# GAME PASS 2: Reactive defense, mana reactions, and clean retry

Date: 2026-09-22
Owner: `animerpg-game-hourly`
Stack base at admission: `7a4aaffd68891c2088c182749d14ac64ca66464c` from PR #14
Target after parent acceptance: `main`

## Scope

This packet changes game-domain behavior only. It does not edit renderer, editor, platform, CMake, CI, generic asset systems, local schedulers, or release/deployment paths. It preserves the custom Astral Engine and the single persistent protagonist design.

Allowed implementation paths:
- `Engine/Scene/CombatSandbox.*`
- `Engine/Scene/ShadowbladeActions.*`
- `Engine/Scene/LandmarkEncounter.*`
- matching deterministic tests
- this task and `Docs/Agents/animerpg-hourly/` receipts

## Five reference-game increments

### GAME-007 Enemy attack telegraph and impact timeline
Original adaptation: a deterministic scheduled enemy hit with explicit windup, one active attack at a time, bounded player health, and exactly-once impact resolution. This is the prerequisite timing surface for the reactive-defense mechanics below. It is inspired by the readable incoming-attack timing used by fast anime action RPGs, not copied animation or encounter data.

### GAME-002 Perfect dodge and earned counter
Wuthering Waves' current PlayStation description explicitly lists Extreme Evasion and Dodge Counter as combat mechanics. Astral adapts that idea to one bounded dodge window. A successful late-window dodge cancels one scheduled hit and grants one one-second counter opportunity; early attempts do not consume the attack, out-of-range counters do not consume the opportunity, and a successful counter consumes it exactly once.

### GAME-008 Perfect guard/parry into posture damage
Zenless Zone Zero's combat emphasizes accessible action with strategic depth; documented Defensive Assist examples parry an imminent hit and deal Daze. Astral adapts that pattern to the existing single-protagonist guard fantasy: a tighter perfect-guard window cancels the incoming hit and deals bounded posture damage to the target, potentially triggering the existing stagger state. This is not a party-switch mechanic.

### GAME-009 Solar/Umbral mana reaction
Genshin Impact's current PlayStation listing describes elemental reactions created by combining elements. Astral uses original setting-specific affinities instead: Solar and Umbral. Applying the opposite affinity consumes both to create an `Eclipse` reaction for bounded bonus damage. Reapplying the same affinity does not duplicate reaction damage.

### GAME-010 Earned combo finisher
Granblue Fantasy: Relink describes Link Attacks and Chain Bursts, while Zenless Zone Zero uses chained/stun follow-ups. Astral keeps one protagonist: three successful hits within the existing combo window arm one contextual finisher. Failed range checks preserve it, success consumes it, and timeout/defeat clear it.

## Community increment

### QOL-003 Clean encounter retry
Historical Genshin community posts repeatedly asked for retry to restore transient combat state instead of carrying stale cooldowns. Examples include Reddit discussions published 2022-01-19, 2023-02-20, and 2023-09-11. This is design input, not a claim that current Genshin still has the same limitation.

Astral adaptation: the Lincoln training encounter gets an explicit retry path that resets the training target, player combat state, telegraph/counter/combo/affinity state, Shadowblade resource, cooldowns, and guard state. The encounter remains active, but the one-time completion reward history is preserved so replay cannot farm progression rewards.

## Source record

Accessed 2026-09-22:
- Genshin Impact PlayStation store: https://store.playstation.com/en-us/concept/10000896/
- Zenless Zone Zero PlayStation: https://www.playstation.com/en-us/games/zenless-zone-zero/
- Wuthering Waves PlayStation: https://www.playstation.com/en-us/games/wuthering-waves/
- Granblue Fantasy: Relink PlayStation: https://www.playstation.com/en-us/games/granblue-fantasy-relink/
- Community retry examples: https://www.reddit.com/r/Genshin_Impact/comments/s7gnmq/ , https://www.reddit.com/r/Genshin_Impact/comments/1175sfb/ , https://www.reddit.com/r/GenshinImpact/comments/16fjxtf/

## Acceptance

1. Invalid/non-finite attack schedules cannot mutate combat state.
2. Telegraph impacts resolve exactly once and cap player damage at zero health.
3. Perfect dodge distinguishes early and valid timing, grants exactly one bounded counter, and invalid counter attempts do not consume it.
4. Perfect guard distinguishes early and valid timing, prevents player damage, and applies bounded target posture damage.
5. Three valid combo hits arm one finisher; timeout, target defeat, and successful consumption clear it; out-of-range use preserves it.
6. Solar + Umbral or Umbral + Solar triggers exactly one Eclipse reaction; same-affinity and `None` do not.
7. Retry restores transient combat/action state but never resets `rewardGranted_`; replay completion grants zero additional progression reward.
8. Existing pass-one posture-recovery and Fatal Strike combo regressions remain green.

## Verification plan

- Focused Linux C++17 source fixture with `-Wall -Wextra -Werror` for the new combat state machine.
- Existing repository deterministic tests in hosted Windows Debug and Release CI through the stacked PR.
- Exact-head independent Codex review before merge.
- No native playable claim until an interactive Windows input/UI integration exists and is actually run.

Stop on a deterministic regression, unresolved exact-head review finding, failed required hosted check, parent-PR conflict, or an engine-worker ownership conflict.