# Task M1 — Astral Engine Window and Core Loop

task_id: M1-astral-window-core-loop
title: Implement Astral Engine Milestone 1
owner: Engine Coding Agent (coordinated by Unreal Specialist)
priority: P0
dependencies: Native Windows C++ compiler and CMake for runtime verification
allowed_files:
- CMakeLists.txt
- Engine/Core/*
- Engine/Math/*
- Engine/Platform/*
- Engine/Renderer/*
- Game/Main.cpp
- Tests/MathTests.cpp
- Scripts/verify_milestone1.py
- README.md
forbidden_actions:
- Unreal or Unity dependencies
- combat, multiplayer, external APIs, plugins, asset downloads
- deleting or rewriting unrelated files
allowed_commands:
- cmake configure/build/ctest
- python Scripts/verify_milestone1.py
- git status/diff

goal: Produce the smallest native Windows executable satisfying the PRD's Milestone 1 starter prompt.

acceptance_criteria:
- Window opens and closes cleanly.
- Main loop tracks delta time and updates FPS.
- Escape input exits.
- GDI renderer clears a stable color.
- Logger writes startup and frame timing records to console/file.
- Vec3 and Mat4 smoke tests pass.
- Clean clone can configure/build with documented prerequisites.

test_instructions:
1. Run `python Scripts/verify_milestone1.py`.
2. Configure/build/ctest using README commands.
3. Launch AstralGame, observe window/title/log, press Escape, and repeat once.

risk_level: medium
expected_output: source diff, build/test output, runtime evidence, known limitations, next packet recommendation
