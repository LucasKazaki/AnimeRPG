# Task M2-1 — Debug Scene Foundation

task_id: M2-1-debug-scene-foundation
title: Add transform hierarchy, orthographic camera, disk-loaded static mesh, and debug grid
owner: Engine Coding Agent (coordinated by Unreal Specialist)
priority: P0
dependencies: M1 baseline commit 15de0d2; verified MSVC/CMake toolchain

## Context

The PRD's Milestone 2 requires static mesh rendering, camera, transform hierarchy, asset loading, basic materials, and a debug grid. This packet intentionally implements the smallest coherent slice using the existing Win32/GDI backend. It is a renderer/scene contract spike, not a production DirectX renderer.

## Allowed files

- CMakeLists.txt
- Engine/Math/Math.h
- Engine/Assets/StaticMesh.h
- Engine/Assets/StaticMesh.cpp
- Engine/Scene/Transform.h
- Engine/Scene/Transform.cpp
- Engine/Scene/Camera.h
- Engine/Scene/Camera.cpp
- Engine/Renderer/Renderer.h
- Engine/Renderer/Renderer.cpp
- Engine/Platform/Win32Application.h
- Engine/Platform/Win32Application.cpp
- Game/Main.cpp
- Game/Assets/debug_triangle.mesh
- Tests/MathTests.cpp
- Tests/SceneTests.cpp
- Scripts/verify_milestone2.py
- Tasks/M2-1-debug-scene-foundation.md
- Docs/QA/MILESTONE-2.md
- Docs/Decision-Log.md
- Docs/Planning/MILESTONES.md

## Forbidden actions

- Unreal or Unity dependencies
- DirectX/Vulkan integration in this packet
- gameplay, combat, player controller, physics, AI, networking, or multiplayer
- external APIs, asset downloads, plugins, credentials, or new installations
- changes outside this worktree or unlisted files
- destructive cleanup or history rewriting

## Goal

Render a deterministic, disk-loaded static triangle over a debug grid with a minimal scene transform and orthographic camera contract.

## Requirements

- Add a `Transform` with local position/scale and parent-child world-position calculation.
- Add an orthographic `Camera` that maps world coordinates into the Win32 client viewport.
- Add a simple text `.mesh` loader containing vertices and line/triangle indices; malformed or missing files fail gracefully.
- Render a debug grid and the loaded triangle with the existing GDI backend.
- Keep M1 clear color, FPS title/logging, Escape exit, and clean close behavior intact.
- Use tests for transform hierarchy and mesh loading failure/success.
- Keep the asset format documented by the fixture and implementation comments.

## Acceptance criteria

1. `AstralGame` still builds Debug and Release.
2. CTest passes all scene/math tests.
3. The sample mesh loads from disk at runtime.
4. The window visibly shows a grid and triangle with deterministic colors.
5. Parent-child transform test proves child world position includes parent position.
6. Camera maps the origin to the viewport center within one pixel for an even viewport.
7. Missing/malformed mesh files return a failure without crashing.
8. M1 input, title, logging, clear-color, and clean-close behavior remain intact.
9. Static verification confirms the new source/asset markers.

## Test instructions

```text
cmake -S . -B Build -G "Visual Studio 17 2022" -A x64
cmake --build Build --config Debug --parallel
ctest --test-dir Build -C Debug --output-on-failure
cmake --build Build --config Release --parallel
ctest --test-dir Build -C Release --output-on-failure
python Scripts/verify_milestone2.py
```

Manual runtime checks: launch `Build/Debug/AstralGame.exe` from the repository root, confirm the grid and triangle appear, confirm the title updates, inspect `astral.log`, close the window, and verify exit code 0. Do not claim runtime acceptance from source checks alone.

## Risks

- GDI is a temporary backend and does not prove the future DirectX/Vulkan renderer contract.
- The custom mesh format is intentionally minimal and must be replaced or extended before production art integration.
- The current application loop renders continuously with a device context; this packet should not expand into a platform/render-loop refactor.

## Expected output

Source diff, task QA report, independent review result, exact native build/test/runtime evidence, commit on `task/m2-planning`, and next-task recommendation.
