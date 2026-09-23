# E11 Editor Runtime Smoke evidence, 2026-09-23

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. Fresh independent review of receipt head `c86387c73c3311a6a2ba045de13b35ee8b683cf0` found one remaining shutdown-side-effect race: the smoke could complete its final PID/process-liveness ownership check, then the launched editor could exit before `PostMessageW(WM_CLOSE)`. If Windows reused the PID and HWND value in that interval, the old final close could target an unrelated window while the original process had already exited cleanly.

Current shutdown-ownership implementation candidate: `89a0eb0ac0d56cef9a76b83876aa4ecab31c0a6f`.
`Tests/EditorRuntimeSmoke.cpp` blob: `8937ef0de9560a3a3c5fce104ad4a255c4bf783a`.
`CMakeLists.txt` remains blob `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.
Production editor source is unchanged.

## Reproduced defect and repair

Before `89a0eb0...`, `CloseEditor` performed `WindowOwnedByProcess(window, processId)` and then separately called `PostMessageW(window, WM_CLOSE, ...)`. The prior `b9eebe721...` liveness checks prevented acceptance after the retained process handle had already signaled, but they did not bind the HWND identity through the later close side effect.

Commit `89a0eb0ac0d56cef9a76b83876aa4ecab31c0a6f` retains the original primary-thread handle and thread ID already returned by `CreateProcessW`, verifies that the final top-level HWND belongs to that exact launched PID and exact launched thread, suspends only that verified owner thread, rechecks process/thread liveness and PID/TID ownership while it cannot execute user-mode code, enqueues only the asynchronous `WM_CLOSE`, and immediately resumes before any wait. A nonzero prior suspend count, failed enqueue, unexpected resume count, dead process/thread, or PID/TID mismatch fails closed and falls back to the existing owned-process/job cleanup path.

No target window procedure is invoked synchronously and no resource wait or production-engine operation occurs while the editor UI thread is suspended. Microsoft explicitly warns that `SuspendThread` is debugger-oriented and unsuitable as general synchronization because waiting on resources held by a suspended thread can deadlock. This use is therefore restricted to the verification harness and the minimal final asynchronous enqueue interval.

GitHub commit metadata records 38 additions and 5 deletions, all in `Tests/EditorRuntimeSmoke.cpp`. No production source, CMake, workflow, dependency, graphics API, game content, scheduler configuration, release state, or architecture changed.

Historical receipt correction: GitHub metadata for prior commit `b9eebe721b7907245caaad5771bc9e96bac20680` records 13 additions and 6 deletions, not 15 additions and 6 deletions. Its retained process-handle liveness behavior and previous hosted results are unchanged.

## Primary research

Accessed 2026-09-23 UTC:

- Microsoft Learn, `SuspendThread`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-suspendthread
  - successful suspension stops the specified thread's user-mode execution and returns its previous suspend count; Microsoft warns against general synchronization use.
- Microsoft Learn, `ResumeThread`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-resumethread
  - decrements the suspend count; return value one means the one suspension was removed and execution is resumed.
- Microsoft Learn, `PostMessageW`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-postmessagew
  - posts to the message queue associated with the thread that created the target window and returns without waiting for processing.
- Microsoft Learn, `DestroyWindow`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-destroywindow
  - a thread cannot use `DestroyWindow` to destroy a window created by another thread.
- Microsoft Learn, `GetWindowThreadProcessId`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowthreadprocessid
  - identifies both the creating thread and owning process for an HWND.
- Microsoft Learn, `WaitForSingleObject`: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
  - retained process/thread handles are waitable and permit zero-time liveness checks.

Public Win32 API semantics only. No proprietary UE/Unity source was copied and no dependency was imported.

## Portable regression evidence

Disposable C++17 shutdown-pin state-machine fixture SHA-256: `b13727b0264e2c54f0303939c1effca5c0d56eca4ea6922d1616ff29ef1e9127`.

```text
g++ -std=c++17 -Wall -Wextra -Werror /tmp/e11_shutdown_pin_fixture.cpp -o /tmp/e11_gcc
/tmp/e11_gcc
# exit 0

clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /tmp/e11_shutdown_pin_fixture.cpp -o /tmp/e11_clang
ASAN_OPTIONS=detect_leaks=1 /tmp/e11_clang
# exit 0
```

Covered cases: valid live PID/TID ownership with one suspend/resume cycle accepted; dead process rejected; dead thread rejected; PID mismatch rejected; TID mismatch rejected; pre-existing suspend count rejected; enqueue failure rejected; unexpected resume count rejected. This is source-logic evidence only, not Win32 runtime evidence.

Sandbox repository checkout was attempted but failed because `github.com` could not be resolved. No usable Windows SDK/interactive desktop was available, so no sandbox production Win32 compile or GUI execution is claimed.

## Hosted verification state

Source implementation candidate: `89a0eb0ac0d56cef9a76b83876aa4ecab31c0a6f`.
Synthetic PR integration commit: `76b9edbfb8351d4223f042efe5a57a98746ba17d`.
Tested base parent of that synthetic merge: `3c3babd46c4539d474b7ea78ab01d5ed71dde7ac`.
Source parent of that synthetic merge: `89a0eb0ac0d56cef9a76b83876aa4ecab31c0a6f`.

- Windows build and deterministic tests: run `35859925271`, job `107177344930`, PASS, completed 2026-09-23T12:23:39Z. Every reported job step passed, including R0/repository safety contracts, Release assertion/CTest safety, VS2022 x64 configure, Debug build and deterministic tests, Release build and deterministic tests, prerequisite/runtime checks, static milestone verifiers, and clean tracked-tree verification.
- profiling capture portability: run `35859925269`, PASS.
- release manifest integrity: run `35859925251`, PASS.

These hosted checks establish compile and deterministic-suite compatibility for the PR integration tree. They do not execute the interactive GUI smoke and are not native workstation acceptance.

## Retained hardening

- `EditorContainmentTests` remains selected by hosted deterministic suites while interactive `EditorRuntimeSmoke` remains excluded;
- deterministic containment executes real worker-local `CleanupProcess` and verifies direct-process plus supervisor whole-job cleanup to zero active processes;
- normal-success containment rejects a zero-exit worker that leaves a descendant;
- every cross-process shell read remains bounded and revalidates expected ownership/identity;
- original shell HWND/class identity, semantic Static/Button bindings, Outliner rows and selection, complete Inspector fixtures, `LBS_NOTIFY`, stable single top-level-window identity, owned cleanup, startup containment, positive width/height, normal resize, and narrow resize checks remain required;
- accepted HWND ownership requires retained launched-process liveness around the PID lookup;
- final close additionally requires exact original PID/TID ownership and pins that verified creating thread only across the asynchronous close enqueue, then resumes before waiting.

## Deferred native acceptance

`native_evidence` remains empty. After the final receipt tree has a fresh clean independent review, the registered Windows executor must still run `EditorContainmentTests` and interactive `EditorRuntimeSmoke` in Debug and Release on one owned desktop. Retain:

- exact reviewed source/receipt SHA;
- Windows/machine identity;
- MSVC and CMake versions;
- GPU and driver identity;
- exact commands, complete stdout/stderr, exit codes, UTC timestamps;
- default-startup, normal-size, and 420x260 narrow-window screenshots;
- process inspection proving zero owned contained processes after failure/interruption;
- evidence that the top-level editor HWND belongs to the retained launch thread at final close;
- evidence that the final close path leaves the launch thread resumed and does not act on an unrelated HWND after the launched process/thread terminates.

Issue #7 remains open. The historical R0 runner was not invoked. Clean-machine packaging, comparative frame/RAM/VRAM evidence, broader stress/recovery, and the required 24-hour soak remain unresolved outside this E11 repair.

## Acceptance state

Portable shutdown-pin state-machine fixture under GCC: PASS.
Portable shutdown-pin fixture under Clang ASan+UBSan: PASS.
Implementation candidate `89a0eb0...`: implemented; exact one-file 38-addition/5-deletion diff verified.
Hosted PR-integration checks for implementation candidate: PASS through synthetic merge `76b9edb...` on base `3c3babd...`.
Fresh independent review of the final receipt head: pending.
Native interactive Windows: deferred to registered local executor.
Independent final acceptance: false.
UE5/Unity parity claim: false.

## Single next action

Request fresh independent review of the exact final receipt head containing this repair and corrected evidence. If clean, hand that exact reviewed tree to the registered Windows executor for Debug/Release containment and interactive GUI acceptance.