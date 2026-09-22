# E11 Editor Runtime Smoke task, 2026-09-22

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. This packet may harden the native Windows smoke and its evidence, but it must not add scene mutation/serialization, transform gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed this pass: `b3a2b1bf8f2b0c356d5b352006c48cb86532426b`. Do not rebase, merge, force-push, or absorb unrelated game-worker work.
Current code candidate: `9543ec4b022d2bd2249eb8560a2912d0606c0d5f`.
Current smoke blob: `63ae08458f6c277b184ce1d4ab1543cb8be5d92e`.
Integrated editor source fixture: `Tools/AstralEditorMain.cpp` blob `5142e632a79c89d0d0ce3efe87e752456f802185`; that production editor file is not owned by this packet and was not modified.

Allowed paths only:

- `CMakeLists.txt`
- `Tests/EditorRuntimeSmoke.cpp`
- `Tasks/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`
- `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`
- `Docs/Research/ENGINE-CAPABILITIES.json`

One active writer only. Stop on unexpected edits outside these paths, stale ownership, an introduced regression, any architecture/dependency change, or a gate requiring the registered Windows desktop.

## Selected gap and implementation

Source inspection found another runtime-smoke false-pass path. The smoke drives the editor through direct Win32 messages. Before this candidate it proved the top-level editor and Outliner/assets list boxes were visible and correctly owned, but it did not prove those interactive surfaces were enabled. Direct `SendMessage` calls can still read or mutate a disabled control, so a disabled editor, disabled Outliner, or disabled asset browser could satisfy the automated checks even though a human could not use those surfaces with normal mouse/keyboard input.

Candidate `9543ec4b022d2bd2249eb8560a2912d0606c0d5f` repairs that gap without changing the production editor:

- `DirectVisibleControlOwnedByProcessAndParent` now also requires `IsWindowEnabled`, so every validated Outliner/assets operation fails closed if the list box is disabled;
- `ValidateShellState` requires the process-owned top-level editor to remain visible and enabled;
- selection and resize paths recheck top-level enabled/visible state immediately before interaction and during asynchronous resize polling;
- the Outliner is revalidated as enabled before `LB_SETCURSEL` and again before `LBN_SELCHANGE` notification;
- the existing five pending toolbar buttons remain required to be visible and disabled, preserving the truthful unsupported-tool contract;
- all prior exact row identities, Inspector fixtures, child-HWND continuity, bounded messages, resize containment/state, process-owned cleanup, Release assertions, and exclusive-desktop requirements remain unchanged.

No proprietary engine source was copied and no dependency, production editor source, architecture, or graphics API changed.

## Research basis, rechecked 2026-09-22

Primary behavioral/API references:

- Microsoft Learn, `IsWindowEnabled`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindowenabled
- Microsoft Learn, `EnableWindow`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-enablewindow
- Epic Games, Unreal Engine 5.8, Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
- Epic Games, Unreal Engine 5.8, Selecting Actors: https://dev.epicgames.com/documentation/unreal-engine/selecting-actors-in-unreal-engine
- Epic Games, Unreal Engine 5.8, Unreal Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
- Unity Technologies, Unity 6.0, Hierarchy window: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html

Microsoft documents that `IsWindowEnabled` determines whether a window is enabled for mouse and keyboard input and that a child receives input only when both enabled and visible. `EnableWindow` states that disabling a window prevents mouse/key input and a disabled control cannot receive keyboard focus or user access. Epic documents user selection and modification through the Outliner and synchronized selection with the viewport/Details panel. Unity documents its Hierarchy as the scene-object management surface. Applicability here is behavioral only: an acceptance smoke for an interactive editor surface must not pass solely because direct test messages work against a disabled control. These references do not establish UE5/Unity parity or license copying proprietary implementations.

## Portable reproduction

Disposable coordinator-sandbox fixture: `/mnt/data/e11_interaction_enabled_fixture.cpp`.
SHA-256: `546a88fc599be0a54a69e6fa5e288b67fb6980b9052708e29196425101b38a13`.

The fixture models the old and hardened acceptance predicates. The old predicate accepts disabled top-level, disabled Outliner, and disabled Assets cases when all other state is correct. The hardened predicate accepts the baseline, rejects each disabled interactive-surface mutation, and still rejects a pending toolbar button that becomes enabled.

Commands executed:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror /mnt/data/e11_interaction_enabled_fixture.cpp -o /mnt/data/e11_interaction_enabled_fixture
/mnt/data/e11_interaction_enabled_fixture
clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /mnt/data/e11_interaction_enabled_fixture.cpp -o /mnt/data/e11_interaction_enabled_fixture_san
ASAN_OPTIONS=detect_leaks=1 /mnt/data/e11_interaction_enabled_fixture_san
sha256sum /mnt/data/e11_interaction_enabled_fixture.cpp
```

Results: GCC C++17 warning-clean compile/execution PASS; Clang C++17 ASan+UBSan warning-clean compile/execution PASS; no sanitizer finding. This is source-logic evidence only, not native Win32 execution.

## Acceptance contract

The smoke may report PASS only when all of the following remain true:

1. One stable visible, enabled, process-owned top-level editor HWND is preserved through startup and final validation.
2. Top-level class/title are exact, and the original twelve direct process-owned child HWND/class identities remain unchanged.
3. Five pending toolbar buttons retain exact labels, visibility and disabled state.
4. Shell statics, status text, Inspector fixture, Outliner/Assets identities and control IDs remain exact.
5. Outliner and Assets remain visible and enabled interactive list-box surfaces owned by the editor process.
6. Outliner has exactly five exact ordered rows: `Scene Root`, `Camera`, `Directional Light`, `Cube`, `Floor`.
7. Assets has exactly four exact ordered rows: `Primitive/Cube`, `Primitive/Plane`, `Camera`, `DirectionalLight`.
8. Cube selection plus bounded `LBN_SELCHANGE` preserves row 3 selection, exact Cube Inspector state, enabled Outliner state, and the original shell-control handles.
9. Bounded asynchronous 800x600 and 420x260 resize checks preserve top-level enabled/visible state, ownership, original controls, containment, complete shell state, and exact row fixtures.
10. Shutdown revalidates ownership, posts one `WM_CLOSE`, obtains exit code 0 within the bounded wait, and failure cleanup can terminate only the retained child process handle.
11. Cross-process synchronous messages remain bounded with `SendMessageTimeoutW`; `EditorRuntimeSmoke` remains registered through `astral_add_test`, `RUN_SERIAL`, and the existing CTest timeout.

Never weaken an acceptance check to make the gate green.

## Verification state

The GitHub compare from prior head `8ca118792ecae2e37f8f80f461aaa72f7585a119` to candidate `9543ec4b022d2bd2249eb8560a2912d0606c0d5f` is exactly one commit and exactly one changed file, `Tests/EditorRuntimeSmoke.cpp`, with 25 additions and 17 deletions. The write was re-read at smoke blob `63ae08458f6c277b184ce1d4ab1543cb8be5d92e`.

The immediately preceding evidence-only head `8ca118792ecae2e37f8f80f461aaa72f7585a119` completed all hosted workflows successfully: Windows `35779487811`, profiling `35779487558`, and release-manifest `35779487492`. Those runs predate this source change and are not acceptance of candidate `9543ec4...`.

For candidate `9543ec4...`, hosted workflows were triggered automatically. At first observation, profiling run `35785913791` completed successfully while Windows run `35785913785` and release-manifest run `35785913903` were still in progress. Do not infer their result until completed. Hosted deterministic CTest intentionally excludes every `RuntimeSmoke`, so even a green hosted build is compile/non-runtime regression evidence only.

The clean Codex review of prior tree `bad411c9cfc1c7a3b6ec7038fee1b188f53130f2` predates this source change and cannot accept candidate `9543ec4...`. A fresh independent review of the final evidence head is required. Same-author inspection is not independent acceptance.

## Registered-local handoff

Run the final candidate on one owned interactive Windows desktop using external build output:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, full stdout/stderr, exit codes, UTC timestamps, and normal plus narrow/short screenshots. Human-visible acceptance must confirm all five exact Outliner rows, all four exact asset rows, mouse/keyboard usability of the enabled Outliner/assets surfaces, Cube synchronization across Outliner/Inspector/viewport, truthful disabled pending tools, panel containment, and continuity of the original shell control inventory.

## Stop and next action

E11 remains partial. `native_evidence` remains empty and final independent acceptance remains false. Do not begin dependent scene-document, gizmo, undo/redo, save/reopen, or other editor feature work based on hosted compilation alone. Issue #7 remains open; do not invoke the historical R0 runner.

Single next useful action: finish exact-head hosted verification and fresh independent review for this enabled-state candidate, then execute Debug and Release `EditorRuntimeSmoke` on the registered Windows desktop with retained receipts/screenshots.
