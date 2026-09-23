# E11 Editor Runtime Smoke task, 2026-09-23

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. This packet may harden the native Windows smoke, its recovery supervisor, deterministic containment tests, and evidence. It must not add scene mutation/serialization, gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, rebase, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Admitted baseline from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently observed `main` before this pass: `a356b4ac9ae30e755962a782d2fc4d74e3b5fc5e` (separate game-worker work, not absorbed here).
Previously reviewed receipt head: `b4997045acf0e85c7a805d98f86657756492e3df`.
Startup-containment implementation candidate: `6f1ad24ab6b394bd69bebc940fde2ddf5fe8eefb`.
`Tests/EditorRuntimeSmoke.cpp` blob after the repair: `b9985b3a33abfd558545dfcebc1437f18c91d5d5`.
`CMakeLists.txt` remains blob `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.

Allowed paths only: `CMakeLists.txt`, `Tests/EditorRuntimeSmoke.cpp`, this task, `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`, and `Docs/Research/ENGINE-CAPABILITIES.json`. One active writer only. Do not rebase, merge, force-push, or absorb unrelated work.

## Selected reproducible verification defect

The smoke already verifies the original 12 direct child HWND/class identities, semantic shell state, process ownership, visibility, positive area, client containment, and exact state after the explicit 800x600 and 420x260 resizes. It did not run `DirectChildrenContained` on the default startup layout before the first `SetWindowPos`.

That leaves a false-pass path: a required editor surface can start outside the client rectangle or with zero/non-positive extent, then a later `WM_SIZE` caused by the test's first resize can repair the layout. The old sequence would accept the repaired post-resize state and never prove that the editor's actual default startup layout was usable.

Microsoft documents that `GetClientRect` returns the current client width/height with exclusive lower-right coordinates, while `GetWindowRect` returns each child bounding rectangle with the same exclusive right/bottom convention. The existing containment predicate is therefore suitable for the startup gate too. Epic UE 5.8 and Unity 6.0 document their viewport/outliner/details or hierarchy/inspector surfaces as immediately usable editor surfaces, so an editor-parity smoke should not allow an invalid initial shell merely because a later synthetic resize repairs it.

## Bounded implementation

Commit `6f1ad24ab6b394bd69bebc940fde2ddf5fe8eefb` changes only `Tests/EditorRuntimeSmoke.cpp`:

- after initial semantic `ValidateShellState` and before any selection or resize, the real worker now calls `DirectChildrenContained` on the captured original control inventory;
- failure at startup therefore enters the existing owned-process cleanup path before any test resize can normalize the editor layout;
- the PASS receipt now says the children remained contained from startup through both resizes.

The commit diff is intentionally 4 additions / 1 deletion. No production editor source, CMake registration, workflow, dependency, graphics API, game content, scheduler configuration, release state, or architecture changed.

## Research basis, rechecked 2026-09-23 UTC

- Microsoft Learn, `GetClientRect`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getclientrect
  - applicability: returns the window client area with `(0,0)` upper-left and width/height in right/bottom; lower-right is exclusive.
- Microsoft Learn, `GetWindowRect`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowrect
  - applicability: returns the child bounding rectangle in screen coordinates; lower-right is exclusive and can be mapped into the parent client space.
- Epic Games, Unreal Engine 5.8, `Unreal Editor Interface`: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
  - applicability: the Level Viewport, Outliner, Details panel and content access are normal editor surfaces, with selection synchronization between viewport/outliner/details.
- Epic Games, Unreal Engine 5.8, `Viewport Toolbar`: https://dev.epicgames.com/documentation/unreal-engine/viewport-toolbar
  - applicability: current UE toolbar explicitly manages smaller-viewport overflow instead of treating collapsed controls as acceptable.
- Unity 6.0 Manual, `The Hierarchy window`: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html
  - applicability: Hierarchy views and manages all objects in the scene.
- Unity 6.0 Manual, `Inspect items`: https://docs.unity3d.com/6000.0/Documentation/Manual/InspectorItems.html
  - applicability: Inspector is the selected-object/component/property surface.

Behavioral/API comparison only. No proprietary engine source was copied and no dependency was added.

## Portable verification

Disposable C++17 startup-sequence fixture SHA-256: `18f7bc0e6e099f2fef439f79c024e0fd4a87a6c765296f34e7eb0776089068af`.

- `g++ -std=c++17 -Wall -Wextra -Werror`: PASS.
- `clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer`, leak detection enabled: PASS.
- fixture proves the old sequence can pass when startup has a zero-width required child but the first resize repairs it;
- repaired sequence rejects that startup state before the resize;
- positive-control startup and resized layouts remain accepted.

This is source-logic evidence only. The sandbox cannot resolve `github.com` for a checkout and has no Windows SDK/interactive desktop, so no sandbox Win32 build or GUI execution is claimed.

## Hosted verification state

The implementation write triggered fresh pull-request workflows for source candidate `6f1ad24ab6b394bd69bebc940fde2ddf5fe8eefb`:

- Windows build and deterministic tests: run `35848735600`, observed `in_progress` during this record write;
- profiling capture portability: run `35848735553`, observed `in_progress`;
- release manifest integrity: run `35848735669`, observed `in_progress`.

Do not count those runs as passed until their final conclusions are read. Because these are `pull_request` workflows with default checkout, final evidence must record the source head separately from GitHub's synthetic PR merge and tested base.

## Retained E11 hardening and gates

1. deterministic `EditorContainmentTests` is selected by hosted suites while interactive `EditorRuntimeSmoke` remains excluded;
2. the containment self-test exercises real worker-local `CleanupProcess`, proves the descendant backstop remains, then requires supervisor whole-job cleanup to zero active processes;
3. the shared normal-success path rejects a zero-exit worker that leaves a contained descendant;
4. source SHA, tested synthetic PR merge SHA, and tested base SHA are distinct evidence identities;
5. the runtime smoke requires original HWND/class continuity, semantic Static/Button identity, `LBS_NOTIFY`, bounded cross-process reads, selection/Inspector synchronization, stable single top-level-window identity, positive width/height and client containment at startup, and the same invariants after normal and narrow resizes.

`native_evidence` remains empty. Independent acceptance for this changed source is false until fresh review completes. Issue #7 remains open, so the historical R0 runner is blocked and must not be invoked.

## Registered native handoff

Only after this exact changed source/receipt tree has green hosted checks and a fresh clean independent review, the registered Windows executor should use one owned interactive desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact reviewed source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, default-startup plus normal and 420x260 narrow-window screenshots, and process inspection proving zero owned contained processes after any failure or interruption. The default-startup screenshot/output must establish that every required direct child already has positive area and lies inside the editor client before the smoke changes the top-level size.

## Rollback and stop conditions

Rollback only the startup containment call if the approved default fixture has valid required surfaces but the predicate demonstrably misclassifies their mapped Win32 geometry. Stop before production-runtime change, workflow edit outside packet authority, rebase, merge, R0 execution, scheduler operation, dependency addition, graphics/API change, or game-content work. Never weaken a native acceptance assertion to make the gate green.

## Single next useful action

Finish the fresh hosted checks for the changed tree, record the tested synthetic merge/base identity, and obtain a fresh independent review. If clean, hand that exact reviewed tree to the registered Windows executor for Debug/Release containment plus interactive GUI acceptance.