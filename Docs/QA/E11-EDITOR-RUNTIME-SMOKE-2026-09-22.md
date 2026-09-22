# E11 Editor Runtime Smoke evidence, 2026-09-22

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. This packet verifies the already-integrated Win32 `AstralEditor`; it does not authorize scene mutation/serialization, transform gizmos, Play-in-Editor, asset import, graphics/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed this pass: `b3a2b1bf8f2b0c356d5b352006c48cb86532426b`.
Current code candidate: `0d2524c5e22035e3bf3a0f762e57616f896a9f57`.
`Tests/EditorRuntimeSmoke.cpp` blob: `c493cb962f86a054e0e2a60dcd934ef4610196fd`.
Latest exact hosted evidence head before this documentation reconciliation: `988186b917a3b9e547224ffb1d6618c607490fbb`.
Integrated editor source fixture: `Tools/AstralEditorMain.cpp` blob `5142e632a79c89d0d0ce3efe87e752456f802185`; production editor source was not modified by this packet.

No rebase, force push, merge, dependency addition, architecture/API change, local scheduler mutation, content restart, deployment, or release was performed.

## Gap repaired

The integrated editor exposes five fixed Outliner rows in order: `Scene Root`, `Camera`, `Directional Light`, `Cube`, `Floor`. The prior smoke checked the five-row count but only asserted identities at rows 0 and 3. A regression renaming or reordering `Camera`, `Directional Light`, or `Floor` could therefore pass.

Candidate `0d2524c5...` removes that false-pass path. The smoke now defines the exact five-row Outliner fixture, derives the expected count from the fixture, reads every row through the existing bounded identity-revalidated list-box helper, and requires every row and order to match. Existing exact asset identities, Scene Root/Cube Inspector state, Cube selection synchronization, original child-HWND continuity, resize containment/state, bounded cross-process messages, process-owned cleanup, Release assertions, and exclusive-desktop requirements remain unchanged.

## Primary-source research, accessed 2026-09-22

Behavioral comparison only. No proprietary engine source was copied and no dependency was imported.

- Epic Games, Unreal Engine 5.8, Unreal Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
- Epic Games, Unreal Engine 5.8, Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
- Epic Games, Unreal Engine 5.8, Selecting Actors: https://dev.epicgames.com/documentation/unreal-engine/selecting-actors-in-unreal-engine
- Unity Technologies, Unity 6.0, Hierarchy window: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html
- Unity Technologies, Unity 6.0, Inspector: https://docs.unity3d.com/6000.0/Documentation/Manual/Inspector.html

Epic documents the Outliner as a hierarchical tree of level content and documents synchronized Outliner/Viewport selection with the Details panel reflecting selection. Unity documents the Hierarchy as the scene-object management surface and the Inspector as the selected-object property surface. For this packet, those references justify checking every identity in the fixed procedural scene fixture instead of accepting a row count plus sentinel rows. They do not imply feature parity or licensing permission to copy an implementation.

## Portable reproduction and compiler evidence

Disposable coordinator-sandbox fixture: `/mnt/data/e11_outliner_identity_fixture.cpp`.
SHA-256: `8944112d252d2b2c83a28d2e598b6d47ea516266319853e5abadeb19dc170f63`.

The fixture demonstrates that the old pair-only predicate accepts a wrong `Camera`, wrong `Directional Light`, wrong `Floor`, and reordered middle/tail rows, while the complete five-row predicate rejects each mutation and accepts the exact fixture.

Commands recorded for this candidate:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror /mnt/data/e11_outliner_identity_fixture.cpp -o /mnt/data/e11_outliner_identity_fixture
/mnt/data/e11_outliner_identity_fixture
clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /mnt/data/e11_outliner_identity_fixture.cpp -o /mnt/data/e11_outliner_identity_fixture_san
ASAN_OPTIONS=detect_leaks=1 /mnt/data/e11_outliner_identity_fixture_san
sha256sum /mnt/data/e11_outliner_identity_fixture.cpp
```

Results: GCC warning-clean compile/execution PASS; Clang ASan+UBSan warning-clean compile/execution PASS; no sanitizer finding. This is source-logic evidence only, not native Win32 execution.

## Hosted verification

For exact code candidate `0d2524c5e22035e3bf3a0f762e57616f896a9f57`:

- profiling capture portability run `35773477112`: `completed/success`;
- release manifest integrity run `35773476964`: `completed/success`;
- Windows build/deterministic-test run `35773476962`: `completed/cancelled` after a newer evidence head superseded it. No pass is claimed for the cancelled run.

The same smoke blob `c493cb962f86a054e0e2a60dcd934ef4610196fd` was then present in exact evidence head `988186b917a3b9e547224ffb1d6618c607490fbb`, whose hosted workflows all completed successfully:

- Windows build and deterministic tests run `35773676765`: `completed/success`;
- profiling capture portability run `35773676786`: `completed/success`;
- release manifest integrity run `35773676793`: `completed/success`.

The Windows workflow covers repository safety contracts, VS2022 x64 configure, MSVC Debug build/deterministic tests, MSVC Release build/deterministic tests, dependency/prerequisite/runtime-policy checks, static verification, and clean-tree verification. The historical R0 runner itself was not executed.

Hosted deterministic CTest intentionally excludes every `RuntimeSmoke`, so these results are compile/non-runtime regression evidence. They are not native editor GUI acceptance.

## Independent review state

The previous clean Codex review completed on superseded head `75727914e37f3c16628607c9b7bb232949359dbe`. Candidate `0d2524c5...` changes `Tests/EditorRuntimeSmoke.cpp`, so that earlier review cannot accept the new source. A fresh independent review of the final evidence tree is required. Same-author inspection is not independent acceptance.

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

Retain source SHA, machine/Windows identity, MSVC and CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, and normal plus narrow/short screenshots. Human-visible acceptance must confirm all five exact Outliner rows; all four exact asset rows; Cube synchronization across Outliner, Inspector and viewport `Selected:` text; truthful shell labels/status; panel containment after both sizes; and continuity of the original twelve child HWND identities.

## Result

Status: **complete fixed-Outliner identity verification implemented; source-logic fixture passed GCC and Clang ASan+UBSan; exact evidence head `988186b...` passed hosted Windows Debug/Release deterministic checks plus profiling and release-manifest workflows; fresh independent review and native Debug/Release RuntimeSmoke remain pending**.

E11 remains a partial editor-shell candidate, not UE5/Unity parity. No native GUI, GPU/performance, clean-machine, stress/recovery, soak, or final independent runtime acceptance claim is made. Issue #7 remains open and the historical R0 runner was not invoked.

Single next useful action: obtain fresh independent review of the final evidence tree, then execute Debug and Release `EditorRuntimeSmoke` on the registered Windows desktop with the required receipts/screenshots.
