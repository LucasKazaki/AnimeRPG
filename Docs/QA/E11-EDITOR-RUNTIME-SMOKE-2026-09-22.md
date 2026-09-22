# E11 Editor Runtime Smoke evidence, 2026-09-22

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. The packet verifies the already-integrated Win32 editor shell and does not authorize scene mutation/serialization, transform gizmos, Play-in-Editor, asset import, graphics/API changes, dependencies, game content, scheduler operations, deployment, release, or R0 execution.

Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Current independently moving `main` re-read during this pass: `1ac6bf8d54219968effadd17e575aafb4ebd3847`. No rebase, merge, force push or unrelated game-work absorption was performed.

Current code candidate: `b28b8cc079c92b925b3f59c223e67651a238d276`.
`Tests/EditorRuntimeSmoke.cpp` blob: `6e8ab5a04e87d7b41d2cf1abf68803c79881e3c9`.
Integrated editor source fixture: `Tools/AstralEditorMain.cpp` blob `5142e632a79c89d0d0ce3efe87e752456f802185`; not modified by this packet.

## Finding repaired

The prior smoke checked exact list contents, control counts and shell text, but did not fully establish that the Outliner and Assets handles returned by `GetDlgItem` were currently visible, exact-class, process-owned direct children before the first list-box reads. The cached Inspector handle was identified from the initial child snapshot and visibility-checked, but it was not revalidated for process ownership, direct parent, class and visibility immediately before the post-notification Inspector read. A hidden, misbound, stale, or recycled child HWND could therefore weaken the shell-surface contract while the fixture values still appeared correct.

Candidate `b28b8cc...` adds two fail-closed helpers:

- `DirectVisibleChildOwnedByProcessAndParent`: launched-process ownership for editor and child, exact direct parent, expected Win32 class and visible state.
- `DirectVisibleControlOwnedByProcessAndParent`: all of the above plus exact `GetDlgItem(parent, controlId)` identity.

The initial surface gate now requires Outliner ID 1001 and Assets ID 1002 to be visible process-owned direct `ListBox` children, and Inspector to be a visible process-owned direct `Static`. The Outliner guard is repeated before selection, notification and post-notification reads. The Inspector guard is repeated before the Cube Inspector read. No production editor source, dependency, timeout, graphics API, or test registration was changed.

## Primary-source research, rechecked 2026-09-22

References are behavioral/API references only. No proprietary source, artwork, asset or dependency was copied/imported.

- Epic Games, Unreal Engine 5.8, Unreal Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
  - Distinguishes Level Viewport, Outliner, Details panel and Content Drawer as named editor surfaces and documents Outliner/Details selection behavior.
- Epic Games, Unreal Engine 5.8, Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
  - Defines Outliner as a hierarchical actor-selection surface.
- Epic Games, Unreal Engine 5.8, Content Browser: https://dev.epicgames.com/documentation/en-us/unreal-engine/content-browser-in-unreal-engine
  - Defines Content Browser as the primary asset management/view surface. Astral's procedural four-row list remains only a bounded verification fixture.
- Microsoft `GetDlgItem`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getdlgitem
  - Supports identifying the unique child for a parent/control-ID pair.
- Microsoft `GetWindowThreadProcessId`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowthreadprocessid
  - Supports checking which process created a window and fails with zero for an invalid HWND.
- Microsoft `GetParent`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getparent
  - Supports validating the expected parent/owner relationship.
- Microsoft `IsWindowVisible`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindowvisible
  - Checks `WS_VISIBLE` along the ancestor chain.

Access date: 2026-09-22.

## Portable reproduction

Disposable C++17 fixture SHA-256:
`641888fe4da9a018e0116a76990ce81b10246edfc6a42cd2a6dafc70a1b3e161`

Commands executed in the coordinator sandbox:

```bash
g++ -std=c++17 -Wall -Wextra -Werror -pedantic /tmp/e11_surface_identity_fixture.cpp -o /tmp/e11_surface_identity_fixture
/tmp/e11_surface_identity_fixture
clang++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer /tmp/e11_surface_identity_fixture.cpp -o /tmp/e11_surface_identity_fixture_san
ASAN_OPTIONS=detect_leaks=1 /tmp/e11_surface_identity_fixture_san
sha256sum /tmp/e11_surface_identity_fixture.cpp
```

Results:

- GCC warning-clean compile and execution: PASS, printed `surface identity/visibility contract fixture: PASS`.
- Clang ASan+UBSan warning-clean compile and execution with leak detection: PASS, printed the same marker.
- Fixture hash: `641888fe4da9a018e0116a76990ce81b10246edfc6a42cd2a6dafc70a1b3e161`.

This fixture proves only the portable guard model. It is not production Win32/native editor execution.

## Hosted exact-candidate evidence

Exact code candidate `b28b8cc079c92b925b3f59c223e67651a238d276` passed Windows Server 2022 run `35743225947`, job `106798072594`. Successful steps included checkout/external build root, R0 parser-only and runner-safety contracts, PE/prerequisite/runtime-policy contracts, Release assertion and CTest safety contracts, VS2022 x64 configure, MSVC Debug build and deterministic Debug tests, MSVC Release build and deterministic Release tests, Release dependency/prerequisite checks, static milestone verifiers and clean tracked-tree verification. The historical R0 runner itself was not executed.

Additional exact-candidate checks:

- Profiling capture portability run `35743225660`: PASS.
- Release manifest integrity run `35743225770`: PASS.

Hosted deterministic CTest intentionally excludes every `RuntimeSmoke`. These hosted results therefore prove compilation and deterministic non-runtime regression status, not native editor GUI execution.

## Independent review state

The prior evidence head `3385769790f8c428c2f84de85c522138b9cae189` completed its narrow Codex evidence-consistency recheck at `2026-09-22T14:46:17Z` with no new reported finding. That does not review this pass's new runtime-smoke code.

A fresh independent review of candidate `b28b8cc...` plus its evidence is required. Same-author inspection is not counted as independent acceptance.

## Native evidence and handoff

`native_evidence` remains empty. The coordinator did not access or claim a registered Windows interactive desktop.

Run exact code candidate `b28b8cc079c92b925b3f59c223e67651a238d276` on one owned interactive Windows desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain source SHA, machine/Windows identity, MSVC and CMake versions, GPU/driver identity, exact commands, full stdout/stderr, exit codes, UTC timestamps and screenshots at normal and narrow/short sizes. Human-visible acceptance must confirm exact `OUTLINER`, `INSPECTOR`, and `ASSETS / DEFAULT PRIMITIVES` labels; exact four asset rows; truthful E11 status text; Outliner visibly remains on Cube while Inspector shows Cube; viewport `Selected:` text reflects Cube; and panels do not bleed during resize.

## Result

Status: **shell-surface handle identity/ownership/parent/class/visibility verification is hardened; portable warning-clean and sanitizer fixtures pass; exact-candidate Windows Debug/Release, profiling and release-manifest workflows pass; fresh independent review and native Debug/Release RuntimeSmoke remain pending**.

E11 remains a partial editor-shell candidate, not UE5/Unity parity. No native GUI, GPU/performance, clean-machine, stress/recovery, soak, or final independent runtime acceptance claim is made. Issue #7 remains separate and open, and R0 was not invoked.

Single next useful action: obtain fresh independent review of the exact code/evidence state, then execute Debug and Release `EditorRuntimeSmoke` on the registered Windows desktop with the required receipts/screenshots.
