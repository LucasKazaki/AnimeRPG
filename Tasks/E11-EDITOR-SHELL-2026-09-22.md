# E11.0 Astral Editor shell

Date: 2026-09-22

## Scope

Implement one bounded, non-content editor-tooling slice on top of `64e35980781c875d6f62e798e3bfa17fdc093897` (PR #8). This is not an Unreal/Unity parity claim and does not resume game content. It adds a separate Win32/GDI `AstralEditor` executable without changing `AstralGame`, the production graphics API, dependencies, or engine architecture.

The full E11 contract still depends on E02/E03/E04. This packet only establishes a visible editor frame and deterministic panel layout so later packets can connect real scene ownership, asset identity, save/reopen, gizmos, undo/redo and play-in-editor.

## Primary-source research

Accessed 2026-09-22:

- Epic, Unreal Engine 5.8, Unreal Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
  - Relevant minimum concepts: toolbar, level viewport, Outliner, Details panel and Content Drawer/Browser.
- Epic, Unreal Engine 5.8, Content Browser: https://dev.epicgames.com/documentation/en-us/unreal-engine/content-browser-in-unreal-engine
  - Relevant minimum concept: project asset browsing/management is a first-class editor surface.
- Unity Manual, Unity 6.1 Scene view navigation: https://docs.unity3d.com/Manual/SceneViewNavigation.html
  - Relevant minimum concept: Scene view is the authoring view, distinct from the final Game view.
- Unity Manual editor-interface documentation (current/manual family): https://docs.unity3d.com/Manual/UsingTheEditor.html
  - Relevant minimum concepts: Hierarchy, Scene view, Inspector, Project window and Play controls.

No proprietary source was copied. The panel arrangement is an original Astral implementation using Win32 child controls and GDI.

## Allowed paths

- `CMakeLists.txt`
- `Engine/Editor/EditorLayout.h`
- `Engine/Editor/EditorLayout.cpp`
- `Tools/AstralEditorMain.cpp`
- `Tests/EditorLayoutTests.cpp`
- `Tasks/E11-EDITOR-SHELL-2026-09-22.md`
- `Docs/QA/E11-EDITOR-SHELL-2026-09-22.md`
- `Docs/Research/ENGINE-CAPABILITIES-2026-09-20.md`
- `Docs/Research/ENGINE-CAPABILITIES.json`

## Required behavior

1. Build a separate `AstralEditor` Windows executable using only existing Win32/GDI system libraries.
2. Show a toolbar, selection-linked Outliner, central procedural viewport, Inspector, asset/default-primitives browser and status area.
3. Keep not-yet-implemented controls honest. Play is disabled; save/reopen, gizmos, undo/redo, import and real scene mutation remain explicitly pending.
4. Use only procedural editor fixtures (`Scene Root`, camera, light, cube and floor), not paused game content or production art.
5. Resize without negative panel geometry or panel overlap.
6. Do not modify `AstralGame`, change GDI, install dependencies or invoke R0.

## Tests and evidence

Portable source check:

```bash
g++ -std=c++17 -Wall -Wextra -Werror -I. Engine/Editor/EditorLayout.cpp Tests/EditorLayoutTests.cpp -o editor_layout_tests
./editor_layout_tests
```

Windows hosted/local gate:

```powershell
cmake -S . -B ../AnimeRPG-e11-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-build -C Debug --output-on-failure -E RuntimeSmoke
cmake --build ../AnimeRPG-e11-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-build -C Release --output-on-failure -E RuntimeSmoke
```

Registered local interactive acceptance, still required after hosted compilation:

- Launch `AstralEditor.exe` from the exact candidate SHA.
- Capture machine/toolchain identity, command line, UTC timestamps, exit status and screenshots.
- Verify Outliner selection changes Inspector text and selected viewport label.
- Resize repeatedly at 800x600, 1280x720, 1440x900 and maximized desktop size; verify panels remain contained and usable.
- Verify `AstralGame` still launches separately and no game-content behavior is changed by this packet.

## Stop and rollback

Stop on deterministic compile/test failure, unexpected change outside allowed paths, or any requirement to change the graphics API/dependency set. Preserve the failing receipt rather than weakening tests. Rollback is branch-only: abandon/revert this packet; it has no data migration and must not alter shared local runtime state.

## Deferred E11 work

Real scene document ownership/serialization, editable transforms and gizmos, asset import/database integration, drag/drop, undo/redo, save/reopen, Play/Stop, docking, menus, logs/console, 2D authoring, accessibility, tutorial/onboarding and starter-project content remain open. Native GUI acceptance and independent review are required before merge approval.
