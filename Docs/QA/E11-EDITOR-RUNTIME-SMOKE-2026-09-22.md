# E11 Editor Runtime Smoke evidence, 2026-09-22

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. This packet verifies the already-integrated Win32 `AstralEditor`; it does not authorize scene mutation/serialization, gizmos, Play-in-Editor, asset import, graphics/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed: `b3a2b1bf8f2b0c356d5b352006c48cb86532426b`.
Current code candidate: `9543ec4b022d2bd2249eb8560a2912d0606c0d5f`.
`Tests/EditorRuntimeSmoke.cpp` blob: `63ae08458f6c277b184ce1d4ab1543cb8be5d92e`.
Integrated editor source fixture: `Tools/AstralEditorMain.cpp` blob `5142e632a79c89d0d0ce3efe87e752456f802185`; production editor source was not modified.

## Finding and repair

The previous smoke could false-pass a visible, correctly owned but disabled editor surface because it exercised controls through direct Win32 messages instead of actual mouse/keyboard input. Candidate `9543ec4...` therefore adds enabled-state validation to the interactive surfaces while preserving the intentional disabled state of unsupported toolbar actions.

The top-level editor must remain visible and enabled during shell validation, selection, and resize. Outliner and Assets list boxes must remain visible, enabled, correctly owned, direct children of the editor, correct class, and correct control ID. The Outliner is revalidated before selection and before `LBN_SELCHANGE`. Existing exact rows, Inspector fixtures, original 12-child HWND continuity, bounded cross-process messages, resize containment/full-state checks, and process-owned shutdown remain intact.

GitHub compare `8ca118792ecae2e37f8f80f461aaa72f7585a119...9543ec4b022d2bd2249eb8560a2912d0606c0d5f` reports exactly one commit and one changed file, `Tests/EditorRuntimeSmoke.cpp`, with 25 additions and 17 deletions. The resulting source was re-read at blob `63ae08458f6c277b184ce1d4ab1543cb8be5d92e`.

## Primary-source research, accessed 2026-09-22

- Microsoft Learn, `IsWindowEnabled`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindowenabled
- Microsoft Learn, `EnableWindow`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-enablewindow
- Epic Games, UE 5.8 Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
- Epic Games, UE 5.8 Selecting Actors: https://dev.epicgames.com/documentation/unreal-engine/selecting-actors-in-unreal-engine
- Epic Games, UE 5.8 Unreal Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
- Unity Technologies, Unity 6.0 Hierarchy: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html

Microsoft documents that enabled state controls mouse/keyboard input, a child receives input only if enabled and visible, and a disabled control cannot receive keyboard focus or user access. Epic documents interactive Actor selection/modification through the Outliner and synchronized viewport/Details selection. Unity documents the Hierarchy as its scene-object management surface. These references are used only for behavior/API comparison; no proprietary source was copied and no dependency was imported.

## Portable reproduction and compiler evidence

Disposable sandbox fixture: `/mnt/data/e11_interaction_enabled_fixture.cpp`.
SHA-256: `546a88fc599be0a54a69e6fa5e288b67fb6980b9052708e29196425101b38a13`.

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror /mnt/data/e11_interaction_enabled_fixture.cpp -o /mnt/data/e11_interaction_enabled_fixture
/mnt/data/e11_interaction_enabled_fixture
clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /mnt/data/e11_interaction_enabled_fixture.cpp -o /mnt/data/e11_interaction_enabled_fixture_san
ASAN_OPTIONS=detect_leaks=1 /mnt/data/e11_interaction_enabled_fixture_san
sha256sum /mnt/data/e11_interaction_enabled_fixture.cpp
```

GCC warning-clean compile/execution: PASS. Clang ASan+UBSan warning-clean compile/execution: PASS with no sanitizer finding. The fixture proves the old predicate accepts disabled top-level, disabled Outliner, and disabled Assets mutations, while the hardened predicate rejects each mutation and still rejects an erroneously enabled pending toolbar action. This is source-logic evidence only, not native Win32 execution.

## Hosted verification

Exact candidate `9543ec4b022d2bd2249eb8560a2912d0606c0d5f` completed every available hosted workflow successfully:

- Windows build and deterministic tests run `35785913785`, job `106942555966`: `completed/success`, exact head `9543ec4...`, completed 2026-09-22T21:20:51Z. The job passed repository/R0 safety contracts, VS2022 x64 configure, MSVC Debug build and deterministic tests, MSVC Release build and deterministic tests, release dependency/prerequisite/runtime-policy checks, static milestone verifiers, and clean-tree verification.
- profiling capture portability run `35785913791`: `completed/success`.
- release manifest integrity run `35785913903`: `completed/success`.

Hosted deterministic CTest intentionally excludes every `RuntimeSmoke`. These results prove hosted compilation and deterministic non-runtime regression status, not native GUI acceptance. The historical R0 runner itself was not executed.

## Independent review state

The prior clean Codex review of tree `bad411c9cfc1c7a3b6ec7038fee1b188f53130f2` predates this source change and does not accept candidate `9543ec4...`. Fresh independent source review of the final evidence head remains required. Same-author inspection is not independent acceptance.

## Native evidence and handoff

`native_evidence` remains empty. No registered Windows interactive desktop was accessed or claimed by this coordinator.

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, and normal plus narrow/short screenshots. Human-visible acceptance must confirm exact Outliner/assets rows, mouse/keyboard usability of those enabled surfaces, Cube synchronization across Outliner/Inspector/viewport, truthful disabled pending tools, panel containment at both sizes, and continuity of the original 12 shell controls.

## Result

Status: **enabled-state false-pass repaired; portable GCC and Clang ASan+UBSan mutation fixture passed; exact candidate hosted Windows Debug/Release deterministic checks, profiling, and release-manifest workflows passed; fresh independent review and native Debug/Release `EditorRuntimeSmoke` remain pending**.

E11 remains a partial editor-shell candidate, not UE5/Unity parity. No native GUI, GPU/performance, clean-machine, stress/recovery, soak, or final independent runtime acceptance claim is made. Issue #7 remains open and the historical R0 runner was not invoked.

Single next useful action: obtain fresh independent review of the final evidence head, then execute Debug and Release `EditorRuntimeSmoke` on the registered Windows desktop with the required receipts/screenshots.
