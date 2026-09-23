# E11 Editor Runtime Smoke task, 2026-09-23

## Scope and ownership

Bounded verification-only packet for the integrated Win32 `AstralEditor`. Owned branch: `engine/2026-09-22-editor-runtime-smoke`. Admitted baseline: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.

Allowed paths only:

- `CMakeLists.txt`
- `Tests/EditorRuntimeSmoke.cpp`
- this task
- `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`
- `Docs/Research/ENGINE-CAPABILITIES.json`

Do not add editor/game features, change rendering or graphics API architecture, add dependencies, alter scheduler/runtime state, merge/rebase, deploy/release, invoke R0, or restart paused engine-worker content. One active writer only. Issue #7 is still open, so the historical R0 runner remains blocked.

## Selected finding and reproducible failure model

The latest unresolved current independent-review thread is `PRRT_kwDOTo2Ig86lXSlD`, top-level comment `4087994063`, P2: **Pin the HWND owner through show-state posts**.

The show-state code present through branch head `bb2d903c022c996ab69f692a673e0759efc684f9` checked launched-process HWND ownership, visibility and enabled state immediately before `ShowWindowAsync(SW_MAXIMIZE)` and `ShowWindowAsync(SW_RESTORE)`, but the checks returned before the side effect. Because `ShowWindowAsync` posts a show-state event instead of waiting for completion, the launched owner could exit after validation and Windows could recycle the cached HWND before the enqueue. An unrelated window could then receive the show-state request.

A disposable source-logic state-machine fixture reproduces this failure class by changing the owner after the initial check but before the post. The repaired predicate rejects that transition and never issues the side effect.

## Bounded implementation

Source repair: `f097819e2904a5a24e45f98e8bd4813f1af2c58e`.

Parent: `bb2d903c022c996ab69f692a673e0759efc684f9`.

`Tests/EditorRuntimeSmoke.cpp` blob: `8b0d633e0c4f34fcf8ccd836468b06e46e8d81be`.

GitHub compare: one changed file, `Tests/EditorRuntimeSmoke.cpp`, 62 additions and 21 deletions. No production editor/game source, CMake registration, workflow, dependency, graphics API, scheduler configuration, release/deployment state, or game content changed.

The repair adds `PostShowStateWhileOwnerPinned(...)` and passes the retained `CreateProcessW` primary-thread ID/handle plus process handle into `MaximizeRestoreAndCheck`. For both `SW_MAXIMIZE` and `SW_RESTORE`, the helper:

1. requires the retained process and primary-thread handles to remain nonsignaled;
2. requires the cached top-level HWND to remain visible, enabled and owned by the exact launched PID/TID;
3. suspends only that exact primary owner thread and rejects any nonzero prior suspend count;
4. requires `GetThreadContext(..., CONTEXT_CONTROL)` to succeed as the post-suspend execution barrier;
5. while the creator thread is stopped, rechecks process/thread liveness, exact HWND PID/TID ownership, visibility and enabled state;
6. issues only the asynchronous `ShowWindowAsync` request;
7. immediately calls `ResumeThread` and requires a previous suspend count of one before any show-state poll, shell validation, containment check or wait.

The helper fails closed on identity/liveness change, pre-existing suspension, context failure, enqueue failure, or unexpected resume state. It performs no target-dependent wait while the target thread is suspended.

The prior resize owner pin, 1.5-second resize phase deadline, show-state phase deadlines, bounded message waits, semantic shell checks, exact pre-maximize rectangle restore, containment checks, Cube selection/Inspector synchronization, cleanup, and hardened final close remain in force.

## Primary-source basis, rechecked 2026-09-23

- Microsoft `ShowWindowAsync`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindowasync . It sets a window's show state without waiting for completion and posts a show-window event to the target window's message queue.
- Microsoft `GetWindowThreadProcessId`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowthreadprocessid . It returns the thread that created the window and optionally its process ID; an invalid HWND returns zero.
- Microsoft `SuspendThread`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-suspendthread . It stops user-mode execution and returns the previous suspend count. Microsoft warns it is primarily a debugger facility and not general synchronization, so this harness confines it to a short verification-only owner-pin interval and does not wait on target-owned work while suspended.
- Microsoft `GetThreadContext`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadcontext . Microsoft states a valid context cannot be obtained for a running thread, so successful `CONTEXT_CONTROL` capture after suspension is the execution barrier before final identity validation and enqueue.
- Microsoft `ResumeThread`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-resumethread . A return value of one means the suspended thread was restarted.
- Epic Unreal Engine 5.8, Using Editor Viewports: https://dev.epicgames.com/documentation/unreal-engine/using-editor-viewports-in-unreal-engine . Perspective 3D, orthographic 2D, multiple viewport layouts, maximized viewports and immersive editing remain comparison workflows.
- Epic Unreal Engine 5.8, `FMaximizeViewportCommand`: https://dev.epicgames.com/documentation/unreal-engine/API/Editor/LevelEditor/FLevelViewportLayout/FMaximizeViewportCommand . Maximize/immersive commands can be queued until the first tick when the parent window exists, reinforcing that maximize is lifecycle-sensitive asynchronous editor behavior.
- Unity 6.0, `EditorWindow.maximized`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/EditorWindow-maximized.html . Maximized docked editor-window state remains a comparison workflow property.

These are public behavioral/API references only. No proprietary Epic/Unity source was copied and no dependency was imported.

## Portable source-logic evidence

Disposable fixture: `/mnt/data/e11_show_state_owner_pin_fixture.cpp` during this coordinator run only.

SHA-256: `6110abf51993b7c2666bc95ef3089989fa448f84b0933c14b67984633c4f1ce1`.

Executed successfully:

```text
g++ (Debian 14.2.0-19) 14.2.0
g++ -std=c++17 -Wall -Wextra -Werror -pedantic e11_show_state_owner_pin_fixture.cpp
result: e11 show-state owner pin fixture: PASS

clang version 17.0.0
clang++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer e11_show_state_owner_pin_fixture.cpp
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
result: e11 show-state owner pin fixture: PASS
```

The fixture covers the accepted path plus owner recycling after the barrier, process death, thread death, missing context barrier, pre-existing suspension, visibility/enabled loss after the pin, enqueue failure, and bad resume count. This is source-logic evidence only, not native Win32 GUI evidence. No MinGW cross-compiler was available in the sandbox, so no sandbox compile of the production Win32 source is claimed.

## Hosted source verification

Exact source `f097819e2904a5a24e45f98e8bd4813f1af2c58e` is green in all three currently associated hosted workflows:

- Windows build and deterministic tests: run `35931194787`, job `107417915549`, `success`, completed `2026-09-23T23:00:47Z`;
- profiling capture portability: run `35931194699`, `success`;
- release manifest integrity: run `35931194743`, `success`.

The Windows job passed repository/R0 safety contracts, Visual Studio 2022 x64 configuration, Debug build and deterministic tests, Release build and deterministic tests, runtime dependency/prerequisite checks, static milestone verifiers, and clean tracked-tree verification.

Hosted CTest still intentionally excludes interactive `EditorRuntimeSmoke`. These runs establish hosted compilation/deterministic/integration evidence only, not native desktop, GPU, screenshot, packaging, performance, stress/recovery or soak acceptance.

## Retained acceptance gates

`native_evidence` remains empty. E11 final acceptance remains false. A fresh independent review is required on the exact post-evidence head after task, QA and capability records are synchronized. Review by the implementation author is not independent acceptance.

After that exact head is clean-reviewed, the registered Windows executor must use one owned interactive desktop and run Debug and Release `EditorContainmentTests` plus interactive `EditorRuntimeSmoke`, retaining source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, screenshots and zero-owned-process cleanup proof.

The native matrix remains: untouched startup, 800x600, 1280x720, 1440x900, actual maximized desktop followed by exact restore to the pre-maximize outer screen rectangle, and 420x260 or closest OS-permitted narrow state. Evidence must show resize, maximize and restore side effects are issued only while the exact launched window-owner thread is pinned, the owner thread is resumed before any completion polling, all deadlines and shell/containment assertions hold, and final close is safe. Separately launch `AstralGame` from the same source/build as a no-regression check.

Clean-machine packaging, comparative frame/RAM/VRAM measurement, broader stress/recovery, all remaining E00-E17 capability gaps, and the required 24-hour soak remain unresolved.

## Rollback and stop conditions

Rollback only this verification repair if native evidence proves the owner-pin contract invalid under the existing owned-desktop requirements. Never weaken the acceptance test to obtain green results. Stop before production-runtime changes, dependency/API changes, unrelated workflow changes, rebase/merge, R0 execution, scheduler operations, release/deployment, or game-content work.

## Single next useful action

Synchronize the QA and capability records to source `f097819e2904a5a24e45f98e8bd4813f1af2c58e`, verify the final evidence-head hosted checks, obtain fresh independent review on that exact final head, then hand that reviewed SHA to the registered Windows executor for the retained native Debug/Release matrix and separate `AstralGame` launch. Do not start another dependent editor feature while this QA gate is unresolved.
