# E11 Editor Runtime Smoke, 2026-09-22

## Scope

Add and harden one native Windows runtime-smoke test for the already-integrated Astral Editor shell. This packet does not add scene mutation, serialization, transform gizmos, Play-in-Editor, asset importing, a graphics-API change, game content, or any dependency.

Baseline: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1` (`main` when admitted).
Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Current code candidate: `40d2e3fe2b42ff9177be5c88b36e1711506f6f8b`.
Local execution authority remains the existing Company Runtime only. The GitHub coordinator does not claim a registered local worktree or desktop session.

Allowed implementation paths:
- `CMakeLists.txt`
- `Tests/EditorRuntimeSmoke.cpp`
- `Tasks/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`
- `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`
- `Docs/Research/ENGINE-CAPABILITIES.json`

No other path is authorized by this packet.

## Dependency and reason

The integrated E11 shell has hosted Debug/Release build evidence, but its capability record still has no registered-local native GUI receipt. Existing game runtime smokes launch `AstralGame`; none exercise `AstralEditor`. This packet adds a bounded real-editor test surface without claiming that hosted CI can execute an interactive Windows desktop check.

The test remains named `EditorRuntimeSmoke` so `cmake/AstralTestSafety.cmake` applies the existing `RUN_SERIAL` and 180-second timeout contract. Hosted deterministic CI compiles the test but intentionally excludes `RuntimeSmoke` execution.

## Primary-source research, rechecked 2026-09-22

Behavioral/API references only. No Epic, Unity, or Microsoft source/assets are copied and no dependency is imported.

- Epic Games, Unreal Engine 5.8, Content Browser: https://dev.epicgames.com/documentation/en-us/unreal-engine/content-browser-in-unreal-engine
  - Applicability: the Content Browser is the editor surface for viewing, organizing, finding, and working with project assets. Astral's asset browser is currently only a fixed procedural fixture, so the smoke should at least prove the identities it claims to expose rather than merely count rows.
- Unity Manual, Unity 6.0, Project window reference: https://docs.unity3d.com/6000.0/Documentation/Manual/ProjectView.html
  - Applicability: the Project window is the primary surface for locating project assets and displays individual asset identities/types. Astral's four placeholder entries are a much smaller verification fixture, not feature parity.
- Microsoft `LB_GETTEXT`: https://learn.microsoft.com/windows/win32/controls/lb-gettext
  - Applicability: retrieves the string at a zero-based list-box index and returns `LB_ERR` for an invalid index.
- Microsoft `LB_GETTEXTLEN`: https://learn.microsoft.com/windows/win32/controls/lb-gettextlen
  - Applicability: provides a safe allocation bound before `LB_GETTEXT`; the existing bounded helper rejects invalid indices and oversized text.
- Microsoft `EnumWindows`: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-enumwindows
- Microsoft `GetWindowThreadProcessId`: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-getwindowthreadprocessid
- Microsoft `SendMessageTimeoutW`: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-sendmessagetimeoutw
- Microsoft `SetWindowPos`: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setwindowpos

## Current bounded finding: asset-browser count was not identity evidence

At prior head `387285c3542a43356a39961ede906fd6cbf9458a`, `EditorRuntimeSmoke` required `assetCount == 4` but never read the four asset rows. A fixture with four wrong or reordered labels could therefore satisfy the asset-browser assertion. The integrated editor currently inserts these exact procedural placeholders in order:

1. `Primitive/Cube`
2. `Primitive/Plane`
3. `Camera`
4. `DirectionalLight`

Candidate `40d2e3fe2b42ff9177be5c88b36e1711506f6f8b` keeps the count assertion and additionally reads all four rows through the existing bounded `LB_GETTEXTLEN`/`LB_GETTEXT` helper and requires the exact identities and order above. Missing, extra, reordered, truncated, or renamed rows now fail. No new side effect is introduced.

A disposable C++17 source-logic fixture demonstrates the false-pass distinction: the old modeled count-only contract accepts same-count wrong, reordered, or truncated names, while the exact-identity contract rejects them and accepts only the expected four-item sequence. Fixture SHA-256: `9ab73331bf418f66c379fad013bfcfb45785cbe316284477dc0b663bd78c6108`. This is source-logic evidence only, not production Win32 execution.

## Acceptance test

`EditorRuntimeSmoke` must launch the exact built `AstralEditor` executable in a separate process and fail unless all of the following are observed:

1. the same visible top-level editor HWND owned by the launched process remains the only such process-owned window for 20 consecutive 50 ms observations at startup and again after interactions;
2. class `AstralEditorWindow` and title `Astral Editor 0.1`;
3. exactly the required 12 direct E11 child controls: five `Button`, two `ListBox`, and five `Static` controls;
4. all five pending Select/Move/Rotate/Scale/Play buttons are visible and disabled;
5. the Outliner has five rows, starts with exact row `Scene Root`, and contains exact row `Cube` at index 3; the Assets list has exactly four rows and, in order, exactly `Primitive/Cube`, `Primitive/Plane`, `Camera`, `DirectionalLight`;
6. the initial Inspector text exactly matches the complete Scene Root fixture;
7. before changing selection, the cached Outliner must still belong to the launched PID, be a direct child of the editor, and remain control ID 1001; selecting Cube and emitting bounded `LBN_SELCHANGE` must leave `LB_GETCURSEL == 3`, selected text `Cube`, and the complete Cube Inspector fixture;
8. before each normal 800x600 and narrow/short 420x260 resize, the editor HWND must still belong to the launched PID; each `SWP_ASYNCWINDOWPOS` resize must complete within the bounded poll and every direct child must remain inside the client rectangle;
9. shutdown must revalidate window ownership, post one `WM_CLOSE`, terminate within a bounded wait, and exit code 0; failure cleanup may terminate only the retained process handle created by this smoke.

Cross-process synchronous messages use `SendMessageTimeoutW` with a one-second deadline. The smoke does not use global keyboard or mouse injection.

## Verification

Coordinator disposable fixture:

```bash
g++ -std=c++17 -Wall -Wextra -Werror /tmp/e11_asset_identity_fixture.cpp -o /tmp/e11_asset_identity_fixture
/tmp/e11_asset_identity_fixture
clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /tmp/e11_asset_identity_fixture.cpp -o /tmp/e11_asset_identity_fixture_san
ASAN_OPTIONS=detect_leaks=1 /tmp/e11_asset_identity_fixture_san
sha256sum /tmp/e11_asset_identity_fixture.cpp
```

Both compiler/run paths exited 0 and printed `asset identity contract fixture: PASS`.

Hosted Windows evidence for exact code candidate `40d2e3fe2b42ff9177be5c88b36e1711506f6f8b`:

- Windows Server 2022 run `35726708407`, job `106742016089`: success. Safety contracts, VS2022 x64 configure, MSVC Debug build/tests, MSVC Release build/tests, dependency/prerequisite checks, static milestone verifiers, and clean-tree verification all passed.
- Profiling capture portability run `35726708416`: success.
- Release manifest integrity run `35726708369`: success.

The workflow's R0-related steps parsed the runner and exercised safety-contract tests only. The historical R0 runner itself was not executed. Hosted deterministic tests intentionally excluded every `RuntimeSmoke`, so these receipts establish compilation and non-runtime regressions, not native GUI acceptance.

Registered-local native execution, on one owned interactive Windows desktop, remains:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain source SHA, machine identity, Windows version, MSVC/CMake versions, GPU/driver identity, exact commands, stdout/stderr, exit codes, UTC timestamps, and screenshots of the normal and narrow/short editor states. Human-visible acceptance should also confirm the four asset-browser labels, Cube selection synchronization, and panel containment.

## Stop and rollback

Stop at the first deterministic build/test failure introduced by this packet and keep the failing logs. Do not weaken `EditorRuntimeSmoke`, `astral_add_test`, Release assertions, `RUN_SERIAL`, or timeout rules to obtain green CI. Do not run R0.

Rollback is deletion of this packet's new test/task/QA files and capability-map entry plus the corresponding `CMakeLists.txt` registration on the owned branch only. Do not rewrite history or alter `main`.

The single next useful action is registered-local Debug and Release `EditorRuntimeSmoke` execution on the exact final head with the required receipts/screenshots, followed by fresh independent review of that exact head. Dependent scene-document, transform-gizmo, save/reopen, and undo/redo work remains gated on that acceptance.