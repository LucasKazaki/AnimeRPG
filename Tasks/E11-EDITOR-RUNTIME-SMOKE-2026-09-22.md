# E11 Editor Runtime Smoke task, 2026-09-23

## Scope and ownership

Bounded verification-only packet for the integrated Win32 `AstralEditor` on owned branch `engine/2026-09-22-editor-runtime-smoke`. Admitted baseline remains `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.

Allowed paths only:

- `CMakeLists.txt`
- `Tests/EditorRuntimeSmoke.cpp`
- this task
- `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`
- `Docs/Research/ENGINE-CAPABILITIES.json`

Do not add editor/game features, change rendering or graphics API architecture, add dependencies, alter scheduler/runtime state, merge/rebase, deploy/release, invoke R0, or restart paused engine-worker content. One active writer only. Issue #7 was rechecked on 2026-09-23 and remains open, so the historical R0 runner remains blocked.

## Exact starting checkpoint for this pass

- PR #13 is open, draft, mergeable and unmerged.
- pre-pass evidence/control head: `4f1996e98a24c0478157c4ad1de9083ed35f0642`.
- current runtime-smoke source candidate remains `57a7867aa4273def5d1dfbb73766883945881c7c`.
- `Tests/EditorRuntimeSmoke.cpp` blob: `290b4931b3a4083555f0a5ddcae4ee14cbaebe6a`.
- `CMakeLists.txt` blob: `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.
- latest durable observed `main`: `6d22da88402db71843ecd5a35766c0c77e62dca6`.
- current source-associated hosted integration evidence is green: Windows run `35933677666` / job `107425789093`, profiling `35933677505`, release-manifest `35933677460`; those `pull_request` workflows exercised synthetic merge `ec0dee8274e6b3fd1ffeb4fe5480c88590c96fbf` over tested base `6d22da88402db71843ecd5a35766c0c77e62dca6` and source parent `57a7867aa4273def5d1dfbb73766883945881c7c`.
- exact pre-pass evidence head `4f1996e...` also has green hosted evidence: Windows run `35934076273` / job `107427047961`, profiling `35934076239`, release-manifest `35934076556`; its PR integration commit is `9e42ea5a12659d84e852ce8c1e5901bbd98adc9a`, with parents tested base `6d22da88402db71843ecd5a35766c0c77e62dca6` and head `4f1996e98a24c0478157c4ad1de9083ed35f0642`.
- hosted CTest intentionally excludes interactive `EditorRuntimeSmoke`; `native_evidence` remains empty.

## Fresh independent-review reconciliation

Fresh Codex review of exact pre-pass head `4f1996e98a24c0478157c4ad1de9083ed35f0642` completed at `2026-09-23T23:38:32Z`. It produced one current P2 evidence/control finding, thread `PRRT_kwDOTo2Ig86lYKi9`, top-level comment `4088355136`: the task and QA next-action text still told the next operator to repeat evidence synchronization that commits `21ae5174c9dc578da120ffafc11bea5916ee9231` and `4f1996e98a24c0478157c4ad1de9083ed35f0642` had already completed.

This pass removes that stale instruction. Because this evidence/control repair changes the exact branch head, the repaired head still requires fresh independent review before E11 can be accepted. Review by the coordinator is not independent acceptance.

## Additional bounded coordinator audit finding

While reconciling the review, a source-level deadline gap was reproduced in the current `SelectCubeAndNotify` acceptance path. This is a coordinator finding, not an independent-review finding and not native evidence.

Current source establishes a three-second selection deadline and a scoped message deadline, but the first selection poll executes `if (selection == 3) break;` before checking whether the three-second deadline has already expired. The code then enters `PostSelectionMutationWhileOwnerPinned(...)` for `WM_COMMAND/LBN_SELCHANGE`. That owner-pin helper does not receive or check the selection phase deadline before its final `PostMessageW` side effect.

Therefore two bounded cases can issue a selection notification after the advertised three-second phase:

1. `LB_GETCURSEL` first observes Cube at or just after the deadline and breaks before the existing deadline check.
2. Cube is observed just before the deadline, but owner pinning and the `GetThreadContext` barrier consume the remaining time and the helper posts `WM_COMMAND/LBN_SELCHANGE` after the deadline.

Later shell validation is still deadline-bounded, so this finding is not evidence of a false PASS. It is a bounded-side-effect defect: a mutation/notification can be enqueued after the phase that claims to contain all selection synchronization work. Native handoff must not use the current smoke as final acceptance evidence until this is repaired and re-reviewed.

## Reproduction evidence

Disposable coordinator fixture: `/mnt/data/e11_selection_deadline_fixture.cpp` during this pass only.

SHA-256: `383f71febc58529fa9c7e2887936463eaf5ff67c663a2763a27d4d7a3b8c18b7`.

Executed successfully:

```text
g++ (Debian 14.2.0-19) 14.2.0
g++ -std=c++17 -Wall -Wextra -Werror -pedantic e11_selection_deadline_fixture.cpp
=> e11 selection deadline fixture: PASS

clang version 17.0.0
clang++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer e11_selection_deadline_fixture.cpp
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
=> e11 selection deadline fixture: PASS
```

The fixture covers: selection first observed exactly at the phase deadline, selection observed in time followed by an owner-pin/post that finishes after the deadline, and a fully in-budget success case. It demonstrates the control-flow contract only. It is not a Win32 build, GUI run, GPU run, or substitute for native acceptance.

## Primary-source basis rechecked 2026-09-23

- Microsoft `PostMessageW`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-postmessagew . It posts to the queue associated with the thread that created the target window and returns without waiting for processing. The current selection harness therefore must gate the enqueue itself, not only later completion polling.
- Microsoft `GetTickCount64`: https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-gettickcount64 . It returns elapsed milliseconds since system start; its resolution is limited by the system timer. E11 uses it for bounded phase deadlines, so acceptance rules must be consistent at the boundary.
- Microsoft `GetThreadContext`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadcontext . A valid context cannot be obtained for a running thread; the existing owner pin uses this as its post-suspend execution barrier.
- Microsoft `SuspendThread`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-suspendthread . Microsoft describes it as debugger-oriented rather than general synchronization. E11 therefore keeps suspension limited to the short verification-only identity/enqueue interval and performs no target-dependent wait while suspended.
- Epic Unreal Engine 5.8, `Using Editor Viewports`: https://dev.epicgames.com/documentation/unreal-engine/using-editor-viewports-in-unreal-engine . Perspective 3D, orthographic 2D, multi-viewport layouts, maximized viewports and immersive mode remain editor workflow comparison targets.
- Unity 6.0, `EditorWindow.maximized`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/EditorWindow-maximized.html . Maximized editor-window state remains a comparison workflow property.

These are public behavioral/API references only. No proprietary Epic/Unity source was copied and no dependency was imported.

## Required source repair packet

The smallest admitted source repair is still within `Tests/EditorRuntimeSmoke.cpp` and must not change production editor behavior:

1. carry the existing selection-phase deadline into `PostSelectionMutationWhileOwnerPinned(...)` or an equivalent exact helper boundary;
2. reject the asynchronous `LB_SETCURSEL` and `WM_COMMAND/LBN_SELCHANGE` enqueue if the selection deadline is already exhausted immediately before the side effect;
3. when polling observes `LB_GETCURSEL == 3`, reject an at/after-deadline observation before advancing to notification;
4. preserve exact PID/TID owner pinning, zero-prior suspension, `CONTEXT_CONTROL` barrier, asynchronous `PostMessageW`, immediate resume, global 135-second work budget, and all existing shell/containment assertions;
5. add/retain a regression fixture for late-selection and late-owner-pin cases without weakening any existing acceptance assertion.

After implementation, run applicable warning-clean portable fixture checks, hosted Debug/Release deterministic suites, profiling/release-manifest checks, record actual synthetic merge/base/source identities, and obtain fresh independent review of the exact evidence head. Only then may the registered Windows executor run the native E11 matrix.

## Native acceptance matrix retained, but currently blocked by the source repair

After the source repair, hosted checks, evidence synchronization and exact-head independent review are all clean, the registered Windows executor must use one owned interactive desktop and run Debug and Release `EditorContainmentTests` plus interactive `EditorRuntimeSmoke`, retaining source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, screenshots and zero-owned-process cleanup proof.

The native matrix remains: untouched startup, Cube selection/Inspector synchronization, 800x600, 1280x720, 1440x900, actual maximized desktop followed by exact restore to the pre-maximize outer screen rectangle, and 420x260 or closest OS-permitted narrow state. Separately launch `AstralGame` from the same source/build as a no-regression check.

Clean-machine packaging, comparative frame/RAM/VRAM measurement, broader stress/recovery, remaining E00-E17 capability gaps, and the required 24-hour soak remain unresolved. No UE5/Unity parity claim is permitted.

## Rollback and stop conditions

Never weaken the acceptance test to obtain green results. Stop before production-runtime changes, dependency/API changes, unrelated workflow changes, rebase/merge, R0 execution, scheduler operations, release/deployment, or game-content work. Preserve the current source candidate if the deadline repair cannot be made safely in the owned packet and leave exact evidence instead.

## Single next useful action

Repair the selection-phase deadline at the side-effect boundary in `Tests/EditorRuntimeSmoke.cpp` as defined above. Then run the applicable hosted/portable checks, synchronize the task/QA/capability evidence to the resulting exact source and tested PR integration tree, and obtain fresh independent review. If and only if that exact reviewed tree is clean, hand it to the registered Windows executor for the retained Debug/Release native matrix and separate `AstralGame` launch. Do not repeat the already-completed `57a7867...` / `ec0dee8...` evidence synchronization and do not start another dependent editor feature while this gate is unresolved.