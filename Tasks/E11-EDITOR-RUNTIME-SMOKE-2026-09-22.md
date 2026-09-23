# E11 Editor Runtime Smoke task, 2026-09-23

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. This packet may harden the native Windows smoke, its recovery supervisor, deterministic containment tests, and evidence. It must not add scene mutation/serialization, gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, rebase, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Admitted baseline from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest observed `main` before this update: `3eaeb72d65c98f887e827999a78eabb636120dfe`. Unrelated game-worker work was not absorbed or rebased into this engine branch.
Current maximized-state verification candidate: `7c68db8bd58e62cc54c39566723e11fd5385e2da`.
`Tests/EditorRuntimeSmoke.cpp` blob: `b7d595d509606a4f607914e244d8f1fc01dfb210`.
`CMakeLists.txt` remains blob `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.

Allowed paths only: `CMakeLists.txt`, `Tests/EditorRuntimeSmoke.cpp`, this task, `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`, and `Docs/Research/ENGINE-CAPABILITIES.json`. One active writer only. Do not rebase, merge, force-push, or absorb unrelated work.

## Selected verification gap

The admitted E11 shell matrix requires untouched startup, 800x600, 1280x720, 1440x900, maximized desktop state, and a deliberately narrow state. The runtime smoke already automated the four fixed sizes but still left maximized behavior entirely to manual evidence. That left a dependency-ready verification gap: no executable assertion proved that entering the native Windows maximized state preserves shell identity, selection/Inspector synchronization, positive-area containment, and disabled pending-tool state, or that restore returns to a valid normal state.

This pass hardens verification only. It does not enable editor features or change production runtime behavior.

## Bounded implementation

Commit `7c68db8bd58e62cc54c39566723e11fd5385e2da` changes only `Tests/EditorRuntimeSmoke.cpp`:

- adds a 3-second bounded show-state deadline;
- adds `MaximizeRestoreAndCheck(...)` after the retained 1440x900 check and before the 420x260 narrow check;
- records the pre-maximize outer width/height and rejects an empty normal rectangle;
- requests maximize with `ShowWindowAsync(..., SW_MAXIMIZE)` and polls until `IsZoomed` is true;
- while maximized, requires the original child HWND inventory, semantic shell, selection/Inspector state, disabled pending tools, positive child area, and client containment to settle successfully before the deadline;
- requests `SW_RESTORE`, requires `IsZoomed` to clear, requires the pre-maximize outer width/height to return, and revalidates the same shell/containment invariants;
- retains startup, 800x600, 1280x720, 1440x900 and 420x260 validation, stable top-level-window checks, process-identity hardening, recovery containment, and safe final close;
- updates PASS text so retained native output identifies the maximize/restore state explicitly.

No production editor source, CMake registration, workflow, dependency, graphics API, scheduler configuration, game content, release state, or deployment state changed in this implementation commit.

## Research basis, rechecked 2026-09-23 UTC

- Microsoft Learn `ShowWindowAsync`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindowasync
  - posts a show-state request without blocking the caller; success means the operation was started. Applicability: bounded cross-thread maximize/restore request.
- Microsoft Learn `ShowWindow`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow
  - defines `SW_MAXIMIZE` / `SW_SHOWMAXIMIZED` and `SW_RESTORE`. Applicability: requested show states.
- Microsoft Learn `IsZoomed`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iszoomed
  - reports whether a window is maximized. Applicability: positive confirmation of native maximized state rather than inferring it from pixel dimensions.
- Epic, Unreal Engine 5.8, Using Editor Viewports: https://dev.epicgames.com/documentation/unreal-engine/using-editor-viewports-in-unreal-engine
  - documents perspective 3D and orthographic 2D viewports, multi-viewport layouts, and maximized/immersive viewport workflows. Applicability: editor layout verification includes materially different and maximized authoring states.
- Epic, Unreal Engine 5.8, Viewport Toolbar: https://dev.epicgames.com/documentation/unreal-engine/viewport-toolbar
  - documents switching between maximized selected viewport and multi-layout workflows.
- Unity 6.0 `EditorWindow.maximized`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/EditorWindow-maximized.html
  - exposes maximized editor-window state.
- Unity 6.0 `FullScreenMode.MaximizedWindow`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/FullScreenMode.MaximizedWindow.html
  - identifies the OS maximized-window mode on Windows/macOS.

Public documentation is used for behavioral/API comparison only. No proprietary Unreal Engine or Unity source was copied and no dependency was imported.

## Verification state

The content-identical pre-change checkpoint `e0dfdd56013cb8596a658a4737121c37b258f025` had green Windows, profiling and release-manifest workflows plus a fresh Codex review with no major issue. It did not automate maximize/restore.

For candidate `7c68db8...`, GitHub commit inspection reports one changed file, `Tests/EditorRuntimeSmoke.cpp`. A disposable portable C++17 show-state state-machine fixture, SHA-256 `a3a0b46c139470dfd93ca0a630338f02b728c09ee9e362e5997e86eb0cfed2c3`, passed warning-clean GCC and Clang ASan+UBSan with leak detection. That fixture is source-logic evidence only, not Win32 GUI evidence.

At the evidence-update checkpoint, profiling run `35895133184` had passed. Windows run `35895133126`, job `107297064090`, had passed repository/R0 safety contracts, VS2022 x64 configure, Debug build, and deterministic Debug tests and was still progressing through Release. Release-manifest run `35895133112` was still in progress. These observations are not promoted to final exact-head acceptance; final workflow conclusions are pinned in PR metadata/checkpoint after the evidence writes.

Hosted deterministic CTest intentionally excludes interactive `EditorRuntimeSmoke`, so hosted green checks can establish compilation and deterministic containment coverage but cannot establish native GUI/GPU acceptance.

## Retained gates

1. `EditorContainmentTests` remains hosted and deterministic; interactive `EditorRuntimeSmoke` remains a native gate.
2. The shell smoke requires one stable process-owned top-level window, the original 12 child HWND/class identities, semantic Static/Button bindings, disabled pending tools, exact Outliner/assets rows, `LBS_NOTIFY`, selection/Inspector synchronization, bounded cross-process messages, positive-area containment, and safe shutdown.
3. Automated runtime states now cover untouched startup, 800x600, 1280x720, 1440x900, native Windows maximized+restored, and 420x260.
4. Human-visible screenshots and usability at actual maximized desktop dimensions remain native evidence requirements. Automation does not substitute for that visual evidence.
5. `native_evidence` remains empty. Independent final acceptance remains false until the exact post-evidence receipt receives fresh independent review and registered native Windows acceptance is retained.
6. Issue #7 remains open, so the historical R0 runner is blocked and must not be invoked.

## Registered native handoff

Only after green hosted checks and a fresh clean independent review of the exact post-evidence receipt tree, the registered Windows executor should use one owned interactive desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact reviewed source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, and process inspection proving zero owned contained processes after any failure or interruption.

Capture screenshots for untouched startup, 800x600, 1280x720, 1440x900, actual maximized desktop state, and 420x260 or the closest OS-permitted narrow state. Confirm panels/controls remain contained, viewport paint does not bleed into adjacent surfaces, selection/Inspector state remains synchronized, pending toolbar actions remain disabled, maximize restores cleanly, and final close leaves the launch thread resumed without acting on an unrelated/recycled HWND. Separately launch `AstralGame` from the same exact source/build as a no-regression check.

## Rollback and stop conditions

Rollback only the maximize/restore verification addition if native evidence shows the OS show-state contract cannot be established reliably under the existing bounded owned-desktop requirements, or if it causes a reproducible regression. Do not drop the maximized acceptance state to make the gate pass. Preserve the failure and repair the verification path instead.

Stop before production-runtime change, workflow edit outside packet authority, rebase, merge, R0 execution, scheduler operation, dependency addition, graphics/API change, or game-content work. Never weaken a native acceptance assertion to make the gate green.

## Single next useful action

Pin the post-write evidence receipt in PR metadata, complete its hosted workflows, obtain fresh independent review of that exact receipt tree, then hand only that reviewed tree to the registered Windows executor for Debug/Release containment plus the full interactive state matrix.
