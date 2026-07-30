# Milestone 4 QA — Combat Sandbox

Date: 2026-07-30
Branch: `task/m4-combat-sandbox`
Worktree: `C:/AI/worktrees/AnimeRPG/m4-combat-sandbox`

## Implemented contract

- Deterministic, Win32-independent `CombatSandbox` domain with a training dummy at `(3, 0, 0)`, 100 health, and a 3.5-unit XY attack range.
- Light attack: 25 damage and 0.4-second cooldown.
- Heavy attack: 60 damage and 1.0-second cooldown.
- One shared next-attack gate rejects attempts during the cooldown established by the attack that landed.
- Out-of-range, cooldown, and post-defeat attempts apply zero damage.
- Live edge-triggered `J`/`K` input uses the existing `GetAsyncKeyState` loop; holding a key does not retrigger every frame.
- Existing GDI scene renders the dummy, health bar, alive/defeated colors, player mesh, and grid.
- Window title reports controls, dummy life/health, last attack/result/damage, FPS, and player position.
- Escape and window-close behavior remain on the M3 path.

## Automated evidence

Out-of-source build tree: `C:/AI/builds/AnimeRPG/m4-combat-sandbox`.

Configure:

`cmake -S C:/AI/worktrees/AnimeRPG/m4-combat-sandbox -B C:/AI/builds/AnimeRPG/m4-combat-sandbox -G "Visual Studio 17 2022" -A x64`

Result: PASS with MSVC 19.44.35228.0 and Windows SDK 10.0.26100.0.

Debug:

- `cmake --build C:/AI/builds/AnimeRPG/m4-combat-sandbox --config Debug --parallel` — PASS.
- `ctest --test-dir C:/AI/builds/AnimeRPG/m4-combat-sandbox -C Debug --output-on-failure -V` — PASS, 4/4.

Release:

- `cmake --build C:/AI/builds/AnimeRPG/m4-combat-sandbox --config Release --parallel` — PASS.
- `ctest --test-dir C:/AI/builds/AnimeRPG/m4-combat-sandbox -C Release --output-on-failure -V` — PASS, 4/4.

`CombatSandboxTests` executes checks in both Debug and Release (it does not depend on release-disabled `assert`) and covers light damage, heavy damage, out-of-range rejection, light and heavy cooldown boundaries, defeat, and no post-defeat damage.

## Automated native runtime smoke

`M4RuntimeSmoke` is a native Win32 CTest target built solely from the packet-allowed `Tests/CombatSandboxTests.cpp`. It launches the configuration-matched `AstralGame`, finds the process-owned window, drives the real `GetAsyncKeyState` path with `SendInput`, reads title feedback, and exits through Escape.

Observed controlled path in both Debug and Release:

1. Initial: `Dummy: Alive HP: 100/100`.
2. `J`: `Dummy: Alive HP: 75/100 | Last: Light Hit -25`.
3. Immediate `K`: health remains 75; `Last: Heavy Cooldown`.
4. `K` after light cooldown: `Dummy: Alive HP: 15/100 | Last: Heavy Hit -60`.
5. `J` after heavy cooldown: `Dummy: Defeated HP: 0/100 | Last: Light Hit -15`.
6. Post-defeat `J`: health remains 0; `Last: Light Already Defeated`.
7. Escape: process exit code 0.

Result: `M4 AUTOMATED NATIVE RUNTIME SMOKE: PASS` in Debug and Release.

## Scope and quality gates

- No downloads, dependencies, plugins, assets, APIs, or in-source build outputs were added.
- Runtime smoke was added in a packet-allowed file; `Scripts/verify_milestone3.py` was not modified because it is outside the M4 allowed-file list.
- Final `python Scripts/verify_milestone3.py`: PASS (M3 regression gate).
- Final `git diff --check`: PASS.
- Final changed-file scope audit: PASS; every changed path is listed in the M4 packet.

## Risks and follow-up

- GDI markers and title text are debug presentation, intentionally not production rendering or art.
- Combat uses planar XY distance and a single dummy by packet design; no physics, facing cone, AI, animation, or networking is claimed.
- Highest-value next milestone remains M5 National Mall blockout, preserving the P0 sequence and building a traversal space around the now-verified movement/combat loop.
