# E11 Editor Runtime Smoke task, 2026-09-22

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. It may harden the native Windows smoke and its evidence, but it must not add scene mutation/serialization, gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed: `b3a2b1bf8f2b0c356d5b352006c48cb86532426b`.
Current code candidate: `006fabd386cd47937b5ce6f7eca627d6960d1c1f`.
Current smoke blob: `ab0957df75fee0129797f7fff6a7fab6c871d30c`.
Integrated editor source fixture: `Tools/AstralEditorMain.cpp` blob `5142e632a79c89d0d0ce3efe87e752456f802185`; production editor source was not modified.

Allowed paths only: `CMakeLists.txt`, `Tests/EditorRuntimeSmoke.cpp`, this task, `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`, and `Docs/Research/ENGINE-CAPABILITIES.json`. One active writer only. Do not rebase, merge, force-push, or absorb unrelated game-worker work.

## Selected review findings and repairs

Fresh independent review of evidence tree `e1b3b5a64c21fb47aba5980b66493b4795010580` reported two P2 false-pass paths.

First, the smoke fabricated `LBN_SELCHANGE` after programmatic selection but did not prove the Outliner retained `LBS_NOTIFY`. Microsoft documents that user-driven `LBN_SELCHANGE` is sent only by a list box with `LBS_NOTIFY`, so loss of that style could break normal mouse/keyboard Inspector synchronization while the smoke still passed. Candidate `006fabd...` now reads the live style with `GetWindowLongPtrW(..., GWL_STYLE)`, requires `LBS_NOTIFY`, and revalidates PID, parent, class, control ID, visibility, and enabled state before and after the style read. The style is checked in full shell validation, immediately before `LB_SETCURSEL`, and again before the bounded notification.

Second, the smoke searched all visible `Static` controls for required text each time. If two Static controls exchanged roles/captions, the expected text set could remain present and pass. Candidate `006fabd...` now binds five distinct initial semantic Static HWNDs for Outliner label, Inspector label, Inspector body, Assets label, and status. Every later shell-state validation reads the expected text from those exact original HWNDs. The existing original 12-child HWND/class continuity gate remains in force, so replacement controls and semantic-role swaps both fail closed.

All prior enabled-state, exact-row, Inspector, bounded-message, resize, containment, ownership, handle-continuity, disabled-pending-tool, cleanup, Release-assertion, and exclusive-desktop requirements remain unchanged. No production editor source, dependency, architecture, or graphics API changed.

## Research basis, rechecked 2026-09-22

Primary behavioral/API references:

- Microsoft Learn, `LBN_SELCHANGE`: https://learn.microsoft.com/en-us/windows/win32/controls/lbn-selchange
- Microsoft Learn, List Box Styles / `LBS_NOTIFY`: https://learn.microsoft.com/en-us/windows/win32/controls/list-box-styles
- Microsoft Learn, `GetWindowLongW` / `GWL_STYLE`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowlongw
- Microsoft Learn, `IsWindowEnabled`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindowenabled
- Epic Games, UE 5.8 Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
- Epic Games, UE 5.8 Selecting Actors: https://dev.epicgames.com/documentation/unreal-engine/selecting-actors-in-unreal-engine
- Epic Games, UE 5.8 Unreal Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
- Unity Technologies, Unity 6.0 Hierarchy: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html

Microsoft documents that `LBN_SELCHANGE` for user input is sent only by a list box with `LBS_NOTIFY`, and that `GWL_STYLE` retrieves current window styles. Epic documents the Outliner as an interactive Actor selection surface synchronized with the viewport and Details panel. Unity documents the Hierarchy as the scene-object management surface. These references are behavioral/API comparison only. No proprietary implementation was copied and no dependency was imported.

## Portable reproduction

Disposable sandbox fixture: `/mnt/data/e11_semantic_notify_fixture.cpp`.
SHA-256: `469803e6556d87fc3f5139e0145cd7133aed1d9b9eda5e3b0f9e224464372bc9`.

The fixture models the two review findings. The former predicate accepts an Outliner with notifications disabled and accepts swaps among semantic Static slots as long as the expected text set remains present. The hardened predicate requires the notification contract and exact text per bound original semantic handle.

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror /mnt/data/e11_semantic_notify_fixture.cpp -o /mnt/data/e11_semantic_notify_fixture
/mnt/data/e11_semantic_notify_fixture
clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /mnt/data/e11_semantic_notify_fixture.cpp -o /mnt/data/e11_semantic_notify_fixture_san
ASAN_OPTIONS=detect_leaks=1 /mnt/data/e11_semantic_notify_fixture_san
sha256sum /mnt/data/e11_semantic_notify_fixture.cpp
```

Results: GCC C++17 warning-clean compile/execution PASS; Clang C++17 ASan+UBSan warning-clean compile/execution PASS; no sanitizer finding. This is source-logic evidence only, not native Win32 execution.

## Hosted verification

Exact code candidate `006fabd386cd47937b5ce6f7eca627d6960d1c1f` completed all available hosted workflows successfully:

- Windows build and deterministic tests `35787602365`, job `106948082419`: `completed/success` on exact head, completed 2026-09-22T21:38:00Z. Repository/R0 safety contracts, VS2022 x64 configuration, MSVC Debug build/tests, MSVC Release build/tests, dependency/prerequisite/runtime-policy checks, static milestone verifiers, and clean-tree verification passed.
- profiling capture portability `35787602447`: `completed/success`.
- release manifest integrity `35787602218`: `completed/success`.

Hosted deterministic CTest intentionally excludes every `RuntimeSmoke`, so this is compile/non-runtime regression evidence only. The historical R0 runner itself was not executed.

Review-thread replies identify this exact repair and fixture. Fresh independent review must be requested against the final evidence head after the task/QA/capability reconciliation, because the review of `e1b3b5a...` is the review that raised these two findings and cannot accept its own repair.

## Acceptance contract

The smoke may report PASS only if all existing E11 invariants remain true plus both new requirements:

1. Outliner remains an enabled, visible, process-owned direct `ListBox` at control ID 1001 with `LBS_NOTIFY`, including immediately before selection and notification.
2. Outliner label, Inspector label, Inspector body, Assets label, and status retain their initially bound distinct semantic HWNDs and expected text through selection, both resizes, and final validation.

The established contract also retains one stable visible/enabled process-owned top-level window, exact 12-child identity/class continuity, five exact visible disabled pending buttons, exact five ordered Outliner rows, exact four ordered Assets rows, exact Scene Root/Cube Inspector fixtures, post-notification Cube synchronization, bounded 800x600 and 420x260 asynchronous resizes with containment/full-state validation, bounded cross-process messages, and process-owned exit/cleanup. Never weaken an acceptance check to make the gate green.

## Registered-local handoff

Run the final candidate on one owned interactive Windows desktop using external build output:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, full stdout/stderr, exit codes, UTC timestamps, and normal plus narrow/short screenshots. Human-visible acceptance must confirm all five exact Outliner rows, all four exact asset rows, real mouse/keyboard Outliner selection updating Inspector/viewport, truthful disabled pending tools, panel containment, and continuity of the original semantic/control HWNDs.

## Stop and next action

E11 remains partial. `native_evidence` is empty and final independent acceptance is false. Issue #7 remains open; do not invoke the historical R0 runner. Do not begin dependent scene-document, gizmo, undo/redo, save/reopen, or other editor feature work based on hosted compilation alone.

Single next useful action: reconcile the final evidence head, obtain fresh independent review of that exact tree, then execute Debug and Release `EditorRuntimeSmoke` on the registered Windows desktop with retained receipts/screenshots.
