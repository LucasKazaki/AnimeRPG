# GAME PASS 29: Field Guide and Map QoL

Date: 2026-09-24
Owner: separate AnimeRPG GAME worker
Base: `e38760070e0d021293e9f907834e61e6a6edca23`
Target: `main`

## Authority and boundary

This is bounded GAME work under Lucas's standing hourly GAME-worker authorization. Preserve the custom C++17 Astral Engine and the existing `LandmarkInteraction -> ExplorationFieldGuide` ownership path. Do not modify renderer/platform/editor/import/animation/audio/physics infrastructure, shared CMake/CI, R0, deployment, networking, local Company Runtime state, or another worker's PR.

Allowed paths:

- `Engine/Scene/ExplorationFieldGuide.h`
- `Engine/Scene/LandmarkInteraction.h`
- `Tests/ExplorationFieldGuidePass29Tests.inc`
- `Tests/ThoughtCommandsTests.cpp`
- `Tasks/GAME-PASS29-FIELD-GUIDE-MAP-QOL-2026-09-24.md`
- `Docs/Agents/animerpg-hourly/PASS29-BACKLOG.json`
- `Docs/Agents/animerpg-hourly/RUN-2026-09-24-PASS29.md`
- `Docs/Agents/animerpg-hourly/STATE.json`

`LandmarkInteraction.h` was added to the packet after independent review correctly found that several new mutators were reachable only on standalone `ExplorationFieldGuide` instances. The repair is limited to thin owner-level forwarding APIs plus regression coverage through the real production owner. It does not move authority out of `ExplorationFieldGuide` or introduce a parallel framework.

## Live gap

`ExplorationFieldGuide` already owns National Mall operations, discovery chronology, journal state, one manual pin, and the field evidence consumed by Shadow Crypt and Mana Reactor gating. Before this packet it lacked operation-aware hints, bounded multi-stop route management, next-incomplete operation tracking, unread journal notices, a consolidated briefing, and route-category visibility filtering. `LandmarkInteraction` owns the live production instance, so any player-affecting mutator added here must also be callable through that owner.

## Research provenance, accessed 2026-09-24

Primary official comparator evidence:

- HoYoverse, **"Teyvat Interactive Map" Version 2.6 Update**, published 2022-03-30: quick area location, personal pin creation/editing, filter list, own/synced pin viewing, and pin distributions. https://www.hoyolab.com/article/4029408
- HoYoverse, **Version "Luna III" Update Details**, published 2025-12-02: Adventurer Handbook enemy tracking can simultaneously display up to three nearby objective locations. https://www.hoyolab.com/article_pre/21389
- HoYoverse, **Teyvat Interactive Map Usage Guide**: retained official guidance for syncing and using pins during exploration. https://www.hoyolab.com/article/17673509

Five distinct comparator lessons mapped into this packet are: actionable next-location guidance, deterministic objective selection, fixed-capacity multi-target tracking, personal route-pin management, and consolidated/filterable navigation information. These are interaction lessons only. No Genshin map content, UI art, names, code, monetization, or world structure is copied.

Community improvement evidence:

- Original Genshin player discussion, 2023-09-20, requested more pin icons/colors and the ability to turn pins on/off, with replies also requesting scoped pin management: https://www.reddit.com/r/Genshin_Impact/comments/16nle96/
- A 2026-01-15 retrospective still lists `Pin filter` among older requested QoL while noting that many historical requests were gradually implemented: https://www.reddit.com/r/Genshin_Impact/comments/1qd7l2l/
- An older 2020 player discussion independently asked for a pin filter that displays only selected pin types: https://www.reddit.com/r/Genshin_Impact/comments/jt1oi7/

Later official map-management improvements exist, so this packet does **not** claim current Genshin still lacks all filtering. The narrower per-category in-game visibility behavior was not established as resolved in the inspected current official sources. Treat the request as corroborated player preference, not consensus.

## Five comparator features plus one community increment

### GAME-142: operation-aware next-step hint
`CurrentHint()` reports the next bounded action for Mall Survey, Rift Investigation, or Shadow Crypt Lead without mutating state. Acceptance: evidence advances hints deterministically and completed operations report completion.

### GAME-143: deterministic next-incomplete operation tracking
`TrackNextIncompleteOperation()` advances cyclically to the next incomplete operation and is a no-op when all are complete. The production owner exposes the same mutation through `TrackNextIncompleteFieldOperation()`.

### GAME-144: bounded multi-stop field route
A fixed three-stop route preserves insertion order, rejects invalid/duplicate/visited entries, supports individual/batch removal, auto-prunes visited stops, and preserves legacy single-pin precedence. Production owner forwarding covers add/remove/clear.

### GAME-145: unread field-journal notices
Newly unlocked notes begin unread; read-one/read-all acknowledgement is explicit and idempotent; duplicate evidence cannot resurrect a read notice. Production owner forwarding makes acknowledgement reachable on live state.

### GAME-146: consolidated field briefing
`Briefing()` returns authoritative tracked operation/progress, hint, target, route count, journal counts, and completed-operation count. `LandmarkInteraction::FieldBriefing()` exposes that snapshot through the live owner without duplicating state.

### QOL-030: route-pin visibility filter
Route stops use original Astral categories `Objective`, `Resource`, and `Note`; `All` or one category controls route-target visibility without deleting hidden stops. Invalid filters fail closed. Production owner forwarding exposes this behavior on live state.

## Verification plan

1. Keep pass-29 coverage registered through the existing `ThoughtCommandsTests` aggregate without changing CMake.
2. Preserve pass-14 field-guide behavior and add pass-29 invalid-enum, route capacity/ordering/pruning, visibility filter, journal idempotency, operation wraparound/all-complete, briefing non-mutation, and production-owner forwarding regressions.
3. Use exact-head hosted Windows Debug/Release tests and the Release-manifest lane. If repair changes the SHA, old green runs are superseded.
4. Require fresh independent review of the exact final candidate with no unresolved material findings.
5. Immediately before merge, re-read `main`, PR head, changed paths, checks, and review state, then merge only with `expected_head_sha`.

The existing Windows workflow's core commands remain:

```powershell
cmake -S . -B "$env:BUILD_ROOT" -G "Visual Studio 17 2022" -A x64
cmake --build "$env:BUILD_ROOT" --config Debug --parallel
ctest --test-dir "$env:BUILD_ROOT" -C Debug --output-on-failure -E "RuntimeSmoke" --no-tests=error
cmake --build "$env:BUILD_ROOT" --config Release --parallel
ctest --test-dir "$env:BUILD_ROOT" -C Release --output-on-failure -E "RuntimeSmoke" --no-tests=error
python Scripts/verify_milestone1.py
python Scripts/verify_milestone2.py
python Scripts/verify_milestone3.py
git diff --check
git status --porcelain --untracked-files=no
```

The Release-manifest workflow retains its existing package/runtime contract tests, duplicate Release build diagnostics, manifest create/verify commands, `git diff --check`, and clean-tree check. No new workflow or dependency is introduced.

## Evidence boundary

These features are integrated into the production game-domain owner/API after the owner-forwarding repair, but there is still no Win32/controller/menu field-guide UI for these interactions. Hosted deterministic tests are not a native interactive playtest. Native-playable count remains zero. No GPU/rendering, art/audio, cross-process persistence, deployment/release, or Genshin/ZZZ parity claim is made.