# E11 Editor Runtime Smoke evidence, 2026-09-22

## Checkpoint

This is the current evidence record for the bounded `EditorRuntimeSmoke` packet admitted from `main` at `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1` on branch `engine/2026-09-22-editor-runtime-smoke`.

Scope remains verification-only. The packet adds and hardens one native Windows smoke for the already-integrated `AstralEditor`; it does not add scene mutation or serialization, transform gizmos, Play-in-Editor, asset import, renderer/API changes, dependencies, game content, scheduler operations, deployment, release, or R0 execution.

Current `main` was re-read at `7950687e3f9180787957e52394bf13cea49c35bf` after separate game-worker merges. This packet was not rebased, force-pushed, or merged with that unrelated work. A compare against that current main still reports only this packet's five authorized paths, but exact integration evidence must be re-established against then-current `main` before any eventual merge.

## Current exact implementation

Latest code candidate: `8def2808f679a6a2360e98e9d24c59aaece25fe0`.

Relevant published blobs:

- `Tests/EditorRuntimeSmoke.cpp`: `7234b0dad2977421e2cd4a584ae4ef44c6f4832f`
- `CMakeLists.txt`: `945e0f5e28a0ecd91338541984cdc240bf986f82`
- `Tasks/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`: `855a48974a24f94a50880aea418f6dba471e7bde`

`CMakeLists.txt` registers the test through `astral_add_test`. The `RuntimeSmoke` suffix keeps the existing Release-assertion protection, `RUN_SERIAL`, and 180-second timeout contract. Hosted deterministic CTest intentionally excludes RuntimeSmoke execution.

The smoke currently requires all of the following before reporting PASS:

1. The same visible top-level HWND owned by the launched `AstralEditor` process is the only such process-owned window for 20 consecutive 50 ms observations at startup and again after interaction.
2. The top-level window has class `AstralEditorWindow` and title `Astral Editor 0.1`.
3. Exactly 12 direct E11 child controls exist with the expected class counts, and all five pending toolbar actions remain visible and disabled.
4. The Outliner contains five rows, the Assets list contains four placeholders, Outliner index 0 is exactly `Scene Root`, Outliner index 3 is exactly `Cube`, and the initial selection is index 0.
5. The initial Inspector text exactly matches the complete Scene Root fixture.
6. Immediately before the side-effecting `LB_SETCURSEL`, the cached Outliner HWND is revalidated as owned by the exact launched PID, directly parented by the owned editor HWND, and still returned by `GetDlgItem(editor, 1001)`. Successful `LB_SETCURSEL` is accepted on the documented non-`LB_ERR` contract rather than assuming the return value equals the requested index.
7. The same Outliner ownership/parent/control-ID guard is repeated before the synthetic bounded `LBN_SELCHANGE` notification and before post-notification reads. The smoke then separately requires `LB_GETCURSEL == 3`, the selected row text to equal `Cube`, and the Inspector to exactly match the complete Cube fixture.
8. Immediately before each side-effecting 800x600 or 420x260 resize, the saved editor HWND is revalidated with `GetWindowThreadProcessId` against the exact launched process ID. Invalid/recycled HWND or PID mismatch fails closed without `SetWindowPos`. Valid resizes use `SWP_ASYNCWINDOWPOS`, complete inside a bounded poll, and must leave every direct child control inside the actual client rectangle.
9. Shutdown independently revalidates HWND process ownership immediately before one `WM_CLOSE`, waits boundedly for exit code 0, and failure cleanup may terminate only the retained handle for the process launched by the smoke.

Cross-process synchronous messages use `SendMessageTimeoutW` with a one-second deadline. No global keyboard/mouse injection is used.

## Primary-source research rechecked 2026-09-22

Behavioral/API references only. No Epic, Unity, or Microsoft source/assets are copied and no dependency is imported.

- Epic Games, Unreal Engine 5.8, Unreal Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
- Epic Games, Unreal Engine 5.8, Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
- Unity Manual, Unity 6.0, Inspect items: https://docs.unity3d.com/6000.0/Documentation/Manual/InspectorItems.html
- Microsoft Win32 `LB_SETCURSEL`: https://learn.microsoft.com/windows/win32/controls/lb-setcursel
  - Applicability: Microsoft specifies `LB_ERR` on error and does not specify that a successful call returns the selected index. Therefore the smoke accepts a non-`LB_ERR` setter result and verifies the actual selection separately with `LB_GETCURSEL`.
- Microsoft Win32 `LB_GETCURSEL`: https://learn.microsoft.com/windows/win32/controls/lb-getcursel
  - Applicability: a single-selection list box returns the zero-based selected index or `LB_ERR` when there is no selection; this is the post-action selection oracle.
- Microsoft Win32 `GetParent`: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-getparent
  - Applicability: for a child window it returns the parent handle, used to reject a stale/recycled Outliner HWND that is no longer directly parented by the editor.
- Microsoft Win32 `GetWindowThreadProcessId`: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-getwindowthreadprocessid
  - Applicability: associates the cached editor/control HWNDs with the exact launched process and returns zero for an invalid handle.
- Microsoft Win32 `SendMessageTimeoutW`: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-sendmessagetimeoutw
- Microsoft Win32 `SetWindowPos`: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setwindowpos

## Independent-review findings addressed in this pass

Independent Codex review of exact prior head `b0c47bda86d157a411e3d501a062cb3975c1d9f0` found two runtime-only defects that hosted compilation could not expose:

1. **P1, `LB_SETCURSEL` return contract.** The old smoke required the setter's return value to equal requested index 3. Microsoft only documents `LB_ERR` as the error result, so a successful setter result that is not 3 caused a false failure before `WM_COMMAND` was sent. Candidate `8def280...` now treats any non-`LB_ERR` setter result as success and relies on the existing independent post-notification `LB_GETCURSEL == 3` check for the actual index.
2. **P2, cached Outliner HWND side effect.** The old smoke could send `LB_SETCURSEL` through a cached child HWND without proving it still belonged to the launched process and editor. Candidate `8def280...` adds `DirectControlOwnedByProcessAndParent`, requiring the editor and child HWNDs to belong to the launched PID, `GetParent(outliner) == editor`, and `GetDlgItem(editor, 1001) == outliner` before selection, before notification, and before final selection reads.

Prior independent-review repairs retained by this candidate include stable exact top-level-window cardinality, bounded asynchronous resize completion, owned-handle-only cleanup, exact Outliner item identities, full Inspector fixture comparisons, post-notification Outliner/Inspector synchronization, and process-ownership revalidation before each resize and shutdown.

## Coordinator sandbox evidence

A disposable C++17 source-logic fixture models both repaired contracts. It demonstrates that the old modeled setter check rejects a non-error success result when it is not equal to the requested index, while the repaired contract accepts non-`LB_ERR` and verifies the selected index separately. It also rejects invalid parent/control handles, wrong process IDs, wrong direct-parent handles, and wrong control IDs before permitting a side effect.

Fixture SHA-256: `0b8292699af0a133f2fb0aedd64fbfc0bc72c3b9178d3ab15d61d2669eaacbe0`.

Commands executed:

```bash
g++ -std=c++17 -Wall -Wextra -Werror /mnt/data/e11_selection_ownership_fixture.cpp -o /mnt/data/e11_selection_ownership_fixture
/mnt/data/e11_selection_ownership_fixture
clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /mnt/data/e11_selection_ownership_fixture.cpp -o /mnt/data/e11_selection_ownership_fixture_san
ASAN_OPTIONS=detect_leaks=1 /mnt/data/e11_selection_ownership_fixture_san
sha256sum /mnt/data/e11_selection_ownership_fixture.cpp
```

Both compile/run paths exited 0 and printed `selection return + direct-control ownership fixture: PASS`. This is source-logic evidence only, not production Win32/native execution.

## Hosted Windows evidence for exact code candidate

GitHub Actions Windows Server 2022 run `35721134686`, job `106724068290`, executed against exact candidate `8def2808f679a6a2360e98e9d24c59aaece25fe0` and completed successfully on 2026-09-22.

Successful steps included checkout/external build-root setup, R0 parser-only plus runner-safety contracts, PE dependency and Windows prerequisite/runtime-policy checks, Release-assertion and CTest safety contracts, Visual Studio 2022 x64 configure, MSVC Debug build and deterministic Debug tests, MSVC Release build and deterministic Release tests, Release dependency/prerequisite checks, static milestone verifiers, and clean tracked-tree verification. The historical R0 runner itself was not executed.

Additional workflows on the same exact candidate also passed:

- Profiling capture portability run `35721134613`.
- Release manifest integrity run `35721134657`.

These hosted runs establish Debug/Release compilation of the real Win32 smoke and green non-runtime regressions. They do **not** establish native GUI execution because hosted deterministic CTest intentionally excludes every RuntimeSmoke target.

## Deferred native and independent evidence

The coordinator does not have the registered Windows interactive desktop and does not claim to have executed `EditorRuntimeSmoke` natively. E11 therefore still has no registered-local native GUI receipt and no independent final acceptance. This packet is not evidence of UE5/Unity editor parity.

Registered-local execution remains:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

The registered executor must retain exact source SHA, machine and Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, full stdout/stderr, exit codes, UTC timestamps, and screenshots at normal and narrow/short sizes. Human-visible acceptance should confirm the Outliner visibly remains on Cube when the Inspector shows the Cube fixture and that viewport/panels do not bleed during resize.

## Result and next action

Status: **the two latest independent-review defects are repaired and exact-candidate hosted Debug/Release compilation is green; native editor smoke execution and fresh independent review of the final evidence head remain pending**.

The single next useful action is to run exact-current-head `EditorRuntimeSmoke` in Debug and Release on the registered owned Windows desktop, preserve the required receipts/screenshots, and obtain fresh independent review of the exact final head. Do not start dependent scene-document, transform-gizmo, save/reopen, or undo/redo work on hosted compilation alone. Issue #7 remains separate and open; the R0 runner was not executed by this packet.
