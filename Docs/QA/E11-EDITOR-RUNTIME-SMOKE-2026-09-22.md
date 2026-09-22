# E11 Editor Runtime Smoke evidence, 2026-09-22

## Checkpoint

This is the current evidence record for the bounded `EditorRuntimeSmoke` packet admitted from `main` at `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1` on branch `engine/2026-09-22-editor-runtime-smoke`.

Scope remains verification-only. The packet adds and hardens one native Windows smoke for the already-integrated `AstralEditor`; it does not add scene mutation or serialization, transform gizmos, Play-in-Editor, asset import, renderer/API changes, dependencies, game content, scheduler operations, deployment, release, or R0 execution.

## Current exact implementation

Latest code candidate: `a8c6344266a0cf765267c1394146859d91921ab1`.

Relevant published blobs for that candidate:

- `Tests/EditorRuntimeSmoke.cpp`: `1b9316fa5b595d49f329c91644e69532993f16f6`
- `CMakeLists.txt`: `945e0f5e28a0ecd91338541984cdc240bf986f82`
- `Tasks/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`: `855a48974a24f94a50880aea418f6dba471e7bde`

`CMakeLists.txt` registers the test through `astral_add_test`. The `RuntimeSmoke` suffix keeps the existing Release-assertion protection, `RUN_SERIAL`, and 180-second timeout contract. Hosted deterministic CTest intentionally excludes RuntimeSmoke execution.

The smoke currently requires all of the following before reporting PASS:

1. The same visible top-level HWND owned by the launched `AstralEditor` process is the only such process-owned window for 20 consecutive 50 ms observations at startup and again after interaction.
2. The top-level window has class `AstralEditorWindow` and title `Astral Editor 0.1`.
3. Exactly 12 direct E11 child controls exist with the expected class counts, and all five pending toolbar actions remain visible and disabled.
4. The Outliner contains five rows, the Assets list contains four placeholders, Outliner index 0 is exactly `Scene Root`, Outliner index 3 is exactly `Cube`, and the initial selection is index 0.
5. The initial Inspector text exactly matches the complete Scene Root fixture.
6. After setting Outliner index 3 and sending bounded `LBN_SELCHANGE`, the smoke re-queries `LB_GETCURSEL`, requires index 3 to remain selected, re-reads that selected row and requires it to equal `Cube`, and separately requires the Inspector to exactly match the complete Cube fixture. This closes the reviewed false-pass path where the handler could update the Inspector and then clear or move the Outliner selection.
7. Normal 800x600 and narrow/short 420x260 resizes use `SWP_ASYNCWINDOWPOS`, complete inside a bounded poll, and leave every direct child control inside the actual client rectangle.
8. Shutdown revalidates HWND process ownership immediately before one `WM_CLOSE`, waits boundedly for exit code 0, and failure cleanup may terminate only the retained handle for the process launched by the smoke.

Cross-process synchronous messages use `SendMessageTimeoutW` with a one-second deadline. No global keyboard/mouse injection is used.

## Primary-source research rechecked 2026-09-22

- Epic Games, Unreal Engine 5.8, Unreal Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
  - Applicability: Epic documents that selecting an Actor in the Level Viewport or Outliner updates the selected Actor state and Details panel. Astral's runtime smoke therefore treats Outliner selection and Inspector contents as one synchronized contract, not two independent observations.
  - Licensing: behavioral/interface reference only. No Epic source, UI assets, artwork, or project content is copied.
- Epic Games, Unreal Engine 5.8, Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
  - Applicability: the Outliner is a selection surface for scene Actors.
  - Licensing: behavioral reference only.
- Unity Manual, Unity 6.0, Inspect items: https://docs.unity3d.com/6000.0/Documentation/Manual/InspectorItems.html
  - Applicability: Unity's Inspector reflects the current selected GameObject/item, reinforcing the same selection-to-properties synchronization requirement.
  - Licensing: behavioral reference only. No Unity source or assets are copied.
- Microsoft Win32 `LB_GETCURSEL`: https://learn.microsoft.com/windows/win32/controls/lb-getcursel
  - Applicability: re-query the current single-selection list-box index after the `LBN_SELCHANGE` notification returns; `LB_ERR` means there is no selection.
- Microsoft Win32 `LB_GETTEXTLEN`: https://learn.microsoft.com/windows/win32/controls/lb-gettextlen
  - Applicability: size the bounded buffer before reading the selected Outliner row.
- Microsoft Win32 `LB_GETTEXT`: https://learn.microsoft.com/windows/win32/controls/lb-gettext
  - Applicability: read and compare the selected Outliner row identity after notification.
- Microsoft Win32 `SendMessageTimeoutW`: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-sendmessagetimeoutw
  - Applicability: keep cross-process list-box and command interactions locally bounded.
- Microsoft Win32 `SetWindowPos`: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setwindowpos
  - Applicability: `SWP_ASYNCWINDOWPOS` prevents the smoke from synchronously blocking on a different input queue during resize.
- Microsoft Win32 `GetWindowThreadProcessId`: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-getwindowthreadprocessid
  - Applicability: bind observed HWNDs to the exact process launched by the smoke before interaction and close.

## Independent-review findings addressed in this pass

Independent Codex review of prior exact head `a31853975e26e85792fc43f760af673fccb63fe2` identified two P2 issues:

- The smoke proved `LB_SETCURSEL` returned index 3 before `LBN_SELCHANGE`, then checked only the Inspector after the handler returned. A handler regression could therefore leave the Inspector showing Cube while the Outliner no longer selected Cube.
- This QA receipt still described an older candidate and blob set, so it could send the native executor to superseded evidence.

Code candidate `a8c6344266a0cf765267c1394146859d91921ab1` addresses the first issue by re-reading both selected index and selected item identity after notification. This document addresses the second by making the exact current candidate, blob, source references, commands, hosted receipts, limitations, and local handoff authoritative here.

A new independent review of the exact final evidence head is still required. Review by the implementation author is not independent acceptance.

## Coordinator sandbox evidence

A disposable C++17 source-logic fixture models the post-notification synchronization rule. It accepts only `{selected index 3, selected text Cube, complete Cube Inspector}` and rejects a cleared/moved selection, the wrong selected row text, or truncated Inspector text.

Fixture SHA-256: `5513af73909d7dc98c119c369d96d2c25ff3b77eb69d902cc7cd86d2f802dada`.

Commands executed:

```bash
g++ -std=c++17 -Wall -Wextra -Werror /tmp/e11_post_notify_fixture.cpp -o /tmp/e11_post_notify_fixture
/tmp/e11_post_notify_fixture
clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /tmp/e11_post_notify_fixture.cpp -o /tmp/e11_post_notify_fixture_san
ASAN_OPTIONS=detect_leaks=1 /tmp/e11_post_notify_fixture_san
sha256sum /tmp/e11_post_notify_fixture.cpp
```

All compile/run commands exited 0. This fixture is source-logic evidence only. It is not a substitute for running the production Win32 smoke.

## Hosted Windows evidence for the exact code candidate

GitHub Actions Windows Server 2022 run `35709984811`, job `106687972286`, executed against exact code candidate `a8c6344266a0cf765267c1394146859d91921ab1` and completed successfully on 2026-09-22.

Successful steps included:

- checkout and external build-root setup;
- R0 parser-only step plus runner-safety contracts, without executing the historical R0 runner;
- PE dependency, Windows prerequisite/runtime-policy, Release-assertion, and CTest safety contract checks;
- Visual Studio 2022 x64 configure;
- MSVC Debug build and deterministic Debug tests excluding RuntimeSmoke;
- MSVC Release build, runtime-dependency/prerequisite checks, and deterministic Release tests excluding RuntimeSmoke;
- static milestone verifiers;
- clean tracked-tree verification.

Additional workflows on the same exact code candidate also passed:

- Profiling capture portability run `35709984809`.
- Release manifest integrity run `35709984801`.

These hosted runs establish that the real Win32 `EditorRuntimeSmoke` source compiles in Debug and Release with the project configuration and that the non-runtime regression gates remain green. They do **not** establish native GUI execution because hosted deterministic CTest intentionally excludes every RuntimeSmoke target.

## Deferred native and independent evidence

The coordinator does not have the registered Windows interactive desktop and does not claim to have executed `EditorRuntimeSmoke` natively. E11 therefore still has no registered-local native GUI receipt and no independent acceptance. This packet is not evidence of UE5/Unity editor parity.

Registered-local execution remains:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

The registered executor must retain the exact source SHA, machine and Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, full stdout/stderr, exit codes, UTC timestamps, and screenshots at normal and narrow/short sizes. Human-visible acceptance should confirm the Outliner selection visibly remains on Cube when the Inspector shows the Cube fixture, and that the viewport/panels do not bleed during resize.

## Result and next action

Status: **post-notification Outliner/Inspector synchronization repair is hosted Debug/Release compile verified; QA receipt is current; native editor smoke execution and fresh independent review remain pending**.

The single next useful action is to run exact-current-head `EditorRuntimeSmoke` in Debug and Release on the registered owned Windows desktop, preserve the required receipts/screenshots, and obtain independent review of the exact final head. Do not start dependent scene-document, transform-gizmo, save/reopen, or undo/redo work on hosted compilation alone. Issue #7 remains separate and open; the R0 runner was not executed by this packet.
