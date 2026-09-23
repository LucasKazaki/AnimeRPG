# E11 Editor Runtime Smoke evidence, 2026-09-23

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. This pass found a second geometry-sequencing false-pass in the native smoke: the required 12-child containment/positive-area predicate ran after the explicit 800x600 and 420x260 resizes, but not on the editor's default startup layout. A startup-only geometry failure could therefore be repaired by the first test resize and escape detection.

Admitted baseline from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently observed `main` before this pass: `a356b4ac9ae30e755962a782d2fc4d74e3b5fc5e`.
Previously reviewed receipt head: `b4997045acf0e85c7a805d98f86657756492e3df`.
Startup-containment implementation candidate: `6f1ad24ab6b394bd69bebc940fde2ddf5fe8eefb`.
`Tests/EditorRuntimeSmoke.cpp` blob: `b9985b3a33abfd558545dfcebc1437f18c91d5d5`.
`CMakeLists.txt` remains blob `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.
Production editor source is unchanged.

## Reproduced defect and repair

Before `6f1ad24...`, the initial sequence captured the 12 original child controls and validated their semantic state, then selected Cube, then invoked `ResizeAndCheck`. `DirectChildrenContained` was called only from `ResizeAndCheck`. A required control could therefore start outside the client area or with zero/non-positive extent and still pass if the first `SetWindowPos` generated a `WM_SIZE` that repaired the layout.

Commit `6f1ad24ab6b394bd69bebc940fde2ddf5fe8eefb` adds a `DirectChildrenContained(...)` call immediately after the initial `ValidateShellState(...)` and before selection or any resize. The existing predicate already enforces original HWND/class continuity, process ownership, direct parent, visibility, in-client geometry, positive width and positive height. Failure now reaches the existing owned-process cleanup path before any synthetic resize can normalize the shell. The PASS text was updated to state that containment held from startup through both resizes.

GitHub commit inspection shows exactly 4 additions and 1 deletion, all in `Tests/EditorRuntimeSmoke.cpp`. No production source, CMake, workflow, dependency, graphics API, game content, scheduler configuration, release state, or architecture changed.

## Primary research

Accessed 2026-09-23 UTC:

- Microsoft Learn, `GetClientRect`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getclientrect
  - returns the current client rectangle with `(0,0)` upper-left, width/height in right/bottom, and an exclusive lower-right edge.
- Microsoft Learn, `GetWindowRect`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowrect
  - returns a window bounding rectangle in screen coordinates, also using exclusive right/bottom coordinates.
- Epic Games, Unreal Engine 5.8, `Unreal Editor Interface`: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
  - the normal editor exposes a Level Viewport, Outliner, Details panel and content access as usable surfaces and synchronizes selection between editor surfaces.
- Epic Games, Unreal Engine 5.8, `Viewport Toolbar`: https://dev.epicgames.com/documentation/unreal-engine/viewport-toolbar
  - current smaller-viewport behavior explicitly manages toolbar overflow rather than accepting collapsed required controls.
- Unity 6.0 Manual, `The Hierarchy window`: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html
  - the Hierarchy views and manages all scene objects.
- Unity 6.0 Manual, `Inspect items`: https://docs.unity3d.com/6000.0/Documentation/Manual/InspectorItems.html
  - the Inspector is the selected-object/component/property surface.

Behavioral/API comparison only. No proprietary UE/Unity source was copied and no dependency was imported.

## Portable regression evidence

Disposable fixture SHA-256: `18f7bc0e6e099f2fef439f79c024e0fd4a87a6c765296f34e7eb0776089068af`.

```text
g++ -std=c++17 -Wall -Wextra -Werror /tmp/e11_startup_containment_fixture.cpp -o /tmp/e11_startup_containment_gcc
/tmp/e11_startup_containment_gcc
# exit 0

clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /tmp/e11_startup_containment_fixture.cpp -o /tmp/e11_startup_containment_clang
ASAN_OPTIONS=detect_leaks=1 /tmp/e11_startup_containment_clang
# exit 0
```

The fixture models a 12-control startup shell with one zero-width required child and a later resized shell in which all controls are valid. It asserts that the old sequence, which checks only after the resize, passes; the repaired sequence, which validates startup first, rejects; and a fully valid startup/resized pair remains accepted. This is source-logic evidence only, not Win32 runtime evidence.

Sandbox repository checkout was also attempted with `git clone --filter=blob:none --no-checkout https://github.com/LucasKazaki/AnimeRPG.git`; it failed before checkout with `Could not resolve host: github.com`. The sandbox has no usable Windows interactive desktop/SDK path here, so no production Win32 compile or GUI result is claimed.

## Hosted verification state

The implementation commit triggered these pull-request workflows:

- Windows build and deterministic tests: run `35848735600`, observed `in_progress` during the record write;
- profiling capture portability: run `35848735553`, observed `in_progress`;
- release manifest integrity: run `35848735669`, observed `in_progress`.

They are attempted, not passed, until final conclusions are retrieved. These workflows use GitHub's pull-request integration ref with default checkout, so later evidence must preserve source SHA, synthetic merge SHA, and tested base SHA separately.

## Retained hardening

- `EditorContainmentTests` remains selected by hosted deterministic suites while interactive `EditorRuntimeSmoke` remains excluded;
- deterministic containment executes the real worker-local `CleanupProcess`, verifies forced direct exit code 2, preserves a descendant long enough to exercise the supervisor backstop, and requires zero active processes afterward;
- the shared normal-success path rejects a zero-exit worker that leaves a contained descendant;
- every cross-process shell read is bounded and revalidates expected ownership/identity;
- original shell HWND/class identity, semantic Static/Button bindings, Outliner rows and selection, complete Inspector fixtures, `LBS_NOTIFY`, stable single top-level-window identity, owned cleanup, and positive width/height are retained;
- containment/positive-area validation now covers default startup before any resize as well as both normal and narrow resize states.

## Deferred native acceptance

`native_evidence` remains empty. The registered Windows executor must still run `EditorContainmentTests` and interactive `EditorRuntimeSmoke` in Debug and Release on one owned desktop after the changed tree has green hosted checks and fresh independent review. Retain:

- exact reviewed source SHA;
- Windows/machine identity;
- MSVC and CMake versions;
- GPU and driver identity;
- exact commands, complete stdout/stderr, exit codes, UTC timestamps;
- default-startup, normal-size, and 420x260 narrow-window screenshots;
- process inspection proving zero owned contained processes after any failure/interruption;
- proof that every required direct child already has positive width/height and lies inside the client before the smoke changes the top-level window size.

Issue #7 remains open. The historical R0 runner was not invoked. Clean-machine packaging, comparative frame/RAM/VRAM evidence, broader stress/recovery, and the required 24-hour soak remain outside this E11 repair and unresolved.

## Acceptance state

Portable startup-sequence regression under GCC: PASS.
Portable startup-sequence regression under Clang ASan+UBSan: PASS.
Implementation candidate `6f1ad24...`: implemented; exact diff verified.
Hosted PR-integration checks for changed source: in progress at this record write.
Fresh independent review of changed source: pending.
Native interactive Windows: deferred to registered local executor.
Independent final acceptance: false.
UE5/Unity parity claim: false.

## Single next action

Read the changed tree's hosted workflow conclusions and exact synthetic merge/base identities, then request a fresh independent review. If both gates are clean, hand that exact reviewed tree to the registered Windows executor for the Debug/Release containment and interactive GUI acceptance packet.