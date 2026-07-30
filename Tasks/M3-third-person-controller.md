# Task M3 — Third-Person Controller Contract

task_id: M3-third-person-controller
title: Add a bounded placeholder controller and camera-follow contract
owner: Engine Coding Agent (coordinated by Unreal Specialist)
priority: P0
status: verified; Lucas accepted the automated native runtime substitute and authorized merge
dependencies: M2-1 commit 9ff0b75 merged into main; existing Clock, Transform, OrthographicCamera, and Win32Application loop

## Context

The approved product path is the custom C++ Astral Engine, not Unreal. `Docs/Planning/MILESTONES.md` places M3 after the M2 renderer/scene foundation and before combat. The current M2 worktree has a Win32 loop, `Scene::Transform`, and `Scene::OrthographicCamera`, but no player/controller type. This packet defines the smallest movement slice without claiming that it is implemented.

## Allowed files

- `Engine/Scene/PlayerController.h` (new)
- `Engine/Scene/PlayerController.cpp` (new)
- `Engine/Scene/Transform.h`
- `Engine/Scene/Transform.cpp`
- `Engine/Scene/Camera.h`
- `Engine/Scene/Camera.cpp`
- `Engine/Platform/Win32Application.h`
- `Engine/Platform/Win32Application.cpp`
- `Game/Main.cpp` only if required to wire the controller
- `Tests/SceneTests.cpp`
- `CMakeLists.txt`
- `Tasks/M3-third-person-controller.md`
- `Docs/QA/MILESTONE-3.md`
- `Docs/Decision-Log.md`
- `Docs/Planning/MILESTONES.md`
- `Scripts/verify_milestone3.py`

## Forbidden actions

- No Unreal or Unity dependencies.
- No DirectX/Vulkan integration in this packet.
- No combat, damage, AI, animation, art assets, physics engine, networking, multiplayer, plugins, or external APIs.
- No downloads, credentials, license acceptance, public deployment, firewall changes, destructive cleanup, or history rewriting.
- No edits outside the listed worktree or allowed files.
- Do not merge without Lucas approval.

## Goal

Move a placeholder entity through a deterministic test scene using bounded keyboard input and delta time, with the camera following the entity through the existing scene/camera contracts.

## Requirements

- Add a minimal controller/entity state using existing `Transform` and `OrthographicCamera` types.
- Define explicit movement speed and deterministic input-to-velocity behavior for WASD.
- Use the existing `Clock::Tick()` delta time; do not introduce a second timing loop.
- Define a bounded collision contract suitable for a placeholder scene; a full physics system is out of scope.
- Keep Escape/close behavior, debug rendering, FPS/title updates, and M2 mesh loading intact.
- Add automated tests for movement direction, delta-time scaling, camera follow, and the collision boundary contract.

## Acceptance criteria

1. Debug and Release `AstralGame` builds succeed.
2. CTest passes all existing tests plus the M3 controller tests.
3. A placeholder entity moves in response to WASD using delta time.
4. The camera follows the placeholder using a documented offset/contract.
5. Movement remains within the declared bounded collision region.
6. M2 debug mesh/grid rendering and clean close behavior remain intact.
7. Static verification covers the new controller source and test markers.
8. Runtime evidence records window dimensions, movement input, camera response, boundary behavior, and clean exit; source checks alone are insufficient. Lucas explicitly accepts the 2026-07-30 automated native runtime smoke documented in `Docs/QA/MILESTONE-3.md` as the substitute for manual runtime QA.

## Test instructions

```text
cmake --build Build --config Debug --parallel
ctest --test-dir Build -C Debug --output-on-failure
cmake --build Build --config Release --parallel
ctest --test-dir Build -C Release --output-on-failure
python Scripts/verify_milestone3.py
```

The exact runtime probe and any new static-verification script must be documented in `Docs/QA/MILESTONE-3.md` by the implementation/QA pass.

## Expected output

A dedicated worktree and branch, source diff limited to this packet, failing-then-passing tests for each new behavior, Debug/Release build output, CTest output, static gate output, manual runtime evidence, independent review, QA report, decision-log entry, and a merge recommendation. No merge is implied by this packet.

## Next gate

The M2-1 merge decision was resolved on 2026-07-28 and recorded in `Docs/Decision-Log.md`. On 2026-07-30 Lucas explicitly authorized autonomous M3 unblocking and accepted a genuine automated native runtime smoke in place of manual runtime QA. With all release gates green, M3 is authorized for merge and M4 may begin in a fresh isolated worktree.
