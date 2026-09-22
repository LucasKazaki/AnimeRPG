# E11 Editor Runtime Smoke task, 2026-09-22

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. This packet may harden the native Windows smoke and its evidence, but it must not add scene mutation/serialization, transform gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed in this pass: `755faabfb5f04d5bc07324d91cbceb261cdc1060`; do not rebase, merge, or absorb unrelated game-worker work in this packet.
Current code candidate: `46b9a018af5dd6a1ae01d414497ee1e6ef294dfb`.
Current smoke blob: `98ab2d576da85b5f298bd83bb5135c79fc21b2b0`.
Integrated editor source fixture blob: `Tools/AstralEditorMain.cpp` `5142e632a79c89d0d0ce3efe87e752456f802185`; that production editor file is not owned by this packet and was not modified.

Allowed paths only:

- `CMakeLists.txt`
- `Tests/EditorRuntimeSmoke.cpp`
- `Tasks/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`
- `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`
- `Docs/Research/ENGINE-CAPABILITIES.json`

One active writer only. No force push, destructive cleanup, merge, release, deployment, dependency import, repository-permission change, architecture/graphics-API change, or Company Runtime mutation.

## Selected review finding and repair

Fresh independent Codex review on the prior E11 head reported one current P2 false-pass gap: post-resize validation freshly enumerated controls and could therefore accept a `WM_SIZE` path that destroyed and recreated one or more controls with the same classes, captions, IDs, contents, visibility and enabled state. The packet contract requires the original shell control identities to survive interaction and resizing.

Candidate `46b9a018af5dd6a1ae01d414497ee1e6ef294dfb` repairs that gap without modifying production editor code:

- captures the initial set of exactly 12 direct process-owned child HWNDs after the stable top-level window is established;
- compares later inventories against that original HWND plus class set with one-to-one matching, independent of enumeration order;
- fails closed if any original child disappears, any replacement handle appears, or a class associated with an original handle changes;
- performs identity checks before selection, again before `LBN_SELCHANGE`, immediately before each resize, during containment validation, at the start and end of every full shell-state validation, after each resize, and during final validation;
- retains the existing per-send PID/parent/class/visibility/control-ID checks, exact text/list/selection checks, bounded cross-process sends, bounded asynchronous resize, stable-single-window checks, and owned-process-only cleanup.

No assertion, timeout, test registration, dependency, graphics API, or execution authority was weakened or expanded.

## Acceptance contract

The smoke may report PASS only when all of the following hold:

1. The same visible top-level HWND owned by the launched `AstralEditor` process is the only visible process-owned top-level window for 20 consecutive 50 ms observations at startup and again after interaction.
2. Top-level class and title are exactly `AstralEditorWindow` and `Astral Editor 0.1`.
3. Exactly 12 direct process-owned child controls exist with five Buttons, two ListBoxes and five Statics.
4. The initial 12 child HWND plus class identities are retained. Every later selection, resize, containment, shell-state and final validation must observe the same set; a replacement control cannot satisfy the contract merely by matching text/class/ID/state.
5. All five pending toolbar buttons have exact expected captions, are visible, and remain disabled at initial state, after each resize, and at final validation.
6. All four non-Inspector shell Statics have exact integrated text and are visible; Inspector is a visible process-owned direct `Static` with the exact expected Scene Root or Cube fixture.
7. Outliner is the visible process-owned direct `ListBox` at control ID 1001; Assets is the visible process-owned direct `ListBox` at control ID 1002.
8. Every bounded list/text read revalidates the target immediately before its send; two-message text reads revalidate again before the second message so a stale/recycled HWND fails closed.
9. Outliner count is five, row 0 is exactly `Scene Root`, row 3 is exactly `Cube`, and selection is the expected row for the current state.
10. Assets count is four and rows are exactly, in order, `Primitive/Cube`, `Primitive/Plane`, `Camera`, `DirectionalLight`.
11. Cube selection plus bounded `LBN_SELCHANGE` must result in Outliner selection 3, selected row `Cube`, and exact Cube Inspector state without replacing any original shell child HWND.
12. Before and during each side-effecting resize, the saved editor HWND remains owned by the launched PID. The 800x600 and 420x260 resizes use `SWP_ASYNCWINDOWPOS`, complete within the bounded poll, keep every original direct child inside the client rectangle, preserve all original child handles, and preserve the entire required shell state.
13. Before shutdown, editor HWND ownership is revalidated; exactly one `WM_CLOSE` is posted, exit is bounded and must be code 0, and failure cleanup may terminate only the retained process handle created by the smoke.

Cross-process synchronous messages remain bounded through `SendMessageTimeoutW` with the existing one-second timeout. `EditorRuntimeSmoke` remains registered through `astral_add_test`, `RUN_SERIAL`, and the existing 180-second CTest timeout. Do not weaken assertions, timeout, Release-assertion protection, or exclusive-desktop requirements to make a gate green.

## Primary-source research rechecked 2026-09-22

Behavioral/API references only. No proprietary engine source, artwork, assets or dependency was copied or imported.

- Epic Games, Unreal Engine 5.8, Unreal Editor Interface: https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-editor-interface
  - Applicability: Level Viewport, Outliner, Details and Content Browser/Drawer are distinct editor surfaces whose identity and state are part of the workflow.
- Unity Technologies, Unity 6.0 (6000.0), The Hierarchy window: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html
  - Applicability: Hierarchy is the scene-object management surface; Astral's test is a minimum editor-shell contract, not parity.
- Microsoft `IsWindow`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindow
  - Applicability: Microsoft warns that HWNDs can be destroyed and recycled, so semantic equivalence of a newly found control is not proof that the original window survived.
- Microsoft `EnumChildWindows`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-enumchildwindows
  - Applicability: enumerates child HWNDs and documents behavior for controls created/destroyed during enumeration; repeated inventory comparisons are fail-closed checkpoints.
- Microsoft `GetDlgItem`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getdlgitem
  - Applicability: retrieves the current child HWND for a unique control ID; it does not by itself establish continuity with the initial child HWND.

Access date: 2026-09-22.

## Verification evidence

Disposable portable contract fixture SHA-256: `619092d28540a53ee81e93efa29c93efc6571845de6d631a38800f4460c9ff4e`.

Executed in the coordinator sandbox:

```bash
g++ -std=c++17 -Wall -Wextra -Werror -pedantic /tmp/e11_handle_inventory_fixture.cpp -o /tmp/e11_handle_inventory_fixture
/tmp/e11_handle_inventory_fixture
clang++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer /tmp/e11_handle_inventory_fixture.cpp -o /tmp/e11_handle_inventory_fixture_san
ASAN_OPTIONS=detect_leaks=1 /tmp/e11_handle_inventory_fixture_san
sha256sum /tmp/e11_handle_inventory_fixture.cpp
clang++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsyntax-only -I/tmp/winstub /tmp/EditorRuntimeSmoke.new.cpp
```

The GCC and Clang ASan+UBSan fixture executions exited 0 and printed `initial HWND inventory preservation contract fixture: PASS`. The fixture accepts the same handles in a different enumeration order and rejects a replacement handle, a class change, a duplicate replacement and a missing control. The exact rewritten smoke source also passed the warning-clean Clang syntax check against a disposable minimal Win32 declaration shim. These are source-logic/compiler checks only, not native Win32 editor execution.

Exact code candidate `46b9a018af5dd6a1ae01d414497ee1e6ef294dfb` passed hosted checks:

- Windows Server 2022 run `35754110237`, job `106835418831`: PASS. Successful steps include R0 parser-only/safety contracts, VS2022 x64 configure, MSVC Debug build plus deterministic tests, MSVC Release build plus deterministic tests, Release dependency/prerequisite/runtime-policy checks, static milestone verifiers and clean tracked-tree verification. The historical R0 runner itself was not executed.
- Profiling capture portability run `35754109941`: PASS.
- Release manifest integrity run `35754109935`: PASS.

Hosted deterministic CTest intentionally excludes every `RuntimeSmoke`, so these results prove compilation and deterministic non-runtime regression status, not native GUI execution.

## Independent review state

The current P2 finding was raised against the prior code/evidence state and is repaired by candidate `46b9a018...`. Because runtime-smoke code changed, a fresh independent review of the exact final code/evidence head is required before this packet can advance. Same-author inspection is not independent acceptance. Native runtime acceptance remains separate and pending.

## Registered-local handoff

Run exact code candidate `46b9a018af5dd6a1ae01d414497ee1e6ef294dfb` on one owned interactive Windows desktop using external build output:

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

Single next useful action: obtain fresh independent review of the exact final code/evidence head, then run Debug and Release `EditorRuntimeSmoke` on the registered Windows desktop with retained receipts/screenshots.
