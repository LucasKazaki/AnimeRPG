# E11 Editor Runtime Smoke, 2026-09-22

## Scope

Add one native Windows runtime-smoke test for the already-integrated Astral Editor shell. This packet does not add scene mutation, serialization, transform gizmos, Play-in-Editor, asset importing, a graphics-API change, game content, or any dependency.

Baseline: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1` (`main` when admitted).
Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Local execution authority remains the existing Company Runtime only. The GitHub coordinator does not claim a registered local worktree or desktop session.

Allowed implementation paths:
- `CMakeLists.txt`
- `Tests/EditorRuntimeSmoke.cpp`
- `Tasks/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`
- `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`

No other path is authorized by this packet.

## Dependency and reason

The integrated E11 shell has hosted Debug/Release build evidence, but its capability record still has no native GUI receipt. Existing game runtime smokes launch `AstralGame`; none exercise `AstralEditor`. This packet therefore adds a native test surface without claiming that hosted CI can execute an interactive Windows desktop check.

The test must remain named `EditorRuntimeSmoke` so `cmake/AstralTestSafety.cmake` applies the existing `RUN_SERIAL` and 180-second timeout contract. Hosted deterministic CI may compile the test but continues to exclude `RuntimeSmoke` execution.

## Current primary-source research

Read 2026-09-22:

- Epic Games, Unreal Engine 5.8, Unreal Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
  - Applicability: a comparable editor exposes a viewport, Outliner, Details panel, content surface, toolbar, and tool state that can be observed as a coherent application shell.
  - Licensing: behavioral/interface reference only. No Epic source, artwork, or assets are copied.
- Unity Manual, current editor/search documentation: https://docs.unity3d.com/jp/current/Manual/search-use-provider.html
  - Applicability: Unity treats Project and Hierarchy as first-class editor surfaces and searches loaded-scene objects and project assets separately. Astral's current Outliner/assets surfaces remain only procedural placeholders.
  - Licensing: behavioral reference only. No Unity source or assets are copied.
- Microsoft Win32 `EnumChildWindows`: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-enumchildwindows
  - Applicability: enumerate the editor's child controls without depending on fixed child handles in the smoke process.
- Microsoft Win32 `GetClassNameW`: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-getclassnamew
  - Applicability: classify observed child controls by actual Win32 class.
- Microsoft Win32 `IsWindowEnabled`: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-iswindowenabled
  - Applicability: verify the E11 contract that Select/Move/Rotate/Scale/Play remain disabled until implemented.

## Acceptance test

`EditorRuntimeSmoke` must launch the exact built `AstralEditor` executable in a separate process and fail unless all of the following are observed:

1. one visible top-level editor window owned by the launched process;
2. title `Astral Editor 0.1`;
3. exactly the required 12 direct E11 child controls: five `BUTTON`, two `LISTBOX`, and five `STATIC` controls;
4. all five pending tool buttons are visible and disabled;
5. the Outliner has five items and starts with `Scene Root` selected;
6. the initial Inspector text corresponds to `Scene Root`;
7. selecting `Cube` through the Outliner message contract and emitting `LBN_SELCHANGE` updates the Inspector to the Cube fixture;
8. after normal and deliberately narrow/short resizes, every direct child-control rectangle remains inside the actual client rectangle;
9. `WM_CLOSE` terminates the editor process within a bounded wait and the process exits with code 0.

The smoke must use bounded waits and must terminate only the process it launched if cleanup is required. It must not use global keyboard/mouse injection.

## Verification

Hosted/source checks for the exact candidate:

```powershell
python Scripts/test_test_safety.py
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -E RuntimeSmoke --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -E RuntimeSmoke --no-tests=error
```

Registered-local native execution, on one owned interactive desktop:

```powershell
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain source SHA, machine identity, Windows version, MSVC/CMake versions, GPU/driver identity, exact commands, stdout/stderr, exit codes, UTC timestamps, and screenshots of the normal and narrow/short editor states. The automated smoke is additional evidence, not a substitute for the required human-visible interactive receipt or independent review.

## Stop and rollback

Stop at the first deterministic build/test failure introduced by this packet and keep the failing logs. Do not weaken `EditorRuntimeSmoke`, `astral_add_test`, Release assertions, `RUN_SERIAL`, or timeout rules to obtain green CI. Do not run R0.

Rollback is deletion of this packet's new test/task/QA files and the corresponding `CMakeLists.txt` registration on the owned branch only. Do not rewrite history or alter `main`.