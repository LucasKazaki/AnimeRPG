# M5 — Perspective Wireframe World Blockout

## Objective

Evolve the working M3 controller and M4 combat sandbox into a visibly three-dimensional, traversable wireframe world blockout using only the existing C++17 Win32/GDI engine.

The player must be able to move through a perspective-projected ground grid and see simple landmark proxies at different depth positions. The camera must follow player traversal into and across the scene so apparent size/position changes demonstrate a genuine perspective world rather than the old orthographic debug grid.

## Player-facing behavior

- Preserve M3 WASD controller behavior and M4 J/K combat behavior.
- Render the ground as a perspective-projected X/Z grid, with player `x/y` traversal mapped consistently to the world ground plane.
- Render at least three deterministic wireframe landmark proxies at distinct depth/position coordinates; use procedural line geometry only.
- Use a perspective camera with an explicit positive near plane and deterministic projection rules.
- The camera follows the player so the world responds as the player traverses.
- Retain a visible player marker and M4 training dummy/combat state in the world.
- Window title must include `M5` and current player position; retain a compact combat/dummy state signal.
- Escape/window close retain clean exit.

## Allowed files

- `Tasks/M5-national-mall-blockout.md`
- `CMakeLists.txt`
- `Engine/Platform/Win32Application.h`
- `Engine/Platform/Win32Application.cpp`
- `Engine/Renderer/Renderer.h`
- `Engine/Renderer/Renderer.cpp`
- `Engine/Scene/Camera.h`
- `Engine/Scene/Camera.cpp`
- `Engine/Scene/WorldBlockout.h`
- `Engine/Scene/WorldBlockout.cpp`
- `Tests/SceneTests.cpp`
- `Tests/WorldBlockoutTests.cpp`
- `Tests/M5RuntimeSmoke.cpp`
- `Docs/QA/MILESTONE-5.md`
- `Docs/Decision-Log.md`
- `Docs/Planning/MILESTONES.md`

Do not edit other files or add an external dependency/API without an amended packet that states why it is necessary, includes source/license/version/checksum provenance, and preserves reproducibility and rollback.

## Technical constraints

- C++17 and existing Astral types only.
- No DirectX, Vulkan, OpenGL, Unreal/Unity, plugin, download, asset, physics, AI, networking, texture, lighting, rasterizer, or renderer-wide redesign.
- Use GDI line rendering and procedural geometry only.
- Keep projection and world-blockout state deterministic and unit-testable without a Win32 window.
- Build out of source only; never configure CMake in the source root.

## Automated acceptance evidence

1. Unit tests cover perspective projection at the screen center, depth scaling, vertical orientation, and rejection at/before the near plane.
2. Unit tests verify all world-blockout landmarks are deterministic and have valid wireframe dimensions/positions.
3. Debug and Release builds pass.
4. Debug and Release CTest pass, including an M5 native runtime smoke that launches the actual game, observes `M5` title/position evidence across controlled movement, and verifies a clean Escape exit.
5. Existing M3 and M4 tests remain green.
6. `git diff --check` passes and changed files stay within this packet.

## Merge gate

Autonomous commit/merge to `main` is allowed after all required gates pass. The resulting merge must be followed by a clean, isolated next-task worktree selected from the updated product state rather than a rigid prewritten plan.

## Stop conditions

Stop and report evidence if the packet requires a graphics API, third-party dependency, new asset source, out-of-packet file, or human-only runtime claim. Do not represent wireframe perspective as a full production 3D engine.
