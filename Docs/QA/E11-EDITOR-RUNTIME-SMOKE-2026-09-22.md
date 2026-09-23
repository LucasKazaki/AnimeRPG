# E11 Editor Runtime Smoke evidence, 2026-09-23

## Current checkpoint

Branch: `engine/2026-09-22-editor-runtime-smoke`.

Current implementation source: `f097819e2904a5a24e45f98e8bd4813f1af2c58e`.

Parent before this repair: `bb2d903c022c996ab69f692a673e0759efc684f9`.

`Tests/EditorRuntimeSmoke.cpp` blob: `8b0d633e0c4f34fcf8ccd836468b06e46e8d81be`.

`CMakeLists.txt` remains unchanged at blob `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.

Latest observed `main`: `6d22da88402db71843ecd5a35766c0c77e62dca6`.

Issue #7 remains open. Historical R0 was not invoked. `native_evidence` remains empty. E11 final acceptance and UE5/Unity parity remain false.

## Finding and repair

Current independent-review thread `PRRT_kwDOTo2Ig86lXSlD`, comment `4087994063`, identified a P2 stale/recycled-HWND side-effect race in the native maximize/restore verification. The prior code validated PID ownership, visibility and enabled state, then returned to the caller before the side-effecting `ShowWindowAsync(SW_MAXIMIZE)` or `ShowWindowAsync(SW_RESTORE)`. The launched editor could exit in that interval and Windows could reuse the HWND for an unrelated window.

Repair commit `f097819e2904a5a24e45f98e8bd4813f1af2c58e` changes only `Tests/EditorRuntimeSmoke.cpp`, with 62 additions and 21 deletions relative to `bb2d903c022c996ab69f692a673e0759efc684f9`.

The new `PostShowStateWhileOwnerPinned(...)` helper uses the retained `CreateProcessW` process and primary-thread handles and ID. It requires exact PID/TID ownership and live process/thread handles, suspends only the exact window-owning primary thread with zero prior suspend count, captures `CONTEXT_CONTROL` as the post-suspend execution barrier, rechecks liveness plus PID/TID/visible/enabled state while that creator thread cannot run, issues only the asynchronous `ShowWindowAsync` request, and immediately resumes the owner before any show-state poll or wait. It fails closed on liveness/identity change, pre-existing suspension, context failure, enqueue failure, or unexpected resume state.

Both `SW_MAXIMIZE` and `SW_RESTORE` now use this owner-pin helper. `MaximizeRestoreAndCheck` receives the retained thread ID/handle and process handle. Existing show-state deadlines, nested bounded message waits, exact restore to the pre-maximize outer rectangle, the prior resize owner pin and 1.5-second deadline, semantic shell checks, child containment, Cube selection/Inspector synchronization, cleanup and final owner-pinned close are preserved.

No production editor/game source, CMake registration, dependency, graphics API, architecture, scheduler configuration, release/deploy state, or game content changed.

## Primary-source research

Accessed/rechecked 2026-09-23:

- Microsoft Learn, `ShowWindowAsync`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindowasync . It changes the show state without waiting and posts a show-window event to the given window's message queue.
- Microsoft Learn, `GetWindowThreadProcessId`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowthreadprocessid . It identifies the thread that created the HWND and optionally its process; invalid HWNDs return zero.
- Microsoft Learn, `SuspendThread`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-suspendthread . It increments the suspend count and stops user-mode execution. Microsoft warns it is primarily for debuggers and should not be used as general synchronization. The harness therefore keeps the interval short and performs no target-dependent wait while suspended.
- Microsoft Learn, `GetThreadContext`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadcontext . Microsoft states a valid context cannot be obtained for a running thread; successful capture after suspension is the execution barrier used before final identity validation.
- Microsoft Learn, `ResumeThread`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-resumethread . A successful return of one means the suspended thread was restarted.
- Epic Unreal Engine 5.8, Using Editor Viewports: https://dev.epicgames.com/documentation/unreal-engine/using-editor-viewports-in-unreal-engine . Perspective 3D, orthographic 2D, multi-viewport, maximized and immersive workflows remain editor comparison targets.
- Epic Unreal Engine 5.8, `FMaximizeViewportCommand`: https://dev.epicgames.com/documentation/unreal-engine/API/Editor/LevelEditor/FLevelViewportLayout/FMaximizeViewportCommand . Maximize/immersive requests can be queued until the first tick when the parent window exists.
- Unity 6.0, `EditorWindow.maximized`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/EditorWindow-maximized.html . Maximized docked editor-window state remains a comparison workflow property.

Behavioral/API comparison only. No proprietary Epic/Unity source was copied and no dependency was imported.

## Portable regression evidence

Disposable source-logic fixture SHA-256: `6110abf51993b7c2666bc95ef3089989fa448f84b0933c14b67984633c4f1ce1`.

It accepts only the intended owner-pinned enqueue path and rejects owner recycling after the barrier, process death, thread death, missing context barrier, pre-existing suspension, hidden/disabled state after the pin, enqueue failure, and invalid resume count.

Executed during this pass:

```text
g++ (Debian 14.2.0-19) 14.2.0
g++ -std=c++17 -Wall -Wextra -Werror -pedantic e11_show_state_owner_pin_fixture.cpp
=> e11 show-state owner pin fixture: PASS

clang version 17.0.0
clang++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer e11_show_state_owner_pin_fixture.cpp
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
=> e11 show-state owner pin fixture: PASS
```

No `x86_64-w64-mingw32-g++` was available in the sandbox. The fixture is source-logic evidence only and is not a compile or execution of the production Win32 smoke.

## Hosted verification

Exact source `f097819e2904a5a24e45f98e8bd4813f1af2c58e` is green in all currently associated hosted workflows:

- Windows build and deterministic tests: run `35931194787`, job `107417915549`, `success`, started `2026-09-23T22:58:31Z`, completed `2026-09-23T23:00:47Z`;
- profiling capture portability: run `35931194699`, `success`;
- release manifest integrity: run `35931194743`, `success`.

The Windows job passed repository/R0 safety contracts, Visual Studio 2022 x64 configuration, Debug build, deterministic Debug tests, Release build, deterministic Release tests, PE/runtime/prerequisite checks, static milestone verifiers, and clean tracked-tree verification.

Hosted CTest intentionally excludes interactive `EditorRuntimeSmoke`, so these hosted passes do not establish real desktop maximize/restore behavior, GPU behavior, screenshots, clean-machine packaging, comparative performance, wider stress/recovery, or soak acceptance.

## Review state

The implementation in this pass was authored by the coordinator and therefore does not count as independent review. The current P2 thread must be answered with the exact repair and evidence, then a fresh independent review must run on the exact post-evidence head after this QA record, the task and the capability map are synchronized.

Do not reuse a clean review of an earlier SHA as acceptance for the final receipt.

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

Retain exact reviewed source SHA, machine and Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps and zero-owned-process proof after failure/interruption. Capture screenshots for untouched startup, 800x600, 1280x720, 1440x900, actual maximized desktop, exact restore to the pre-maximize outer rectangle, and 420x260 or closest OS-permitted narrow state.

The native receipt must demonstrate that resize, maximize and restore side effects target only the exact launched editor owner while its creating thread is pinned; the owner is resumed before any completion polling; resize and show-state phase budgets are respected; semantic control identities, positive-area containment, Cube selection/Inspector synchronization and disabled pending tools survive every state; and final close/cleanup leaves no owned editor process. Separately launch `AstralGame` from the same source/build as a no-regression check.

## Remaining gaps

The broader catalogue remains unresolved: runtime/jobs/memory, scene ownership/serialization, complete asset pipeline/resources, GPU rendering/materials, lighting/shadows/reflections, large-world streaming/detail, animation, physics/collision, AI/navigation, audio, genuine 2D, networking, profiling/budgets, packaging/platforms, terrain/foliage, particles/VFX, cinematics/sequencing, scripting/reflection, input/replay, accessibility, localization, plugins/extensions, additional platforms, comparative performance/memory/reliability, clean-machine packaging, stress/recovery and the required 24-hour soak.

`native_evidence`: empty.

E11 final acceptance: false.

UE5/Unity parity claim: false.

## Single next action

Synchronize `Docs/Research/ENGINE-CAPABILITIES.json` to this exact source/evidence chain, verify final evidence-head hosted checks, obtain fresh independent review of that exact head, then hand the clean-reviewed SHA to the registered Windows executor for the retained native matrix and separate `AstralGame` launch. Do not admit another dependent editor feature before this QA gate resolves.
