# E11 Editor Runtime Smoke evidence, 2026-09-22

## Checkpoint

This is the current evidence record for the bounded `EditorRuntimeSmoke` packet admitted from `main` at `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1` on branch `engine/2026-09-22-editor-runtime-smoke`.

Scope remains verification-only. The packet adds and hardens one native Windows smoke for the already-integrated `AstralEditor`; it does not add scene mutation or serialization, transform gizmos, Play-in-Editor, asset import, renderer/API changes, dependencies, game content, scheduler operations, deployment, release, or R0 execution.

Current `main` was observed at `84dc005e4e6259faf7209265def762339a6c0cf9` after unrelated game-worker changes. This packet was not rebased or force-pushed onto that work. Exact integration evidence must be re-established against then-current `main` before any eventual merge.

## Current exact implementation

Latest code candidate: `c9992789543b324c9f5a1121e49a4adae347d375`.

Relevant published blobs for that candidate:

- `Tests/EditorRuntimeSmoke.cpp`: `c861d5ebaceed045aa594eedba598c6fcac024eb`
- `CMakeLists.txt`: `945e0f5e28a0ecd91338541984cdc240bf986f82`
- `Tasks/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`: `855a48974a24f94a50880aea418f6dba471e7bde`

`CMakeLists.txt` registers the test through `astral_add_test`. The `RuntimeSmoke` suffix keeps the existing Release-assertion protection, `RUN_SERIAL`, and 180-second timeout contract. Hosted deterministic CTest intentionally excludes RuntimeSmoke execution.

The smoke currently requires all of the following before reporting PASS:

1. The same visible top-level HWND owned by the launched `AstralEditor` process is the only such process-owned window for 20 consecutive 50 ms observations at startup and again after interaction.
2. The top-level window has class `AstralEditorWindow` and title `Astral Editor 0.1`.
3. Exactly 12 direct E11 child controls exist with the expected class counts, and all five pending toolbar actions remain visible and disabled.
4. The Outliner contains five rows, the Assets list contains four placeholders, Outliner index 0 is exactly `Scene Root`, Outliner index 3 is exactly `Cube`, and the initial selection is index 0.
5. The initial Inspector text exactly matches the complete Scene Root fixture.
6. After setting Outliner index 3 and sending bounded `LBN_SELCHANGE`, the smoke re-queries `LB_GETCURSEL`, requires index 3 to remain selected, re-reads that selected row and requires it to equal `Cube`, and separately requires the Inspector to exactly match the complete Cube fixture.
7. Immediately before each side-effecting 800x600 or 420x260 resize, the saved editor HWND is revalidated with `GetWindowThreadProcessId` against the exact launched process ID. An invalid/recycled HWND, failed ownership query, or PID mismatch fails closed without calling `SetWindowPos`. Valid resizes use `SWP_ASYNCWINDOWPOS`, complete inside a bounded poll, and must leave every direct child control inside the actual client rectangle.
8. Shutdown independently revalidates HWND process ownership immediately before one `WM_CLOSE`, waits boundedly for exit code 0, and failure cleanup may terminate only the retained handle for the process launched by the smoke.

Cross-process synchronous messages use `SendMessageTimeoutW` with a one-second deadline. No global keyboard/mouse injection is used.

## Primary-source research rechecked 2026-09-22

Behavioral/API references only. No Epic, Unity, or Microsoft source/assets are copied and no dependency is imported.

- Epic Games, Unreal Engine 5.8, Unreal Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
- Epic Games, Unreal Engine 5.8, Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
- Unity Manual, Unity 6.0, Inspect items: https://docs.unity3d.com/6000.0/Documentation/Manual/InspectorItems.html
- Microsoft Win32 `LB_GETCURSEL`: https://learn.microsoft.com/windows/win32/controls/lb-getcursel
- Microsoft Win32 `LB_GETTEXTLEN`: https://learn.microsoft.com/windows/win32/controls/lb-gettextlen
- Microsoft Win32 `LB_GETTEXT`: https://learn.microsoft.com/windows/win32/controls/lb-gettext
- Microsoft Win32 `SendMessageTimeoutW`: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-sendmessagetimeoutw
- Microsoft Win32 `SetWindowPos`: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setwindowpos
  - `SWP_ASYNCWINDOWPOS` posts the request to the owning thread when input queues differ, avoiding an unbounded synchronous cross-thread resize.
- Microsoft Win32 `GetWindowThreadProcessId`: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-getwindowthreadprocessid
  - Returns the creating thread and optionally the owning process ID; an invalid HWND returns zero. This is used to fail closed before each side-effecting resize and again before close.

## Independent-review finding addressed in this pass

Independent Codex review of exact prior head `243b1080805dd4f0f751f53900354edf42a2d04c` found one P2 false-side-effect path: the smoke revalidated HWND ownership before `WM_CLOSE`, but not immediately before `SetWindowPos`. If the editor destroyed its frame or exited after interaction and Windows recycled that numeric HWND, the smoke could resize an unrelated same-integrity process before eventually failing.

Code candidate `c9992789543b324c9f5a1121e49a4adae347d375` closes that path. `ResizeAndCheck` now receives `process.dwProcessId` and calls `WindowOwnedByProcess` immediately before each `SetWindowPos`. Both normal and narrow/short call sites pass the exact launched PID. The review thread was replied to with the candidate, blob, fixture, and hosted evidence. Fresh independent review of the exact final evidence head remains required; implementation-author checking is not independent acceptance.

Prior independent-review repairs retained by this candidate include stable exact top-level-window cardinality, bounded asynchronous resize completion, owned-handle-only cleanup, exact Outliner item identities, full Inspector fixture comparisons, and post-notification Outliner/Inspector selection synchronization.

## Coordinator sandbox evidence

A disposable C++17 source-logic fixture models the resize-ownership rule. The old modeled behavior permits a resize based only on possession of a saved handle; the repaired contract proceeds only when the ownership query succeeds and the observed PID equals the launched PID. It rejects both a recycled-handle PID and a failed ownership query.

Fixture SHA-256: `04816a99fcaeabb0c08b8b33c5f6342c1d14a40a7d414c50e84833afd50e7a26`.

Commands executed:

```bash
g++ -std=c++17 -Wall -Wextra -Werror /mnt/data/e11_resize_ownership_fixture.cpp -o /mnt/data/e11_resize_ownership_fixture
/mnt/data/e11_resize_ownership_fixture
clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /mnt/data/e11_resize_ownership_fixture.cpp -o /mnt/data/e11_resize_ownership_fixture_san
ASAN_OPTIONS=detect_leaks=1 /mnt/data/e11_resize_ownership_fixture_san
sha256sum /mnt/data/e11_resize_ownership_fixture.cpp
```

Both compile/run paths exited 0 and printed `resize ownership guard fixture: PASS`. This is source-logic evidence only. It is not production Win32/native execution.

## Hosted Windows evidence for the exact code candidate

GitHub Actions Windows Server 2022 run `35715362678`, job `106705505711`, executed against exact code candidate `c9992789543b324c9f5a1121e49a4adae347d375` and completed successfully on 2026-09-22.

Successful steps included checkout/external build-root setup, R0 parser-only plus runner-safety contracts, PE dependency and Windows prerequisite/runtime-policy checks, Release-assertion and CTest safety contracts, Visual Studio 2022 x64 configure, MSVC Debug build and deterministic Debug tests, MSVC Release build and deterministic Release tests, Release dependency/prerequisite checks, static milestone verifiers, and clean tracked-tree verification. The historical R0 runner itself was not executed.

Additional workflows on the same exact candidate also passed:

- Profiling capture portability run `35715362665`.
- Release manifest integrity run `35715362650`.

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

The registered executor must retain the exact source SHA, machine and Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, full stdout/stderr, exit codes, UTC timestamps, and screenshots at normal and narrow/short sizes. Human-visible acceptance should confirm the Outliner visibly remains on Cube when the Inspector shows the Cube fixture and that viewport/panels do not bleed during resize.

## Result and next action

Status: **resize ownership is now fail-closed and hosted Debug/Release compile verified; native editor smoke execution and fresh independent review remain pending**.

The single next useful action is to run exact-current-head `EditorRuntimeSmoke` in Debug and Release on the registered owned Windows desktop, preserve the required receipts/screenshots, and obtain independent review of the exact final head. Do not start dependent scene-document, transform-gizmo, save/reopen, or undo/redo work on hosted compilation alone. Issue #7 remains separate and open; the R0 runner was not executed by this packet.
