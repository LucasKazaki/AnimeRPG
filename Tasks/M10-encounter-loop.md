# M10 — Landmark Encounter Loop

## Objective

Add one deterministic gameplay increment on top of the verified M9 discovery ledger and M4/M7 combat: discovering the Lincoln Memorial can activate one bounded training encounter. The active/completed state must be visible, defeating the existing training dummy completes it, and completion grants one capped Shadowblade resource reward through the existing resource API.

This is not an enemy-AI system, spawn/reset system, quest framework, renderer rewrite, content pass, or second encounter.

## Player-facing behavior

- Discover the Lincoln Memorial with the existing `E` interaction to activate the training encounter while its existing dummy is alive.
- The encounter has visible `Locked`, `Active`, and `Completed` states.
- Existing `J`/`K` attacks and `L` fatal strike remain the only ways to damage the dummy; their range, guard, resource, and cooldown rules remain authoritative.
- Defeating the dummy while the encounter is active completes it exactly once.
- Completion restores up to 30 Shadowblade resource through `RestoreResource`; the existing maximum-resource cap applies.
- Repeat landmark interaction or later updates cannot reactivate the encounter or farm its reward.

## Allowed files

- `Tasks/M10-encounter-loop.md`
- `CMakeLists.txt`
- `Engine/Scene/LandmarkEncounter.h`
- `Engine/Scene/LandmarkEncounter.cpp`
- `Engine/Platform/Win32Application.h`
- `Engine/Platform/Win32Application.cpp`
- `Engine/Renderer/Renderer.h`
- `Engine/Renderer/Renderer.cpp`
- `Tests/LandmarkEncounterTests.cpp`
- `Tests/M10RuntimeSmoke.cpp`

Do not edit unrelated files. Do not download or install dependencies. Configure and build only in an external build tree, never in the source root.

## Required evidence

1. Focused deterministic tests cover discovery-gated activation, the single designated landmark, unavailable/defeated target rejection, active state, completion, capped reward, and repeat-update/reactivation safety.
2. Debug and Release builds succeed and complete CTest passes in both configurations.
3. `M10RuntimeSmoke` launches the actual native `AstralGame`, discovers Lincoln, observes visible active encounter state, defeats the dummy through the real input path, observes visible completion/reward state, verifies the reward is not repeated, and exits cleanly with Escape.
4. Existing runtime smokes remain green in Debug and Release as part of complete CTest.
5. `git diff --check` and an allowlist scope audit pass.
6. The task commit may be merged into `main` only after the task and main worktrees are clean apart from known pre-existing untracked files, no concurrent writer/build is active, and a clean integration check succeeds.

## Stop conditions

Stop and report the exact blocker rather than broadening scope if this increment requires downloads, source-root CMake output, dummy spawning/reset, changed combat/resource/cooldown/guard rules, unrelated edits, concurrent writers, or unsupported manual-only evidence.

## Completion evidence — 2026-07-30

- Confirmed no CMake, CTest, MSBuild, compiler, or IDE build process was active before implementation.
- Used only the external build tree `C:/AI/builds/AnimeRPG/m10-encounter-loop`; no source-root configure or download was performed.
- Visual Studio 2022 x64 configure — PASS.
- Debug build and complete CTest — PASS, 14/14.
- Release build and complete CTest — PASS, 14/14.
- Focused `LandmarkEncounterTests` — PASS.
- `M10RuntimeSmoke` — PASS in Debug and Release against the native `AstralGame`: M9 Lincoln discovery activated the encounter, active/completed title and GDI states were observed, M4/M7 fatal combat defeated the bounded dummy, completion restored 30 shadow resource through the capped API, a post-defeat attack did not repeat the reward, and Escape exited with code 0.
- Existing M4, M5, M7, M8, and M9 native runtime smokes — PASS in Debug and Release as part of complete CTest.
