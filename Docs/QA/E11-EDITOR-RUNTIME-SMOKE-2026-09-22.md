# E11 Editor Runtime Smoke evidence, 2026-09-23

## Current checkpoint

Branch: `engine/2026-09-22-editor-runtime-smoke`.

Current implementation source: `57a7867aa4273def5d1dfbb73766883945881c7c`.

Parent before this repair: `a87ff45c236deb4fa5f9a734192e8dfad1beade4`.

`Tests/EditorRuntimeSmoke.cpp` blob: `290b4931b3a4083555f0a5ddcae4ee14cbaebe6a`.

`CMakeLists.txt` remains unchanged at blob `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.

Latest observed `main`: `6d22da88402db71843ecd5a35766c0c77e62dca6`.

Issue #7 remains open. Historical R0 was not invoked. `native_evidence` remains empty. E11 final acceptance and UE5/Unity parity remain false.

## Finding and repair

Independent-review thread `PRRT_kwDOTo2Ig86lX2XO`, comment `4088223052`, identified a P2 stale/recycled-HWND side-effect race in `SelectCubeAndNotify`. Before this repair, the smoke validated its cached Outliner HWND and synchronously sent `LB_SETCURSEL`, then later revalidated the editor/Outliner and synchronously sent `WM_COMMAND/LBN_SELCHANGE`. The launched editor could exit after either validation and Windows could recycle the cached handle before the side effect, allowing an unrelated same-integrity window to be mutated before later validation noticed the failure.

Repair commit `57a7867aa4273def5d1dfbb73766883945881c7c` changes only `Tests/EditorRuntimeSmoke.cpp`, with 133 additions and 24 deletions relative to parent `a87ff45c236deb4fa5f9a734192e8dfad1beade4`.

The new `PostSelectionMutationWhileOwnerPinned(...)` helper uses the retained `CreateProcessW` process and primary-thread handles and ID. It requires exact PID/TID ownership for both the top-level editor and bound Outliner, validates the Outliner parent/class/control ID/visibility/enabled state and `LBS_NOTIFY`, suspends only the exact owner thread with zero prior suspend count, captures `CONTEXT_CONTROL` as the post-suspend execution barrier, rechecks liveness and exact editor/Outliner identity while the creator thread cannot execute user-mode code, posts only the asynchronous selection mutation or `WM_COMMAND/LBN_SELCHANGE`, and immediately resumes the owner before any poll or wait. It fails closed on identity/liveness change, pre-existing suspension, context failure, enqueue failure, or unexpected resume state.

`SelectCubeAndNotify` now receives the retained thread ID/handle and process handle and owns a three-second selection phase budget. It posts `LB_SETCURSEL` while the owner is pinned, resumes, then polls bounded `LB_GETCURSEL` until index 3 is actually observed. It then pins again for the asynchronous `WM_COMMAND/LBN_SELCHANGE`, resumes, and polls complete shell validation until exact Cube selection and the full Cube Inspector fixture are synchronized before the same deadline. The existing global work budget and bounded cross-process reads remain in force.

No production editor/game source, CMake registration, dependency, graphics API, architecture, scheduler configuration, release/deploy state, or game content changed.

## Primary-source research

Accessed/rechecked 2026-09-23:

- Microsoft Learn, `PostMessageW`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-postmessagew . It posts to the message queue associated with the thread that created the specified window and returns without waiting. The two posted messages in this repair are system-defined messages below `WM_USER`.
- Microsoft Learn, `LB_SETCURSEL`: https://learn.microsoft.com/en-us/windows/win32/controls/lb-setcursel . It selects a zero-based row in a single-selection list box; `lParam` is unused and `LB_ERR` is the documented error result.
- Microsoft Learn, `GetThreadContext`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadcontext . A valid context cannot be obtained for a running thread, supporting the post-suspend execution barrier.
- Microsoft Learn, `SuspendThread`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-suspendthread . It stops user-mode execution and returns the prior suspend count. Microsoft warns it is debugger-oriented and not general synchronization; this harness therefore confines it to a short verification-only enqueue interval and performs no target-dependent wait while suspended.
- Microsoft Learn, `ResumeThread`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-resumethread . The harness requires the prior count to be one when removing its own suspension.
- Epic Unreal Engine 5.8, Using Editor Viewports: https://dev.epicgames.com/documentation/unreal-engine/using-editor-viewports-in-unreal-engine . Perspective 3D, orthographic 2D, multi-viewport, maximized and immersive workflows remain editor comparison targets.
- Unity 6.0, `EditorWindow.maximized`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/EditorWindow-maximized.html . Maximized editor-window state remains a comparison workflow property.

Behavioral/API comparison only. No proprietary Epic/Unity source was copied and no dependency was imported.

## Portable regression evidence

Disposable source-logic fixture SHA-256: `0fac685ae3ab5d6f33b37be53f06167e007db7c654414d2460191fb3e66cc374`.

It accepts only the intended owner-pinned enqueue path and rejects recycled editor ownership, recycled Outliner ownership, wrong parent/identity, dead process/thread, pre-existing suspension, missing context barrier, enqueue failure, bad resume state, and late/unsynchronized selection or Inspector state.

Executed during this pass:

```text
g++ (Debian 14.2.0-19) 14.2.0
g++ -std=c++17 -Wall -Wextra -Werror -pedantic e11_selection_owner_pin_fixture.cpp
=> e11 selection owner pin fixture: PASS

clang version 17.0.0
clang++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer e11_selection_owner_pin_fixture.cpp
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
=> e11 selection owner pin fixture: PASS
```

This is source-logic evidence only and is not a compile or execution of the production Win32 smoke.

## Hosted verification and exact PR integration identity

The source head associated with this run is `57a7867aa4273def5d1dfbb73766883945881c7c`.

Because these workflows use the `pull_request` event and default checkout, the tested integration tree is GitHub synthetic merge `ec0dee8274e6b3fd1ffeb4fe5480c88590c96fbf`, not the raw source commit. The fetched merge object has exactly two parents:

- tested base: `6d22da88402db71843ecd5a35766c0c77e62dca6`;
- associated source head: `57a7867aa4273def5d1dfbb73766883945881c7c`.

Hosted results for that source/integration pair:

- Windows build and deterministic tests: run `35933677666`, job `107425789093`, `success`, started `2026-09-23T23:27:18Z`, completed `2026-09-23T23:29:35Z`;
- profiling capture portability: run `35933677505`, `success`;
- release manifest integrity: run `35933677460`, `success`.

The Windows job passed repository/R0 safety contracts, Visual Studio 2022 x64 configuration, Debug build, deterministic Debug tests, Release build, deterministic Release tests, runtime dependency/prerequisite checks, static milestone verifiers, and clean tracked-tree verification.

Hosted CTest intentionally excludes interactive `EditorRuntimeSmoke`, so these hosted passes do not establish real desktop selection behavior, resize/maximize/restore behavior, GPU behavior, screenshots, clean-machine packaging, comparative performance, wider stress/recovery, or soak acceptance.

## Review state

The implementation in this pass was authored by the coordinator and therefore is not independent review. The current source P2 is repaired, but exact-final-head acceptance remains pending until the task, this QA receipt and the capability map are synchronized and a fresh independent review of that exact post-evidence head is clean.

A second current review finding, thread `PRRT_kwDOTo2Ig86lX2XS`, requires durable hosted evidence to keep the raw source association separate from the actual tested synthetic merge/base. This receipt does so with `ec0dee8...` / `6d22da8...` / `57a7867...`.

A third current finding, thread `PRRT_kwDOTo2Ig86lX2XW`, noted that the task/QA next-action text still requested evidence synchronization already completed for the prior source. The records are advanced here to the genuinely pending final evidence-head review and native handoff.

## Native acceptance matrix

After exact-final-head hosted checks and fresh independent review are clean, the registered Windows executor should use one owned interactive desktop and run both Debug and Release:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact reviewed source SHA, machine and Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps and zero-owned-process proof after success, failure or interruption. Capture screenshots for untouched startup, 800x600, 1280x720, 1440x900, actual maximized desktop, exact restore to the pre-maximize outer rectangle, and 420x260 or closest OS-permitted narrow state.

The native receipt must demonstrate that selection, notification, resize, maximize and restore side effects target only the exact launched owner while its creating thread is pinned; the owner is resumed before any completion polling; selection, resize and show-state phase budgets are respected; semantic control identities, positive-area containment, Cube selection/Inspector synchronization and disabled pending tools survive every state; and final close/cleanup leaves no owned editor process. Separately launch `AstralGame` from the same source/build as a no-regression check.

## Remaining gaps

The broader catalogue remains unresolved: runtime/jobs/memory, scene ownership/serialization, complete asset pipeline/resources, GPU rendering/materials, lighting/shadows/reflections, large-world streaming/detail, animation, physics/collision, AI/navigation, audio, genuine 2D, networking, profiling/budgets, packaging/platforms, terrain/foliage, particles/VFX, cinematics/sequencing, scripting/reflection, input/replay, accessibility, localization, plugins/extensions, additional platforms, comparative performance/memory/reliability, clean-machine packaging, stress/recovery and the required 24-hour soak.

`native_evidence`: empty.

E11 final acceptance: false.

UE5/Unity parity claim: false.

## Single next action

Update the capability map for source `57a7867aa4273def5d1dfbb73766883945881c7c`, tested synthetic merge `ec0dee8274e6b3fd1ffeb4fe5480c88590c96fbf`, tested base `6d22da88402db71843ecd5a35766c0c77e62dca6`, this selection-owner-pin repair and the retained empty native evidence. Then obtain fresh independent review of the exact post-evidence head. If clean, hand that exact SHA to the registered Windows executor for the retained native matrix and separate `AstralGame` launch. Do not admit another dependent editor feature before this QA gate resolves.
