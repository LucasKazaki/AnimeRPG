# E11 Editor Runtime Smoke evidence, 2026-09-23

## Current checkpoint

Branch: `engine/2026-09-22-editor-runtime-smoke`.
Implementation candidate: `b2c836013c8212e9432e8c2e861551b5c8f3646b`.
`Tests/EditorRuntimeSmoke.cpp` blob: `33eefbd64c55d882df5ac5452b0ed02de5d767db`.
`CMakeLists.txt` blob: `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.
Latest observed `main`: `7dfaeeb340e57d1024a8bc818c65c82cd391d4ae`.
Production editor source is unchanged.

Fresh independent review of prior receipt head `d2748b64fc606df4ea78fa668b2ac0512b15383a` completed with one new P2 on `Tests/EditorRuntimeSmoke.cpp`: `SuspendThread` success alone did not establish a barrier proving the owner thread had actually reached a suspended state before HWND revalidation and `PostMessageW`. That review is not acceptance of the changed code.

## Repair

Commit `b2c836013c8212e9432e8c2e861551b5c8f3646b` changes only `Tests/EditorRuntimeSmoke.cpp`. After the exact launch thread is suspended with prior count zero, `CloseEditor` now initializes a `CONTEXT` with `CONTEXT_CONTROL` and requires `GetThreadContext` to succeed. Only after that barrier does the smoke repeat process/thread liveness and exact PID/TID HWND ownership checks and enqueue asynchronous `WM_CLOSE`. It then resumes the launch thread, requires `ResumeThread` to report previous count one, and waits for the retained process handle only after resume.

Failure of the context barrier, retained liveness, PID/TID identity, enqueue, or resume verification fails closed and preserves the owned-process/job cleanup path. No synchronous window procedure call, production-engine operation, or wait on a target-owned resource occurs while the target thread is suspended.

GitHub commit inspection confirms this source commit changes only `Tests/EditorRuntimeSmoke.cpp`. No production editor code, CMake registration, workflow, dependency, graphics API, scheduler configuration, content, merge, release, or deployment changed.

## Primary-source research

Accessed 2026-09-23 UTC:

- Microsoft Learn `GetThreadContext`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadcontext
  - requires `THREAD_GET_CONTEXT`; Microsoft states a valid context cannot be obtained for a running thread and directs callers to suspend the thread first. This is used as the explicit post-suspend barrier.
- Microsoft Learn `SuspendThread`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-suspendthread
  - increments the suspend count and stops user-mode execution; debugger-oriented and not general synchronization, so this usage is limited to the verification harness.
- Microsoft Learn `PROCESS_INFORMATION`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/ns-processthreadsapi-process_information
  - `hThread` and `dwThreadId` identify the newly created process's primary thread and are retained by this smoke.
- Microsoft Learn `ResumeThread`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-resumethread
  - previous count one verifies removal of the one harness suspension.
- Existing retained basis: Microsoft `PostMessageW`, `GetWindowThreadProcessId`, `WaitForSingleObject`, and `DestroyWindow` documentation.

Public Win32 API semantics only. No proprietary Unreal Engine or Unity source was copied and no dependency was imported.

## Portable regression evidence

Disposable C++17 suspension-barrier state-machine fixture SHA-256: `df17c2d0cbb8f1c8cd853d8adb5e0108b6b5a4b9755e9a6bcfc4c97f26d792ae`.

```text
g++ -std=c++17 -Wall -Wextra -Werror /tmp/e11_suspend_barrier_fixture.cpp -o /tmp/e11_gcc
/tmp/e11_gcc
# exit 0

clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /tmp/e11_suspend_barrier_fixture.cpp -o /tmp/e11_clang
ASAN_OPTIONS=detect_leaks=1 /tmp/e11_clang
# exit 0
```

Accepted case: zero previous suspend count, context barrier true, retained process/thread live, PID/TID match, close enqueue succeeds, resume previous count one.
Rejected cases: context barrier false, dead process, dead thread, PID mismatch, TID mismatch, pre-existing suspension, enqueue failure, resume previous count zero, and resume previous count two.

This is source-logic evidence only. No native Win32 GUI result is claimed from the sandbox.

## Hosted verification

The pull-request workflows associated with source candidate `b2c836013c8212e9432e8c2e861551b5c8f3646b` completed successfully:

- tested synthetic PR integration commit: `b305a5ff8320c35f8c564c0c9210f8b57d089d0d`;
- tested base parent: `7dfaeeb340e57d1024a8bc818c65c82cd391d4ae`;
- source parent: `b2c836013c8212e9432e8c2e861551b5c8f3646b`;
- Windows build and deterministic tests: run `35866993050`, PASS;
- profiling capture portability: run `35866993020`, PASS;
- release manifest integrity: run `35866993023`, PASS.

The synthetic merge identity is recorded separately from the source head because pull-request CI tests GitHub's integration tree. Hosted deterministic tests are not native interactive `EditorRuntimeSmoke` evidence.

## Retained hardening and acceptance state

- `EditorContainmentTests` remains hosted and deterministic; interactive `EditorRuntimeSmoke` remains a separate native gate.
- Containment still exercises worker-local direct cleanup, whole-job zero-active-process cleanup, and rejection of a successful worker that leaves descendants.
- Shell smoke still requires stable process-owned top-level identity, original 12 child HWND/class identities, bound Static/Button semantics, disabled pending toolbar tools, exact Outliner/assets rows, `LBS_NOTIFY`, selection/Inspector synchronization, bounded cross-process messages, startup containment, positive child area, 800x600 and 420x260 containment/state checks.
- PID/HWND ownership retains launched-process handle liveness checks.
- Final close now additionally requires exact launch-thread ownership plus successful suspended-thread context capture before the final revalidation/enqueue, followed by verified resume before any wait.

Portable GCC fixture: PASS.
Portable Clang ASan+UBSan fixture: PASS.
Hosted workflows for `b2c836...`: PASS.
Fresh independent review of this changed source/evidence tree: pending.
Native interactive Windows: pending.
`native_evidence`: empty.
Independent final acceptance: false.
UE5/Unity parity claim: false.

## Native handoff

After a fresh clean independent review, the registered Windows executor must run Debug and Release `EditorContainmentTests` and `EditorRuntimeSmoke` on one owned interactive desktop. Retain exact reviewed source SHA, Windows/machine identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, default-startup/800x600/420x260 screenshots, and zero-contained-process proof after failure or interruption. Final-close evidence must confirm the launch thread is resumed and no unrelated/recycled HWND receives the close operation.

Issue #7 remains open. The historical R0 runner was not invoked. Clean-machine packaging, comparative frame/RAM/VRAM measurements, broader stress/recovery, remaining capability-catalogue gaps, and the required 24-hour soak remain unresolved.

## Single next action

Obtain fresh independent review of the exact post-receipt tree containing `b2c836013c8212e9432e8c2e861551b5c8f3646b` and these updated records. If clean, hand that exact reviewed tree to the registered Windows executor for native Debug/Release containment plus interactive editor-smoke acceptance.