# E11 Editor Runtime Smoke evidence, 2026-09-23

## Current checkpoint

Branch: `engine/2026-09-22-editor-runtime-smoke`.

Current implementation source: `9b3aa8ebe320649b9319ba653838db53dde38dde`.

`Tests/EditorRuntimeSmoke.cpp` blob: `97d374db438a1f8b4f0d161e9de6f823da0c792d`.

`CMakeLists.txt` remains unchanged for this continuation at blob `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.

Latest observed `main` at the source CI integration point: `6d22da88402db71843ecd5a35766c0c77e62dca6`.

Issue #7 remains open. Historical R0 was not invoked. `native_evidence` remains empty. E11 final acceptance and UE5/Unity parity remain false.

## Finding and repair chain

Fresh independent Codex review of exact head `3f093a98f5af4257ab180dcc15183aa47073a7df` completed at `2026-09-23T22:33:42Z` and produced two P2 findings.

First, the immediately adjacent ownership check added in `95177d199bedb6195c0d89aa0d05708af04f6512` still did not pin ownership through the side-effecting `SetWindowPos(..., SWP_ASYNCWINDOWPOS)` call. The editor could exit after the helper returned, Windows could recycle the HWND, and a different visible/enabled window could receive the resize.

Second, the task and QA records were stale relative to the actual source/evidence chain. This file and the task record have been rewritten as current authoritative receipts rather than retaining a misleading handoff SHA.

Repair commit `9b3aa8ebe320649b9319ba653838db53dde38dde` changes only `Tests/EditorRuntimeSmoke.cpp`, 64 additions and 14 deletions. It adds `PostResizeWhileOwnerPinned(...)` and routes every asynchronous resize through the retained `CreateProcessW` process/primary-thread handles and ID.

The helper requires exact PID/TID ownership and live process/thread handles, suspends only the exact window-owning primary thread, requires zero prior suspend count, captures `CONTEXT_CONTROL` as a post-suspend barrier, revalidates process/thread liveness plus PID/TID ownership and visible/enabled state while the creator thread cannot exit/recycle its HWND, issues only the asynchronous resize post, and resumes immediately before any completion wait or poll. Unexpected suspension state, context failure, identity/liveness change, post failure, or bad resume count fails closed.

The existing 1.5-second resize deadline, nested message-budget capping, exact maximize/restore rectangle, semantic shell, Cube selection/Inspector synchronization, containment, cleanup, and hardened final close remain unchanged in meaning.

No production editor/game source, dependency, graphics API, architecture, scheduler configuration, release/deploy state, or game content changed.

## Primary-source research

Accessed/rechecked 2026-09-23:

- Microsoft Learn, `SetWindowPos`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowpos . `SWP_ASYNCWINDOWPOS` can post the request to the owning thread instead of blocking the caller.
- Microsoft Learn, `GetWindowThreadProcessId`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowthreadprocessid . It supplies the creating thread and optional process identity used to reject stale/recycled HWNDs.
- Microsoft Learn, `SuspendThread`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-suspendthread . It suspends user-mode execution and returns the prior suspend count. Because Microsoft warns against using it for general synchronization, Astral confines it to the short verification-harness owner-pin interval and performs no target-dependent wait while suspended.
- Microsoft Learn, `GetThreadContext`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadcontext . Valid thread context cannot be obtained for a running thread, so successful context capture after suspension acts as the execution barrier before final identity validation and the asynchronous post.
- Microsoft Learn, `ResumeThread`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-resumethread . The previous suspend count verifies the single harness-owned suspension is removed before polling.
- Epic Unreal Engine 5.8, Using Editor Viewports: https://dev.epicgames.com/documentation/unreal-engine/using-editor-viewports-in-unreal-engine . Perspective 3D, orthographic 2D, multi-layout, maximized, and immersive authoring remain editor comparison workflows.
- Unity 6.0, `EditorWindow.maximized`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/EditorWindow-maximized.html . Maximized editor state remains a comparison workflow property.

Behavioral/API comparison only. No proprietary Epic/Unity source copied and no dependency imported.

## Portable regression evidence

Disposable C++17 owner-pin state-machine fixture SHA-256: `a092dedb8ba74fb0df0e967d0d2465f7229532b6a73edabb0b8a3e1e83fa5432`.

It covers the accepted path plus recycled owner after the barrier, process death, missing context barrier, pre-existing suspension, resize-post failure, and unexpected resume count. It passed warning-clean GCC 14.2 C++17 and Clang 17 C++17 with ASan+UBSan and leak detection. Both executions printed `e11 resize owner pin fixture: PASS`.

This fixture proves source-logic properties only. It is not native Win32 GUI evidence.

## Hosted verification and PR integration provenance

Source `9b3aa8ebe320649b9319ba653838db53dde38dde` has all source-associated hosted checks green:

- Windows build and deterministic tests: run `35929455430`, job `107412305793`, `success`;
- profiling capture portability: run `35929455501`, `success`;
- release manifest integrity: run `35929455423`, `success`.

The Windows job passed repository/R0 safety contracts, Visual Studio 2022 x64 configuration, Debug build/tests, Release build/tests, PE/runtime/prerequisite checks, static milestone verifiers, and clean tracked-tree verification.

GitHub's synthetic PR integration commit for this source is `90175bb0edf372473056374a0f30cacb684defbe`, with parents base `6d22da88402db71843ecd5a35766c0c77e62dca6` and source `9b3aa8ebe320649b9319ba653838db53dde38dde`.

Hosted CTest intentionally excludes interactive `EditorRuntimeSmoke`. Hosted success therefore does not establish actual desktop resize/maximize behavior, GPU behavior, screenshots, clean-machine packaging, measured performance, stress/recovery, or soak acceptance.

## Review state

The prior P2 source thread was answered with exact implementation and fixture evidence. The task/QA stale-record finding is addressed by the current authoritative task and QA receipts.

Fresh independent review is still required on the exact final post-evidence head after the task, QA, and capability map are synchronized. Code review by the same author is not independent acceptance.

## Native acceptance matrix

After exact-head hosted checks and fresh independent review are clean, the registered Windows executor should use one owned interactive desktop and run both Debug and Release:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact reviewed source SHA, machine and Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, and proof of zero owned contained processes after failure/interruption. Capture untouched startup, 800x600, 1280x720, 1440x900, actual maximized desktop, exact restore to the pre-maximize outer rectangle, and 420x260 or closest OS-permitted narrow state.

The receipt must demonstrate that each resize side effect is issued only while the exact launched window-owner thread is pinned, every resize plus shell/containment acceptance stays within the 1.5-second phase deadline, semantic control identity and positive-area containment survive all states, Cube selection and Inspector stay synchronized, pending tools stay disabled, and final shutdown is safe. Separately launch `AstralGame` from the same source/build as a no-regression check.

## Remaining gaps

The broader capability catalogue remains unresolved: runtime/jobs/memory, scene ownership/serialization, full asset pipeline, GPU rendering/materials, lighting/shadows/reflections, large-world streaming/detail, animation, physics/collision, AI/navigation, audio, genuine 2D, networking, profiling/budgets, packaging/platforms, terrain/foliage, particles/VFX, cinematics, scripting/reflection, input/replay, accessibility/localization, plugins/extensions, additional platforms, comparative performance/memory/reliability, clean-machine packaging, stress/recovery, and the required 24-hour soak.

`native_evidence`: empty.

E11 final acceptance: false.

UE5/Unity parity claim: false.

## Single next action

Synchronize the capability map to this receipt, obtain clean hosted checks and a fresh independent review on the exact final evidence head, then hand that exact reviewed SHA to the registered Windows executor for the retained native matrix and separate `AstralGame` launch. Do not admit another dependent editor feature before this QA gate resolves.
