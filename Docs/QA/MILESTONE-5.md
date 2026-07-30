# Milestone 5 QA — Perspective Wireframe World Blockout

Date: 2026-07-30
Branch: `task/m5-national-mall-blockout`
Worktree: `C:/AI/worktrees/AnimeRPG/m5-national-mall-blockout`

## Implemented contract

- Deterministic `PerspectiveCamera` with a 60-degree vertical field of view, positive 0.5-unit near plane, fixed pitched basis, depth query, rejection at/before the near plane, and player-follow position.
- Existing controller XY maps explicitly to world X/Z. The live M5 controller starts at `(0, 0)`, traverses a bounded `[-18, 18] x [-4, 72]` ground region, and keeps the M3 normalized WASD path.
- A procedural perspective X/Z grid spans `[-20, 20] x [-5, 80]` with two-unit spacing.
- Three deterministic procedural wireframe proxies occupy distinct positions/depths: a roofed Lincoln Memorial block at `(-8, 0, 18)`, a shallow Reflecting Pool block at `(4, 0, 39)`, and an obelisk-shaped Washington Monument at `(5, 0, 68)`.
- The old loaded debug triangle is no longer the live player view. A procedural purple 3D pyramid marks the player; the M4 training dummy is a wireframe box with health state.
- `M5 Perspective Mall`, WASD/J/K controls, player X/Z position, dummy health/life, and last combat result remain visible in the title.
- Escape and window close retain the existing clean exit path.

## Automated evidence

Out-of-source build tree: `C:/AI/builds/AnimeRPG/m5-national-mall-blockout`.

Configure:

`cmake -S C:/AI/worktrees/AnimeRPG/m5-national-mall-blockout -B C:/AI/builds/AnimeRPG/m5-national-mall-blockout -G "Visual Studio 17 2022" -A x64`

Result: PASS with MSVC 19.44.35228.0 and Windows SDK 10.0.26100.0.

Debug:

- `cmake --build C:/AI/builds/AnimeRPG/m5-national-mall-blockout --config Debug --parallel` — PASS.
- `ctest --test-dir C:/AI/builds/AnimeRPG/m5-national-mall-blockout -C Debug --output-on-failure -V` — PASS, 6/6.

Release:

- `cmake --build C:/AI/builds/AnimeRPG/m5-national-mall-blockout --config Release --parallel` — PASS.
- `ctest --test-dir C:/AI/builds/AnimeRPG/m5-national-mall-blockout -C Release --output-on-failure -V` — PASS, 6/6.

`AstralSceneTests` has release-active checks for center projection, inverse depth scaling, upright vertical orientation, near-plane rejection, behind-camera rejection, and player-follow mapping. `WorldBlockoutTests` has release-active checks for deterministic landmark kinds, coordinates, dimensions, distinct depths, valid grid bounds, and XY-to-XZ mapping. Existing math, M3 scene/controller, M4 combat, and M4 native runtime tests remain green.

## Automated native M5 runtime smoke

`M5RuntimeSmoke` launches the configuration-matched actual `AstralGame`, finds its process-owned window, drives `J`, `W`, and Escape through `SendInput` into the real `GetAsyncKeyState` loop, reads title state, and captures the live GDI client pixels.

Observed in both Debug and Release:

1. Initial M5 title and position: `Pos: (0.000000, 0.000000)`.
2. Combat persistence: `Dummy: Alive HP: 75/100 | Last: Light Hit -25` after controlled `J`.
3. Controlled `W` traversal changed world Z to approximately `3.6` while X remained zero.
4. Captured frames contained exact GDI colors for the perspective grid, all three independently colored landmark proxies, player marker, and dummy.
5. Debug initial pixel counts (grid/Lincoln/pool/monument/player/dummy): `37622/4043/1449/536/1736/2618`.
6. Debug moved pixel counts: `38717/4741/1591/568/1736/3722`; the frame hash changed after movement.
7. Release initial counts matched the deterministic static frame; release moved counts were `38712/4740/1591/568/1736/3711`, with a changed frame hash.
8. Escape terminated the actual game with exit code 0.

Result: `M5 AUTOMATED NATIVE RUNTIME SMOKE: PASS` in Debug and Release. Pixel capture is retried to avoid observing the intentionally immediate-mode GDI scene between clear and final line draws; it never fabricates or substitutes frame evidence.

## Scope and quality gates

- No downloads, external dependencies, plugins, assets, graphics APIs, or source-tree CMake outputs were introduced.
- All changed paths are listed in `Tasks/M5-national-mall-blockout.md`.
- `python Scripts/verify_milestone3.py`: PASS.
- `git diff --check`: PASS.
- Changed-file scope audit: PASS.

## Honest capability and risks

This is a visibly distinct 3D perspective wireframe traversal capability: geometric depth controls screen size, ground lines converge, fixed world landmarks respond to a player-follow camera, and the smoke verifies actual rendered colors and frame change. It is not a production 3D engine, National Mall art pass, hidden-surface renderer, collision world, or physically accurate site reconstruction. GDI immediate-mode drawing may show tearing and has no occlusion; these are explicit limits of the bounded milestone.

After M5, product priority favors the P0 M7 Shadowblade gameplay packet over the P1 M6 destruction prototype. The next worktree should be selected from that evidence rather than assuming numerical milestone order.
