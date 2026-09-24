# E11 Editor Runtime Smoke evidence, 2026-09-23

## Current checkpoint

Branch: `engine/2026-09-22-editor-runtime-smoke`.

Current runtime-smoke source candidate remains `57a7867aa4273def5d1dfbb73766883945881c7c` with `Tests/EditorRuntimeSmoke.cpp` blob `290b4931b3a4083555f0a5ddcae4ee14cbaebe6a`. `CMakeLists.txt` remains blob `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.

Pre-pass evidence/control head was `4f1996e98a24c0478157c4ad1de9083ed35f0642`. Latest durable observed `main` is `6d22da88402db71843ecd5a35766c0c77e62dca6`.

Issue #7 was rechecked and remains open. Historical R0 was not invoked. `native_evidence` remains empty. E11 final acceptance and the UE5/Unity parity claim remain false.

## Existing source repair retained

Source `57a7867...` is the selection-owner-pin repair. It changes only `Tests/EditorRuntimeSmoke.cpp` relative to parent `a87ff45c236deb4fa5f9a734192e8dfad1beade4`, with 133 additions and 24 deletions.

`PostSelectionMutationWhileOwnerPinned(...)` requires retained process/thread liveness, exact launched PID/TID ownership for both editor and Outliner, exact Outliner parent/control-ID/class/visibility/enabled/LBS_NOTIFY identity, zero-prior-count suspension, and a successful `GetThreadContext(CONTEXT_CONTROL)` barrier. It revalidates identity while the creating thread is stopped, posts only asynchronous `LB_SETCURSEL` or `WM_COMMAND/LBN_SELCHANGE`, then immediately resumes before any completion poll or wait.

The previous stale/recycled-HWND selection side-effect finding remains repaired. Resize, show-state and final-close owner pins, bounded shell reads, exact restore rectangle, process-tree containment and cleanup remain unchanged by this evidence-only pass.

## Hosted evidence and PR integration provenance

For source `57a7867aa4273def5d1dfbb73766883945881c7c`, source-associated hosted checks are green:

- Windows run `35933677666`, job `107425789093`, success;
- profiling run `35933677505`, success;
- release-manifest run `35933677460`, success.

These are `pull_request` workflows. The actual tested integration commit for that source is synthetic merge `ec0dee8274e6b3fd1ffeb4fe5480c88590c96fbf`, with tested base parent `6d22da88402db71843ecd5a35766c0c77e62dca6` and source parent `57a7867aa4273def5d1dfbb73766883945881c7c`.

The exact pre-pass evidence head `4f1996e98a24c0478157c4ad1de9083ed35f0642` also had green hosted evidence:

- Windows run `35934076273`, job `107427047961`, success, completed `2026-09-23T23:34:22Z`;
- profiling run `35934076239`, success;
- release-manifest run `35934076556`, success.

Its actual PR integration commit was `9e42ea5a12659d84e852ce8c1e5901bbd98adc9a`, with parents tested base `6d22da88402db71843ecd5a35766c0c77e62dca6` and evidence head `4f1996e98a24c0478157c4ad1de9083ed35f0642`.

The Windows hosted suite passed repository/R0 safety contracts, VS2022 x64 configuration, Debug and Release builds plus deterministic tests, runtime/prerequisite checks, static verifiers and clean-tree verification. Hosted CTest intentionally excludes interactive `EditorRuntimeSmoke`, so none of these hosted passes establish real GUI/GPU behavior, screenshots, clean-machine packaging, measured performance, stress/recovery or soak acceptance.

## Fresh exact-head independent review

Fresh Codex review of exact pre-pass evidence head `4f1996e98a24c0478157c4ad1de9083ed35f0642` completed at `2026-09-23T23:38:32Z`.

It produced one current P2 evidence/control finding, review thread `PRRT_kwDOTo2Ig86lYKi9`, top-level comment `4088355136`: the task and this QA receipt still directed the next operator to repeat evidence synchronization that had already been completed by `21ae5174c9dc578da120ffafc11bea5916ee9231` and `4f1996e98a24c0478157c4ad1de9083ed35f0642`.

This pass removes the stale action. Because the evidence/control files changed after the review, independent acceptance remains false until the new exact head receives a fresh independent review.

## New coordinator audit finding: selection phase can enqueue after its deadline

Static inspection of current `SelectCubeAndNotify` found a bounded-side-effect gap that must be repaired before native acceptance. This is a coordinator finding, not independent QA and not native evidence.

The function establishes a three-second selection deadline and a scoped message deadline, but the Cube-selection poll executes `if (selection == 3) break;` before checking whether the deadline has already elapsed. It then calls `PostSelectionMutationWhileOwnerPinned(...)` for `WM_COMMAND/LBN_SELCHANGE`. That helper verifies owner identity and establishes the suspension/context barrier, but it does not receive or check the selection-phase deadline immediately before `PostMessageW`.

Two reproducible control-flow cases can therefore enqueue the notification after the advertised phase:

1. Cube is first observed exactly at or after the three-second deadline, and the loop breaks before the deadline rejection.
2. Cube is observed just before the deadline, but owner-pinning work consumes the remaining budget and the notification is posted after the deadline.

The subsequent shell-validation loop is still deadline-bounded, so this is not evidence that the test can falsely return PASS. It is evidence that the side effect itself is not fully contained by the phase boundary the receipt claims.

## Portable regression evidence for the new finding

Disposable fixture: `/mnt/data/e11_selection_deadline_fixture.cpp` during this pass only.

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

The fixture covers a selection first observed at the deadline, an in-time selection followed by an owner-pin/post that lands after the deadline, and a fully in-budget success case. This is source-logic evidence only. It is not a production Win32 compile, interactive desktop run, GPU test, or substitute for native acceptance.

## Primary-source research rechecked 2026-09-23

- Microsoft `PostMessageW`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-postmessagew . It posts a message to the queue of the thread that created the target window and returns without waiting for processing. A phase that claims to bound a selection side effect therefore must gate the enqueue itself.
- Microsoft `GetTickCount64`: https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-gettickcount64 . It returns elapsed milliseconds since system startup, with resolution limited by the system timer. The same boundary rule must be applied consistently to observations and side-effect admission.
- Microsoft `GetThreadContext`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadcontext . A valid context cannot be obtained for a running thread, supporting the existing post-suspend barrier.
- Microsoft `SuspendThread`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-suspendthread . Microsoft states that it is primarily debugger-oriented and not intended as a general synchronization primitive, so E11 confines it to the short verification-only identity/enqueue interval and does not wait on target work while suspended.
- Epic Unreal Engine 5.8, `Using Editor Viewports`: https://dev.epicgames.com/documentation/unreal-engine/using-editor-viewports-in-unreal-engine . Perspective 3D, orthographic 2D, multiple viewport layouts, maximized viewports and immersive mode remain editor workflow references.
- Unity 6.0, `EditorWindow.maximized`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/EditorWindow-maximized.html . Maximized editor-window state remains a workflow reference.

Behavior/API references only. No proprietary Epic/Unity source was copied and no dependency was imported.

## Required repair before native handoff

The next source change must stay inside the existing bounded E11 packet and should change only `Tests/EditorRuntimeSmoke.cpp` unless evidence requires otherwise:

1. carry the existing selection deadline into `PostSelectionMutationWhileOwnerPinned(...)` or an equivalent helper boundary;
2. reject `LB_SETCURSEL` and `WM_COMMAND/LBN_SELCHANGE` if the phase has expired immediately before the asynchronous `PostMessageW` side effect;
3. reject `LB_GETCURSEL == 3` when that observation occurs at or after the deadline rather than breaking first;
4. preserve PID/TID owner pinning, zero-prior suspension, `CONTEXT_CONTROL` barrier, immediate resume, global work budget and existing semantic/containment assertions;
5. retain a deterministic regression fixture for both late cases and a normal success case;
6. rerun applicable hosted Debug/Release checks and record the actual tested synthetic merge/base/source identities; then obtain fresh independent review of the exact evidence head.

No native E11 acceptance should be claimed from the current `57a7867...` source after this finding. The registered Windows executor should wait for this source repair and its independent review rather than spend native acceptance time on a known-bounded test defect.

## Native acceptance matrix retained after the repair

Once the repair is implemented, hosted evidence is green and the exact evidence head is independently clean-reviewed, the registered Windows executor should use one owned interactive desktop and run both Debug and Release:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact reviewed SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, zero-owned-process proof, and screenshots for untouched startup, Cube selection/Inspector synchronization, 800x600, 1280x720, 1440x900, actual maximized desktop, exact restore to the pre-maximize outer rectangle, and 420x260 or closest OS-permitted narrow state. Separately launch `AstralGame` from the same source/build as a no-regression check.

## Remaining gaps

The capability catalogue remains unresolved beyond E11: runtime/jobs/memory, scene ownership/serialization, complete asset pipeline/resources, GPU rendering/materials, lighting/shadows/reflections, large-world streaming/detail, animation, physics/collision, AI/navigation, audio, genuine 2D, networking, profiling/budgets, packaging/platforms, terrain/foliage, particles/VFX, cinematics/sequencing, scripting/reflection, input/replay, accessibility, localization, plugins/extensions, additional platforms, comparative performance/memory/reliability, clean-machine packaging, stress/recovery and the required 24-hour soak.

`native_evidence`: empty.

E11 final acceptance: false.

UE5/Unity parity claim: false.

## Single next action

Repair the selection-phase deadline at the asynchronous side-effect boundary in `Tests/EditorRuntimeSmoke.cpp`, run the applicable fixture and hosted verification, update the durable capability evidence to the resulting source and actual tested PR integration tree, and request fresh independent review. If and only if that exact repaired tree is clean-reviewed, hand it to the registered Windows executor for the retained native matrix and separate `AstralGame` launch. Do not repeat the already-completed prior evidence synchronization.