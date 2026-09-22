# E11 Editor Runtime Smoke evidence, 2026-09-22

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. This packet verifies the already-integrated Win32 `AstralEditor`; it does not authorize scene mutation/serialization, gizmos, Play-in-Editor, asset import, graphics/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed: `012ca98f6aa1ec49bbec830666830de5ab7d1874`.
Current code candidate: `904f98ed5c0b1ec78d2a675b0e33efa35da81537`.
`Tests/EditorRuntimeSmoke.cpp` blob: `a75860fe35d0bac1ed07620abfad0b638c33d09b`.
Integrated editor source fixture: `Tools/AstralEditorMain.cpp` blob `5142e632a79c89d0d0ce3efe87e752456f802185`; production editor source was not modified.

## Retained independent-review repairs

Independent Codex review of evidence tree `e1b3b5a64c21fb47aba5980b66493b4795010580` raised two P2 false-pass findings. The current candidate retains the repairs first implemented in `006fabd386cd47937b5ce6f7eca627d6960d1c1f`:

1. Outliner user-selection semantics require the live `LBS_NOTIFY` style before relying on `LBN_SELCHANGE`.
2. Outliner label, Inspector label/body, Assets label, and status are bound to five distinct original `Static` HWNDs so caption/body swaps cannot satisfy validation on the wrong controls.

The smoke still revalidates process ownership, direct parent, class, control ID where applicable, visibility, enabled state, exact list contents, exact Inspector text, the original 12-child HWND/class inventory, resize containment/full state, stable top-level cardinality, disabled pending toolbar actions, bounded cross-process calls, and clean normal shutdown.

## New cleanup-safety finding and repair

This pass found a verification/recovery defect in the smoke itself. The former `CleanupProcess` used the retained owned process handle, which was good, but it silently ignored `TerminateProcess` failure and ignored the result of the bounded cleanup `WaitForSingleObject`. A failure path could therefore return from the smoke while the launched `AstralEditor` process was still alive.

Candidate `904f98e...` changes only `Tests/EditorRuntimeSmoke.cpp`:

- if the process handle is already signaled, cleanup requires a successful terminal `GetExitCodeProcess` result and rejects `STILL_ACTIVE`;
- otherwise it records `TerminateProcess` failure instead of swallowing it;
- after successful forced termination it requires `WaitForSingleObject(process, 2000) == WAIT_OBJECT_0`;
- timeout, failed wait, and unexpected wait states are appended to the smoke's existing failure reason;
- after a signaled wait, it again verifies a non-`STILL_ACTIVE` process exit code;
- no window enumeration, PID search, or unrelated-process termination was added. Cleanup acts only on the retained process handle returned by `CreateProcessW`.

Normal PASS still requires the process-owned `WM_CLOSE` path to exit cleanly with exit code 0. Forced termination is only failure containment, never acceptance.

## Primary-source research, accessed 2026-09-22

- Microsoft Learn, `TerminateProcess`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-terminateprocess
- Microsoft Learn, `WaitForSingleObject`: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
- Microsoft Learn, `GetExitCodeProcess`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getexitcodeprocess
- Microsoft Learn, `LBN_SELCHANGE`: https://learn.microsoft.com/en-us/windows/win32/controls/lbn-selchange
- Microsoft Learn, List Box Styles / `LBS_NOTIFY`: https://learn.microsoft.com/en-us/windows/win32/controls/list-box-styles
- Microsoft Learn, `GetWindowLongW` / `GWL_STYLE`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowlongw
- Epic Games, UE 5.8 Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
- Epic Games, UE 5.8 Selecting Actors: https://dev.epicgames.com/documentation/unreal-engine/selecting-actors-in-unreal-engine
- Epic Games, UE 5.8 Unreal Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
- Unity Technologies, Unity 6.0 Hierarchy: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html

Microsoft states that `TerminateProcess` is asynchronous when used on another process and instructs callers that need certainty to wait on the process handle. `WaitForSingleObject` distinguishes signaled, timeout, and failure states. `GetExitCodeProcess` reports `STILL_ACTIVE` while a process has not terminated. Those are the direct API grounds for the cleanup repair. Epic and Unity sources are retained only as behavioral editor comparison references. No proprietary implementation was copied and no dependency was imported.

## Portable cleanup-contract fixture

Disposable coordinator-sandbox fixture: `/tmp/e11_cleanup_contract_fixture.cpp`.
SHA-256: `b606e0bf509b52aedcd5f1b2abc1e8763e4fc5f3548849f6acceda4dc20a9a63`.

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror /tmp/e11_cleanup_contract_fixture.cpp -o /tmp/e11_cleanup_contract_fixture
/tmp/e11_cleanup_contract_fixture
clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /tmp/e11_cleanup_contract_fixture.cpp -o /tmp/e11_cleanup_contract_fixture_san
ASAN_OPTIONS=detect_leaks=1 /tmp/e11_cleanup_contract_fixture_san
sha256sum /tmp/e11_cleanup_contract_fixture.cpp
```

GCC C++17 warning-clean compile/execution: PASS. Clang C++17 ASan+UBSan warning-clean compile/execution: PASS with no sanitizer finding. The fixture demonstrates that the former cleanup predicate could claim completion after termination failure, cleanup timeout, exit-code-query failure, or a still-active result, while the hardened contract rejects each case. This is portable source-logic evidence only, not native Win32 execution.

Earlier notification/semantic-role fixture remains retained evidence for the prior repair: SHA-256 `469803e6556d87fc3f5139e0145cd7133aed1d9b9eda5e3b0f9e224464372bc9`.

## Exact-candidate hosted verification

Exact code candidate `904f98ed5c0b1ec78d2a675b0e33efa35da81537` completed all available hosted workflows successfully:

- Windows build and deterministic tests `35789862102`, job `106955510235`: `completed/success`, exact head `904f98e...`, completed 2026-09-22T22:01:51Z. Passed repository/R0 safety contracts, VS2022 x64 configure, MSVC Debug build/tests, MSVC Release build/tests, Release dependency/prerequisite/runtime-policy checks, static milestone verifiers, and clean-tree verification.
- profiling capture portability `35789862264`: `completed/success`.
- release manifest integrity `35789862116`: `completed/success`.

Hosted deterministic CTest intentionally excludes every `RuntimeSmoke`, so these results establish compilation and deterministic non-runtime regression status only. The historical R0 runner itself was not executed. Issue #7 remains open.

## Native and independent acceptance state

`native_evidence` remains empty. This coordinator did not access or claim a registered Windows interactive desktop. The cleanup source changed after the last independent review, so fresh independent source review of the final evidence tree is required. The earlier review of `e1b3b5a...` is evidence for the findings it raised, not acceptance of this later candidate.

Run the final candidate on one owned interactive Windows desktop using external build output:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, and normal plus narrow/short screenshots. Human-visible acceptance must confirm exact Outliner/assets rows, actual mouse/keyboard Outliner selection causing Inspector/viewport synchronization, correct semantic label/body placement, truthful disabled pending tools, panel containment at both sizes, and original control-handle continuity. If any runtime smoke fails, retain evidence that the owned editor process reached a terminal state after cleanup; do not rerun until an unexplained survivor or cleanup failure is understood.

## Result

Status: **prior independent-review false-pass repairs retained; failure cleanup now verifies terminal state of the owned editor process; portable cleanup mutation fixture passed GCC and Clang ASan+UBSan; exact code candidate passed hosted Windows Debug/Release deterministic checks, profiling, and release-manifest workflows; fresh final-head independent review and native Debug/Release `EditorRuntimeSmoke` remain pending**.

E11 remains a partial editor-shell candidate, not UE5/Unity parity. No native GUI, GPU/performance, clean-machine, stress/recovery, soak, or final independent runtime acceptance claim is made. Issue #7 remains open and the historical R0 runner was not invoked.

Single next useful action: complete exact final-evidence reconciliation and fresh independent review, then execute Debug and Release `EditorRuntimeSmoke` on the registered Windows desktop with required receipts/screenshots and verified owned-process cleanup on failure.
