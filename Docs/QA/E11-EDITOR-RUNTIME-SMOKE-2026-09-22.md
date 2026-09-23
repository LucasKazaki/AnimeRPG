# E11 Editor Runtime Smoke evidence, 2026-09-23

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. The selected defect was a native-smoke false-pass: a direct editor child with zero width or zero height could still satisfy the old containment predicate if it retained `WS_VISIBLE` and stayed numerically inside the client rectangle.

Admitted baseline from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently observed `main`: `a356b4ac9ae30e755962a782d2fc4d74e3b5fc5e`.
Previously clean-reviewed source head: `a8d1bad1135eca3ee74f689e02ee6df1672514b3`.
Positive-area implementation candidate: `2c1d2315c0e0f4bf915a3b11d27d8088f4af6429`.
Current task-record commit after the implementation: `b1f82acdeb49841812ad329173413f0ec95ebb23`.
`Tests/EditorRuntimeSmoke.cpp` blob: `1458a74e15f2dcf46730c6f07a4ea7bb7343c389`.
`CMakeLists.txt` remains blob `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.
Production editor source is unchanged.

## Reproduced defect and repair

Before `2c1d2315...`, `DirectChildrenContained` rejected inverted child geometry with `right < left` or `bottom < top`, but accepted equality. A required control could therefore have a mapped client rectangle with `right == left` or `bottom == top` and still pass the containment stage.

The repair changes those two comparisons to `<=`, so all 12 original process-owned direct child HWNDs must retain positive width and positive height after the 800x600 and 420x260 resize checks. The failure diagnostic now says `child outside client or empty`. No other runtime-smoke assertion was weakened or broadened.

GitHub commit inspection of `2c1d2315...` confirms the semantic code diff is limited to those comparisons and that diagnostic string. The contents write also removed the file's final newline; this is recorded as non-semantic diff noise rather than omitted from the receipt.

## Primary research

Accessed 2026-09-23 UTC:

- Microsoft Learn, `IsWindowVisible`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindowvisible
  - `IsWindowVisible` reports `WS_VISIBLE` state through the parent chain. A nonzero result does not prove useful rendered area.
- Microsoft Learn, `GetWindowRect`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowrect
  - returns a bounding rectangle with exclusive right/bottom edges; equal edges have zero extent.
- Epic Games, Unreal Engine 5.8, `Unreal Editor Interface`: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
  - Level Viewport, Outliner, Details and content access are first-class visible editor surfaces, with selection synchronization.
- Epic Games, Unreal Engine 5.8, `Viewport Toolbar`: https://dev.epicgames.com/documentation/unreal-engine/viewport-toolbar
  - the UE 5.6+ toolbar has explicit overflow behavior for smaller viewports rather than treating collapsed tools as valid presentation.
- Unity 6.0 Manual, `The Hierarchy window`: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html
  - Hierarchy is the scene-object management surface.
- Unity 6.0 Manual, `Inspect items`: https://docs.unity3d.com/6000.0/Documentation/Manual/InspectorItems.html
  - Inspector presents selected GameObject/component/material or asset state.

These are behavioral/API comparison references only. No proprietary UE/Unity source was copied and no dependency was imported.

## Portable regression evidence

Disposable fixture SHA-256:

`9a729ce88011e2f83bdc2d89795431647c445df706381d5f89cfc9115b4ef531`

Executed in the coordinator sandbox:

```text
g++ -std=c++17 -Wall -Wextra -Werror e11_positive_area_fixture.cpp -o e11_positive_area_fixture_gcc
./e11_positive_area_fixture_gcc
# positive-area containment fixture: PASS

clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer e11_positive_area_fixture.cpp -o e11_positive_area_fixture_clang
ASAN_OPTIONS=detect_leaks=1 ./e11_positive_area_fixture_clang
# positive-area containment fixture: PASS
```

Fixture cases:

- positive in-bounds rectangles: accepted by repaired rule;
- zero-width in-bounds rectangle: old rule accepts, repaired rule rejects;
- zero-height in-bounds rectangle: old rule accepts, repaired rule rejects;
- negative/inverted or out-of-client rectangles: repaired rule rejects.

This fixture models only the changed geometry predicate. It is not a Win32/runtime substitute. The sandbox has no Windows SDK/Win32 headers, so no production Windows compile or GUI run is claimed.

## Hosted verification state

Source candidate `2c1d2315...` triggered:

- Windows build and deterministic tests: run `35842586238`;
- profiling capture portability: run `35842586031`;
- release manifest integrity: run `35842586139`.

At the first observation after the source write, all three were `in_progress`. No pass is claimed yet. For the Windows `pull_request` run, default checkout tests the synthetic PR merge ref, so final evidence must record the source head separately from the synthetic tested merge SHA and its tested base.

The earlier clean Codex review of `a8d1bad...` does not cover this source change. Fresh independent review of the final E11 source/evidence tree is required before independent acceptance.

## Retained hardening

The current packet still includes the earlier verified controls:

- `EditorContainmentTests` does not match hosted `-E "RuntimeSmoke"` and is therefore selected by deterministic suites;
- deterministic containment executes the real worker-local `CleanupProcess`, requires forced exit code `2`, proves a descendant remains, then requires supervisor whole-job cleanup to zero active processes;
- the shared normal-success supervisor path rejects a zero-exit worker that leaves a contained descendant;
- every cross-process shell read uses bounded messages and identity/ownership checks;
- original shell HWND/class identity, semantic Static/Button bindings, Outliner rows and selection, complete Inspector fixtures, `LBS_NOTIFY`, normal/narrow resize state, and owned cleanup remain required.

## Deferred native acceptance

`native_evidence` remains empty. The registered interactive Windows executor must still run both `EditorContainmentTests` and `EditorRuntimeSmoke` in Debug and Release on one owned desktop and retain:

- exact reviewed source SHA;
- Windows/machine identity;
- MSVC and CMake versions;
- GPU and driver identity;
- exact commands, complete stdout/stderr, exit codes, UTC timestamps;
- normal and 420x260 narrow-window screenshots;
- process inspection proving zero owned contained processes after any failure/interruption;
- evidence that every required direct child retains positive width and positive height.

Issue #7 remains open. The historical R0 runner was not invoked. Clean-machine packaging, comparative frame/RAM/VRAM evidence, broader stress/recovery, and the required 24-hour soak remain outside this bounded E11 repair and unresolved.

## Acceptance state

Attempted: portable predicate regression under GCC and Clang sanitizers, PASS.
Implemented: positive-area requirement in production `EditorRuntimeSmoke`, commit `2c1d2315...`.
Hosted CI: in progress at first observation, not yet accepted.
Native interactive Windows: deferred to registered local executor.
Independent review of changed source: pending.
Independent final acceptance: false.
UE5/Unity parity claim: false.

## Single next action

When the already-triggered workflows finish, pin the Windows run's synthetic tested merge/base and investigate any failure before expanding scope. Then request fresh independent review of the exact final E11 source/evidence tree. Only after both are clean should the registered Windows executor run the Debug/Release containment and interactive GUI acceptance packet.