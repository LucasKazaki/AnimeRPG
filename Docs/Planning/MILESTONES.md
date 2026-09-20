# Milestone Backlog and Vertical Slice

> Historical product roadmap, not the active task queue. Read
> `../../GAME_DEVELOPMENT_CONTROL.md` and `../Project-Status.md` first.
> The September 17 director scope pauses game-content work until engine acceptance.
> Implementation M8/M9/M10 mean Thought Commands / Landmark Interaction / Landmark
> Encounter. They do not mark the original M9 summon or M10 enemy set complete.

Source: Product Requirements Document v0.2, extracted 2026-07-21.

## Staged backlog

| ID | Milestone | Exit evidence | Priority |
|---|---|---|---|
| M0 | Repository, rules, task packets, build probe | Clean-clone configure/build instructions; ownership and safety docs | P0 |
| M1 | Astral window/core loop | Native window, loop, input, logs, stable delta time, clear color, math test | P0 |
| M2 | Renderer/scene foundation | Static mesh, camera, transforms, asset load, debug grid | P0 |
| M3 | Third-person controller | Movement, camera follow, jump, sprint, collision, dodge stub | P0 |
| M4 | Combat sandbox | Light/heavy attacks, hit detection, damage, dummy death | P0 |
| M5 | National Mall blockout | Lincoln Memorial, Reflecting Pool, Washington Monument traversal | P0 |
| M6 | Destruction prototype | Breakable props, crystals, boss pillar, reset behavior | P1 |
| M7 | Shadowblade | Shadow Dash, Fatal Strike, Guard, Command, resource/cooldowns | P0 |
| M8 | Thought commands | Slow-time, typed rule parser, ability/summon/environment routing | P0 |
| M9 | Shadow summon | Follow, attack, protect, focus boss, interrupt caster, retreat | P0 |
| M10 | Enemy set | Melee, caster, brute with distinct AI | P1 |
| M11 | Shadow Crypt | Enter, 3 encounters, mini-boss, reward, reset | P0 |
| M12 | Mana Reactor | Enter, crystals/anchors, Gate Warden boss, completion/reset | P0 |
| M13 | Arc Mage/Aegis prototypes | Class switching; two abilities each | P1 |
| M14 | Art pass 1 | Male/female characters, enemies, readable materials/VFX | P1 |
| M15 | QA/vertical slice lock | 15–30 minute completion; no P0 crashes; test report | P0 |

## Smallest executable vertical slice
M1 is the first gate. The first playable combat slice begins only after M1–M3 are verified: move a placeholder character in a windowed test scene. Combat and art are intentionally not started in parallel until those contracts exist.

## Historical next queued packet (before implementation M8)

The following paragraph is retained as history, not a current work assignment:

M8 Thought Commands in a fresh isolated worktree after verified M7 is merged. Dynamic selection favors this P0 player-command capability over the P1 M6 destruction prototype. A new packet must bound slow-time semantics, parser grammar, ability/summon/environment routing contracts, live controls, focused tests, and native runtime evidence before implementation begins.

## M3 implementation evidence

The bounded controller/camera contract is implemented and verified on `task/m3-third-person-controller` in the dedicated M3 worktree. Debug and Release builds, both CTest runs, static verification, and the Lucas-accepted automated native runtime substitute are passing; see `Docs/QA/MILESTONE-3.md`.

## M4 implementation evidence

The bounded deterministic combat sandbox is implemented and verified on `task/m4-combat-sandbox`: light/heavy attacks, range and cooldown rejection, dummy damage/defeat, live J/K input, GDI state feedback, focused tests, and an automated native runtime path pass in Debug and Release. See `Docs/QA/MILESTONE-4.md`.

## M5 implementation evidence

The bounded perspective wireframe world is implemented and verified on `task/m5-national-mall-blockout`: deterministic perspective projection, positive near-plane rejection, player-follow traversal on an X/Z grid, three deterministic landmark proxies at distinct depths, retained player/dummy combat state, and captured native GDI frame evidence pass in Debug and Release. See `Docs/QA/MILESTONE-5.md`.

## M7 implementation evidence

The bounded deterministic Shadowblade action kit is implemented and verified on `task/m7-shadowblade`: Q dash through the bounded controller path, L fatal strike through the combat domain, held left-Shift guard/conflict state, capped resource and cooldown rules, visible GDI/title feedback, and actual-game runtime input evidence pass in Debug and Release. Existing M4/M5 runtime smokes remain green. See `Docs/QA/MILESTONE-7.md`.
