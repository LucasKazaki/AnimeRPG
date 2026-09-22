# E11.0 Astral Editor shell evidence

Date: 2026-09-22

## Candidate and boundaries

Branch: `engine/2026-09-22-editor-shell`
Baseline: `64e35980781c875d6f62e798e3bfa17fdc093897` (PR #8 head at packet creation)
Hosted-Windows-verified implementation candidate: `e721aeb1e320c21953b291346202ec8c14ebf236`

This packet adds a separate `AstralEditor` executable. It does not alter `AstralGame`, change the GDI graphics API, install dependencies, touch Company Runtime state, invoke R0, resume game content, merge, release or deploy anything. Issue #7 remains open, so the historical R0 runner remains outside this packet.

## What was implemented

- A deterministic editor panel-layout model under `Engine/Editor`.
- A standalone Win32/GDI `AstralEditor 0.1` shell with:
  - toolbar surfaces for Select/Move/Rotate/Scale and a deliberately disabled `Play (pending)` button;
  - an Outliner containing only procedural editor fixtures;
  - a central GDI authoring viewport with grid, axes and placeholder cube;
  - selection-linked Inspector text;
  - an Assets / Default Primitives browser with Cube, Plane, Camera and DirectionalLight placeholders;
  - a status line that explicitly lists major deferred editor capabilities.
- A native CTest target for deterministic panel geometry.
- Capability-map status changed only to `partial_editor_shell_candidate`, with registered-local native GUI evidence and independent acceptance still empty/false.

This is the first visible editor frame, not a UE5/Unity-equivalent editor. Transform buttons do not mutate objects yet. Save/reopen, serialization, gizmos, undo/redo, drag/drop, import, Play/Stop, docking, editor/runtime isolation tests, real rendering/material previews and tutorial onboarding remain open.

## Research basis

Primary sources accessed 2026-09-22:

- Unreal Engine 5.8 `Unreal Editor Interface`, Epic Games: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
- Unreal Engine 5.8 `Content Browser`, Epic Games: https://dev.epicgames.com/documentation/en-us/unreal-engine/content-browser-in-unreal-engine
- Unity 6.1 `Scene view navigation`, Unity Technologies: https://docs.unity3d.com/Manual/SceneViewNavigation.html
- Unity editor interface manual family: https://docs.unity3d.com/Manual/UsingTheEditor.html

Applicability: these sources establish the core authoring surfaces used by the comparison engines: viewport/Scene view, hierarchy/Outliner, Inspector/Details, project/content browser and play controls. They are functional references only. No proprietary source, assets or UI artwork were copied.

## Sandbox/compiler evidence

The container could not clone GitHub because DNS resolution for `github.com` failed. That failure was preserved rather than retried repeatedly.

A disposable partial fixture containing the exact published `EditorLayout.h`, `EditorLayout.cpp` and `EditorLayoutTests.cpp` source was compiled and executed with:

```text
g++ -std=c++17 -Wall -Wextra -Werror -I/tmp/e11 \
  /tmp/e11/Engine/Editor/EditorLayout.cpp \
  /tmp/e11/Tests/EditorLayoutTests.cpp \
  -o /tmp/e11/editor_layout_tests
/tmp/e11/editor_layout_tests
```

Result: PASS, exit 0, output `EditorLayoutTests: PASS`.

Git blob identity for the three sandbox-tested files:

- `Engine/Editor/EditorLayout.h`: `723ce6906b7fd9faaa4402c7774dd3a49e8e5f86`
- `Engine/Editor/EditorLayout.cpp`: `994625ff7484683ce377fb63d8aa947de58bf6ab`
- `Tests/EditorLayoutTests.cpp`: `4bd5ba7f56548de465e7acdbac3cb010e8d7ce51`

The header hash was re-read from GitHub after publication and matched the sandbox hash. The Windows editor shell itself cannot be compiled in this Linux container because the Windows SDK/headers are unavailable; hosted Windows and registered local Windows evidence are separate gates.

## Tests covered

The layout test checks 1280x720, 1920x1080, 800x600, 320x200, zero-size and negative-size inputs. It verifies every region stays inside the clamped client rectangle and that toolbar/content/status plus Outliner/viewport/Inspector/assets do not overlap. It also pins the intended 1280x720 layout dimensions.

This is deterministic layout evidence only. It does not prove native control creation, DPI behavior, font scaling, keyboard focus, paint correctness, accessibility or interactive editor behavior.

## Hosted Windows regression and repair

The first PR #10 run, `35651030497` / job `106503202564`, failed before CMake configuration in `Verify Release assertion and CTest safety contracts`. The failure was a verifier-assumption regression caused by adding a second non-test product executable: `Scripts/test_test_safety.py` collected every `add_executable` in the repository, exempted only the literal `AstralGame`, then required every remaining product or test target to appear in `astral_add_test`.

The repair did not exempt `AstralEditor` by name and did not weaken test safety. `test_all_native_test_targets_use_the_safety_helper` now scopes target discovery to the existing `if(BUILD_TESTING)` block, then still requires exact equality between every executable declared in that block and every `astral_add_test` target. The raw `add_test` prohibition remains. Release-assertion enforcement, 60-second domain-test timeout, 180-second RuntimeSmoke timeout and RuntimeSmoke serialization tests are unchanged.

Repair commit / exact hosted candidate: `e721aeb1e320c21953b291346202ec8c14ebf236`.
`Scripts/test_test_safety.py` blob: `874e6d83d30a536aaac900ae6a3e587ee1001f6d`.

The follow-up hosted Windows Server 2022 run `35651231169`, job `106503998064`, passed all steps for exact candidate `e721aeb1e320c21953b291346202ec8c14ebf236`:

- R0 runner Python parse: PASS. The runner was parsed only, not invoked.
- Release assertion / CTest safety contract suite: PASS.
- Visual Studio 2022 x64 configure: PASS.
- MSVC Debug build, including `AstralEditor`: PASS.
- deterministic Debug CTests, including `EditorLayoutTests`: PASS.
- MSVC Release build, including `AstralEditor`: PASS.
- deterministic Release CTests: PASS.
- static milestone verifiers: PASS.
- clean tracked tree check: PASS.

The job completed successfully at 2026-09-21T20:29:30Z. Hosted CI proves compilation and deterministic non-interactive tests on the ephemeral Windows runner. It is not registered-local GUI evidence and not independent acceptance.

## Registered local Windows handoff

The registered local executor should use a clean owned worktree at exact code candidate `e721aeb1e320c21953b291346202ec8c14ebf236` (or a later evidence-only descendant only after confirming no code drift) and retain source SHA, Windows version, compiler/SDK version, GPU/driver identity where captured, exact commands, stdout/stderr, exit codes, UTC timestamps and screenshots.

Run:

```powershell
cmake -S . -B ../AnimeRPG-e11-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-build -C Debug --output-on-failure -E RuntimeSmoke
& ../AnimeRPG-e11-build/Debug/AstralEditor.exe
```

Interactive checks:

1. Confirm the editor opens as a separate application titled `Astral Editor 0.1`.
2. Capture a screenshot at initial size.
3. Select Camera, Directional Light, Cube and Floor in the Outliner. Confirm the Inspector and viewport selection label update for each.
4. Resize at 800x600, 1280x720, 1440x900 and maximized size. Capture at least the smallest and largest states and verify no pane overlaps or leaves the client region.
5. Confirm `Play (pending)` is visibly disabled and no control falsely implies save, undo or real scene editing exists.
6. Launch `AstralGame.exe` separately and confirm the editor packet did not replace or alter the production game executable.

Release should then be built/tested with the same retained receipt set. GUI/native acceptance remains incomplete until these exact checks exist.

## Capability-to-evidence status

- E11 UI/editor tooling: advanced from `debug HUD only` to a source/hosted-build-verified candidate for a separate editor frame. Still partial and not registered-local/interactively or independently accepted.
- E02 scene ownership/serialization: unchanged, partial.
- E03 asset pipeline: unchanged except PR #8 candidate; no asset database/import integration added here.
- E04 GPU rendering/materials: unchanged, GDI baseline retained.
- E12 genuine 2D: unchanged.
- E14 profiling: unchanged.
- E15 packaging/platforms: unchanged.
- E16 remaining catalogue, including terrain/VFX/cinematics/scripting/input-replay/accessibility/localization/platforms: unchanged and unresolved.
- E17 comparative acceptance: blocked; no parity claim.

## Exact changed paths in the bounded packet

- `CMakeLists.txt`
- `Engine/Editor/EditorLayout.h`
- `Engine/Editor/EditorLayout.cpp`
- `Tools/AstralEditorMain.cpp`
- `Tests/EditorLayoutTests.cpp`
- `Scripts/test_test_safety.py`
- `Tasks/E11-EDITOR-SHELL-2026-09-22.md`
- `Docs/QA/E11-EDITOR-SHELL-2026-09-22.md`
- `Docs/Research/ENGINE-CAPABILITIES.json`

`Docs/Research/ENGINE-CAPABILITIES-2026-09-20.md` was admitted for possible update but was not modified in this packet.

## Next useful action

Run the registered-local interactive editor acceptance above and preserve screenshots/receipts. If that passes, the next bounded E11/E02 packet should connect the editor to a small versioned scene-document model with editable transforms plus save/reopen and undo/redo tests. Do not implement gizmos or drag/drop on ad-hoc game state before that ownership/serialization contract exists.

## 2026-09-22 bounded coordinate-offset overflow repair

A later source audit found one more deterministic E11 geometry defect after the `Contains`/`Overlaps` edge calculations had already been promoted to `int64_t`. The layout generators themselves still evaluated signed-`int` coordinate offsets such as `toolbar.x + padding`, `toolbar.y + verticalOffset`, `panel.x + horizontalPadding`, `panel.y + bodyTop`, and repeated toolbar-button advancement. A disposable pre-fix UBSan reproduction against `EditorLayout.cpp` blob `471798b9816b48a9bdfa195bdd80c56acfa19f01` called the layout helpers with an origin near `INT_MAX` and failed with:

```text
runtime error: signed integer overflow: 2147483645 + 8 cannot be represented in type 'int'
```

Primary upstream reference, accessed 2026-09-22: Clang `UndefinedBehaviorSanitizer`, https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html. It explicitly identifies signed integer overflow as undefined behavior detected by UBSan and documents `-fsanitize=undefined`. Current Epic UE 5.8 editor references were also rechecked at https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface and https://dev.epicgames.com/documentation/unreal-engine/viewport-toolbar to keep this repair scoped to the active editor-tooling packet. These are behavior/upstream references only; no external source or dependency was imported.

The fix adds an internal `SaturatingAdd(int, int)` helper that promotes both operands to `std::int64_t`, clamps the sum to the representable `int` range, and only then converts back to `int`. The toolbar, panel-content, and status-content offset calculations now use that helper. Normal client-layout behavior is unchanged. The regression tests add `INT_MAX`-adjacent toolbar, panel, and status origins and require defined saturated results instead of wraparound/UB.

Exact implementation sequence:

- `ac57d43b0ead0f6f3a3ee4562b19456e034240d9`: implementation change in `Engine/Editor/EditorLayout.cpp`.
- `c2f57053153c9dcc63e8db4af43a84a23960daf0`: test follow-up and exact hosted code candidate.
- Published `EditorLayout.cpp` blob: `6696ee65131980f37cbf698e780c2188f699945d`.
- Published `EditorLayoutTests.cpp` blob: `d78dcd1dec79c9296af389bbe112f0c1dbe3140a`.

A disposable partial fixture containing the exact modified sources passed each of these checks before publication:

```text
g++ -std=c++17 -Wall -Wextra -Werror -I/tmp/e11 \
  /tmp/e11/Engine/Editor/EditorLayout.cpp /tmp/e11/Tests/EditorLayoutTests.cpp \
  -o /tmp/e11/editor_layout_gcc && /tmp/e11/editor_layout_gcc

clang++ -std=c++17 -O2 -Wall -Wextra -Werror -I/tmp/e11 \
  /tmp/e11/Engine/Editor/EditorLayout.cpp /tmp/e11/Tests/EditorLayoutTests.cpp \
  -o /tmp/e11/editor_layout_release && /tmp/e11/editor_layout_release

clang++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -fno-sanitize-recover=undefined -I/tmp/e11 \
  /tmp/e11/Engine/Editor/EditorLayout.cpp /tmp/e11/Tests/EditorLayoutTests.cpp \
  -o /tmp/e11/editor_layout_san
ASAN_OPTIONS=detect_leaks=1 /tmp/e11/editor_layout_san
```

All three passed with exit 0 and `EditorLayoutTests: PASS`. A later attempt to re-download the already-published blobs directly from `raw.githubusercontent.com` inside the container failed at DNS resolution and was not treated as additional evidence. The connector re-read the published files at exact candidate `c2f5705...` and confirmed the expected blob IDs above.

Hosted Windows Server 2022 workflow run `35679288804`, job `106592495400`, completed successfully for exact candidate `c2f57053153c9dcc63e8db4af43a84a23960daf0`. Test-safety contracts, VS2022 x64 configure, MSVC Debug build/tests, MSVC Release build/tests, static milestone verifiers, and clean tracked-tree verification all passed. The workflow only parsed `Scripts/invoke_r0_release_candidate.py`; it did not invoke R0.

This repair does not establish native interactive GUI acceptance. The registered local handoff remains the E11 launch/selection/resize/separate-game checks above, now using `c2f57053153c9dcc63e8db4af43a84a23960daf0` as the latest hosted code candidate unless a later evidence-only descendant is proven not to change code. Independent acceptance remains required. The single next useful action is still that registered-local interactive receipt plus independent review; scene documents, transform editing, save/reopen, undo/redo, and gizmos remain deferred until the active E11 gate is accepted.
