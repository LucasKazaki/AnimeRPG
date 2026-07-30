# M7 — Shadowblade Action Kit

## Objective

Add a distinct Shadowblade gameplay layer to the M5 perspective wireframe world and M4 combat sandbox. The player must be able to dash, execute a high-impact fatal strike, guard, and manage a deterministic shadow resource/cooldown model. This is new playable behavior, not a renderer-only or title-only change.

## Scope

Implement a C++17 `ShadowbladeActions` domain module with deterministic state and clear outcomes:

- **Shadow Dash**: `Q`; requires sufficient resource and available cooldown; performs a bounded forward movement through the existing controller path.
- **Fatal Strike**: `L`; consumes resource, requires the dummy to be in strike range, and applies distinct high damage through the existing combat domain.
- **Guard**: left Shift held; represents an active guarded state, blocks action activation conflicts, and is rendered/titled visibly.
- **Resource/cooldowns**: capped resource, defined costs, cooldown countdown using positive delta time only, and deterministic rejection reasons.
- **World/UI feedback**: retain the M5 perspective blockout, landmarks, player, and dummy. Add clear GDI/title feedback for Shadowblade action/cooldown/resource/guard state; do not rely only on logs.
- **Integration**: wire input into the live Win32 loop without breaking M3 movement, M4 combat, M4/M5 runtime smokes, or Escape exit.

## Allowed files

- `Tasks/M7-shadowblade.md`
- `Engine/Scene/ShadowbladeActions.h`
- `Engine/Scene/ShadowbladeActions.cpp`
- `Engine/Scene/CombatSandbox.h`
- `Engine/Scene/CombatSandbox.cpp`
- `Engine/Platform/Win32Application.h`
- `Engine/Platform/Win32Application.cpp`
- `Engine/Renderer/Renderer.h`
- `Engine/Renderer/Renderer.cpp`
- `Tests/ShadowbladeActionsTests.cpp`
- `Tests/M7RuntimeSmoke.cpp`
- `CMakeLists.txt`
- `Docs/QA/MILESTONE-7.md`
- `Docs/Decision-Log.md`
- `Docs/Planning/MILESTONES.md`

Do not edit unrelated files. No dependencies, downloads, engine/framework replacement, networking, art import, AI, physics, character animation, or public deployment.

## Acceptance evidence

1. Focused deterministic tests cover resource caps/costs, successful/rejected dash, cooldown guard, fatal-strike range/resource behavior, dummy damage, guard conflict behavior, and invalid delta handling.
2. Debug and Release builds succeed; complete CTest passes in both configurations.
3. `M7RuntimeSmoke` launches the actual `AstralGame`, observes M7 title/state, exercises Q/L/guard through the real input path, observes a bounded gameplay state change, and verifies clean Escape exit.
4. Existing M4/M5 runtime smoke tests remain green.
5. `git diff --check` is clean and no out-of-scope files are changed.

## Integration policy

Lucas explicitly delegates local commits and merges once the listed tests, scope audit, Debug/Release gates, runtime evidence, and clean diff all pass. Commit and merge only then; create the next fresh dynamic task worktree immediately afterward. If any gate fails, do not merge and record the exact reproducible blocker.

## Completion evidence — 2026-07-30

- Confirmed before resuming that no CMake, CTest, compiler, linker, or coordinating MSBuild build was active. Eight orphaned `/nodeReuse:true` MSBuild worker nodes from the terminated process were idle; no concurrent build was started.
- Repaired the interrupted runtime smoke's repeated-Q synchronization so the live edge-triggered loop observes key release before the second press.
- Canonical external build tree: `C:/AI/builds/AnimeRPG/m7-shadowblade`.
- Configure: `cmake -S C:/AI/worktrees/AnimeRPG/m7-shadowblade -B C:/AI/builds/AnimeRPG/m7-shadowblade -G "Visual Studio 17 2022" -A x64` — PASS.
- Debug build — PASS; complete Debug CTest — PASS, 8/8.
- Release build — PASS; complete Release CTest — PASS, 8/8.
- `M7RuntimeSmoke` — PASS in Debug and Release: actual game title/state observed, held guard and guard-blocked Q observed, fatal strike applied 80 damage, dash moved bounded live position from Z 0 to Z 6, repeated Q reported cooldown, GDI resource/guard pixels were captured, and Escape exited with code 0.
- Existing M4 and M5 runtime smokes — PASS in Debug and Release.
- `python Scripts/verify_milestone3.py` — PASS; `git diff --check` — PASS; allowlist scope audit — PASS.
- Full evidence and capability limits: `Docs/QA/MILESTONE-7.md`.
