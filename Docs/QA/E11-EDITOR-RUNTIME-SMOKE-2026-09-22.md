# E11 Editor Runtime Smoke evidence, 2026-09-22

## Checkpoint

This is the current evidence record for the bounded `EditorRuntimeSmoke` packet admitted from `main` at `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1` on branch `engine/2026-09-22-editor-runtime-smoke`.

Scope remains verification-only. The packet adds and hardens one native Windows smoke for the already-integrated `AstralEditor`; it does not add scene mutation or serialization, transform gizmos, Play-in-Editor, asset import, renderer/API changes, dependencies, game content, scheduler operations, deployment, release, or R0 execution.

Current `main` was re-read at `f68e0917b38a0b4780d94a409874bb94df818163` after separate game-worker merges. This packet was not rebased, force-pushed, or merged with that unrelated work. Compare from current `main` to code-reviewed E11 head `c8e0bd2e3952f7b77a3f9701cf69af8544f39620` reports `diverged`, 28 commits ahead and 82 behind, while this PR remains limited to its five authorized paths. PR #13 currently reports non-mergeable against current `main`; exact integration evidence must be re-established after native acceptance and before any eventual merge decision.

## Current exact implementation

Latest code candidate: `40d2e3fe2b42ff9177be5c88b36e1711506f6f8b`.
Code-reviewed evidence head: `c8e0bd2e3952f7b77a3f9701cf69af8544f39620`.

Relevant blobs at the code-reviewed head:

- `Tests/EditorRuntimeSmoke.cpp`: `49391f99aa8e63b7dbbf794451251f0bac605c5d`
- `CMakeLists.txt`: `945e0f5e28a0ecd91338541984cdc240bf986f82`
- task record at that head: `2fb529f94295957edb1cb82cfe86dd81fb8aa8f5`
- capability-map blob at that head: `391b9abf46db60ef98cfc1906875cd67ce244b32`

Later commits in this pass update evidence records only. They do not modify `Tests/EditorRuntimeSmoke.cpp` or `CMakeLists.txt`.

`CMakeLists.txt` registers the test through `astral_add_test`. The `RuntimeSmoke` suffix keeps the existing Release-assertion protection, `RUN_SERIAL`, and 180-second timeout contract. Hosted deterministic CTest intentionally excludes RuntimeSmoke execution.

The smoke requires all of the following before reporting PASS:

1. The same visible top-level HWND owned by the launched `AstralEditor` process is the only such process-owned window for 20 consecutive 50 ms observations at startup and again after interaction.
2. The top-level window has class `AstralEditorWindow` and title `Astral Editor 0.1`.
3. Exactly 12 direct E11 child controls exist with the expected class counts, and all five pending toolbar actions remain visible and disabled.
4. The Outliner contains five rows, index 0 is exactly `Scene Root`, index 3 is exactly `Cube`, and initial selection is index 0.
5. The Assets list contains exactly four rows and the rows are exactly, in order, `Primitive/Cube`, `Primitive/Plane`, `Camera`, `DirectionalLight`.
6. The initial Inspector text exactly matches the complete Scene Root fixture.
7. Immediately before side-effecting `LB_SETCURSEL`, the cached Outliner HWND is revalidated as owned by the launched PID, directly parented by the owned editor HWND, and still returned by `GetDlgItem(editor, 1001)`. Successful `LB_SETCURSEL` is accepted on the documented non-`LB_ERR` contract rather than assuming the return value equals the requested index.
8. The same Outliner ownership/parent/control-ID guard is repeated before the bounded `LBN_SELCHANGE` notification and before post-notification reads. The smoke then separately requires `LB_GETCURSEL == 3`, selected row text `Cube`, and the complete Cube Inspector fixture.
9. Immediately before each side-effecting 800x600 or 420x260 resize, the saved editor HWND is revalidated with `GetWindowThreadProcessId` against the launched PID. Valid resizes use `SWP_ASYNCWINDOWPOS`, complete inside a bounded poll, and must leave every direct child inside the actual client rectangle.
10. Shutdown independently revalidates HWND process ownership immediately before one `WM_CLOSE`, waits boundedly for exit code 0, and failure cleanup may terminate only the retained process handle created by the smoke.

Cross-process synchronous messages use `SendMessageTimeoutW` with a one-second deadline. No global keyboard/mouse injection is used.

## Finding repaired by the current implementation

At prior head `387285c3542a43356a39961ede906fd6cbf9458a`, the asset-browser assertion verified only `assetCount == 4`. Four incorrect, renamed, reordered, or truncated rows could therefore pass as long as the count remained four. The editor implementation itself inserts four named procedural placeholders, so the smoke was claiming more asset-surface confidence than it actually checked.

Candidate `40d2e3fe2b42ff9177be5c88b36e1711506f6f8b` adds an exact four-item expected sequence and reads every asset row using the existing bounded `ReadListboxText`, which uses `LB_GETTEXTLEN` followed by `LB_GETTEXT`. The initial fixture now fails on any missing, extra, reordered, renamed, truncated, invalid-index, or read-failure case. No process/window side effect was added.

## Primary-source research rechecked 2026-09-22

Behavioral/API references only. No proprietary source, artwork, assets, or dependencies were copied or imported.

- Epic Games, Unreal Engine 5.8, Content Browser: https://dev.epicgames.com/documentation/en-us/unreal-engine/content-browser-in-unreal-engine
  - Applicability: Epic describes the Content Browser as the primary editor area for viewing, organizing, finding, and working with project assets. Astral's fixed four-entry browser is only a procedural verification fixture, but its claimed entries should be identity-checked.
- Epic Games, Unreal Engine 5.8, Unreal Editor Interface: https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-editor-interface
  - Applicability: Epic documents synchronized selection across the Level Viewport, Outliner and Details panel. Astral's automated smoke currently verifies Outliner and Inspector synchronization; viewport selection text remains a human-visible native acceptance item rather than an automated claim.
- Unity Manual, Unity 6.0, Project window reference: https://docs.unity3d.com/6000.0/Documentation/Manual/ProjectView.html
  - Applicability: Unity identifies the Project window as the primary way to navigate/find project assets and displays individual asset identities/types. This supports identity-level evidence rather than count-only evidence for an editor asset surface.
- Microsoft `LB_GETTEXT`: https://learn.microsoft.com/windows/win32/controls/lb-gettext
- Microsoft `LB_GETTEXTLEN`: https://learn.microsoft.com/windows/win32/controls/lb-gettextlen
- Microsoft `GetWindowThreadProcessId`: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-getwindowthreadprocessid
- Microsoft `SendMessageTimeoutW`: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-sendmessagetimeoutw
- Microsoft `SetWindowPos`: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setwindowpos

## Coordinator sandbox evidence

A disposable C++17 fixture models the repaired asset-identity contract. It demonstrates that the previous count-only contract accepts same-count wrong names, reordered names, and truncated names, while the repaired contract accepts only the exact expected four-item sequence and rejects missing/extra/reordered/renamed/truncated sequences.

Fixture SHA-256: `9ab73331bf418f66c379fad013bfcfb45785cbe316284477dc0b663bd78c6108`.

Commands executed:

```bash
g++ -std=c++17 -Wall -Wextra -Werror /tmp/e11_asset_identity_fixture.cpp -o /tmp/e11_asset_identity_fixture
/tmp/e11_asset_identity_fixture
clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /tmp/e11_asset_identity_fixture.cpp -o /tmp/e11_asset_identity_fixture_san
ASAN_OPTIONS=detect_leaks=1 /tmp/e11_asset_identity_fixture_san
sha256sum /tmp/e11_asset_identity_fixture.cpp
```

Both compile/run paths exited 0 and printed `asset identity contract fixture: PASS`. This is source-logic evidence only, not production Win32/native execution. A full sandbox clone was attempted once in this pass and stopped on `Could not resolve host: github.com`; the same unchanged network failure was not retried.

## Hosted Windows evidence for exact code-reviewed head

GitHub Actions Windows Server 2022 run `35727159651`, job `106743549477`, executed against exact head `c8e0bd2e3952f7b77a3f9701cf69af8544f39620` and completed successfully on 2026-09-22.

Successful steps included checkout/external build-root setup, R0 parser-only plus runner-safety contracts, PE dependency and Windows prerequisite/runtime-policy checks, Release-assertion and CTest safety contracts, Visual Studio 2022 x64 configure, MSVC Debug build and deterministic Debug tests, MSVC Release build and deterministic Release tests, Release dependency/prerequisite checks, static milestone verifiers, and clean tracked-tree verification. The historical R0 runner itself was not executed.

Additional workflows on the exact same head also passed:

- Profiling capture portability run `35727159588`.
- Release manifest integrity run `35727159769`.

These hosted runs establish Debug/Release compilation of the real Win32 smoke and green non-runtime regressions. They do **not** establish native GUI execution because hosted deterministic CTest intentionally excludes every RuntimeSmoke target.

## Independent review status

Codex completed a fresh independent review of exact head `c8e0bd2e3952f7b77a3f9701cf69af8544f39620` at `2026-09-22T12:33:45Z`. The PR review summary records the completed review for commit `c8e0bd2`, no new inline finding was created, and the `chatgpt-codex-connector[bot]` added `+1` reaction ID `518891448` at `2026-09-22T12:33:49Z`.

This is independent **code review** evidence for that exact implementation/evidence head. It is not independent runtime acceptance. `native_evidence` remains empty and E11 `independent_acceptance` remains false until the registered-local native run, human-visible checks and retained receipts exist. Later commits in this pass only repair evidence records and do not change the runtime-smoke source.

## Registered-local handoff

Run the code-reviewed implementation on one owned interactive Windows desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine and Windows identity, MSVC/CMake versions, GPU/driver identity, commands, full stdout/stderr, exit codes, UTC timestamps, and screenshots at normal and narrow/short sizes. Human-visible acceptance must confirm the exact four asset-browser entries, Outliner visibly remains on Cube when the Inspector shows the Cube fixture, the viewport's `Selected:` text also reflects Cube, and panels do not bleed during resize.

## Result and next action

Status: **asset-browser identity evidence is hardened; exact-head hosted Debug/Release compilation and non-runtime regressions are green; fresh independent code review of the code/evidence head completed with no new findings; native editor-smoke execution and human-visible acceptance remain pending**.

The single next useful action is registered-local Debug and Release `EditorRuntimeSmoke` execution on the code-reviewed implementation with retained receipts/screenshots. Do not start dependent scene-document, transform-gizmo, save/reopen, or undo/redo work on hosted compilation and code review alone. Issue #7 remains separate and open; the historical R0 runner was not executed by this packet.