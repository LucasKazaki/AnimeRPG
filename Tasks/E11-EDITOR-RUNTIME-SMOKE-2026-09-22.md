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

## Selected finding and failure model

Independent-review thread `PRRT_kwDOTo2Ig86lX2XO`, top-level comment `4088223052`, identified a P2 stale/recycled-HWND race in `SelectCubeAndNotify`. Before this pass, the smoke validated the cached Outliner HWND and then used synchronous `LB_SETCURSEL`; it later revalidated the editor and Outliner, then synchronously delivered `WM_COMMAND/LBN_SELCHANGE`. Each validation returned before its side effect. If the launched editor exited and Windows recycled one of those HWND values in the gap, the smoke could mutate or notify an unrelated same-integrity window before later validation failed.

This packet repairs only that verification-harness race. It does not change production editor behavior.

## Bounded implementation

Source repair: `57a7867aa4273def5d1dfbb73766883945881c7c`.

Parent: `a87ff45c236deb4fa5f9a734192e8dfad1beade4`.

`Tests/EditorRuntimeSmoke.cpp` blob: `290b4931b3a4083555f0a5ddcae4ee14cbaebe6a`.

GitHub compare from the parent shows one changed file, `Tests/EditorRuntimeSmoke.cpp`, with 133 additions and 24 deletions. No production editor/game source, CMake registration, workflow, dependency, graphics API, scheduler configuration, release/deployment state, or game content changed.

The repair adds `PostSelectionMutationWhileOwnerPinned(...)` and passes the retained `CreateProcessW` primary-thread ID/handle plus process handle into `SelectCubeAndNotify`. For both the Outliner selection mutation and the subsequent selection notification, the helper:

1. requires retained process and primary-thread handles to remain nonsignaled;
2. requires the top-level editor HWND and bound Outliner HWND to belong to the exact launched PID/TID, with the Outliner still the direct `kOutlinerId` `ListBox`, visible, enabled and carrying `LBS_NOTIFY`;
3. suspends only that exact owner thread and rejects any nonzero prior suspend count;
4. requires `GetThreadContext(..., CONTEXT_CONTROL)` to succeed as the post-suspend execution barrier;
5. while the creator thread is stopped, rechecks process/thread liveness plus exact editor/Outliner PID/TID and semantic identity;
6. posts only the asynchronous `LB_SETCURSEL` or `WM_COMMAND/LBN_SELCHANGE` message while the exact creator is pinned;
7. immediately calls `ResumeThread` and requires a previous suspend count of one before any selection or Inspector poll.

The selection phase is bounded by a new three-second deadline. After the asynchronous `LB_SETCURSEL` post, the smoke polls bounded `LB_GETCURSEL` until index 3 is observed. It then owner-pins and posts `WM_COMMAND/LBN_SELCHANGE`, and polls complete `ValidateShellState` until the exact Cube selection and full Cube Inspector fixture synchronize inside the same phase deadline. Message waits and poll sleeps are capped by the phase and existing 135-second work budgets.

This uses `PostMessageW` rather than a synchronous send while the target thread is suspended, so the harness performs no target-dependent wait in the pinned interval. The prior resize/show-state/final-close owner pins, resize/show-state deadlines, exact restore rectangle, shell/containment assertions, process-tree containment and cleanup remain unchanged.

## Primary-source basis, rechecked 2026-09-23

- Microsoft `PostMessageW`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-postmessagew . It posts a message to the queue associated with the thread that created the specified window and returns without waiting. Both messages used here are system messages below `WM_USER`; `LB_SETCURSEL` has no `lParam` payload.
- Microsoft `LB_SETCURSEL`: https://learn.microsoft.com/en-us/windows/win32/controls/lb-setcursel . It selects the zero-based row in a single-selection list box; `lParam` is unused and `LB_ERR` is the documented error result.
- Microsoft `GetThreadContext`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadcontext . A valid context cannot be obtained for a running thread, so successful `CONTEXT_CONTROL` capture after suspension is used as the execution barrier before final identity validation and enqueue.
- Microsoft `SuspendThread`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-suspendthread . It stops user-mode execution and returns the previous suspend count. Microsoft warns it is debugger-oriented and not general synchronization, so the harness confines it to a short verification-only enqueue interval and performs no wait on target-owned work while suspended.
- Microsoft `ResumeThread`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-resumethread . The harness requires the prior count to be one when removing its own suspension.
- Epic Unreal Engine 5.8, Using Editor Viewports: https://dev.epicgames.com/documentation/unreal-engine/using-editor-viewports-in-unreal-engine . Perspective 3D, orthographic 2D, multiple viewport layouts, maximized viewports and immersive editing remain editor comparison workflows.
- Unity 6.0, `EditorWindow.maximized`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/EditorWindow-maximized.html . Maximized editor-window state remains a comparison workflow property.

These are public behavioral/API references only. No proprietary Epic/Unity source was copied and no dependency was imported.

## Portable source-logic evidence

Disposable fixture: `/mnt/data/e11_selection_owner_pin_fixture.cpp` during this coordinator run only.

SHA-256: `0fac685ae3ab5d6f33b37be53f06167e007db7c654414d2460191fb3e66cc374`.

Executed successfully:

```text
g++ (Debian 14.2.0-19) 14.2.0
g++ -std=c++17 -Wall -Wextra -Werror -pedantic e11_selection_owner_pin_fixture.cpp
result: e11 selection owner pin fixture: PASS

clang version 17.0.0
clang++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer e11_selection_owner_pin_fixture.cpp
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
result: e11 selection owner pin fixture: PASS
```

The fixture rejects recycled editor ownership, recycled Outliner ownership, wrong parent/identity, dead process/thread, pre-existing suspension, missing context barrier, enqueue failure, unexpected resume count, and late/unsynchronized selection or Inspector state. This is source-logic evidence only, not native Win32 GUI evidence. No sandbox production Win32 execution is claimed.

## Hosted source verification and PR integration provenance

Source head `57a7867aa4273def5d1dfbb73766883945881c7c` is associated with all three green hosted workflows:

- Windows build and deterministic tests: run `35933677666`, job `107425789093`, `success`, completed `2026-09-23T23:29:35Z`;
- profiling capture portability: run `35933677505`, `success`;
- release manifest integrity: run `35933677460`, `success`.

These are `pull_request` workflows. Default checkout therefore exercised GitHub synthetic integration commit `ec0dee8274e6b3fd1ffeb4fe5480c88590c96fbf`, whose fetched commit has exactly two parents: tested base `6d22da88402db71843ecd5a35766c0c77e62dca6` and source head `57a7867aa4273def5d1dfbb73766883945881c7c`. Source association and tested merge identity are recorded separately and must not be conflated.

The Windows job passed repository/R0 safety contracts, Visual Studio 2022 x64 configuration, Debug build and deterministic tests, Release build and deterministic tests, runtime dependency/prerequisite checks, static milestone verifiers, and clean tracked-tree verification. Hosted CTest still intentionally excludes interactive `EditorRuntimeSmoke`.

## Retained acceptance gates

`native_evidence` remains empty. E11 final acceptance remains false. Independent review of an earlier source/evidence SHA does not accept this new source. A fresh independent review is required on the exact post-evidence head after this task, QA record and capability map are synchronized.

After that exact head is clean-reviewed, the registered Windows executor must use one owned interactive desktop and run Debug and Release `EditorContainmentTests` plus interactive `EditorRuntimeSmoke`, retaining source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, screenshots and zero-owned-process cleanup proof.

The native matrix remains: untouched startup, 800x600, 1280x720, 1440x900, actual maximized desktop followed by exact restore to the pre-maximize outer screen rectangle, and 420x260 or closest OS-permitted narrow state. Evidence must additionally show the Cube selection and `LBN_SELCHANGE` notification target only the exact pinned launched editor/Outliner owner, that the owner thread is resumed before polling, and that Cube/Inspector synchronization settles within the three-second selection budget. Separately launch `AstralGame` from the same source/build as a no-regression check.

Clean-machine packaging, comparative frame/RAM/VRAM measurement, broader stress/recovery, all remaining E00-E17 capability gaps, and the required 24-hour soak remain unresolved.

## Rollback and stop conditions

Rollback only this verification repair if native evidence proves the owner-pin contract invalid under the existing owned-desktop requirements. Never weaken the acceptance test to obtain green results. Stop before production-runtime changes, dependency/API changes, unrelated workflow changes, rebase/merge, R0 execution, scheduler operations, release/deployment, or game-content work.

## Single next useful action

Synchronize the QA receipt and capability map to source `57a7867aa4273def5d1dfbb73766883945881c7c` and tested integration merge `ec0dee8274e6b3fd1ffeb4fe5480c88590c96fbf`, then obtain fresh independent review of the exact post-evidence head. If that review is clean, hand that exact reviewed SHA to the registered Windows executor for the retained native Debug/Release matrix and separate `AstralGame` launch. Do not start another dependent editor feature while this QA gate is unresolved.
