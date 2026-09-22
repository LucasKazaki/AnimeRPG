# E11 Editor Runtime Smoke task, 2026-09-22

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. This packet may harden the native Windows smoke and its evidence, but it must not add scene mutation/serialization, transform gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Current independently moving `main` re-read in this pass: `1ac6bf8d54219968effadd17e575aafb4ebd3847`; do not rebase, merge, or absorb unrelated game-worker work in this packet.
Current code candidate: `b28b8cc079c92b925b3f59c223e67651a238d276`.
Current smoke blob: `6e8ab5a04e87d7b41d2cf1abf68803c79881e3c9`.
Integrated editor source fixture blob: `Tools/AstralEditorMain.cpp` `5142e632a79c89d0d0ce3efe87e752456f802185`; that production editor file is not owned by this packet and was not modified.

Allowed paths only:

- `CMakeLists.txt`
- `Tests/EditorRuntimeSmoke.cpp`
- `Tasks/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`
- `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`
- `Docs/Research/ENGINE-CAPABILITIES.json`

One active writer only. No force push, destructive cleanup, merge, release, deployment, dependency import, repository-permission change, architecture/graphics-API change, or Company Runtime mutation.

## Selected false-pass and repair

The prior smoke proved exact list contents and static labels, but its initial `GetDlgItem` handles for the Outliner and Assets surfaces were not required to be visible, class-correct, process-owned direct children before the first cross-process list-box reads. The cached Inspector handle was found from the initial child snapshot and visibility-checked, but was not revalidated for current process ownership, direct parent, class, and visibility immediately before the post-notification text read. A hidden, misbound, stale, or recycled control handle could therefore weaken the claimed shell-surface evidence even though the total child counts and text fixtures looked correct.

Candidate `b28b8cc079c92b925b3f59c223e67651a238d276` adds fail-closed surface validation:

- `DirectVisibleChildOwnedByProcessAndParent` requires current parent/process ownership, direct parent identity, expected Win32 class, and `IsWindowVisible`.
- `DirectVisibleControlOwnedByProcessAndParent` additionally requires `GetDlgItem(parent, expectedControlId)` to resolve to the same handle.
- Outliner must be the visible process-owned direct `ListBox` at ID 1001 before initial reads and before selection, notification, and post-notification reads.
- Assets must be the visible process-owned direct `ListBox` at ID 1002 before initial reads.
- Inspector must be a visible process-owned direct `Static` before initial fixture reads and again before the post-notification Cube Inspector read.

No editor implementation, dependency, graphics API, test timeout, or execution authority changed.

## Acceptance contract

The smoke may report PASS only when all of the following hold:

1. The same visible top-level HWND owned by the launched `AstralEditor` process is the only visible process-owned top-level window for 20 consecutive 50 ms observations at startup and again after interaction.
2. Top-level class and title are exactly `AstralEditorWindow` and `Astral Editor 0.1`.
3. Exactly 12 direct child controls exist with five Buttons, two ListBoxes and five Statics.
4. All five pending toolbar buttons have exact expected captions, remain visible, and remain disabled.
5. All four non-Inspector shell Statics exist with exact integrated text and are visible; the Scene Root Inspector static exists with exact full text and is visible.
6. Outliner is the visible process-owned direct `ListBox` at control ID 1001; Assets is the visible process-owned direct `ListBox` at control ID 1002; Inspector is a visible process-owned direct `Static`.
7. Outliner count is five, row 0 is exactly `Scene Root`, row 3 is exactly `Cube`, and initial selection is row 0.
8. Assets count is four and rows are exactly, in order, `Primitive/Cube`, `Primitive/Plane`, `Camera`, `DirectionalLight`.
9. Before selection, notification and post-notification reads, the Outliner HWND is revalidated against the launched PID, direct parent, exact `ListBox` class, visibility and control ID. After the bounded notification, current selection remains index 3, selected row text is `Cube`, the Inspector is revalidated as a visible process-owned direct `Static`, and its text exactly matches the Cube fixture.
10. Before each side-effecting resize, the saved editor HWND is revalidated against the launched PID. The 800x600 and 420x260 resizes use `SWP_ASYNCWINDOWPOS`, complete within the bounded poll, and every direct child remains within the actual client rectangle.
11. Before shutdown, editor HWND ownership is revalidated; exactly one `WM_CLOSE` is posted, exit is bounded and must be code 0, and failure cleanup may terminate only the retained process handle created by the smoke.

Cross-process synchronous messages remain bounded through `SendMessageTimeoutW` with the existing one-second timeout. `EditorRuntimeSmoke` remains registered through `astral_add_test`, `RUN_SERIAL`, and the existing 180-second CTest timeout. Do not weaken assertions, timeout, Release-assertion protection, or exclusive-desktop requirements to make a gate green.

## Primary-source research rechecked 2026-09-22

Behavioral/API references only. No proprietary engine source, artwork, assets or dependency was copied or imported.

- Epic Games, Unreal Engine 5.8, Unreal Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
  - Applicability: Epic identifies the Level Viewport, Outliner, Details panel and Content Drawer as distinct observable editor surfaces. The Outliner and Details panel synchronize around selected actors.
- Epic Games, Unreal Engine 5.8, Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
  - Applicability: Outliner is a named hierarchical selection surface rather than an anonymous child-control count.
- Epic Games, Unreal Engine 5.8, Content Browser: https://dev.epicgames.com/documentation/en-us/unreal-engine/content-browser-in-unreal-engine
  - Applicability: asset browsing is an identifiable editor surface. Astral's four-row procedural fixture is only a verification placeholder, not feature parity.
- Microsoft `GetDlgItem`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getdlgitem
  - Applicability: for a parent-child pair with a unique child ID, `GetDlgItem` resolves that child handle.
- Microsoft `GetWindowThreadProcessId`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowthreadprocessid
  - Applicability: revalidates which process created a window; an invalid HWND returns zero.
- Microsoft `GetParent`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getparent
  - Applicability: verifies the expected direct editor parent for cached child handles.
- Microsoft `IsWindowVisible`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindowvisible
  - Applicability: requires the claimed editor surface and its ancestor chain to carry `WS_VISIBLE`.

Access date for this packet: 2026-09-22.

## Verification evidence

Portable source-logic fixture SHA-256: `641888fe4da9a018e0116a76990ce81b10246edfc6a42cd2a6dafc70a1b3e161`.

Executed in the coordinator sandbox:

```bash
g++ -std=c++17 -Wall -Wextra -Werror -pedantic /tmp/e11_surface_identity_fixture.cpp -o /tmp/e11_surface_identity_fixture
/tmp/e11_surface_identity_fixture
clang++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer /tmp/e11_surface_identity_fixture.cpp -o /tmp/e11_surface_identity_fixture_san
ASAN_OPTIONS=detect_leaks=1 /tmp/e11_surface_identity_fixture_san
sha256sum /tmp/e11_surface_identity_fixture.cpp
```

Both executions exited 0 and printed `surface identity/visibility contract fixture: PASS`. This fixture proves only the portable source-level guard model. It is not Win32/native editor execution.

Hosted exact-candidate evidence for `b28b8cc079c92b925b3f59c223e67651a238d276`:

- Windows Server 2022 run `35743225947`, job `106798072594`: PASS. Successful steps include R0 parser-only and runner-safety contracts, VS2022 x64 configure, MSVC Debug build plus deterministic tests, MSVC Release build plus deterministic tests, Release dependency/prerequisite checks, static milestone verifiers, and clean tracked-tree verification. The historical R0 runner itself was not executed.
- Profiling capture portability run `35743225660`: PASS.
- Release manifest integrity run `35743225770`: PASS.

Hosted deterministic CTest intentionally excludes every `RuntimeSmoke`, so these results prove compilation and deterministic non-runtime regression status, not native GUI execution.

## Independent review state

The prior evidence head `3385769790f8c428c2f84de85c522138b9cae189` completed its narrow Codex evidence-consistency recheck at `2026-09-22T14:46:17Z` without a new reported finding. That review predates this pass's runtime-smoke code change and does not independently accept candidate `b28b8cc...`.

A fresh independent review of the new code/evidence head is required before this packet can advance. Native runtime acceptance remains separate and pending.

## Registered-local handoff

Run exact current code candidate `b28b8cc079c92b925b3f59c223e67651a238d276` on one owned interactive Windows desktop using external build output:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine and Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, full stdout/stderr, exit codes, UTC timestamps and screenshots at normal and narrow/short sizes. Human-visible acceptance must confirm the exact four asset entries, exact OUTLINER/INSPECTOR/ASSETS labels, truthful E11 status text, Outliner visibly remains on Cube when Inspector shows Cube, viewport `Selected:` text reflects Cube, and panels do not bleed during resize.

## Stop, rollback and next action

Stop on unexpected edits outside the allowed paths, stale ownership, a failing introduced regression, a changed architecture/dependency requirement, or a native gate requiring the registered Windows desktop. Rollback is branch-local revert of this packet; never rewrite shared history.

E11 remains partial. `native_evidence` remains empty and `independent_acceptance` remains false. Do not start dependent scene-document, transform-gizmo, undo/redo or save/reopen implementation based on hosted compilation alone. Issue #7 remains separate and open; do not invoke the historical R0 runner.

Single next useful action: obtain fresh independent review of this exact code/evidence state, then run Debug and Release `EditorRuntimeSmoke` on the registered Windows desktop with retained receipts/screenshots.
