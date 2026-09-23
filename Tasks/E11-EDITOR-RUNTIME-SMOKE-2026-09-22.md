# E11 Editor Runtime Smoke task, 2026-09-23

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. This packet may harden the native Windows smoke, its recovery supervisor, deterministic containment tests, and evidence. It must not add scene mutation/serialization, gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, rebase, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Admitted baseline from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently observed `main`: `a356b4ac9ae30e755962a782d2fc4d74e3b5fc5e` (separate game-worker work, not absorbed here).
Previously clean-reviewed source head before this repair: `a8d1bad1135eca3ee74f689e02ee6df1672514b3`.
Positive-area implementation candidate: `2c1d2315c0e0f4bf915a3b11d27d8088f4af6429`.
Final source/evidence tree reviewed this pass: `1381f720f093b9704f2d30197e92469eee2c1b66`.
`Tests/EditorRuntimeSmoke.cpp` blob: `1458a74e15f2dcf46730c6f07a4ea7bb7343c389`.
`CMakeLists.txt` remains blob `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.

Allowed paths only: `CMakeLists.txt`, `Tests/EditorRuntimeSmoke.cpp`, this task, `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`, and `Docs/Research/ENGINE-CAPABILITIES.json`. One active writer only. Do not rebase, merge, force-push, or absorb unrelated work.

## Selected reproducible verification defect

The native smoke already required every direct editor child to retain its original HWND/class identity, stay `WS_VISIBLE`, remain inside the client rectangle after normal and narrow resizes, and preserve semantic text/state. Its containment predicate nevertheless accepted zero-width and zero-height child rectangles because it rejected only `right < left` and `bottom < top`.

That is a false-pass. Microsoft documents `IsWindowVisible` as a test of `WS_VISIBLE` state on the window and its ancestors; it does not prove useful on-screen area, and drawing can still be clipped. `GetWindowRect` returns the bounding rectangle with an exclusive lower-right edge. A required shell control with `right == left` or `bottom == top` therefore has no positive drawable extent even though the old smoke could still call it visible and contained.

The intended 420x260 narrow-window fixture is not supposed to collapse required editor surfaces. UE 5.8 documents a Level Editor composed of visible Viewport, Outliner, Details and content surfaces, and its current Viewport Toolbar explicitly handles smaller viewports with overflow management rather than treating zero-area controls as acceptable. Unity 6.0 likewise treats the Hierarchy as a visible scene-object management surface and the Inspector as the surface for the selected object's components/properties.

## Bounded implementation

Commit `2c1d2315c0e0f4bf915a3b11d27d8088f4af6429` changes only `Tests/EditorRuntimeSmoke.cpp`:

- `DirectChildrenContained` now rejects `right <= left` and `bottom <= top` for every one of the original 12 process-owned direct child controls.
- The failure message now identifies an out-of-client or empty child rectangle.
- Existing HWND/class continuity, process ownership, parent, visibility, semantic identity, selection, resize deadlines, cleanup, Job Object containment, and native acceptance requirements are unchanged.

No production editor source, CMake registration, workflow, dependency, graphics API, game content, scheduler configuration, release state, or architecture changed. The contents write also removed the file's final newline; that is recorded as non-semantic diff noise, not hidden.

## Research basis, rechecked 2026-09-23 UTC

- Microsoft Learn, `IsWindowVisible`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindowvisible
  - applicability: reports `WS_VISIBLE` state through the parent chain; it is not a positive-area or unobscured-content proof.
- Microsoft Learn, `GetWindowRect`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowrect
  - applicability: returns a window bounding rectangle; right/bottom are exclusive, so equal edges represent zero extent.
- Epic Games, Unreal Engine 5.8, `Unreal Editor Interface`: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
  - applicability: current comparison surface includes Level Viewport, Outliner, Details and content access with selection synchronization.
- Epic Games, Unreal Engine 5.8, `Viewport Toolbar`: https://dev.epicgames.com/documentation/unreal-engine/viewport-toolbar
  - applicability: UE 5.6+ toolbar includes explicit smaller-viewport overflow handling rather than silently collapsing required controls.
- Unity 6.0 Manual, `The Hierarchy window`: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html
  - applicability: Hierarchy is the visible scene-object management surface.
- Unity 6.0 Manual, `Inspect items`: https://docs.unity3d.com/6000.0/Documentation/Manual/InspectorItems.html
  - applicability: Inspector displays selected GameObject/component/material or asset state.

Behavioral/API comparison only. No proprietary engine source was copied and no dependency was added.

## Portable verification

Disposable C++17 predicate fixture SHA-256: `9a729ce88011e2f83bdc2d89795431647c445df706381d5f89cfc9115b4ef531`.

- `g++ -std=c++17 -Wall -Wextra -Werror`: PASS.
- `clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer`, leak detection enabled: PASS.
- positive in-bounds rectangles remain accepted;
- zero-width and zero-height in-bounds rectangles are accepted by the old predicate and rejected by the repaired predicate;
- malformed and out-of-bounds rectangles remain rejected.

This is source-logic evidence only. The sandbox has no Windows SDK/Win32 headers and cannot execute the production smoke. Native Windows evidence remains required.

## Final hosted PR-integration evidence for the reviewed tree

GitHub `pull_request` workflows use the synthetic PR merge with default checkout, so source association and tested integration identity are recorded separately.

- source/evidence head: `1381f720f093b9704f2d30197e92469eee2c1b66`;
- tested synthetic merge: `5d06dc61efe398add72f484efa1f1f8402042c9b`;
- tested base: `a356b4ac9ae30e755962a782d2fc4d74e3b5fc5e`;
- Windows build and deterministic tests: run `35842853870`, job `107121837092`, completed `success` at `2026-09-23T09:27:43Z`;
- profiling capture portability: run `35842853835`, completed `success`;
- release manifest integrity: run `35842853825`, completed `success`.

The Windows job passed repository/R0 safety contracts, Release assertion/CTest safety, VS2022 x64 configure, Debug build and deterministic tests, Release build and deterministic tests, dependency/prerequisite/runtime-policy checks, static milestone verifiers, and clean tracked-tree verification. This is hosted integration evidence for synthetic merge `5d06dc...`; it is not claimed as raw-head execution of `1381f720...` and it is not native interactive GUI evidence.

## Independent review state

Fresh Codex review of exact source/evidence head `1381f720f093b9704f2d30197e92469eee2c1b66` completed at `2026-09-23T09:31:04Z`. It produced one P2 evidence-traceability finding, review comment `4080955609`: the task, QA receipt, and capability map still pointed to the earlier implementation-candidate workflow set and did not record final head `1381f720...`, synthetic merge/base `5d06dc...` / `a356b4ac...`, or the final workflow set above.

This task/QA/capability update is the bounded evidence repair for that finding. The resulting post-write Git commit cannot embed its own not-yet-computed content-addressed SHA without changing that SHA again. Therefore the exact receipt-repair head and its post-write workflow/review state must be pinned in PR #13 metadata/checkpoint instead of starting an infinite evidence-only self-reference chain. Independent acceptance remains false until that repaired exact tree receives a fresh clean review.

## Retained E11 hardening and gates

1. deterministic `EditorContainmentTests` is selected by hosted suites while interactive `EditorRuntimeSmoke` remains excluded;
2. the containment self-test exercises real worker-local `CleanupProcess`, proves the descendant backstop remains, then requires supervisor whole-job cleanup to zero active processes;
3. the shared normal-success path rejects a zero-exit worker that leaves a contained descendant;
4. source SHA, tested synthetic PR merge SHA, and tested base SHA are distinct evidence identities;
5. the runtime smoke requires original HWND/class continuity, semantic Static/Button identity, `LBS_NOTIFY`, bounded cross-process reads, selection/Inspector synchronization, containment after both resizes, and now positive width/height for every required direct child.

`native_evidence` remains empty. Issue #7 is still open, so the historical R0 runner is blocked and must not be invoked.

## Registered native handoff

Only after fresh review of the receipt-repair exact tree is clean, the registered Windows executor should use one owned interactive desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact reviewed source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, normal plus 420x260 narrow-window screenshots, and process inspection proving zero owned contained processes after any failure or interruption. The screenshots and smoke output must show every required direct child has positive width and height.

## Rollback and stop conditions

Rollback only the positive-area predicate change if it rejects the approved fixture despite all required controls having positive extent. Stop before production-runtime change, workflow edit outside packet authority, rebase, merge, R0 execution, scheduler operation, dependency addition, graphics/API change, or game-content work. Never weaken a native acceptance assertion to make the gate green.

## Single next useful action

Pin the exact post-write receipt-repair head in PR #13, verify its hosted workflows, and request a fresh independent review of that exact tree. If clean, hand that exact reviewed tree to the registered Windows executor for Debug/Release containment plus interactive GUI acceptance.