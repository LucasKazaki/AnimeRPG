# E11 Editor Runtime Smoke task, 2026-09-23

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. This packet may harden the native Windows smoke, its recovery supervisor, deterministic containment tests, and evidence. It must not add scene mutation/serialization, gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, rebase, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Admitted baseline from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Current PID-reuse hardening implementation candidate: `b9eebe721b7907245caaad5771bc9e96bac20680`.
`Tests/EditorRuntimeSmoke.cpp` blob after this repair: `8cb031fe7ddcf50af1d95be8b7086456a42d32f1`.
`CMakeLists.txt` remains blob `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.

Allowed paths only: `CMakeLists.txt`, `Tests/EditorRuntimeSmoke.cpp`, this task, `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`, and `Docs/Research/ENGINE-CAPABILITIES.json`. One active writer only. Do not rebase, merge, force-push, or absorb unrelated work.

## Selected reproducible verification defect

Before `b9eebe721...`, editor HWND ownership was accepted from `GetWindowThreadProcessId(hwnd) == launched_pid` alone. Windows process identifiers are valid only for a process lifetime and can later be reused. If the launched editor terminated during a smoke operation and another process was assigned the same numeric PID, the PID-only predicate could accept that unrelated process's window. The smoke could then enumerate, query, resize, send messages to, or close a window it did not launch.

The smoke already retains the process HANDLE returned by `CreateProcessW`. Windows process handles are waitable and stay valid until closed, so the retained handle is a stronger liveness anchor than a reusable numeric PID.

## Bounded implementation

Commit `b9eebe721b7907245caaad5771bc9e96bac20680` changes only `Tests/EditorRuntimeSmoke.cpp`:

- retains the launched editor process handle in `gOwnedProcessHandle` for the smoke lifetime;
- adds `OwnedProcessStillRunning()`, requiring a zero-time `WaitForSingleObject` result of `WAIT_TIMEOUT`;
- changes `WindowOwnedByProcess` to require the retained process handle to remain nonsignaled both before and after `GetWindowThreadProcessId` verifies the HWND's PID;
- routes top-level window enumeration through the same hardened ownership predicate instead of a raw PID equality check;
- clears the global retained-handle reference before closing the process handle;
- updates the PASS receipt to state that the retained `CreateProcess` handle remained nonsignaled around PID-based HWND ownership checks.

GitHub commit inspection shows 15 additions and 6 deletions, all in `Tests/EditorRuntimeSmoke.cpp`. No production editor source, CMake registration, workflow, dependency, graphics API, game content, scheduler configuration, release state, or architecture changed.

## Research basis, rechecked 2026-09-23 UTC

- Microsoft Learn, `Process Handles and Identifiers`: https://learn.microsoft.com/en-us/windows/win32/procthread/process-handles-and-identifiers
  - applicability: `CreateProcess` returns process handles that remain valid until closed; process identifiers are valid only from process creation until process termination.
- Microsoft Learn, `GetWindowThreadProcessId`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowthreadprocessid
  - applicability: reports the process identifier that created a specified window.
- Microsoft Learn, `WaitForSingleObject`: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
  - applicability: process handles are waitable; zero timeout returns immediately; `WAIT_TIMEOUT` means nonsignaled and `WAIT_OBJECT_0` means signaled.
- Microsoft Learn, `TerminateProcess`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-terminateprocess
  - applicability: cross-process termination is asynchronous and a process handle can be waited on when confirmed termination matters.

These are Win32 API semantics used to harden the verification harness. No proprietary UE/Unity source was copied and no dependency was added.

## Portable verification

Disposable C++17 PID-reuse ownership fixture SHA-256: `3cff1d8e63622d5943c61ee9e1827c12e7dc63a7d46b17aeae7a9e7793c70ad6`.

- `g++ -std=c++17 -Wall -Wextra -Werror`: PASS.
- `clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer`, leak detection enabled: PASS.
- live retained process + matching PID: accepted;
- original process exited + same PID reused by another process: old predicate accepts, repaired predicate rejects;
- process alive before lookup but terminates before final acceptance: old predicate accepts, repaired predicate rejects;
- mismatched PID: rejected.

This is source-logic evidence only, not Win32 runtime evidence. Sandbox repository access failed with `Could not resolve host: github.com`, and no usable Windows SDK/interactive desktop was available, so no sandbox production Win32 compile or GUI result is claimed.

## Hosted verification state

For implementation source candidate `b9eebe721b7907245caaad5771bc9e96bac20680`, GitHub generated synthetic pull-request merge `1968d7d40a3c9199bbd53a4bcce7027cdcc98e70` with tested base parent `3c3babd46c4539d474b7ea78ab01d5ed71dde7ac` and source parent `b9eebe721...`.

All three observed pull-request workflows completed successfully:

- Windows build and deterministic tests: run `35854249505`, job `107158864423`, PASS. All reported steps passed, including R0/repository safety contracts, Release assertion/CTest safety, VS2022 x64 configure, Debug build/tests, Release build/tests, prerequisite/runtime checks, static verifiers, and clean tracked tree.
- profiling capture portability: run `35854249577`, PASS.
- release manifest integrity: run `35854249522`, PASS.

These `pull_request` workflows validate the synthetic merge, not raw-head execution. Preserve source SHA, tested synthetic merge SHA, and tested base SHA separately.

## Retained E11 hardening and gates

1. deterministic `EditorContainmentTests` is selected by hosted suites while interactive `EditorRuntimeSmoke` remains excluded;
2. containment self-test executes real worker-local `CleanupProcess`, then requires supervisor whole-job cleanup to zero active processes;
3. normal-success containment rejects a zero-exit worker that leaves a descendant;
4. original 12 child HWND/class identity, semantic Static/Button binding, exact Outliner/assets rows, selection/Inspector synchronization, `LBS_NOTIFY`, bounded messages, positive area, startup containment, normal-size containment, narrow-size containment, and stable single top-level identity remain required;
5. HWND ownership now also requires retained launched-process liveness before and after PID lookup, closing the PID-reuse false-pass path.

`native_evidence` remains empty. Independent acceptance for the changed source remains false until a fresh independent review of the final receipt head completes. Issue #7 remains open, so the historical R0 runner is blocked and must not be invoked.

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

Retain exact reviewed source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, default-startup plus normal and 420x260 narrow-window screenshots, and process inspection proving zero owned contained processes after any failure or interruption. A failure/interruption receipt must also establish that the smoke never accepts or acts on an unrelated window after the launched editor process handle has signaled.

## Rollback and stop conditions

Rollback only the new retained-handle liveness check if native evidence shows the launched editor remains valid while the retained process handle is spuriously signaled, or if the handle lifecycle itself is proven incorrect. Stop before production-runtime change, workflow edit outside packet authority, rebase, merge, R0 execution, scheduler operation, dependency addition, graphics/API change, or game-content work. Never weaken a native acceptance assertion to make the gate green.

## Single next useful action

Update the QA/capability receipts for `b9eebe721...`, obtain a fresh independent review of the final receipt head, then hand that exact reviewed tree to the registered Windows executor for Debug/Release containment plus interactive GUI acceptance.