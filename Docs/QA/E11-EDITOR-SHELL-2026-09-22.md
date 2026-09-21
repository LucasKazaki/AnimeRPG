# E11.0 Astral Editor shell evidence

Date: 2026-09-22

## Candidate and boundaries

Branch: `engine/2026-09-22-editor-shell`
Baseline: `64e35980781c875d6f62e798e3bfa17fdc093897` (PR #8 head at packet creation)
Implementation checkpoint before this evidence file: `089bc31a7c126177d43efe8d3de186ed861918ab`

This packet adds a separate `AstralEditor` executable. It does not alter `AstralGame`, change the GDI graphics API, install dependencies, touch Company Runtime state, invoke R0, resume game content, merge, release or deploy anything.

## What was implemented

- A deterministic editor panel-layout model under `Engine/Editor`.
- A standalone Win32/GDI `AstralEditor 0.1` shell with:
  - toolbar surfaces for Select/Move/Rotate/Scale and a deliberately disabled `Play (pending)` button;
  - an Outliner containing only procedural editor fixtures;
  - a central GDI authoring viewport with grid, axes and placeholder cube;
  - selection-linked Inspector text;
  - an Assets / Default Primitives browser with Cube, Plane, Camera and DirectionalLight placeholders;
  - a status line that explicitly lists major deferred editor capabilities.
- A native CTest target for deterministic panel geometry.
- Capability-map status changed only to `partial_editor_shell_candidate`, with native evidence and independent acceptance still empty/false.

This is the first visible editor frame, not a UE5/Unity-equivalent editor. Transform buttons do not mutate objects yet. Save/reopen, serialization, gizmos, undo/redo, drag/drop, import, Play/Stop, docking, editor/runtime isolation tests, real rendering/material previews and tutorial onboarding remain open.

## Research basis

Primary sources accessed 2026-09-22:

- Unreal Engine 5.8 `Unreal Editor Interface`, Epic Games: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
- Unreal Engine 5.8 `Content Browser`, Epic Games: https://dev.epicgames.com/documentation/en-us/unreal-engine/content-browser-in-unreal-engine
- Unity 6.1 `Scene view navigation`, Unity Technologies: https://docs.unity3d.com/Manual/SceneViewNavigation.html
- Unity editor interface manual family: https://docs.unity3d.com/Manual/UsingTheEditor.html

Applicability: these sources establish the core authoring surfaces used by the comparison engines: viewport/Scene view, hierarchy/Outliner, Inspector/Details, project/content browser and play controls. They are functional references only. No proprietary source, assets or UI artwork were copied.

## Sandbox/compiler evidence

The container could not clone GitHub because DNS resolution for `github.com` failed. That failure was preserved rather than retried repeatedly.

A disposable partial fixture containing the exact published `EditorLayout.h`, `EditorLayout.cpp` and `EditorLayoutTests.cpp` source was compiled and executed with:

```text
g++ -std=c++17 -Wall -Wextra -Werror -I/tmp/e11 \
  /tmp/e11/Engine/Editor/EditorLayout.cpp \
  /tmp/e11/Tests/EditorLayoutTests.cpp \
  -o /tmp/e11/editor_layout_tests
/tmp/e11/editor_layout_tests
```

Result: PASS, exit 0, output `EditorLayoutTests: PASS`.

Git blob identity for the three sandbox-tested files:

- `Engine/Editor/EditorLayout.h`: `723ce6906b7fd9faaa4402c7774dd3a49e8e5f86`
- `Engine/Editor/EditorLayout.cpp`: `994625ff7484683ce377fb63d8aa947de58bf6ab`
- `Tests/EditorLayoutTests.cpp`: `4bd5ba7f56548de465e7acdbac3cb010e8d7ce51`

The first hash was re-read from GitHub after publication and matched the sandbox hash. The Windows editor shell itself cannot be compiled in this Linux container because the Windows SDK/headers are unavailable; hosted Windows and registered local Windows evidence are separate gates.

## Tests covered

The layout test checks 1280x720, 1920x1080, 800x600, 320x200, zero-size and negative-size inputs. It verifies every region stays inside the clamped client rectangle and that toolbar/content/status plus Outliner/viewport/Inspector/assets do not overlap. It also pins the intended 1280x720 layout dimensions.

This is deterministic layout evidence only. It does not prove native control creation, DPI behavior, font scaling, keyboard focus, paint correctness, accessibility or interactive editor behavior.

## Hosted Windows gate

Pending at the time this evidence record was first written. A draft PR should trigger the repository's existing Windows build/test workflow. Do not promote the packet from candidate based only on the portable test.

Required hosted result for the exact candidate:

- MSVC Debug build of `AstralGame` and `AstralEditor` succeeds.
- Non-interactive deterministic CTests, including `EditorLayoutTests`, pass in Debug.
- MSVC Release build succeeds and deterministic CTests pass in Release.
- Existing safety gates remain green.

## Registered local Windows handoff

The registered local executor should use a clean owned worktree at the exact reviewed SHA and retain source SHA, Windows version, compiler/SDK version, GPU/driver identity where captured, exact commands, stdout/stderr, exit codes, UTC timestamps and screenshots.

Run:

```powershell
cmake -S . -B ../AnimeRPG-e11-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-build -C Debug --output-on-failure -E RuntimeSmoke
& ../AnimeRPG-e11-build/Debug/AstralEditor.exe
```

Interactive checks:

1. Confirm the editor opens as a separate application titled `Astral Editor 0.1`.
2. Capture a screenshot at initial size.
3. Select Camera, Directional Light, Cube and Floor in the Outliner. Confirm the Inspector and viewport selection label update for each.
4. Resize at 800x600, 1280x720, 1440x900 and maximized size. Capture at least the smallest and largest states and verify no pane overlaps or leaves the client region.
5. Confirm `Play (pending)` is visibly disabled and no control falsely implies save, undo or real scene editing exists.
6. Launch `AstralGame.exe` separately and confirm the editor packet did not replace or alter the production game executable.

Release should then be built/tested with the same retained receipt set. GUI/native acceptance remains incomplete until these exact checks exist.

## Capability-to-evidence status

- E11 UI/editor tooling: advanced from `debug HUD only` to a source candidate for a separate editor frame. Still partial and not independently accepted.
- E02 scene ownership/serialization: unchanged, partial.
- E03 asset pipeline: unchanged except PR #8 candidate; no asset database/import integration added here.
- E04 GPU rendering/materials: unchanged, GDI baseline retained.
- E12 genuine 2D: unchanged.
- E14 profiling: unchanged.
- E15 packaging/platforms: unchanged.
- E16 remaining catalogue, including terrain/VFX/cinematics/scripting/input-replay/accessibility/localization/platforms: unchanged and unresolved.
- E17 comparative acceptance: blocked; no parity claim.

## Next useful action

First resolve the hosted Windows result for this exact editor candidate and obtain the registered native GUI receipt. If those pass, the next bounded E11 packet should connect the editor to a small versioned scene-document model with editable transforms plus save/reopen and undo/redo tests. Do not implement gizmos or drag/drop on ad-hoc game state before that ownership/serialization contract exists.
