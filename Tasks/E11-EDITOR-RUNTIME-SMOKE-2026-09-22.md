# E11 Editor Runtime Smoke task, 2026-09-22

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. This packet may harden the native Windows smoke and its evidence, but it must not add scene mutation/serialization, transform gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed this pass: `b3a2b1bf8f2b0c356d5b352006c48cb86532426b`. Do not rebase, merge, force-push, or absorb unrelated game-worker work in this packet.
Current code candidate: `0d2524c5e22035e3bf3a0f762e57616f896a9f57`.
Current smoke blob: `c493cb962f86a054e0e2a60dcd934ef4610196fd`.
Latest exact hosted evidence head before this evidence-only reconciliation: `988186b917a3b9e547224ffb1d6618c607490fbb`.
Integrated editor source fixture blob: `Tools/AstralEditorMain.cpp` `5142e632a79c89d0d0ce3efe87e752456f802185`; that production editor file is not owned by this packet and was not modified.

Allowed paths only:

- `CMakeLists.txt`
- `Tests/EditorRuntimeSmoke.cpp`
- `Tasks/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`
- `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`
- `Docs/Research/ENGINE-CAPABILITIES.json`

One active writer only. Stop on unexpected edits outside these paths, stale ownership, an introduced regression, any architecture/dependency change, or a gate requiring the registered Windows desktop.

## Selected gap and implementation

Source inspection found a reproducible false-pass gap in the acceptance test. `AstralEditor` creates five fixed Outliner rows in this order: `Scene Root`, `Camera`, `Directional Light`, `Cube`, `Floor`. The earlier smoke required count 5 but only verified rows 0 and 3. Renamed or reordered rows 1, 2, or 4 could therefore pass while the editor's scene-object surface no longer represented the integrated fixture.

Code candidate `0d2524c5e22035e3bf3a0f762e57616f896a9f57` repairs that gap without changing the production editor:

- adds one exact five-row `kExpectedOutlinerItems` fixture matching the integrated editor source;
- derives expected Outliner and Assets counts from their fixture arrays;
- reads every Outliner row through the existing bounded, PID/parent/class/visibility/control-ID-revalidated list-box path;
- requires rows 0 through 4 to equal `Scene Root`, `Camera`, `Directional Light`, `Cube`, `Floor` in order;
- keeps all existing handle-continuity, selection, Inspector, asset-row, resize, timeout, cleanup, Release-assertion, and exclusive-desktop requirements unchanged.

No proprietary engine source was copied and no dependency was imported.

## Research basis, rechecked 2026-09-22

Primary behavioral references:

- Epic Games, Unreal Engine 5.8, Unreal Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
- Epic Games, Unreal Engine 5.8, Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
- Epic Games, Unreal Engine 5.8, Selecting Actors: https://dev.epicgames.com/documentation/unreal-engine/selecting-actors-in-unreal-engine
- Unity Technologies, Unity 6.0, Hierarchy window: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html
- Unity Technologies, Unity 6.0, Inspector: https://docs.unity3d.com/6000.0/Documentation/Manual/Inspector.html

Epic documents the Outliner as the level's hierarchical object surface and synchronizes selection among Outliner, Viewport and Details. Unity documents the Hierarchy as the scene-object management surface and the Inspector as selected-object state. Applicability here is behavioral only: a fixed editor-shell acceptance fixture must prove all of its object identities, not merely a row count plus sentinels. These public references do not grant permission to copy proprietary source and do not establish engine parity.

## Portable reproduction

Disposable coordinator-sandbox fixture: `/mnt/data/e11_outliner_identity_fixture.cpp`.
SHA-256: `8944112d252d2b2c83a28d2e598b6d47ea516266319853e5abadeb19dc170f63`.

The fixture demonstrates that the former pair-only predicate accepts wrong `Camera`, wrong `Directional Light`, wrong `Floor`, and reordered middle/tail rows, while the complete five-row predicate rejects each mutation and accepts the exact fixture.

Commands executed:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror /mnt/data/e11_outliner_identity_fixture.cpp -o /mnt/data/e11_outliner_identity_fixture
/mnt/data/e11_outliner_identity_fixture
clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /mnt/data/e11_outliner_identity_fixture.cpp -o /mnt/data/e11_outliner_identity_fixture_san
ASAN_OPTIONS=detect_leaks=1 /mnt/data/e11_outliner_identity_fixture_san
sha256sum /mnt/data/e11_outliner_identity_fixture.cpp
```

Results: GCC warning-clean compile/execution PASS; Clang ASan+UBSan warning-clean compile/execution PASS; no sanitizer finding. This is source-logic evidence, not native Win32 execution.

## Acceptance contract

The smoke may report PASS only when all of the following remain true:

1. One stable visible process-owned top-level editor HWND is preserved through startup and final validation.
2. Top-level class/title are exact, and the original twelve direct process-owned child HWND/class identities remain unchanged.
3. Five pending toolbar buttons retain exact labels, visibility and disabled state.
4. Shell statics, status text, Inspector fixture, Outliner/Assets identities and control IDs remain exact.
5. Outliner has exactly five exact ordered rows: `Scene Root`, `Camera`, `Directional Light`, `Cube`, `Floor`.
6. Assets has exactly four exact ordered rows: `Primitive/Cube`, `Primitive/Plane`, `Camera`, `DirectionalLight`.
7. Cube selection plus bounded `LBN_SELCHANGE` preserves row 3 selection, exact Cube Inspector state, and the original shell-control handles.
8. Bounded asynchronous 800x600 and 420x260 resize checks preserve ownership, the original controls, containment, complete shell state, and exact row fixtures.
9. Shutdown revalidates ownership, posts one `WM_CLOSE`, obtains exit code 0 within the bounded wait, and failure cleanup can terminate only the retained child process handle.
10. Cross-process synchronous messages remain bounded with `SendMessageTimeoutW`; `EditorRuntimeSmoke` remains registered through `astral_add_test`, `RUN_SERIAL`, and the existing CTest timeout.

Never weaken an acceptance check to make the gate green.

## Verification state

The code write was verified from GitHub history: candidate `0d2524c5e22035e3bf3a0f762e57616f896a9f57` changed only `Tests/EditorRuntimeSmoke.cpp` for the complete-five-row repair. Its profiling run `35773477112` and release-manifest run `35773476964` passed. Its Windows run `35773476962` was cancelled after a newer evidence head superseded it, so no pass is claimed for that cancelled run.

Exact evidence head `988186b917a3b9e547224ffb1d6618c607490fbb`, containing the same smoke blob `c493cb962f86a054e0e2a60dcd934ef4610196fd`, then completed all hosted workflows successfully:

- Windows build and deterministic tests run `35773676765`: `completed/success`;
- profiling capture portability run `35773676786`: `completed/success`;
- release manifest integrity run `35773676793`: `completed/success`.

Hosted deterministic CTest intentionally excludes every `RuntimeSmoke`, so these results prove compile and deterministic non-runtime regression status only. They do not establish native editor GUI execution.

The prior clean independent Codex review applies to superseded tree `75727914e37f3c16628607c9b7bb232949359dbe`. Because `0d2524c5...` changes smoke code, a fresh independent review of the final evidence tree is required. Same-author inspection is not independent acceptance.

## Registered-local handoff

Run the final code candidate on one owned interactive Windows desktop using external build output:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, full stdout/stderr, exit codes, UTC timestamps, and normal plus narrow/short screenshots. Human-visible acceptance must confirm all five exact Outliner rows, all four exact asset rows, Cube synchronization across Outliner/Inspector/viewport, truthful shell labels/status, panel containment, and continuity of the original shell control inventory.

## Stop and next action

E11 remains partial. `native_evidence` remains empty and final independent acceptance remains false. Do not begin dependent scene-document, gizmo, undo/redo, save/reopen, or other editor feature work based on hosted compilation alone. Issue #7 remains separate and open; do not invoke the historical R0 runner.

Single next useful action: independently review the final evidence head for this complete-Outliner candidate, then execute Debug and Release `EditorRuntimeSmoke` on the registered Windows desktop with retained receipts/screenshots.
