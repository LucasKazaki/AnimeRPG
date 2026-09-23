# E11 Editor Runtime Smoke evidence, 2026-09-23

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. This pass found a process-identity false-pass in the native smoke: HWND ownership was trusted from numeric PID equality alone. Windows PIDs are only valid for a process lifetime and may later be reused. If the launched editor exited during the smoke and another process acquired the same PID, the old predicate could accept and interact with an unrelated window.

PID-reuse hardening implementation candidate: `b9eebe721b7907245caaad5771bc9e96bac20680`.
`Tests/EditorRuntimeSmoke.cpp` blob: `8cb031fe7ddcf50af1d95be8b7086456a42d32f1`.
`CMakeLists.txt` remains blob `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.
Production editor source is unchanged.

## Reproduced defect and repair

Before `b9eebe721...`, `WindowOwnedByProcess` accepted an HWND when `GetWindowThreadProcessId` returned the expected numeric PID. Top-level enumeration also directly compared that PID. This did not establish that the process launched by the smoke was still alive when the PID was observed.

Commit `b9eebe721b7907245caaad5771bc9e96bac20680` retains the `CreateProcessW` process HANDLE in `gOwnedProcessHandle` and adds `OwnedProcessStillRunning()`, using zero-time `WaitForSingleObject`. `WindowOwnedByProcess` now requires that retained handle to be nonsignaled before and after the HWND PID lookup. Top-level enumeration uses the same hardened predicate. The retained global reference is cleared before the process handle is closed.

The before-and-after liveness checks matter because termination may race with the PID lookup itself. A matching numeric PID is accepted only while the retained launched-process handle still reports `WAIT_TIMEOUT`.

GitHub commit inspection shows exactly 15 additions and 6 deletions, all in `Tests/EditorRuntimeSmoke.cpp`. No production source, CMake, workflow, dependency, graphics API, game content, scheduler configuration, release state, or architecture changed.

## Primary research

Accessed 2026-09-23 UTC:

- Microsoft Learn, `Process Handles and Identifiers`: https://learn.microsoft.com/en-us/windows/win32/procthread/process-handles-and-identifiers
  - `CreateProcess` returns process handles valid until closed; process identifiers are valid only from process creation until process termination.
- Microsoft Learn, `GetWindowThreadProcessId`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowthreadprocessid
  - returns the PID of the process that created a specified window.
- Microsoft Learn, `WaitForSingleObject`: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
  - process handles are waitable; a zero timeout returns immediately; `WAIT_TIMEOUT` means nonsignaled and `WAIT_OBJECT_0` means signaled.
- Microsoft Learn, `TerminateProcess`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-terminateprocess
  - cross-process termination is asynchronous and waiting on the process handle is the verification mechanism when termination must be confirmed.

Win32 API semantics only. No proprietary UE/Unity source was copied and no dependency was imported.

## Portable regression evidence

Disposable fixture SHA-256: `3cff1d8e63622d5943c61ee9e1827c12e7dc63a7d46b17aeae7a9e7793c70ad6`.

```text
g++ -std=c++17 -Wall -Wextra -Werror /tmp/e11_pid_reuse_fixture.cpp -o /tmp/e11_pid_reuse_fixture_gcc
/tmp/e11_pid_reuse_fixture_gcc
# exit 0, pid-reuse ownership fixture: PASS

clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /tmp/e11_pid_reuse_fixture.cpp -o /tmp/e11_pid_reuse_fixture_clang
ASAN_OPTIONS=detect_leaks=1 /tmp/e11_pid_reuse_fixture_clang
# exit 0, pid-reuse ownership fixture: PASS
```

Covered cases: live retained process with matching PID accepted; exited original process with same PID reused is accepted by the old predicate and rejected by the new predicate; process alive before lookup but terminated before final acceptance is rejected by the new predicate; mismatched PID is rejected. This is source-logic evidence only, not Win32 runtime evidence.

Sandbox repository checkout was attempted but failed with `Could not resolve host: github.com`. No MinGW cross compiler or usable Windows SDK/interactive desktop was available, so no sandbox production Win32 build or GUI result is claimed.

## Hosted verification state

Source implementation candidate: `b9eebe721b7907245caaad5771bc9e96bac20680`.
Synthetic PR integration commit: `1968d7d40a3c9199bbd53a4bcce7027cdcc98e70`.
Tested base parent of that synthetic merge: `3c3babd46c4539d474b7ea78ab01d5ed71dde7ac`.
Source parent of that synthetic merge: `b9eebe721b7907245caaad5771bc9e96bac20680`.

- Windows build and deterministic tests: run `35854249505`, job `107158864423`, PASS. Every reported job step passed, including R0/repository safety contracts, PE/prerequisite/runtime checks, Release assertion/CTest safety, VS2022 x64 configure, Debug build and deterministic tests, Release build and deterministic tests, static milestone verifiers, and clean tracked tree.
- profiling capture portability: run `35854249577`, PASS.
- release manifest integrity: run `35854249522`, PASS.

These are `pull_request` workflows and validate GitHub's synthetic merge. They are not raw-head execution evidence. Later receipt-only commits must be distinguished from this implementation candidate and its tested synthetic merge.

## Retained hardening

- `EditorContainmentTests` remains selected by hosted deterministic suites while interactive `EditorRuntimeSmoke` remains excluded;
- deterministic containment executes real worker-local `CleanupProcess`, verifies direct-process cleanup plus supervisor whole-job cleanup to zero active processes;
- shared normal-success containment rejects a zero-exit worker that leaves a descendant;
- every cross-process shell read is bounded and revalidates expected ownership/identity;
- original shell HWND/class identity, semantic Static/Button bindings, Outliner rows and selection, complete Inspector fixtures, `LBS_NOTIFY`, stable single top-level-window identity, owned cleanup, startup containment, positive width/height, normal resize, and narrow resize checks remain required;
- every accepted process-owned HWND now additionally requires the retained launched-editor process handle to remain nonsignaled around the numeric PID ownership lookup.

## Deferred native acceptance

`native_evidence` remains empty. After the receipt tree has a fresh clean independent review, the registered Windows executor must still run `EditorContainmentTests` and interactive `EditorRuntimeSmoke` in Debug and Release on one owned desktop. Retain:

- exact reviewed source/receipt SHA;
- Windows/machine identity;
- MSVC and CMake versions;
- GPU and driver identity;
- exact commands, complete stdout/stderr, exit codes, UTC timestamps;
- default-startup, normal-size, and 420x260 narrow-window screenshots;
- process inspection proving zero owned contained processes after failure/interruption;
- evidence that no unrelated HWND is accepted once the retained launched-editor process handle has signaled.

Issue #7 remains open. The historical R0 runner was not invoked. Clean-machine packaging, comparative frame/RAM/VRAM evidence, broader stress/recovery, and the required 24-hour soak remain unresolved outside this E11 repair.

## Acceptance state

Portable PID-reuse ownership fixture under GCC: PASS.
Portable PID-reuse ownership fixture under Clang ASan+UBSan: PASS.
Implementation candidate `b9eebe721...`: implemented; exact one-file diff verified.
Hosted PR-integration checks for implementation candidate: PASS through synthetic merge `1968d7...` on base `3c3babd...`.
Fresh independent review of the final receipt head: pending.
Native interactive Windows: deferred to registered local executor.
Independent final acceptance: false.
UE5/Unity parity claim: false.

## Single next action

Update the capability map, request fresh independent review of the final receipt head, and if clean hand that exact reviewed tree to the registered Windows executor for the Debug/Release containment and interactive GUI acceptance packet.