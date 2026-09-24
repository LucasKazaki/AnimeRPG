# GAME pass 39: Shadow Crypt threat depth and semantic awareness

Date: 2026-09-24 America/New_York  
Owner: `animerpg-game-hourly`  
Baseline: `a607cb671712e974fcc324cd1619b39ec84b9646`  
Target: `main`  
Branch: `game/pass39-shadow-crypt-threat-depth`

## Authority and ownership

This is a bounded GAME packet under Lucas's September 22 standing game-worker authorization in `GAME_DEVELOPMENT_CONTROL.md`, `AGENTS.md`, and `Docs/Agents/ANIMERPG-HOURLY.md`. It deepens the already-merged Shadow Crypt room combat without taking Astral Engine ownership. The custom C++17 engine, original modern-supernatural Washington DC setting, persistent single protagonist, Shadowblade-first development, Shadow Crypt/Mana Reactor content, destructibility direction, and bounded Thought Commands remain unchanged.

Live discovery at admission found `main` at `a607cb671712e974fcc324cd1619b39ec84b9646`. Open PRs #13 and #52 are engine/verification packets; #23, #26, #28, #33, #45, #49, #55, and #60 are art/tooling packets. They are left unchanged. This packet does not touch renderer, platform, editor, import, animation, audio, physics, CMake/workflows, dependencies, R0, networking, release, deployment, or another worker's state.

Allowed production path:
- `Engine/Scene/ShadowCryptSkirmish.h`

Allowed verification/record paths:
- `Tests/ShadowCryptSkirmishPass38Tests.inc`, retaining and extending the already-registered `ThoughtCommandsTests` coverage without a CMake change
- this task packet
- `Docs/Agents/animerpg-hourly/PASS39-BACKLOG.json`
- `Docs/Agents/animerpg-hourly/RUN-2026-09-24-PASS39.md`
- `Docs/Agents/animerpg-hourly/STATE.json`

The live `LandmarkInteraction` production owner already starts/resolves/attacks the authoritative `ShadowCryptSkirmish` and exposes its const state. No shared-owner edit is required for this packet.

## Current research map

Sources accessed 2026-09-24. Comparator mechanics are design lessons only. No proprietary characters, attacks, art, audio, maps, story, code, monetization, or collectible-character architecture is copied.

1. **GAME-192, deterministic attack-pattern variants.** The current Zenless Zone Zero App Store description says combat uses Basic/Special Attacks, Dodge and Parry, and that different opponents have different traits. Astral deepens each original Shadow Crypt role from one fixed telegraph to a deterministic two-variant pattern while preserving each role's semantic response contract. Source: https://apps.apple.com/us/app/zenless-zone-zero/id1606356401, current page/version context 3.2, accessed 2026-09-24.
2. **GAME-193, bounded low-health desperation.** Genshin Impact's official Stygian Onslaught description published 2026-03-02 documents ascending difficulties with tougher enemies and increasingly restrictive rules. Astral adapts the escalating-pressure lesson inside a room fight: a living enemy at or below half health tightens its current response window by 20% and adds six failure damage, with hard lower/upper caps. Source: https://www.hoyolab.com/article/44016747.
3. **GAME-194, precision-defense classification.** ZZZ's current official listing explicitly centers Dodge and Parry counterplay. Astral distinguishes a correct response within the first 30% of the current semantic window as `precision`, without making ordinary exact-boundary success fail. Source: current ZZZ App Store listing above.
4. **GAME-195, earned precision-counter payoff.** ZZZ's current official listing couples Stun with powerful follow-up Chain Attacks. Astral keeps its single protagonist and existing one-shot earned counter, adding only a bounded +8 health and +6 posture payoff when that counter came from a precision defense. Source: current ZZZ App Store listing above.
5. **GAME-196, brace-to-posture counterplay.** ZZZ's Stun/follow-up loop is used as a readability reference. Astral's original Gravebound Bulwark now loses exactly 10 actual posture on a successful semantic `Brace`/Guard response, while still granting no counter when its telegraph says countering is unavailable. Source: current ZZZ App Store listing above.
6. **QOL-040, semantic multi-enemy threat queue.** Recent ZZZ player discussions repeatedly describe reaction-critical indicators or enemies becoming hard to see amid effects or camera movement. A March 11, 2026 thread reports missed enemy indicators and off-screen enemies; a June 28, 2026 visual-clarity thread asks for less obscuring VFX and notes camera movement can hide enemy reads; a May 15, 2026 thread reports wall/focus camera problems, with a reply suggesting disengaging lock as a workaround. Astral does not claim those issues remain unresolved in current ZZZ and does not take camera/renderer ownership. Instead it adds a read-only queue of up to three living threats carrying semantic cue/response data independent of screen color or camera framing. Sources: https://www.reddit.com/r/ZenlessZoneZero/comments/1rqo45m/literally_my_only_problem_with_the_game/ ; https://www.reddit.com/r/ZZZ_Discussion/comments/1ui9kg4/visual_clarity_isnt_great/ ; https://www.reddit.com/r/ZZZ_Official/comments/1tdvoz1/camera_in_battle/ . These are player anecdotes/corroboration, not a vote or current-defect proof.

## Acceptance criteria

- `GAME-192`: each original enemy role alternates between two deterministic named variants; first-cycle pass-38 cue/response/counterability remains compatible; repeated cycles stay bounded.
- `GAME-193`: low-health escalation occurs only for a living enemy at or below half health; response windows never drop below 0.20 s; failure damage never exceeds 40; no health mutation occurs from merely reading the telegraph.
- `GAME-194`: valid very-early correct responses mark `precision`; exact-window-boundary responses remain successful but non-precision; wrong, negative, or nonfinite timing cannot fabricate precision.
- `GAME-195`: only a precision-earned one-shot counter gets the +8/+6 bounded health/posture bonus; ordinary counters preserve legacy 36/24 nominal values; duplicate counter use remains rejected.
- `GAME-196`: a successful Bulwark Guard applies only actual posture delta, may open stagger only at zero, and does not fabricate a counter.
- `QOL-040`: a read-only queue exposes at most three currently living semantic threats in deterministic cursor order; zero/one-item requests are bounded; querying it does not advance patterns, threats, damage, target selection, or mission state.

Pass-38 regression coverage remains in the registered include and is not intentionally weakened: malformed tiers, formations, semantic initial cues, nonfinite timing, exact-boundary defense, one-shot counter, actual posture deltas/recovery, lethal stagger suppression, sticky target focus, production admission/timeline lock, mission damage synchronization and caps, exact-once objective advancement, defeat retry cleanup, and replay cleanup remain exercised.

## Verification and merge contract

Author-side portable smoke executed against the production header text before publication:

```text
g++ -std=c++17 -Wall -Wextra -Werror -pedantic -I/mnt/data \
  /mnt/data/pass39_test.cpp -o /mnt/data/pass39_test
/mnt/data/pass39_test
# PASS: pass39 standalone Shadow Crypt threat-depth checks
```

This portable header-only smoke is not a Windows/native gameplay claim. Before merge, the exact final PR head must pass the repository's hosted Windows Debug/Release deterministic suite and release-manifest integrity checks, including the registered `ThoughtCommandsTests`. A fresh independent Codex review must bind to that exact head with no unresolved material finding. Re-read `main`, PR head, changed files, checks, reviews, and threads immediately before an expected-head merge. If `main` moves, reconcile and rerun affected gates instead of using stale evidence.

Native Win32/controller/menu presentation, rendered semantic warnings, VFX/audio, hands-on combat feel, cross-process persistence, GPU/performance evidence, art acceptance, deployment, release, and Genshin/ZZZ parity are outside this packet and remain unclaimed.
