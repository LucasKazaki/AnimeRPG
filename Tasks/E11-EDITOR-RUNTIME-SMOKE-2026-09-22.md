# E11 Editor Runtime Smoke task, 2026-09-23

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. This packet may harden the native Windows smoke, its recovery supervisor, deterministic containment tests, and evidence. It must not add scene mutation/serialization, gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, rebase, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Admitted baseline from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest observed `main`: `3c3babd46c4539d474b7ea78ab01d5ed71dde7ac`.
Current shutdown-ownership implementation candidate: `89a0eb0ac0d56cef9a76b83876aa4ecab31c0a6f`.
`Tests/EditorRuntimeSmoke.cpp` blob after this repair: `8937ef0de9560a3a3c5fce104ad4a255c4bf783a`.
`CMakeLists.txt` remains blob `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.

Allowed paths only: `CMakeLists.txt`, `Tests/EditorRuntimeSmoke.cpp`, this task, `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`, and `Docs/Research/ENGINE-CAPABILITIES.json`. One active writer only. Do not rebase, merge, force-push, or absorb unrelated work.

## Selected reproducible verification defect

The prior PID-reuse repair in `b9eebe721b7907245caaad5771bc9e96bac20680` correctly requires the retained launched-process handle to remain nonsignaled around `GetWindowThreadProcessId`, but independent review found one remaining time-of-check/time-of-use window in final shutdown. `CloseEditor` performed its last ownership/liveness check and then separately called `PostMessageW(window, WM_CLOSE, ...)`. If the launched editor exited after the check, Windows could recycle both the PID and the numeric HWND value before the post. The smoke could then enqueue `WM_CLOSE` to a window it did not launch and still observe the original process handle as cleanly exited.

The final side effect therefore needs an identity anchor that stays bound across the enqueue, not only a check completed before it.

## Bounded implementation

Commit `89a0eb0ac0d56cef9a76b83876aa4ecab31c0a6f` changes only `Tests/EditorRuntimeSmoke.cpp`:

- `CloseEditor` receives the original `process.dwThreadId` and retained primary-thread HANDLE returned by `CreateProcessW`;
- before shutdown it requires the top-level HWND to belong to the exact launched PID and exact launched thread ID, with both process and thread handles nonsignaled;
- it calls `SuspendThread` only on that exact retained window-owning launch thread and requires the previous suspend count to be zero;
- while the owner thread is suspended, it rechecks process/thread liveness and exact PID/TID ownership, then performs only the asynchronous `PostMessageW(..., WM_CLOSE, ...)` enqueue;
- it immediately calls `ResumeThread` and requires the previous suspend count returned by resume to be exactly one;
- no wait, target window procedure call, synchronous message, lock acquisition, or production-engine work occurs while the target UI thread is suspended;
- only after the UI thread is resumed does the smoke wait for the retained process handle and verify a terminal exit code.

GitHub commit metadata records exactly 38 additions and 5 deletions in `Tests/EditorRuntimeSmoke.cpp`. Production editor source, CMake registration, workflows, dependencies, graphics API, game content, scheduler configuration, release state, and architecture are unchanged.

Historical evidence correction: commit `b9eebe721b7907245caaad5771bc9e96bac20680` is 13 additions and 6 deletions, not 15 additions and 6 deletions. Its behavioral description and prior hosted evidence remain otherwise unchanged.

## Research basis, rechecked 2026-09-23 UTC

- Microsoft Learn, `SuspendThread`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-suspendthread
  - applicability: successful suspension stops user-mode execution for the specified thread and returns its previous suspend count. Microsoft warns this API is primarily debugger-oriented and is not general synchronization because waiting on resources held by a suspended thread can deadlock.
- Microsoft Learn, `ResumeThread`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-resumethread
  - applicability: decrements the suspend count; return value 1 means the thread had one suspension and is restarted by the call.
- Microsoft Learn, `PostMessageW`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-postmessagew
  - applicability: posts to the message queue associated with the thread that created the specified window and returns without waiting for message processing.
- Microsoft Learn, `DestroyWindow`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-destroywindow
  - applicability: a thread cannot use `DestroyWindow` to destroy a window created by another thread. Pinning the exact creating thread across final validation and enqueue closes the owner-thread self-destruction/recreation path addressed by this test repair.
- Microsoft Learn, `GetWindowThreadProcessId`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowthreadprocessid
  - applicability: returns both the creating thread ID and its process ID for an HWND.
- Microsoft Learn, `WaitForSingleObject`: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
  - applicability: retained process and thread handles are waitable; zero timeout distinguishes a nonsignaled live state from termination.

The suspension is deliberately limited to this verification harness and the smallest final enqueue interval. It is not a production synchronization primitive. No proprietary UE/Unity source was copied and no dependency was added.

## Portable verification

Disposable C++17 shutdown-pin state-machine fixture SHA-256: `b13727b0264e2c54f0303939c1effca5c0d56eca4ea6922d1616ff29ef1e9127`.

- `g++ -std=c++17 -Wall -Wextra -Werror`: PASS.
- `clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer`, leak detection enabled: PASS.
- accepted case: matching live process/thread identities, zero prior suspend count, successful enqueue, resume count one;
- rejected cases: dead process, dead thread, PID mismatch, TID mismatch, pre-existing suspend count, enqueue failure, or unexpected resume count.

This fixture checks source-logic state transitions only. It is not Win32 runtime evidence. Sandbox repository access was unavailable because `github.com` did not resolve, and no usable Windows SDK/interactive desktop was available, so no sandbox production Win32 compile or GUI result is claimed.

## Hosted verification state

Implementation source candidate `89a0eb0ac0d56cef9a76b83876aa4ecab31c0a6f` has green pull-request hosted checks:

- synthetic PR merge: `76b9edbfb8351d4223f042efe5a57a98746ba17d`;
- tested base parent: `3c3babd46c4539d474b7ea78ab01d5ed71dde7ac`;
- source parent: `89a0eb0ac0d56cef9a76b83876aa4ecab31c0a6f`;
- Windows build and deterministic tests: run `35859925271`, job `107177344930`, PASS. Reported steps include repository/R0 safety contracts, Release assertion/CTest safety, VS2022 x64 configuration, Debug build/tests, Release build/tests, prerequisite/runtime checks, static milestone verifiers, and clean tracked-tree verification;
- profiling capture portability: run `35859925269`, PASS;
- release manifest integrity: run `35859925251`, PASS.

The workflow run is associated with source head `89a0eb0...`; the PR merge object records the integration tree against base `3c3babd...`. Preserve source, tested integration commit, and tested base separately. Hosted deterministic suites do not replace native interactive `EditorRuntimeSmoke` acceptance.

## Retained E11 hardening and gates

1. deterministic `EditorContainmentTests` is selected by hosted suites while interactive `EditorRuntimeSmoke` remains excluded;
2. containment self-test executes real worker-local `CleanupProcess`, then requires supervisor whole-job cleanup to zero active processes;
3. normal-success containment rejects a zero-exit worker that leaves a descendant;
4. original 12 child HWND/class identity, semantic Static/Button binding, exact Outliner/assets rows, selection/Inspector synchronization, `LBS_NOTIFY`, bounded messages, positive area, startup containment, normal-size containment, narrow-size containment, and stable single top-level identity remain required;
5. HWND ownership requires retained launched-process liveness around PID lookup;
6. final `WM_CLOSE` enqueue additionally pins the exact original window-owning launch thread across repeated PID/TID validation and the enqueue, then resumes the thread before waiting.

`native_evidence` remains empty. Independent acceptance remains false until a fresh independent review of the final receipt head completes and registered native Windows acceptance is retained. Issue #7 remains open, so the historical R0 runner is blocked and must not be invoked.

## Registered native handoff

Only after this changed source/receipt tree has green hosted checks and a fresh clean independent review, the registered Windows executor should use one owned interactive desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact reviewed source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, default-startup plus normal and 420x260 narrow-window screenshots, and process inspection proving zero owned contained processes after any failure or interruption. Native acceptance must confirm the editor top-level HWND is owned by the retained launch thread, final close does not leave that thread suspended, failure paths terminate only the owned process/job tree, and the smoke never accepts or acts on an unrelated HWND after launched-process/thread termination.

## Rollback and stop conditions

Rollback only the shutdown thread-pin repair if native evidence proves the editor top-level HWND is legitimately owned by a different thread than the retained `CreateProcessW` primary thread, if suspend/resume lifecycle is incorrect, or if the final-close pin itself causes a reproducible regression. A rollback must restore a different ownership mechanism that closes the reviewed side-effect race, not the prior false-pass behavior. Stop before production-runtime change, workflow edit outside packet authority, rebase, merge, R0 execution, scheduler operation, dependency addition, graphics/API change, or game-content work. Never weaken a native acceptance assertion to make the gate green.

## Single next useful action

Obtain a fresh independent review of the exact final receipt head containing `89a0eb0...` and the corrected evidence. If that review is clean, hand that exact reviewed tree to the registered Windows executor for Debug/Release `EditorContainmentTests` plus interactive `EditorRuntimeSmoke` acceptance.