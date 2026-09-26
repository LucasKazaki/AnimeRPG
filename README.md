# AnimeRPG / Astral Engine

This repository follows the PRD's custom C++ engine requirement. Unreal is not a production dependency; the Unreal Specialist role coordinates the production loop.

## Milestone 1

The first executable milestone is a native Windows window with a bounded game loop, GDI clear-color renderer stub, keyboard input polling, delta-time/FPS logging, and a small math test executable.

## Build (Windows)

Prerequisites: CMake 3.25+ and a supported C++ compiler (Visual Studio/MSVC or clang-cl). The current machine inventory is recorded in `Docs/Inventory/2026-07-21.md`.

```text
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Run `build/Debug/AstralGame.exe`. Press Escape or close the window to exit. The game writes `astral.log` beside the executable/current working directory.

## Playtest art pass

The playable loop includes a generated anime presentation pass in `Game/Assets`: an astral National Mall backdrop, Shadowblade character, training dummy, and HUD sigil. The renderer loads the source PNGs once, caches playtest-sized bitmaps, and projects the character art into the existing perspective scene. The original colored geometry remains underneath as a debug/evidence fallback if an asset is missing.

## Workflow

Every implementation task must have a packet in `Tasks/`, a dedicated worktree, an independent review, a QA report, and a decision-log entry before merge approval. See `Docs/Agents/DEVELOPMENT_LOOP.md`.
