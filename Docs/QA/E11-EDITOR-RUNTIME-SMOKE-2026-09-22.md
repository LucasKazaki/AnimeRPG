# E11 Editor Runtime Smoke evidence, 2026-09-22

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. This packet verifies the already-integrated Win32 `AstralEditor`; it does not authorize scene mutation/serialization, transform gizmos, Play-in-Editor, asset import, graphics/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed: `b3a2b1bf8f2b0c356d5b352006c48cb86532426b`.
Current code candidate: `9543ec4b022d2bd2249eb8560a2912d0606c0d5f`.
`Tests/EditorRuntimeSmoke.cpp` blob: `63ae08458f6c277b184ce1d4ab1543cb8be5d92e`.
Integrated editor source fixture: `Tools/AstralEditorMain.cpp` blob `5142e632a79c89d0d0ce3efe87e752456f802185`; production editor source was not modified by this packet.

No rebase, force push, merge, dependency addition, architecture/API change, local scheduler mutation, content restart, deployment, release, or R0 execution was performed.

## Gap repaired

The prior smoke could false-pass an editor that was visible and correctly owned but disabled. It selected and read the Outliner through direct Win32 messages, and those messages can exercise controls independently of normal user input. A disabled top-level editor or disabled Outliner/assets list box therefore could satisfy the smoke even though the surface was not usable with normal mouse and keyboard input.

Candidate `9543ec4...` closes that gap:

- the top-level editor must remain process-owned, visible, and enabled during full shell validation, before selection, before resize, and while waiting for async resize completion;
- Outliner and Assets validation now includes `IsWindowEnabled` in addition to PID, parent, class, control-ID, and visibility checks;
- the Outliner is revalidated as enabled before the side-effecting `LB_SETCURSEL` and again before the bounded `LBN_SELCHANGE` notification;
- five unsupported toolbar actions remain required to be visible and disabled;
- exact Outliner/assets rows, Inspector contents, child-HWND continuity, bounded messages, resize containment/full state, cleanup, Release assertions, and exclusive-desktop requirements are preserved.

## Primary-source research, accessed 2026-09-22

Behavioral/API comparison only. No proprietary engine source was copied and no dependency was imported.

- Microsoft Learn, `IsWindowEnabled`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindowenabled
- Microsoft Learn, `EnableWindow`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-enablewindow
- Epic Games, Unreal Engine 5.8, Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
- Epic Games, Unreal Engine 5.8, Selecting Actors: https://dev.epicgames.com/documentation/unreal-engine/selecting-actors-in-unreal-engine
- Epic Games, Unreal Engine 5.8, Unreal Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
- Unity Technologies, Unity 6.0, Hierarchy window: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html

Microsoft specifies that `IsWindowEnabled` tests whether a window is enabled for mouse/keyboard input, and that a child receives input only if both enabled and visible. `EnableWindow` documents that disabling prevents mouse/key input and that a disabled control cannot receive keyboard focus or user access. Epic documents that the Outliner supports Actor selection/modification and synchronizes selection with the viewport/Details panel. Unity documents the Hierarchy as the scene-object management surface. These references justify requiring Astral's tested editor surfaces to be actually interactive rather than merely addressable through test messages. They do not imply parity.

## Portable reproduction and compiler evidence

Disposable coordinator-sandbox fixture: `/mnt/data/e11_interaction_enabled_fixture.cpp`.
SHA-256: `546a88fc599be0a54a69e6fa5e288b67fb6980b9052708e29196425101b38a13`.

The fixture demonstrates that the former acceptance predicate accepts disabled top-level, disabled Outliner, and disabled Assets mutations when the rest of the shell state is intact. The hardened predicate accepts the valid baseline, rejects each disabled interactive-surface mutation, and still rejects an erroneously enabled pending toolbar action.

Commands executed:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror /mnt/data/e11_interaction_enabled_fixture.cpp -o /mnt/data/e11_interaction_enabled_fixture
/mnt/data/e11_interaction_enabled_fixture
clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /mnt/data/e11_interaction_enabled_fixture.cpp -o /mnt/data/e11_interaction_enabled_fixture_san
ASAN_OPTIONS=detect_leaks=1 /mnt/data/e11_interaction_enabled_fixture_san
sha256sum /mnt/data/e11_interaction_enabled_fixture.cpp
```

Results: GCC warning-clean compile/execution PASS; Clang ASan+UBSan warning-clean compile/execution PASS; no sanitizer finding. This is source-logic evidence only, not native Win32 execution.

## GitHub/source verification

GitHub compare `8ca118792ecae2e37f8f80f461aaa72f7585a119...9543ec4b022d2bd2249eb8560a2912d0606c0d5f` reports exactly one commit, exactly one changed file (`Tests/EditorRuntimeSmoke.cpp`), 25 additions and 17 deletions. The resulting smoke was re-read at blob `63ae08458f6c277b184ce1d4ab1543cb8be5d92e` and contains the enabled-state guards described above.

The immediately preceding evidence head `8ca118792ecae2e37f8f80f461aaa72f7585a119` completed all hosted workflows successfully:

- Windows build and deterministic tests `35779487811`: `completed/success`;
- profiling capture portability `35779487558`: `completed/success`;
- release manifest integrity `35779487492`: `completed/success`.

Those checks predate candidate `9543ec4...` and are not attributed to the changed smoke.

For exact source candidate `9543ec4...`, workflows auto-triggered:

- profiling capture portability `35785913791`: `completed/success` at the latest observation;
- Windows build and deterministic tests `35785913785`: still `in_progress` at the latest observation;
- release manifest integrity `35785913903`: still `in_progress` at the latest observation.

No result is claimed for an incomplete workflow. Hosted deterministic CTest intentionally excludes every `RuntimeSmoke`, so hosted success remains compile/non-runtime regression evidence rather than native editor GUI acceptance.

## Independent review state

The clean Codex review on prior evidence tree `bad411c9cfc1c7a3b6ec7038fee1b188f53130f2` predates candidate `9543ec4...` and does not accept this source change. Fresh independent source review is required after the final evidence reconciliation. Same-author inspection is not independent acceptance.

## Native evidence and handoff

`native_evidence` remains empty. This coordinator did not access or claim a registered Windows interactive desktop.

Run the final candidate on one owned interactive Windows desktop using external build output:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine/Windows identity, MSVC and CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, and normal plus narrow/short screenshots. Human-visible acceptance must confirm all five exact Outliner rows; all four exact asset rows; mouse/keyboard usability of Outliner/assets; Cube synchronization across Outliner, Inspector and viewport `Selected:` text; truthful disabled pending tools; panel containment after both sizes; and continuity of the original twelve child HWND identities.

## Result

Status: **enabled-state false-pass repaired and portable mutation fixture passed; exact candidate hosted profiling passed while exact-candidate Windows/release-manifest checks and fresh independent review remain pending; native Debug/Release RuntimeSmoke remains pending**.

E11 remains a partial editor-shell candidate, not UE5/Unity parity. No native GUI, GPU/performance, clean-machine, stress/recovery, soak, or final independent runtime acceptance claim is made. Issue #7 remains open and the historical R0 runner was not invoked.

Single next useful action: complete exact-head hosted checks and fresh independent review, then execute Debug and Release `EditorRuntimeSmoke` on the registered Windows desktop with required receipts/screenshots.
