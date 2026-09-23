# E11 Editor Runtime Smoke task, 2026-09-23

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. This packet may harden the native Windows smoke, its recovery supervisor, deterministic containment tests, and evidence. It must not add scene mutation/serialization, gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, rebase, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Admitted baseline from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest observed `main`: `7dfaeeb340e57d1024a8bc818c65c82cd391d4ae`. This separate game-worker merge was not absorbed or rebased into the engine branch.
Current suspension-barrier implementation candidate: `b2c836013c8212e9432e8c2e861551b5c8f3646b`.
`Tests/EditorRuntimeSmoke.cpp` blob: `33eefbd64c55d882df5ac5452b0ed02de5d767db`.
`CMakeLists.txt` remains blob `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.

Allowed paths only: `CMakeLists.txt`, `Tests/EditorRuntimeSmoke.cpp`, this task, `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`, and `Docs/Research/ENGINE-CAPABILITIES.json`. One active writer only. Do not rebase, merge, force-push, or absorb unrelated work.

## Selected reproducible verification defect

Fresh independent review of exact prior receipt head `d2748b64fc606df4ea78fa668b2ac0512b15383a` found one P2 at `2026-09-23T12:33:54Z`: `SuspendThread` returning success increments the suspend count, but it does not by itself prove that an in-flight transition has reached a state from which the caller can safely treat the target as stopped before revalidating the HWND and posting `WM_CLOSE`. The reviewed code could therefore observe the old HWND before destruction completed and still race with its destruction/reuse before `PostMessageW`.

The close path needs an explicit post-suspend barrier before the repeated PID/TID ownership check and final side effect.

## Bounded implementation

Commit `b2c836013c8212e9432e8c2e861551b5c8f3646b` changes only `Tests/EditorRuntimeSmoke.cpp`:

- retains the exact launched PID, primary-thread ID, process handle, and primary-thread handle already supplied by `CreateProcessW`;
- requires `SuspendThread` to succeed with prior suspend count zero;
- initializes `CONTEXT` with `CONTEXT_CONTROL` and requires `GetThreadContext(thread, &context)` to succeed before any final HWND revalidation or close enqueue;
- only after that barrier does it recheck retained process/thread liveness and exact HWND PID/TID ownership, then call asynchronous `PostMessageW(..., WM_CLOSE, ...)`;
- immediately resumes the thread and requires `ResumeThread` to report previous suspend count one before waiting on the process;
- fails closed when the context barrier, liveness, identity, enqueue, or resume verification fails, preserving the existing owned-process/job cleanup path.

GitHub commit metadata shows 7 additions and 0 deletions in the close logic plus the PASS-text adjustment, all in `Tests/EditorRuntimeSmoke.cpp`. Production editor source, CMake registration, workflows, dependencies, graphics API, game content, scheduler configuration, release state, and architecture are unchanged.

## Primary research basis, rechecked 2026-09-23 UTC

- Microsoft Learn `GetThreadContext`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadcontext
  - requires `THREAD_GET_CONTEXT`; Microsoft states a valid context cannot be obtained for a running thread and directs callers to suspend the thread first. This is the explicit barrier used by this verification harness after `SuspendThread`.
- Microsoft Learn `SuspendThread`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-suspendthread
  - increments the suspend count and stops user-mode execution, but is debugger-oriented and not general synchronization. This packet therefore uses it only for the short verification-harness barrier/revalidation/enqueue interval.
- Microsoft Learn `PROCESS_INFORMATION`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/ns-processthreadsapi-process_information
  - the returned `hThread` is the primary-thread handle used for thread operations; `dwThreadId` identifies that primary thread.
- Microsoft Learn `ResumeThread`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-resumethread
  - decrements the suspend count; previous count one verifies removal of this harness suspension.
- Existing retained basis: `PostMessageW`, `GetWindowThreadProcessId`, `WaitForSingleObject`, and `DestroyWindow` public Win32 documentation.

No proprietary Unreal Engine or Unity source was copied, and no dependency was added.

## Portable verification

Disposable C++17 suspension-barrier state-machine fixture SHA-256: `df17c2d0cbb8f1c8cd853d8adb5e0108b6b5a4b9755e9a6bcfc4c97f26d792ae`.

- `g++ -std=c++17 -Wall -Wextra -Werror`: PASS, exit 0.
- `clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer`, `ASAN_OPTIONS=detect_leaks=1`: PASS, exit 0.
- accepted: zero prior suspend count, context barrier succeeded, process/thread live, PID/TID match, enqueue succeeded, resume previous count one.
- rejected: missing context barrier, dead process, dead thread, PID mismatch, TID mismatch, pre-existing suspension, enqueue failure, and bad resume counts.

This fixture is source-logic evidence only, not Win32 runtime evidence. The sandbox did not provide an interactive Windows desktop/SDK, so no sandbox native GUI result is claimed.

## Hosted verification state

Current source candidate `b2c836013c8212e9432e8c2e861551b5c8f3646b` has completed successful pull-request workflows:

- tested synthetic PR merge: `b305a5ff8320c35f8c564c0c9210f8b57d089d0d`;
- tested base parent: `7dfaeeb340e57d1024a8bc818c65c82cd391d4ae`;
- source parent: `b2c836013c8212e9432e8c2e861551b5c8f3646b`;
- Windows build and deterministic tests: run `35866993050`, PASS;
- profiling capture portability: run `35866993020`, PASS;
- release manifest integrity: run `35866993023`, PASS.

Hosted deterministic suites do not execute the interactive GUI `EditorRuntimeSmoke` and do not establish workstation/native acceptance.

## Retained E11 hardening and gates

1. `EditorContainmentTests` executes in hosted deterministic suites while interactive `EditorRuntimeSmoke` remains separate.
2. Containment coverage exercises worker-local `CleanupProcess`, supervisor whole-job cleanup to zero active processes, and rejection of a zero-exit worker that leaves a descendant.
3. Shell verification retains the original 12 child HWND/class inventory, semantic Static/Button binding, exact Outliner/assets rows, selection/Inspector synchronization, `LBS_NOTIFY`, bounded cross-process messages, positive-area startup and resize containment, normal and 420x260 narrow states, and stable single top-level identity.
4. PID-based HWND checks require the retained launched-process handle to remain live.
5. Final close requires exact original PID/TID ownership plus a successful suspended-thread context barrier before repeated ownership validation and asynchronous `WM_CLOSE`, followed by verified resume before any process wait.

`native_evidence` remains empty. Independent acceptance remains false until the changed source plus evidence receive a fresh independent review and registered native Windows acceptance is retained. Issue #7 remains open, so the historical R0 runner is blocked and must not be invoked.

## Registered native handoff

Only after green hosted checks and a fresh clean independent review of the exact receipt tree, the registered Windows executor should use one owned interactive desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact reviewed source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, default-startup/800x600/420x260 screenshots, and process inspection proving zero owned contained processes after any failure or interruption. Native acceptance must confirm the final close leaves the launch thread resumed and that the smoke never accepts or acts on an unrelated/recycled HWND.

## Rollback and stop conditions

Rollback only this suspension-barrier repair if native evidence shows `GetThreadContext` is unsupported for the retained primary-thread handle in the admitted Windows environment, if the suspend/context/resume lifecycle is incorrect, or if the final-close barrier causes a reproducible regression. A rollback must replace it with an ownership mechanism that closes the reviewed race, not restore the prior false-pass window. Stop before production-runtime change, workflow edit outside packet authority, rebase, merge, R0 execution, scheduler operation, dependency addition, graphics/API change, or game-content work. Never weaken a native acceptance assertion to make the gate green.

## Single next useful action

Obtain fresh independent review of the exact receipt tree containing `b2c836013c8212e9432e8c2e861551b5c8f3646b` and the updated evidence. If that review is clean, hand that exact reviewed tree to the registered Windows executor for Debug/Release `EditorContainmentTests` plus interactive `EditorRuntimeSmoke` acceptance.