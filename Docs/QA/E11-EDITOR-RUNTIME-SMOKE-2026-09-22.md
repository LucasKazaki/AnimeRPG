# E11 Editor Runtime Smoke evidence, 2026-09-22

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. This packet verifies the already-integrated Win32 `AstralEditor`; it does not authorize scene mutation/serialization, gizmos, Play-in-Editor, asset import, graphics/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed: `012ca98f6aa1ec49bbec830666830de5ab7d1874`.
Current code candidate: `66228dfe8014a4a9f29531e44ac669fd0d998f59`.
`Tests/EditorRuntimeSmoke.cpp` blob: `5fd37a7f0779185f4d6c177ae36d1c1f95b43986`.
Integrated editor source fixture: `Tools/AstralEditorMain.cpp` blob `5142e632a79c89d0d0ce3efe87e752456f802185`; production editor source was not modified.

## Independent-review finding selected in this pass

Fresh independent Codex review of exact evidence head `65609db30c3dfa9698a3d6e1dd504a44e34e30b5`, submitted 2026-09-22T22:10:03Z, found one P2 recovery-safety defect in the smoke. Although every `SendMessageTimeoutW` call had a one-second local bound and cleanup itself had a bounded wait, the complete successful path can issue hundreds of cross-process messages. A responsive editor taking roughly 0.6 seconds per message can therefore exceed the test's externally registered CTest `TIMEOUT 180` before control reaches `CleanupProcess`. External CTest termination could bypass the smoke's cleanup contract and leave its owned editor process alive.

Review thread: `PRRT_kwDOTo2Ig86k8Bke`; top-level review comment database ID `4077032305`.

Repository source `cmake/AstralTestSafety.cmake` registers every `RuntimeSmoke` with `RUN_SERIAL TRUE TIMEOUT 180`. This file was read as a contract and was not modified because it is outside the E11 packet's allowed paths.

## Bounded repair

Candidate `66228dfe...` changes only `Tests/EditorRuntimeSmoke.cpp` relative to prior evidence head `65609db...` and adds an internal work deadline while preserving the external timeout:

- `kWorkBudgetMs = 135000` establishes a 135-second internal work phase after successful `CreateProcessW`.
- `RemainingWorkBudget` caps each `SendMessageTimeoutW` call to the lesser of the existing 1000 ms local timeout and the remaining overall work budget.
- A send is not issued with a zero timeout after the work deadline expires.
- startup `WaitForInputIdle`, stable-window polling, resize polling, selection, and clean-close initiation all respect the same internal work deadline.
- work-budget exhaustion becomes an explicit smoke failure rather than waiting for CTest to kill the test process.
- the existing 5-second normal close wait and 2-second forced-cleanup wait remain outside the 135-second work phase. The modeled latest cleanup completion is therefore 142 seconds, leaving 38 seconds before the registered 180-second CTest timeout for process teardown, output flushing, runner jitter, and CTest bookkeeping.
- failure cleanup still operates only on the retained process handle returned by `CreateProcessW`, requires the process to reach a terminal state or records an explicit cleanup failure, and never searches for or terminates unrelated processes.

All earlier acceptance guards remain: one stable visible/enabled process-owned top-level editor window; exact original 12-child HWND/class continuity; five distinct bound semantic Static HWNDs; exact shell labels/status/Inspector fixtures; exact five ordered Outliner rows and four ordered asset rows; Outliner `LBS_NOTIFY`; enabled Outliner/Assets interaction surfaces; five exact visible disabled pending buttons; bounded Cube-selection notification; bounded asynchronous 800x600 and 420x260 resizes with containment and complete-state validation; and clean process-owned normal shutdown with exit code 0.

## Primary-source research, accessed 2026-09-22

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

`SendMessageTimeoutW` provides a per-call timeout, not an overall transaction deadline. `GetTickCount64` provides elapsed uptime in milliseconds and is used only for the smoke's internal deadline. `WaitForInputIdle` accepts a bounded millisecond timeout. CTest's `TIMEOUT` property terminates a test after the configured duration, which is why an internal cleanup margin is required. No proprietary engine source was copied and no dependency was imported.

## Portable runtime-budget fixture

Disposable coordinator-sandbox fixture: `/tmp/e11_runtime_budget_fixture.cpp`.
SHA-256: `5c6e02d39dff5a23a622ba63ae18011d0f4bba7b05742f53ff2d7651351402dc`.

Commands executed:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror /tmp/e11_runtime_budget_fixture.cpp -o /tmp/e11_runtime_budget_fixture
/tmp/e11_runtime_budget_fixture
clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /tmp/e11_runtime_budget_fixture.cpp -o /tmp/e11_runtime_budget_fixture_san
ASAN_OPTIONS=detect_leaks=1 /tmp/e11_runtime_budget_fixture_san
sha256sum /tmp/e11_runtime_budget_fixture.cpp
```

Results: GCC C++17 warning-clean compile/execution PASS. Clang C++17 ASan+UBSan warning-clean compile/execution PASS with no sanitizer finding. The fixture verifies timeout capping at 1000/750/1/0 ms boundaries, refusal to start a send after budget exhaustion, and the static timing contract `135000 + 5000 + 2000 = 142000 < 180000`, leaving a modeled 38000 ms margin. This is portable source-logic evidence only, not native Win32 runtime evidence.

Retained earlier fixture evidence:

- owned-process cleanup contract fixture SHA-256 `b606e0bf509b52aedcd5f1b2abc1e8763e4fc5f3548849f6acceda4dc20a9a63`;
- notification/semantic-role fixture SHA-256 `469803e6556d87fc3f5139e0145cd7133aed1d9b9eda5e3b0f9e224464372bc9`.

## Exact-candidate hosted verification

Exact code candidate `66228dfe8014a4a9f29531e44ac669fd0d998f59` completed all available hosted workflows successfully:

- Windows build and deterministic tests run `35791921687`, job `106962193015`: `completed/success`, exact head `66228dfe...`, completed 2026-09-22T22:23:23Z. The job passed R0 parser/safety contracts, Release assertion/CTest safety contracts, Visual Studio 2022 x64 configure, MSVC Debug build/tests, MSVC Release build/tests, Release dependency/prerequisite/runtime-policy checks, static milestone verifiers, and clean tracked-tree verification.
- profiling capture portability run `35791921821`: `completed/success`.
- release manifest integrity run `35791921696`: `completed/success`.

The workflow parsed and verified R0 safety contracts but did not invoke the historical R0 recovery runner. Hosted deterministic CTest intentionally excludes every `RuntimeSmoke`, so these results establish compilation and deterministic non-runtime regression status only.

## Native and independent acceptance state

`native_evidence` remains empty. This coordinator did not access or claim a registered Windows interactive desktop. Because `Tests/EditorRuntimeSmoke.cpp` changed in `66228dfe...`, a fresh independent review of the final evidence tree is required. The review of `65609db...` is evidence for the timeout finding, not acceptance of this repaired candidate.

Run the final candidate on one owned interactive Windows desktop using external build output:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, and normal plus narrow/short screenshots. Human-visible acceptance must confirm exact Outliner/assets rows, actual mouse/keyboard Outliner selection causing Inspector/viewport synchronization, correct semantic label/body placement, truthful disabled pending tools, panel containment at both sizes, and original control-handle continuity. If any runtime smoke fails, retain evidence that the owned editor process reached a terminal state after cleanup, and verify that internal budget failure occurs before external CTest termination if the editor is artificially slowed or becomes message-responsive-but-slow.

## Result

Status: **the fresh independent P2 timeout/cleanup finding is repaired by a 135-second internal work budget with capped per-call waits; the portable runtime-budget fixture passed GCC and Clang ASan+UBSan; exact candidate `66228dfe...` passed hosted Windows Debug/Release deterministic checks, profiling, and release-manifest workflows; fresh independent review of the repaired final tree and native Debug/Release `EditorRuntimeSmoke` remain pending**.

E11 remains a partial editor-shell candidate, not UE5/Unity parity. No native GUI, GPU/performance, clean-machine, stress/recovery, soak, or final independent runtime acceptance claim is made. Issue #7 remains open and the historical R0 runner was not invoked.

Single next useful action: obtain fresh independent review of the repaired final evidence tree, then execute Debug and Release `EditorRuntimeSmoke` on the registered Windows desktop with required receipts/screenshots and explicit proof that owned-process cleanup completes before CTest's external timeout on any failure.
