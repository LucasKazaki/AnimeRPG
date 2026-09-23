# E11 Editor Runtime Smoke evidence, 2026-09-23

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. The selected defect was a native-smoke false-pass: a direct editor child with zero width or zero height could still satisfy the old containment predicate if it retained `WS_VISIBLE` and stayed numerically inside the client rectangle.

Admitted baseline from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently observed `main`: `a356b4ac9ae30e755962a782d2fc4d74e3b5fc5e`.
Previously clean-reviewed source head before this repair: `a8d1bad1135eca3ee74f689e02ee6df1672514b3`.
Positive-area implementation candidate: `2c1d2315c0e0f4bf915a3b11d27d8088f4af6429`.
Final source/evidence tree reviewed this pass: `1381f720f093b9704f2d30197e92469eee2c1b66`.
`Tests/EditorRuntimeSmoke.cpp` blob: `1458a74e15f2dcf46730c6f07a4ea7bb7343c389`.
`CMakeLists.txt` remains blob `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.
Production editor source is unchanged.

## Reproduced defect and repair

Before `2c1d2315...`, `DirectChildrenContained` rejected inverted child geometry with `right < left` or `bottom < top`, but accepted equality. A required control could therefore have a mapped client rectangle with `right == left` or `bottom == top` and still pass the containment stage.

The repair changes those two comparisons to `<=`, so all 12 original process-owned direct child HWNDs must retain positive width and positive height after the 800x600 and 420x260 resize checks. The failure diagnostic now says `child outside client or empty`. No other runtime-smoke assertion was weakened or broadened. GitHub commit inspection also shows the file lost its final newline in the contents write; that is non-semantic diff noise and is retained as an explicit receipt fact.

## Primary research

Accessed 2026-09-23 UTC:

- Microsoft Learn, `IsWindowVisible`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindowvisible
  - reports `WS_VISIBLE` state through the parent chain; a nonzero result does not prove useful rendered area.
- Microsoft Learn, `GetWindowRect`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowrect
  - returns a bounding rectangle with exclusive right/bottom edges; equal edges have zero extent.
- Epic Games, Unreal Engine 5.8, `Unreal Editor Interface`: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
  - Level Viewport, Outliner, Details and content access are first-class visible editor surfaces with selection synchronization.
- Epic Games, Unreal Engine 5.8, `Viewport Toolbar`: https://dev.epicgames.com/documentation/unreal-engine/viewport-toolbar
  - current toolbar behavior includes explicit overflow handling for smaller viewports instead of accepting collapsed required controls.
- Unity 6.0 Manual, `The Hierarchy window`: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html
  - Hierarchy is the scene-object management surface.
- Unity 6.0 Manual, `Inspect items`: https://docs.unity3d.com/6000.0/Documentation/Manual/InspectorItems.html
  - Inspector presents selected GameObject/component/material or asset state.

Behavioral/API comparison only. No proprietary UE/Unity source was copied and no dependency was imported.

## Portable regression evidence

Disposable fixture SHA-256: `9a729ce88011e2f83bdc2d89795431647c445df706381d5f89cfc9115b4ef531`.

```text
g++ -std=c++17 -Wall -Wextra -Werror e11_positive_area_fixture.cpp -o e11_positive_area_fixture_gcc
./e11_positive_area_fixture_gcc
# positive-area containment fixture: PASS

clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer e11_positive_area_fixture.cpp -o e11_positive_area_fixture_clang
ASAN_OPTIONS=detect_leaks=1 ./e11_positive_area_fixture_clang
# positive-area containment fixture: PASS
```

Cases covered: positive in-bounds accepted; zero-width and zero-height accepted by the old rule but rejected by the repaired rule; malformed/inverted/out-of-client rejected. This models only the changed geometry predicate. It is not Win32/runtime evidence. The sandbox has no Windows SDK/Win32 headers, so no production Windows compile or GUI run is claimed.

## Final hosted PR-integration evidence

The exact source/evidence tree under independent review was `1381f720f093b9704f2d30197e92469eee2c1b66`. GitHub `pull_request` workflows with default checkout tested its synthetic merge, not the raw source commit:

- source/evidence head: `1381f720f093b9704f2d30197e92469eee2c1b66`;
- tested synthetic merge: `5d06dc61efe398add72f484efa1f1f8402042c9b`;
- tested base: `a356b4ac9ae30e755962a782d2fc4d74e3b5fc5e`;
- Windows build and deterministic tests: run `35842853870`, job `107121837092`, `completed/success`, completed `2026-09-23T09:27:43Z`;
- profiling capture portability: run `35842853835`, `completed/success`;
- release manifest integrity: run `35842853825`, `completed/success`.

The Windows job passed repository/R0 safety contracts, Release assertion/CTest safety, VS2022 x64 configuration, Debug build/tests, Release build/tests, runtime dependency and prerequisite policy checks, static milestone verifiers, and clean tracked-tree verification. These results are PR-integration evidence for `5d06dc...`, associated with source `1381f720...`; they are not raw-head execution evidence and do not substitute for interactive native GUI acceptance.

## Independent review and evidence repair

Fresh Codex review of exact source/evidence head `1381f720f093b9704f2d30197e92469eee2c1b66` completed at `2026-09-23T09:31:04Z`. It produced one P2 evidence-only finding, review comment `4080955609`: the durable task, QA receipt, and capability map still pointed to the earlier `2c1d2315...` workflow set and did not record final head `1381f720...`, synthetic merge/base `5d06dc...` / `a356b4ac...`, or the final successful workflow set.

This QA/task/capability update is the repair for that traceability finding. It does not change runtime code. The resulting receipt-repair commit cannot include its own content-addressed SHA without changing that SHA; the exact post-write head and its workflow/re-review status therefore belong in PR #13 metadata/checkpoint instead of creating an infinite self-referential evidence chain. Independent final acceptance remains false until that repaired exact tree receives a fresh clean review.

## Retained hardening

- deterministic `EditorContainmentTests` is selected by hosted suites while interactive `EditorRuntimeSmoke` remains excluded;
- deterministic containment executes the real worker-local `CleanupProcess`, requires forced exit code `2`, proves a descendant remains, then requires supervisor whole-job cleanup to zero active processes;
- the shared normal-success supervisor path rejects a zero-exit worker that leaves a contained descendant;
- every cross-process shell read uses bounded messages and identity/ownership checks;
- original shell HWND/class identity, semantic Static/Button bindings, Outliner rows and selection, complete Inspector fixtures, `LBS_NOTIFY`, normal/narrow resize state, owned cleanup, and positive width/height for all required direct children remain required.

## Deferred native acceptance

`native_evidence` remains empty. The registered interactive Windows executor must still run both `EditorContainmentTests` and `EditorRuntimeSmoke` in Debug and Release on one owned desktop and retain:

- exact freshly reviewed source SHA;
- Windows/machine identity;
- MSVC and CMake versions;
- GPU and driver identity;
- exact commands, complete stdout/stderr, exit codes, UTC timestamps;
- normal and 420x260 narrow-window screenshots;
- process inspection proving zero owned contained processes after any failure/interruption;
- evidence that every required direct child retains positive width and positive height.

Issue #7 remains open. The historical R0 runner was not invoked. Clean-machine packaging, comparative frame/RAM/VRAM evidence, broader stress/recovery, and the required 24-hour soak remain outside this bounded E11 repair and unresolved.

## Acceptance state

Portable predicate regression under GCC: PASS.
Portable predicate regression under Clang ASan+UBSan: PASS.
Implementation candidate `2c1d2315...`: implemented.
Final reviewed source/evidence head `1381f720...`: hosted PR-integration Windows/profiling/release-manifest checks PASS.
Independent review of `1381f720...`: completed with one P2 evidence-traceability finding, repaired by this evidence-only update.
Fresh review of the receipt-repair exact tree: pending until requested after the write.
Native interactive Windows: deferred to registered local executor.
Independent final acceptance: false.
UE5/Unity parity claim: false.

## Single next action

Pin the exact receipt-repair head and its hosted checks in PR #13, request fresh independent review of that exact tree, and if clean hand it to the registered Windows executor for the Debug/Release containment and interactive GUI acceptance packet.