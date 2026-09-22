# E11 Editor Runtime Smoke task, 2026-09-22

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. It may harden the native Windows smoke and its evidence, but it must not add scene mutation/serialization, gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed: `012ca98f6aa1ec49bbec830666830de5ab7d1874`.
Current code candidate: `66228dfe8014a4a9f29531e44ac669fd0d998f59`.
Current smoke blob: `5fd37a7f0779185f4d6c177ae36d1c1f95b43986`.
Integrated editor source fixture: `Tools/AstralEditorMain.cpp` blob `5142e632a79c89d0d0ce3efe87e752456f802185`; production editor source was not modified.

Allowed paths only: `CMakeLists.txt`, `Tests/EditorRuntimeSmoke.cpp`, this task, `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`, and `Docs/Research/ENGINE-CAPABILITIES.json`. One active writer only. Do not rebase, merge, force-push, or absorb unrelated game-worker work.

## Current independent finding and bounded repair

Independent Codex review of exact evidence head `65609db30c3dfa9698a3d6e1dd504a44e34e30b5`, submitted 2026-09-22T22:10:03Z, reported one P2 recovery-safety defect. The smoke bounded each individual `SendMessageTimeoutW` call to one second, but the complete path can issue enough messages that a responsive-but-slow editor can exceed the externally registered CTest `TIMEOUT 180`. If CTest terminates the smoke first, owned-process cleanup may never run.

Repair candidate `66228dfe...` changes only `Tests/EditorRuntimeSmoke.cpp`:

- establishes a 135-second internal work deadline immediately after a successful editor launch;
- caps every bounded cross-process send to the lesser of 1000 ms and the remaining global work budget;
- refuses to issue a send with zero remaining budget;
- applies the same deadline to startup-idle wait, stable-window polling, resize polling, selection, final revalidation, and clean-close initiation;
- reports internal-budget exhaustion as a normal smoke failure, then runs the existing owned-process cleanup path;
- retains the 5-second normal clean-close wait and 2-second forced-cleanup wait outside the work phase, yielding modeled latest cleanup completion at 142 seconds and a 38-second margin before the 180-second external CTest timeout.

The prior cleanup hardening remains: forced termination uses only the retained `CreateProcessW` process handle, `TerminateProcess` failure is recorded, termination must signal within the cleanup deadline, and a terminal non-`STILL_ACTIVE` exit code must be verified. Forced termination is never an acceptance path. Normal PASS still requires process-owned `WM_CLOSE` and exit code 0.

All earlier stable-single-window, process ownership, enabled-state, exact-row, exact-Inspector, semantic-Static binding, `LBS_NOTIFY`, bounded-message, resize, containment, original 12-child handle-continuity, disabled-pending-tool, Release-assertion, and exclusive-desktop requirements remain in force.

## Research basis, rechecked 2026-09-22

Primary behavioral/API references:

- Microsoft Learn, `SendMessageTimeoutW`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendmessagetimeoutw
- Microsoft Learn, `GetTickCount64`: https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-gettickcount64
- Microsoft Learn, `WaitForInputIdle`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-waitforinputidle
- CMake, `TIMEOUT` test property: https://cmake.org/cmake/help/latest/prop_test/TIMEOUT.html
- Microsoft Learn, `TerminateProcess`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-terminateprocess
- Microsoft Learn, `WaitForSingleObject`: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
- Microsoft Learn, `GetExitCodeProcess`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getexitcodeprocess
- Microsoft Learn, `LBN_SELCHANGE`: https://learn.microsoft.com/en-us/windows/win32/controls/lbn-selchange
- Microsoft Learn, List Box Styles / `LBS_NOTIFY`: https://learn.microsoft.com/en-us/windows/win32/controls/list-box-styles
- Epic Games, UE 5.8 Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
- Epic Games, UE 5.8 Selecting Actors: https://dev.epicgames.com/documentation/unreal-engine/selecting-actors-in-unreal-engine
- Unity Technologies, Unity 6.0 Hierarchy: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html

`SendMessageTimeoutW` gives each send a local deadline, not an overall workflow deadline. `GetTickCount64` supplies elapsed uptime for the internal deadline. `WaitForInputIdle` accepts an explicit millisecond timeout. CTest's `TIMEOUT` property externally terminates a test that runs too long, so the smoke must reserve cleanup margin before that boundary. No proprietary implementation was copied and no dependency was imported.

## Portable reproduction

Disposable sandbox fixture: `/tmp/e11_runtime_budget_fixture.cpp`.
SHA-256: `5c6e02d39dff5a23a622ba63ae18011d0f4bba7b05742f53ff2d7651351402dc`.

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror /tmp/e11_runtime_budget_fixture.cpp -o /tmp/e11_runtime_budget_fixture
/tmp/e11_runtime_budget_fixture
clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /tmp/e11_runtime_budget_fixture.cpp -o /tmp/e11_runtime_budget_fixture_san
ASAN_OPTIONS=detect_leaks=1 /tmp/e11_runtime_budget_fixture_san
sha256sum /tmp/e11_runtime_budget_fixture.cpp
```

Results: GCC C++17 warning-clean compile/execution PASS; Clang C++17 ASan+UBSan warning-clean compile/execution PASS with no sanitizer finding. The fixture verifies the remaining-time cap, zero-budget refusal, and `135000 + 5000 + 2000 = 142000 < 180000`, leaving 38000 ms modeled external margin. This remains portable source-logic evidence only, not native Win32 execution.

Retained earlier fixture evidence: owned-process cleanup fixture SHA-256 `b606e0bf509b52aedcd5f1b2abc1e8763e4fc5f3548849f6acceda4dc20a9a63`; notification/semantic-role fixture SHA-256 `469803e6556d87fc3f5139e0145cd7133aed1d9b9eda5e3b0f9e224464372bc9`.

## Hosted verification

Exact code candidate `66228dfe8014a4a9f29531e44ac669fd0d998f59` completed all available hosted workflows successfully:

- Windows build and deterministic tests run `35791921687`, job `106962193015`: `completed/success`, exact head, completed 2026-09-22T22:23:23Z. R0 parser/safety contracts, Release assertion/CTest safety contracts, VS2022 x64 configure, MSVC Debug build/tests, MSVC Release build/tests, dependency/prerequisite/runtime-policy checks, static milestone verifiers, and clean-tree verification passed.
- profiling capture portability run `35791921821`: `completed/success`.
- release manifest integrity run `35791921696`: `completed/success`.

Hosted deterministic CTest intentionally excludes every `RuntimeSmoke`, so this is compile/non-runtime regression evidence only. The workflow parsed and tested R0 safety contracts but did not invoke the historical R0 runner. Issue #7 remains open.

## Acceptance contract

The smoke may report PASS only if the established E11 editor invariants remain true: one stable visible/enabled process-owned top-level window; exact original 12-child HWND/class continuity; five distinct bound semantic Static HWNDs; five exact visible disabled pending buttons; enabled Outliner and Assets list boxes; Outliner `LBS_NOTIFY`; exact five ordered Outliner rows and four ordered Assets rows; exact Scene Root/Cube Inspector fixtures; post-notification Cube synchronization; bounded 800x600 and 420x260 asynchronous resizes with containment/full-state validation; bounded cross-process messages; completion before the internal work deadline; and a clean process-owned exit.

On a failing smoke, cleanup must prove that the owned editor process reached a terminal state or retain an explicit cleanup failure. The smoke must fail internally early enough to preserve cleanup margin before CTest's external timeout. Never weaken an acceptance check to make the gate green.

## Registered-local handoff

Run the final candidate on one owned interactive Windows desktop using external build output:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, full stdout/stderr, exit codes, UTC timestamps, and normal plus narrow/short screenshots. Human-visible acceptance must confirm all five exact Outliner rows, all four exact asset rows, real mouse/keyboard Outliner selection updating Inspector/viewport, truthful disabled pending tools, panel containment, and continuity of the original semantic/control HWNDs. If the smoke fails, additionally verify that no owned `AstralEditor` process survives cleanup. For the timeout repair, exercise a controlled slow/hung failure if the registered executor has a packet-approved way to do so; do not modify production behavior merely to manufacture that condition.

## Stop and next action

E11 remains partial. `native_evidence` is empty and final independent acceptance is false. The timeout-budget source change requires a fresh independent review of the final evidence tree. Issue #7 remains open; do not invoke the historical R0 runner. Do not begin dependent scene-document, gizmo, undo/redo, save/reopen, or other editor feature work based on hosted compilation alone.

Single next useful action: obtain fresh independent review of the repaired final evidence tree, then execute Debug and Release `EditorRuntimeSmoke` on the registered Windows desktop with retained receipts/screenshots and verified owned-process cleanup before external timeout on any failure.
