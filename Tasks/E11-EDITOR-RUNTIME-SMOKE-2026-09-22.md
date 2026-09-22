# E11 Editor Runtime Smoke task, 2026-09-22

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. This packet may harden the native Windows smoke and its evidence, but it must not add scene mutation/serialization, transform gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Current independently moving `main` re-read in this pass: `1ac6bf8d54219968effadd17e575aafb4ebd3847`; do not rebase, merge, or absorb unrelated game-worker work in this packet.
Current code candidate: `7ead712f69b18d4a1be9818ae53fb3e252f06214`.
Current smoke blob: `f4e4c1a14bc2d382fbe32849f5b07bee15e55118`.
Integrated editor source fixture blob: `Tools/AstralEditorMain.cpp` `5142e632a79c89d0d0ce3efe87e752456f802185`; that production editor file is not owned by this packet and was not modified.

Allowed paths only:

- `CMakeLists.txt`
- `Tests/EditorRuntimeSmoke.cpp`
- `Tasks/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`
- `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`
- `Docs/Research/ENGINE-CAPABILITIES.json`

One active writer only. No force push, destructive cleanup, merge, release, deployment, dependency import, repository-permission change, architecture/graphics-API change, or Company Runtime mutation.

## Selected review findings and repair

Independent Codex review of prior evidence head `9931095a40a32e4ec6c93e35a9fb550f7bd1a243` completed at `2026-09-22T15:04:25Z` and reported two P2 false-pass/safety gaps:

1. Cached Outliner, Assets, and Inspector HWNDs were validated at broad checkpoints, but a two-message read such as `LB_GETTEXTLEN` followed by `LB_GETTEXT` did not revalidate the handle before every cross-process send. A destroyed/recycled handle could therefore target a different control or process between messages.
2. After each resize, containment checked only child rectangles. Hidden, relabeled, replaced controls or an unexpectedly enabled pending toolbar action could still satisfy geometry and pass.

Candidate `7ead712f69b18d4a1be9818ae53fb3e252f06214` repairs both findings without changing production editor code:

- `ReadOwnedWindowText`, `ReadValidatedChildText`, `ReadValidatedListboxValue`, and `ReadValidatedListboxText` fail closed unless the target is still process-owned and, for children, still the expected visible direct child/class/control ID immediately before the applicable bounded send. Two-message text reads revalidate again before the second send.
- `DirectChildren` admits only current direct children owned by the launched process.
- `ValidateShellState` checks the exact top-level class/title, 12-control inventory and class counts, five exact visible disabled pending buttons, four exact visible shell static labels/status, exact Inspector text, Outliner/Assets ID/class/visibility/ownership/parent identity, exact list counts/items, and expected selection.
- `ResizeAndCheck` retains the bounded asynchronous resize and containment assertion, then performs full `ValidateShellState` after both 800x600 and 420x260. It also rechecks top-level ownership while polling the asynchronous resize.
- The Cube selection path revalidates the Outliner before the side effect and notification, then requires the complete Cube shell state before continuing.
- Final stable-window revalidation is followed by one more complete Cube shell-state validation before clean shutdown.

No assertion, timeout, test registration, dependency, graphics API, or execution authority was weakened or expanded.

## Acceptance contract

The smoke may report PASS only when all of the following hold:

1. The same visible top-level HWND owned by the launched `AstralEditor` process is the only visible process-owned top-level window for 20 consecutive 50 ms observations at startup and again after interaction.
2. Top-level class and title are exactly `AstralEditorWindow` and `Astral Editor 0.1`.
3. Exactly 12 direct process-owned child controls exist with five Buttons, two ListBoxes and five Statics.
4. All five pending toolbar buttons have exact expected captions, are visible, and remain disabled at initial state, after each resize, and at final validation.
5. All four non-Inspector shell Statics have exact integrated text and are visible; Inspector is a visible process-owned direct `Static` with the exact expected Scene Root or Cube fixture.
6. Outliner is the visible process-owned direct `ListBox` at control ID 1001; Assets is the visible process-owned direct `ListBox` at control ID 1002.
7. Every bounded list/text read revalidates the target immediately before its send; two-message text reads revalidate again before the second message so a stale/recycled HWND fails closed.
8. Outliner count is five, row 0 is exactly `Scene Root`, row 3 is exactly `Cube`, and selection is the expected row for the current state.
9. Assets count is four and rows are exactly, in order, `Primitive/Cube`, `Primitive/Plane`, `Camera`, `DirectionalLight`.
10. Cube selection plus bounded `LBN_SELCHANGE` must result in Outliner selection 3, selected row `Cube`, and exact Cube Inspector state.
11. Before and during each side-effecting resize, the saved editor HWND remains owned by the launched PID. The 800x600 and 420x260 resizes use `SWP_ASYNCWINDOWPOS`, complete within the bounded poll, keep every direct child inside the client rectangle, and preserve the entire required shell inventory, identity, visibility, disabled-state, list contents, selection, and Inspector state.
12. Before shutdown, editor HWND ownership is revalidated; exactly one `WM_CLOSE` is posted, exit is bounded and must be code 0, and failure cleanup may terminate only the retained process handle created by the smoke.

Cross-process synchronous messages remain bounded through `SendMessageTimeoutW` with the existing one-second timeout. `EditorRuntimeSmoke` remains registered through `astral_add_test`, `RUN_SERIAL`, and the existing 180-second CTest timeout. Do not weaken assertions, timeout, Release-assertion protection, or exclusive-desktop requirements to make a gate green.

## Primary-source research rechecked 2026-09-22

Behavioral/API references only. No proprietary engine source, artwork, assets or dependency was copied or imported.

- Epic Games, Unreal Engine 5.8, Unreal Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
  - Applicability: Level Viewport, Outliner, Details panel and Content Browser/Drawer are distinct observable editor surfaces; Outliner/viewport selection drives Details.
- Epic Games, Unreal Engine 5.8, Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
  - Applicability: hierarchical actor selection is an identifiable editor workflow, not merely a child-control count.
- Epic Games, Unreal Engine 5.8, Content Browser: https://dev.epicgames.com/documentation/en-us/unreal-engine/content-browser-in-unreal-engine
  - Applicability: project assets are identifiable, searchable managed items. Astral's four-row procedural fixture is verification only, not parity.
- Unity Technologies, Unity 6.0 (6000.0), The Hierarchy window: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html
  - Applicability: Hierarchy displays every GameObject in a scene and supports organization/visibility; exact surface state matters after layout changes.
- Unity Technologies, Unity 6.0 (6000.0), Project window: https://docs.unity3d.com/6000.0/Documentation/Manual/ProjectView.html
  - Applicability: Project window is the project asset-browsing surface; exact asset identity is a measurable minimum fixture.
- Unity Technologies, Unity 6.0 (6000.0), Inspector items: https://docs.unity3d.com/6000.0/Documentation/Manual/InspectorItems.html
  - Applicability: Inspector content depends on the selected item, matching the selection-synchronization property tested here.
- Microsoft `IsWindow`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindow
  - Applicability: Microsoft explicitly warns that HWNDs can be destroyed and recycled, motivating validation immediately around each cross-process use rather than one-time validation.
- Microsoft `GetWindowThreadProcessId`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowthreadprocessid
  - Applicability: validates which process created a window; invalid HWND returns zero.
- Microsoft `IsWindowVisible`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindowvisible
  - Applicability: verifies the claimed surface remains visible along its ancestor chain.
- Microsoft `IsWindowEnabled`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindowenabled
  - Applicability: verifies pending actions remain disabled; a child receives input only when enabled and visible.

Access date: 2026-09-22.

## Verification evidence

Disposable portable contract fixture SHA-256: `47ad65939c672b33f1aad3831d5dcb770dae96865c698ef7c7774720299f3e57`.

Executed in the coordinator sandbox:

```bash
g++ -std=c++17 -Wall -Wextra -Werror -pedantic /tmp/e11_per_send_resize_fixture.cpp -o /tmp/e11_per_send_resize_fixture
/tmp/e11_per_send_resize_fixture
clang++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer /tmp/e11_per_send_resize_fixture.cpp -o /tmp/e11_per_send_resize_fixture_san
ASAN_OPTIONS=detect_leaks=1 /tmp/e11_per_send_resize_fixture_san
sha256sum /tmp/e11_per_send_resize_fixture.cpp
```

Both executions exited 0 and printed `per-send identity + post-resize shell-state contract fixture: PASS`. The fixture models a handle recycling between the two phases of a text read and post-resize hidden/enabled/relabeled/replaced controls. This is source-logic evidence only, not native Win32 editor execution.

The rewritten production smoke source was also checked with Clang C++17 `-Wall -Wextra -Werror -pedantic -fsyntax-only` against a disposable minimal Win32 declaration shim in the sandbox. That is syntax evidence only; hosted MSVC is the authoritative Windows compile check.

Hosted exact-code-candidate evidence for `7ead712f69b18d4a1be9818ae53fb3e252f06214`:

- Windows Server 2022 run `35746903272`, job `106810756831`: PASS. Successful steps include R0 parser-only/safety contracts, VS2022 x64 configure, MSVC Debug build plus deterministic tests, MSVC Release build plus deterministic tests, Release dependency/prerequisite/runtime-policy checks, static milestone verifiers, and clean tracked-tree verification. The historical R0 runner itself was not executed.
- Profiling capture portability run `35746903276`: PASS.
- Release manifest integrity run `35746903314`: PASS.

Hosted deterministic CTest intentionally excludes every `RuntimeSmoke`, so these results prove compilation and deterministic non-runtime regression status, not native GUI execution.

## Independent review state

The Codex review of prior head `9931095a40a32e4ec6c93e35a9fb550f7bd1a243` completed at `2026-09-22T15:04:25Z` and produced the two P2 findings repaired above. That review does not independently accept candidate `7ead712...`.

Fresh independent review of the current code/evidence state is required before this packet can advance. Same-author inspection is not independent acceptance. Native runtime acceptance remains separate and pending.

## Registered-local handoff

Run exact current code candidate `7ead712f69b18d4a1be9818ae53fb3e252f06214` on one owned interactive Windows desktop using external build output:

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

Single next useful action: obtain fresh independent review of the exact current code/evidence state, then run Debug and Release `EditorRuntimeSmoke` on the registered Windows desktop with retained receipts/screenshots.
