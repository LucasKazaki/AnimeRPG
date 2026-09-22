# E11 Editor Runtime Smoke task, 2026-09-22

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. It may harden the native Windows smoke and its evidence, but it must not add scene mutation/serialization, gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed: `012ca98f6aa1ec49bbec830666830de5ab7d1874`.
Current code candidate: `904f98ed5c0b1ec78d2a675b0e33efa35da81537`.
Current smoke blob: `a75860fe35d0bac1ed07620abfad0b638c33d09b`.
Integrated editor source fixture: `Tools/AstralEditorMain.cpp` blob `5142e632a79c89d0d0ce3efe87e752456f802185`; production editor source was not modified.

Allowed paths only: `CMakeLists.txt`, `Tests/EditorRuntimeSmoke.cpp`, this task, `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`, and `Docs/Research/ENGINE-CAPABILITIES.json`. One active writer only. Do not rebase, merge, force-push, or absorb unrelated game-worker work.

## Prior independent findings retained

Independent Codex review of evidence tree `e1b3b5a64c21fb47aba5980b66493b4795010580` reported two P2 false-pass paths. Candidate `006fabd386cd47937b5ce6f7eca627d6960d1c1f` repaired both and those repairs are retained in the current candidate:

1. The Outliner must retain live `LBS_NOTIFY` semantics before the test relies on `LBN_SELCHANGE`. The smoke validates the live style with `GetWindowLongPtrW(..., GWL_STYLE)` while also revalidating process ownership, parent, class, control ID, visibility, and enabled state.
2. Outliner label, Inspector label/body, Assets label, and status are bound to five distinct original `Static` HWNDs. Later validation reads each expected text from its exact original semantic control, preventing role/caption swaps from false-passing.

All prior stable-single-window, enabled-state, exact-row, exact-Inspector, bounded-message, resize, containment, ownership, 12-child handle-continuity, disabled-pending-tool, Release-assertion, and exclusive-desktop requirements remain in force.

## Bounded cleanup-safety repair selected in this pass

The prior failure cleanup helper could call `TerminateProcess` and then ignore both its return value and the result of the bounded `WaitForSingleObject`. A timeout, wait failure, or failed termination could therefore leave the owned `AstralEditor` process running after the smoke itself returned failure. That violated the packet's owned-process-cleanup contract and made repeated native execution unsafe to rely on.

Current candidate `904f98e...` changes only `Tests/EditorRuntimeSmoke.cpp` and hardens that failure path:

- an already-signaled owned process must have a verifiable terminal exit code, not `STILL_ACTIVE`;
- failure cleanup records `TerminateProcess` failure explicitly;
- after successful `TerminateProcess`, the smoke requires `WaitForSingleObject(process, kCleanupTimeoutMs) == WAIT_OBJECT_0`;
- timeout, wait failure, and unexpected wait states are retained in the test failure reason;
- after the process signals, `GetExitCodeProcess` must succeed and must not report `STILL_ACTIVE`;
- cleanup continues to operate only on the retained owned process handle and never enumerates or terminates unrelated processes.

This does not make forced termination a normal success path. Normal acceptance still requires the editor to close cleanly through the process-owned `WM_CLOSE` path with exit code 0. The hardened helper exists only to make a failing smoke bounded and accountable.

## Research basis, rechecked 2026-09-22

Primary behavioral/API references:

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

Microsoft documents that `TerminateProcess` is asynchronous when terminating another process and explicitly says callers that require certainty should wait on the process handle. `WaitForSingleObject` distinguishes signaled, timeout, and failure states, and `GetExitCodeProcess` reports `STILL_ACTIVE` while a process has not terminated. These API contracts directly justify the cleanup verification. Epic and Unity references remain behavioral comparison sources for the editor-shell workflow only. No proprietary implementation was copied and no dependency was imported.

## Portable cleanup-contract reproduction

Disposable sandbox fixture: `/tmp/e11_cleanup_contract_fixture.cpp`.
SHA-256: `b606e0bf509b52aedcd5f1b2abc1e8763e4fc5f3548849f6acceda4dc20a9a63`.

The fixture models the previous cleanup predicate and the hardened one. It demonstrates that the former path can claim cleanup completion despite termination failure, cleanup timeout, failed exit-code query, or a still-active process, while the hardened predicate rejects each of those cases.

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror /tmp/e11_cleanup_contract_fixture.cpp -o /tmp/e11_cleanup_contract_fixture
/tmp/e11_cleanup_contract_fixture
clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /tmp/e11_cleanup_contract_fixture.cpp -o /tmp/e11_cleanup_contract_fixture_san
ASAN_OPTIONS=detect_leaks=1 /tmp/e11_cleanup_contract_fixture_san
sha256sum /tmp/e11_cleanup_contract_fixture.cpp
```

Results: GCC C++17 warning-clean compile/execution PASS; Clang C++17 ASan+UBSan warning-clean compile/execution PASS with no sanitizer finding. This is portable source-logic evidence only, not native Win32 execution.

The previous notification/semantic-swap fixture remains valid evidence for the retained prior repair: SHA-256 `469803e6556d87fc3f5139e0145cd7133aed1d9b9eda5e3b0f9e224464372bc9`.

## Hosted verification

Exact code candidate `904f98ed5c0b1ec78d2a675b0e33efa35da81537` completed all available hosted workflows successfully:

- Windows build and deterministic tests `35789862102`, job `106955510235`: `completed/success` on exact head, completed 2026-09-22T22:01:51Z. Repository/R0 safety contracts, VS2022 x64 configuration, MSVC Debug build/tests, MSVC Release build/tests, dependency/prerequisite/runtime-policy checks, static milestone verifiers, and clean-tree verification passed.
- profiling capture portability `35789862264`: `completed/success`.
- release manifest integrity `35789862116`: `completed/success`.

Hosted deterministic CTest intentionally excludes every `RuntimeSmoke`, so this is compile/non-runtime regression evidence only. The historical R0 runner itself was not executed. Issue #7 remains open.

## Acceptance contract

The smoke may report PASS only if the established E11 editor invariants remain true: one stable visible/enabled process-owned top-level window; exact original 12-child HWND/class continuity; five distinct bound semantic Static HWNDs; five exact visible disabled pending buttons; enabled Outliner and Assets list boxes; Outliner `LBS_NOTIFY`; exact five ordered Outliner rows and four ordered Assets rows; exact Scene Root/Cube Inspector fixtures; post-notification Cube synchronization; bounded 800x600 and 420x260 asynchronous resizes with containment/full-state validation; bounded cross-process messages; and a clean process-owned exit.

On a failing smoke, cleanup must now either prove that the owned editor process reached a terminal state or retain an explicit cleanup failure in the test output. Never weaken an acceptance check to make the gate green.

## Registered-local handoff

Run the final candidate on one owned interactive Windows desktop using external build output:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, full stdout/stderr, exit codes, UTC timestamps, and normal plus narrow/short screenshots. Human-visible acceptance must confirm all five exact Outliner rows, all four exact asset rows, real mouse/keyboard Outliner selection updating Inspector/viewport, truthful disabled pending tools, panel containment, and continuity of the original semantic/control HWNDs. If the smoke fails, additionally verify from the retained process handle/process inspection that no owned `AstralEditor` process survives cleanup; preserve the failure text rather than retrying blindly.

## Stop and next action

E11 remains partial. `native_evidence` is empty and final independent acceptance is false. The cleanup code change also requires a fresh independent review of the final evidence tree. Issue #7 remains open; do not invoke the historical R0 runner. Do not begin dependent scene-document, gizmo, undo/redo, save/reopen, or other editor feature work based on hosted compilation alone.

Single next useful action: reconcile this candidate into the QA/capability records, obtain fresh independent review of the exact final evidence tree, then execute Debug and Release `EditorRuntimeSmoke` on the registered Windows desktop with retained receipts/screenshots and verified process cleanup on any failure.
