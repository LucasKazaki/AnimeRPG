# E11 Editor Runtime Smoke task, 2026-09-23

## Scope and ownership

Bounded verification-only packet for the integrated Win32 `AstralEditor`. Owned branch: `engine/2026-09-22-editor-runtime-smoke`. Admitted baseline: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`. Allowed paths only: `CMakeLists.txt`, `Tests/EditorRuntimeSmoke.cpp`, this task, `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`, and `Docs/Research/ENGINE-CAPABILITIES.json`.

Do not add editor/game features, change rendering/API architecture, add dependencies, alter scheduler/runtime state, merge/rebase, deploy/release, invoke R0, or restart paused content. One active writer only. Issue #7 remains open, so the historical R0 runner is blocked.

## Current selected finding

Fresh independent Codex review of exact evidence head `3f093a98f5af4257ab180dcc15183aa47073a7df` completed at `2026-09-23T22:33:42Z` and identified a P2 HWND-reuse race in `ResizeAndCheck`.

The prior source `95177d199bedb6195c0d89aa0d05708af04f6512` repeated launched-process ownership immediately before `SetWindowPos`, but that validation still returned before the subsequent visibility/enabled checks and side-effecting `SetWindowPos(..., SWP_ASYNCWINDOWPOS)`. The launched editor could exit in that interval, Windows could recycle the HWND, and an unrelated visible/enabled window could receive the resize.

The same review also found that this task and the QA receipt were stale relative to the current source/evidence chain. This file is the current authoritative task record and supersedes stale handoff SHAs embedded in earlier continuation text.

## Bounded repair

Implementation commit: `9b3aa8ebe320649b9319ba653838db53dde38dde`.

`Tests/EditorRuntimeSmoke.cpp` blob: `97d374db438a1f8b4f0d161e9de6f823da0c792d`.

GitHub commit diff: one changed file, `Tests/EditorRuntimeSmoke.cpp`, 64 additions and 14 deletions.

The repair adds `PostResizeWhileOwnerPinned(...)` and passes the retained `CreateProcessW` primary-thread ID/handle plus process handle into every resize. Before a resize post, the helper:

1. requires the launched process and primary thread handles to remain nonsignaled;
2. requires the cached top-level HWND to remain visible, enabled, and owned by the exact launched PID/TID;
3. suspends only that exact primary owner thread and requires a zero prior suspend count;
4. requires `GetThreadContext(..., CONTEXT_CONTROL)` to succeed as a post-suspend execution barrier;
5. while that creator thread is stopped, rechecks process/thread liveness, HWND PID/TID ownership, visibility, and enabled state;
6. issues only the asynchronous `SetWindowPos(..., SWP_ASYNCWINDOWPOS)` resize;
7. immediately resumes the thread and requires `ResumeThread` to report a previous count of one before any completion poll or wait.

The helper fails closed on liveness or identity mismatch, pre-existing suspension, context failure, post failure, or unexpected resume state. The existing 1.5-second resize phase deadline, scoped cross-process message deadlines, containment/shell checks, exact maximize/restore placement, semantic shell assertions, Cube selection/Inspector synchronization, cleanup, and hardened final close remain in force.

No production editor/game source, CMake registration, workflow, dependency, graphics API, scheduler configuration, release/deployment state, or game content changed.

## Primary-source basis, rechecked 2026-09-23

- Microsoft `SetWindowPos`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowpos . `SWP_ASYNCWINDOWPOS` can post the request to the target window's owning thread rather than blocking the caller.
- Microsoft `GetWindowThreadProcessId`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowthreadprocessid . It returns the creating thread and optional process ID and returns zero for an invalid HWND.
- Microsoft `SuspendThread`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-suspendthread . It suspends user-mode execution and returns the prior suspend count. Microsoft cautions that it is primarily a debugger facility, so this harness uses it only for the short owner-pin interval and performs no target-dependent wait while suspended.
- Microsoft `GetThreadContext`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadcontext . A valid context cannot be obtained for a running thread, so a successful context capture is used as the post-suspend barrier before identity revalidation and the asynchronous post.
- Microsoft `ResumeThread`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-resumethread . It decrements the suspend count and restarts execution when the count reaches zero.
- Epic Unreal Engine 5.8, Using Editor Viewports: https://dev.epicgames.com/documentation/unreal-engine/using-editor-viewports-in-unreal-engine . Perspective 3D, orthographic 2D, multi-viewport, maximized, and immersive authoring remain comparison workflows.
- Unity 6.0, `EditorWindow.maximized`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/EditorWindow-maximized.html . Maximized editor-window state remains a comparison workflow property.

Public behavioral/API documentation only. No proprietary engine source copied and no dependency imported.

## Portable source-logic evidence

Disposable C++17 owner-pin state-machine fixture SHA-256 `a092dedb8ba74fb0df0e967d0d2465f7229532b6a73edabb0b8a3e1e83fa5432` covers the valid path plus recycled-owner-after-barrier, process death, missing context barrier, pre-suspended thread, post failure, and bad resume count. It passed:

```text
g++ 14.2.0: -std=c++17 -Wall -Wextra -Werror -pedantic
clang++ 17.0.0: -std=c++17 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer
ASAN_OPTIONS=detect_leaks=1
```

Both executions printed `e11 resize owner pin fixture: PASS`. This is source-logic evidence only, not native Win32 GUI evidence.

## Hosted source verification

Source commit `9b3aa8ebe320649b9319ba653838db53dde38dde` is green in hosted verification:

- Windows build and deterministic tests: run `35929455430`, job `107412305793`, `success`;
- profiling capture portability: run `35929455501`, `success`;
- release manifest integrity: run `35929455423`, `success`.

The Windows job passed repository/R0 safety contracts, VS2022 x64 configure, Debug build/tests, Release build/tests, release dependency/prerequisite/runtime checks, static milestone verifiers, and clean tracked-tree verification.

GitHub synthetic pull-request merge tested for this source: `90175bb0edf372473056374a0f30cacb684defbe`, whose parents are base `6d22da88402db71843ecd5a35766c0c77e62dca6` and source `9b3aa8ebe320649b9319ba653838db53dde38dde`.

Hosted CTest still intentionally excludes interactive `EditorRuntimeSmoke`. These runs establish hosted compilation/deterministic/integration evidence only, not native GUI/GPU acceptance.

## Retained acceptance gates

`native_evidence` remains empty. E11 final acceptance remains false. Fresh independent review is required on the exact post-evidence head after task, QA, and capability records are current. Same-author review is not independent acceptance.

The registered Windows executor must then run Debug and Release `EditorContainmentTests` plus interactive `EditorRuntimeSmoke` on that exact clean-reviewed SHA, retain machine/Windows/MSVC/CMake/GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, screenshots, and zero-owned-process cleanup proof. The state matrix remains untouched startup, 800x600, 1280x720, 1440x900, actual maximized desktop followed by exact restore to the pre-maximize outer rectangle, and 420x260 or closest OS-permitted narrow state. Evidence must show each resize post targets the exact launched editor owner, each resize acceptance completes within 1.5 seconds, semantic/containment state remains valid, and final close is safe. Separately launch `AstralGame` from the same source/build as a no-regression check.

Clean-machine packaging, comparative frame/RAM/VRAM measurement, wider stress/recovery, the remaining E00-E17 capability catalogue, and the required 24-hour soak remain unresolved.

## Rollback and stop conditions

Rollback only this verification repair if native evidence proves the owner-pin contract invalid under the existing owned-desktop requirements. Do not weaken the acceptance tests to make them green. Stop before production-runtime changes, dependency/API changes, unrelated workflow changes, rebase/merge, R0 execution, scheduler operations, release/deployment, or game-content work.

## Single next useful action

Bring the QA and capability records onto this exact source, obtain clean hosted checks and fresh independent review for the final evidence head, then hand that exact reviewed SHA to the registered Windows executor for the retained native Debug/Release matrix and separate `AstralGame` launch. Do not start another dependent editor feature while this native QA gate remains unresolved.
