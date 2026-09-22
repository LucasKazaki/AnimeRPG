# E11 Editor Runtime Smoke task, 2026-09-22

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. This packet may harden the native Windows smoke and its evidence, but it must not add scene mutation/serialization, transform gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed this pass: `b3a2b1bf8f2b0c356d5b352006c48cb86532426b`. Do not rebase, merge, force-push, or absorb unrelated game-worker work in this packet.
Current code candidate: `0d2524c5e22035e3bf3a0f762e57616f896a9f57`.
Current smoke blob: `c493cb962f86a054e0e2a60dcd934ef4610196fd`.
Integrated editor source fixture blob: `Tools/AstralEditorMain.cpp` `5142e632a79c89d0d0ce3efe87e752456f802185`; that production editor file is not owned by this packet and was not modified.

Allowed paths only:

- `CMakeLists.txt`
- `Tests/EditorRuntimeSmoke.cpp`
- `Tasks/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`
- `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`
- `Docs/Research/ENGINE-CAPABILITIES.json`

One active writer only. Stop on unexpected edits outside these paths, stale ownership, an introduced regression, any architecture/dependency change, or a gate requiring the registered Windows desktop.

## Selected gap and implementation

The prior exact-head independent review was clean, but source inspection found a reproducible false-pass gap in the current acceptance test. `AstralEditor` creates five fixed Outliner rows in this order: `Scene Root`, `Camera`, `Directional Light`, `Cube`, `Floor`. The smoke required count 5 but only verified rows 0 and 3. Therefore renamed or reordered rows 1, 2, or 4 could pass while the editor's scene-object surface no longer represented the integrated fixture.

Code candidate `0d2524c5e22035e3bf3a0f762e57616f896a9f57` repairs that gap without changing the production editor:

- adds one exact five-row `kExpectedOutlinerItems` fixture matching the integrated editor source;
- derives the expected Outliner and Assets counts from their fixture arrays;
- reads every Outliner row through the existing bounded, PID/parent/class/visibility/control-ID-revalidated list-box path;
- requires rows 0 through 4 to equal `Scene Root`, `Camera`, `Directional Light`, `Cube`, `Floor` in order;
- keeps all existing handle-continuity, selection, Inspector, asset-row, resize, timeout, cleanup, Release-assertion, and exclusive-desktop requirements unchanged.

No proprietary engine source was copied and no dependency was imported.

## Research basis, rechecked 2026-09-22

Primary behavioral references:

- Epic Games, Unreal Engine 5.8, Unreal Editor Interface: https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-editor-interface
- Epic Games, Unreal Engine 5.8, Selecting Actors: https://dev.epicgames.com/documentation/en-us/unreal-engine/selecting-actors-in-unreal-engine
- Unity Technologies, Unity 6.0, Hierarchy window: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html
- Unity Technologies, Unity 6.0, Inspect items: https://docs.unity3d.com/6000.0/Documentation/Manual/Inspector.html

Epic documents the Outliner as a hierarchical tree of level content and synchronized selection with the viewport/Details workflow. Unity documents the Hierarchy as the scene-object management surface and the Inspector as reflecting the selected object. Applicability here is behavioral only: an editor-shell smoke should prove the identities of all objects in the fixed test scene rather than accept count plus two sentinel rows. These public docs do not grant permission to copy proprietary source or make an engine-parity claim.

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

Results: GCC warning-clean compile/execution PASS; Clang ASan+UBSan warning-clean compile/execution PASS; no sanitizer finding. This is a source-logic fixture, not native Win32 execution.

## Acceptance contract

The smoke may report PASS only when all of the following remain true:

1. One stable visible process-owned top-level editor HWND is preserved through startup and final validation.
2. Top-level class/title are exact, and the original twelve direct process-owned child HWND/class identities remain unchanged.
3. Five pending toolbar buttons retain exact labels, visibility and disabled state.
4. Shell statics, status text, Inspector fixture, Outliner/Assets identities and control IDs remain exact.
5. Outliner has exactly five rows and every row is exact and ordered: `Scene Root`, `Camera`, `Directional Light`, `Cube`, `Floor`.
6. Assets has exactly four exact ordered rows: `Primitive/Cube`, `Primitive/Plane`, `Camera`, `DirectionalLight`.
7. Cube selection plus bounded `LBN_SELCHANGE` preserves row 3 selection, exact Cube Inspector state, and the original shell-control handles.
8. Bounded asynchronous 800x600 and 420x260 resize checks preserve ownership, the original controls, containment, complete shell state, and the exact row fixtures.
9. Shutdown revalidates ownership, posts one `WM_CLOSE`, obtains exit code 0 within the bounded wait, and failure cleanup can terminate only the retained child process handle.
10. Cross-process synchronous messages remain bounded with `SendMessageTimeoutW`; `EditorRuntimeSmoke` remains registered through `astral_add_test`, `RUN_SERIAL`, and the existing CTest timeout.

Never weaken an acceptance check to make the gate green.

## Verification state

The code write was verified from GitHub commit diff: commit `0d2524c5e22035e3bf3a0f762e57616f896a9f57` changes only `Tests/EditorRuntimeSmoke.cpp` and contains the complete-five-row repair described above.

Hosted workflows triggered for exact code candidate `0d2524c5...`:

- Profiling capture portability run `35773477112`: `completed/success`.
- Windows build and deterministic tests run `35773476962`: in progress at the latest observation; no result claimed yet.
- Release manifest integrity run `35773476964`: in progress at the latest observation; no result claimed yet.

Hosted deterministic CTest intentionally excludes every `RuntimeSmoke`, so even a future green Windows run is compilation/non-runtime evidence, not native editor GUI execution.

The prior independent Codex review applies to the superseded exact tree `75727914e37f3c16628607c9b7bb232949359dbe`. Because this pass changes smoke code, a fresh independent review of the final evidence head is required. Same-author inspection is not independent acceptance.

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

Single next useful action: finish exact-head hosted verification and fresh independent review for this complete-Outliner candidate, then run Debug and Release `EditorRuntimeSmoke` on the registered Windows desktop with retained receipts/screenshots.
