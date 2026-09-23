# E11 Editor Runtime Smoke evidence, 2026-09-23

## Current checkpoint

Branch: `engine/2026-09-22-editor-runtime-smoke`.
Current maximized-state source candidate: `7c68db8bd58e62cc54c39566723e11fd5385e2da`.
`Tests/EditorRuntimeSmoke.cpp` blob: `b7d595d509606a4f607914e244d8f1fc01dfb210`.
`CMakeLists.txt` remains blob `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.
Production editor source is unchanged.

The prior exact-tree checkpoint `e0dfdd56013cb8596a658a4737121c37b258f025` completed Windows run `35887803992`, profiling `35887803914`, and release manifest `35887803910` successfully and received a fresh Codex review at `2026-09-23T16:23:22.283217Z` with no major issue. That checkpoint did not automate maximized-state shell verification.

## Finding and repair

The admitted E11 native matrix contains untouched startup, 800x600, 1280x720, 1440x900, maximized desktop, and a narrow state. Fixed dimensions were already automated, but maximized behavior remained a manual-only assertion. This left a false-confidence gap because the smoke could pass every fixed-size state without ever proving that native maximize preserves shell identity/containment or that restore returns to a valid normal state.

Commit `7c68db8bd58e62cc54c39566723e11fd5385e2da` changes only `Tests/EditorRuntimeSmoke.cpp` and adds `MaximizeRestoreAndCheck(...)` between the retained 1440x900 and 420x260 checks. The helper:

- rejects an invalid/already-maximized starting state;
- records the pre-maximize outer width/height;
- requests `SW_MAXIMIZE` through `ShowWindowAsync`;
- confirms native maximization with `IsZoomed` instead of inferring it from dimensions;
- under a 3-second deadline, requires the original child HWND inventory, semantic shell, Cube selection/Inspector synchronization, disabled pending tools, positive child area, and client containment to settle while maximized;
- requests `SW_RESTORE`, confirms `IsZoomed` clears, requires the pre-maximize width/height to return, and revalidates the same invariants;
- retains all startup, fixed-size, process-identity, cleanup, and safe-close checks.

GitHub commit inspection confirms this implementation commit changed only `Tests/EditorRuntimeSmoke.cpp`. No production editor code, CMake registration, workflow, dependency, graphics API, scheduler setting, game content, merge state, release state, or deployment state changed.

## Primary-source research

Accessed 2026-09-23 UTC:

- Microsoft `ShowWindowAsync`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindowasync
- Microsoft `ShowWindow`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow
- Microsoft `IsZoomed`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iszoomed
- Epic Unreal Engine 5.8, Using Editor Viewports: https://dev.epicgames.com/documentation/unreal-engine/using-editor-viewports-in-unreal-engine
- Epic Unreal Engine 5.8, Viewport Toolbar: https://dev.epicgames.com/documentation/unreal-engine/viewport-toolbar
- Unity 6.0, `EditorWindow.maximized`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/EditorWindow-maximized.html
- Unity 6.0, `FullScreenMode.MaximizedWindow`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/FullScreenMode.MaximizedWindow.html

Microsoft documents `ShowWindowAsync` as posting the show-state request without waiting, `SW_MAXIMIZE` and `SW_RESTORE` as the relevant show states, and `IsZoomed` as the maximized-state query. UE 5.8 documents maximized editor viewport workflows alongside perspective/orthographic and multi-layout authoring. Unity 6.0 exposes editor-window maximization and OS maximized-window mode. These are behavioral/API references only. No proprietary engine source was copied and no dependency was imported.

## Portable source-logic fixture

A disposable C++17 show-state state-machine fixture, SHA-256 `a3a0b46c139470dfd93ca0a630338f02b728c09ee9e362e5997e86eb0cfed2c3`, passed:

- `g++ -std=c++17 -Wall -Wextra -Werror` compile and execution;
- `clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer` compile;
- execution with ASan leak detection enabled.

It requires owned/visible/enabled + maximized + contained + semantically valid shell before accepting the maximized state, and non-maximized + restored pre-maximize dimensions + contained + semantically valid shell before accepting restore. This is source-logic evidence only, not Win32 GUI evidence.

The sandbox could not resolve `github.com` for a repository checkout and did not provide the owned interactive Windows desktop required by the native smoke. No sandbox production build, GUI/GPU result, clean-machine package result, performance result, or soak result is claimed.

## Hosted verification observed for source candidate

For source candidate `7c68db8...`:

- profiling capture portability run `35895133184`: PASS;
- Windows run `35895133126`, job `107297064090`: at the evidence-write checkpoint, repository/R0 safety contracts, Visual Studio 2022 x64 configuration, Debug build and deterministic Debug tests had passed; Release build was still running;
- release-manifest run `35895133112`: still in progress at the evidence-write checkpoint.

The later evidence commits supersede the source candidate as the branch head. Final exact-head workflow conclusions belong in PR metadata/checkpoint to avoid an infinite self-referential evidence-commit chain. A cancelled superseded run is not counted as either a pass or failure.

Hosted deterministic CTest intentionally excludes interactive `EditorRuntimeSmoke`. A hosted compile/test pass therefore does not prove native maximize behavior, GUI/GPU behavior, screenshots, clean-machine launch, or human-visible usability.

## Retained hardening and acceptance state

- `EditorContainmentTests` remains hosted and deterministic; interactive `EditorRuntimeSmoke` remains a separate native gate.
- Containment still covers worker-local cleanup, whole-job zero-active-process cleanup, and rejection of a successful worker that leaves descendants.
- Shell validation requires one stable process-owned top-level editor window, original 12 child HWND/class identities, bound semantic Static/Button HWNDs, disabled pending toolbar tools, exact Outliner/assets rows, `LBS_NOTIFY`, selection/Inspector synchronization, bounded cross-process messages, startup containment, and positive area.
- Automated states now cover startup, 800x600, 1280x720, 1440x900, maximized+restored, and 420x260.
- Human-visible screenshots at actual maximized desktop dimensions remain required native evidence.
- Final close retains exact original launch process/thread ownership, suspended-thread context barrier, asynchronous close enqueue, and verified thread resume before process wait.

`native_evidence`: empty.
Independent final acceptance: false.
UE5/Unity parity claim: false.
Issue #7: open. Historical R0 runner not invoked.

## Native handoff

After the exact post-evidence receipt has green hosted checks and fresh clean independent review, the registered Windows executor must run Debug and Release `EditorContainmentTests` and `EditorRuntimeSmoke` on one owned interactive desktop.

Retain exact reviewed SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, full stdout/stderr, exit codes, UTC timestamps, and zero-contained-process proof after any failure or interruption.

Capture screenshots for:

- untouched default startup;
- 800x600;
- 1280x720;
- 1440x900;
- actual maximized desktop state;
- 420x260 or the closest OS-permitted narrow size.

Verify shell containment and positive area, viewport-paint clipping, semantic shell continuity, Cube selection/Inspector synchronization, pending toolbar disabled state, valid maximize/restore, and safe final close. Separately launch `AstralGame` from the same exact source/build as a no-regression check.

Clean-machine packaging, comparative frame-time/RAM/VRAM measurement, broader stress/recovery, the remaining engine feature catalogue, and the required 24-hour soak remain unresolved.

## Single next action

Complete hosted checks and independent review on the exact post-evidence receipt, then hand that exact reviewed tree to the registered Windows executor. Do not start another dependent editor feature before the native QA gate resolves.
