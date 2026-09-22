# E11 Editor Runtime Smoke task, 2026-09-22

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. It may harden the native Windows smoke and its evidence, but it must not add scene mutation/serialization, gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed: `b3a2b1bf8f2b0c356d5b352006c48cb86532426b`.
Current code candidate: `9543ec4b022d2bd2249eb8560a2912d0606c0d5f`.
Current smoke blob: `63ae08458f6c277b184ce1d4ab1543cb8be5d92e`.
Integrated editor source fixture: `Tools/AstralEditorMain.cpp` blob `5142e632a79c89d0d0ce3efe87e752456f802185`; production editor source was not modified.

Allowed paths only: `CMakeLists.txt`, `Tests/EditorRuntimeSmoke.cpp`, this task, `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`, and `Docs/Research/ENGINE-CAPABILITIES.json`. One active writer only. Do not rebase, merge, force-push, or absorb unrelated game-worker work.

## Selected gap and repair

The smoke drives the editor through direct Win32 messages. Before candidate `9543ec4...` it proved the top-level editor and Outliner/assets list boxes were visible and correctly owned but did not prove those interactive surfaces were enabled. Direct test messages could therefore make a disabled editor, disabled Outliner, or disabled asset browser satisfy the smoke even though a human could not use the surface normally.

Candidate `9543ec4b022d2bd2249eb8560a2912d0606c0d5f` closes that false-pass path:

- `DirectVisibleControlOwnedByProcessAndParent` also requires `IsWindowEnabled`, covering every validated Outliner/assets operation;
- `ValidateShellState` requires the process-owned top-level editor to remain visible and enabled;
- selection and resize paths recheck the top-level enabled/visible state before interaction and during async resize polling;
- the Outliner is revalidated as enabled before `LB_SETCURSEL` and before `LBN_SELCHANGE` notification;
- the five unsupported toolbar actions still must remain visible and disabled;
- exact row identities, Inspector fixtures, original 12-child HWND continuity, bounded messages, resize containment/full-state checks, process-owned cleanup, Release assertions, and exclusive-desktop requirements are unchanged.

GitHub compare `8ca118792ecae2e37f8f80f461aaa72f7585a119...9543ec4b022d2bd2249eb8560a2912d0606c0d5f` reports exactly one commit and one changed file, `Tests/EditorRuntimeSmoke.cpp`, with 25 additions and 17 deletions. The write was re-read at blob `63ae08458f6c277b184ce1d4ab1543cb8be5d92e`.

## Research basis, rechecked 2026-09-22

- Microsoft Learn, `IsWindowEnabled`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindowenabled
- Microsoft Learn, `EnableWindow`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-enablewindow
- Epic Games, UE 5.8 Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
- Epic Games, UE 5.8 Selecting Actors: https://dev.epicgames.com/documentation/unreal-engine/selecting-actors-in-unreal-engine
- Epic Games, UE 5.8 Unreal Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
- Unity Technologies, Unity 6.0 Hierarchy: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html

Microsoft documents that enabled state controls mouse/keyboard input, that a child receives input only if enabled and visible, and that a disabled control cannot receive keyboard focus or user access. Epic documents interactive Actor selection/modification through the Outliner and synchronization with viewport/Details state. Unity documents the Hierarchy as the scene-object management surface. These are behavioral/API references only, not copied implementation or parity evidence.

## Portable reproduction

Disposable sandbox fixture: `/mnt/data/e11_interaction_enabled_fixture.cpp`.
SHA-256: `546a88fc599be0a54a69e6fa5e288b67fb6980b9052708e29196425101b38a13`.

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror /mnt/data/e11_interaction_enabled_fixture.cpp -o /mnt/data/e11_interaction_enabled_fixture
/mnt/data/e11_interaction_enabled_fixture
clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /mnt/data/e11_interaction_enabled_fixture.cpp -o /mnt/data/e11_interaction_enabled_fixture_san
ASAN_OPTIONS=detect_leaks=1 /mnt/data/e11_interaction_enabled_fixture_san
sha256sum /mnt/data/e11_interaction_enabled_fixture.cpp
```

GCC warning-clean compile/execution: PASS. Clang ASan+UBSan warning-clean compile/execution: PASS, no sanitizer finding. The fixture shows the old predicate accepts disabled top-level, disabled Outliner, and disabled Assets mutations, while the hardened predicate rejects them and still rejects an erroneously enabled pending toolbar action. This is source-logic evidence, not native Win32 execution.

## Acceptance contract

The smoke may report PASS only if: one stable visible/enabled process-owned top-level HWND is preserved; exact top-level class/title and original 12 child HWND/class identities remain; five pending toolbar buttons remain exact/visible/disabled; shell labels/status and Inspector remain exact; Outliner/Assets remain exact visible/enabled process-owned list boxes; all five ordered Outliner rows and all four ordered asset rows remain exact; Cube selection/notification preserves row 3, Cube Inspector state, and original handles; bounded 800x600 and 420x260 resizes preserve enabled/visible state, ownership, containment, and full shell state; shutdown exits 0 through the retained process; and every synchronous cross-process message remains bounded. Never weaken an acceptance check to make the gate green.

## Hosted verification

Exact candidate `9543ec4b022d2bd2249eb8560a2912d0606c0d5f` completed all available hosted workflows successfully:

- Windows build and deterministic tests `35785913785`, job `106942555966`: `completed/success` on exact head; safety contracts, VS2022 x64 configure, MSVC Debug build/tests, MSVC Release build/tests, dependency/prerequisite/runtime-policy checks, static verifiers, and clean-tree verification all passed;
- profiling capture portability `35785913791`: `completed/success`;
- release manifest integrity `35785913903`: `completed/success`.

Hosted deterministic CTest intentionally excludes every `RuntimeSmoke`; these are compile/non-runtime regression results only. The historical R0 runner itself was not executed.

The clean Codex review of prior tree `bad411c9cfc1c7a3b6ec7038fee1b188f53130f2` predates this source change and does not accept candidate `9543ec4...`. Fresh independent review of the final evidence head is required.

## Registered-local handoff

Run the final candidate on one owned interactive Windows desktop using external build output:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, full stdout/stderr, exit codes, UTC timestamps, and normal plus narrow/short screenshots. Human-visible acceptance must confirm the exact rows, mouse/keyboard usability of Outliner/assets, Cube synchronization across Outliner/Inspector/viewport, truthful disabled pending tools, containment at both sizes, and continuity of the original shell handles.

## Stop and next action

E11 remains partial. `native_evidence` is empty and final independent acceptance is false. Issue #7 is still open; do not invoke the historical R0 runner. Do not begin dependent editor-feature work based on hosted compilation alone.

Single next useful action: obtain fresh independent review of the final evidence head, then execute Debug and Release `EditorRuntimeSmoke` on the registered Windows desktop with retained receipts/screenshots.
