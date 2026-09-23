# E11 Editor Runtime Smoke task, 2026-09-23

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. It may harden the native Windows smoke, its recovery supervisor, deterministic containment tests, and evidence. It must not add scene mutation/serialization, gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed during this packet: `1042339ee7052fcbff60b1b8bba6840b43019c4e`.
Current code candidate: `81e7052f47cab06060ea69c9f9d4f25ec42145e4`.
`CMakeLists.txt` blob: `ed6a7f44d87241560faf32a57465befd536b59f9`.
`Tests/EditorRuntimeSmoke.cpp` blob: `117c101acc9d65e297c3e0f948a6c3724ff2416d`.
Production editor source is unchanged by this pass.

Allowed paths only: `CMakeLists.txt`, `Tests/EditorRuntimeSmoke.cpp`, this task, `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`, and `Docs/Research/ENGINE-CAPABILITIES.json`. One active writer only. Do not rebase, merge, force-push, or absorb unrelated work.

## Previous evidence gate now clean

Receipt-repair head `973403eeaa93313f0ec68f41d962ef6083eab01e` received a clean Codex re-review completed at `2026-09-23T01:28:13Z` with no new finding. Its exact hosted workflows also completed successfully: Windows build/deterministic tests `35806022252`, profiling capture portability `35806022245`, and release-manifest integrity `35806022278`. This closes the receipt re-review gate for that older exact tree, but it is not independent acceptance of source changed after `973403ee...` and is not native GUI evidence.

## Selected verification gap and implementation

The E11 smoke previously treated the five pending toolbar buttons as an unordered caption set. `ValidateShellState` searched the current child inventory for `Select (pending)`, `Move (pending)`, `Rotate (pending)`, `Scale (pending)`, and `Play (pending)`, then checked only visibility and disabled state. Because the original child-HWND inventory check is order-independent, two original Button HWNDs could exchange captions/semantic roles while all expected captions still existed and the smoke would pass. The analogous Static-control ambiguity had already been repaired by binding semantic HWNDs, but toolbar semantics were not equivalently bound.

Candidate `81e7052f47cab06060ea69c9f9d4f25ec42145e4` repairs that false-pass path without changing the production editor. At initial capture the smoke now:

1. collects the five original process-owned visible disabled Button HWNDs;
2. maps their screen rectangles into the editor client coordinate space;
3. orders them left-to-right and rejects zero-width or overlapping slots;
4. requires those slots to be exactly Select, Move, Rotate, Scale, Play; and
5. retains the exact HWND for every semantic toolbar slot.

Every later `ValidateShellState`, including post-selection, both 800x600 and 420x260 resize validations, and the final state validation, rechecks each retained Button HWND for process ownership, direct parent, class, visibility, disabled state, exact semantic caption, positive width, and left-to-right non-overlap/order. The existing original 12-child HWND/class continuity check remains in force, so replacement controls are still rejected separately.

This packet deliberately does not enable any toolbar action. The five tools remain truthful pending/disabled fixtures.

## Research basis, rechecked 2026-09-23 UTC

- Epic Games, UE 5.8 Viewport Toolbar: https://dev.epicgames.com/documentation/unreal-engine/viewport-toolbar
  - applicability: Epic documents transform tools as ordered, semantically distinct Select/Move/Rotate/Scale workflow controls and says the newer toolbar keeps features in consistent locations by logical category. This is a workflow comparison only.
- Microsoft Learn, `GetWindowRect`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowrect
  - applicability: retrieves a window/control bounding rectangle in screen coordinates, used before mapping the retained Button HWNDs into editor-client coordinates.
- Microsoft Learn, `WM_GETTEXT`: https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-gettext
  - applicability: button window text is the button name/caption, which is the semantic identity checked through the existing bounded cross-process text helper.
- Unity Technologies, Unity 6 `Tool` enum: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/Tool.html
  - applicability: Unity exposes semantically distinct Move/Rotate/Scale editor tools; workflow comparison only.

No proprietary source was copied and no dependency was added.

## Portable mutation fixture

A disposable C++17 source-logic fixture modeled five stable toolbar handles and semantic slots. It passes the correct Select/Move/Rotate/Scale/Play arrangement and rejects caption swaps, positional swaps, enabled/hidden controls, and overlapping slots.

Fixture SHA-256: `23ae0ee31850aab8df8ddcf68ca4a11137c1b3529ca9c9ced1d589cf6606b210`.

Commands/results in the coordinator sandbox:

```text
g++ (Debian 14.2.0-19) 14.2.0
g++ -std=c++17 -Wall -Wextra -Werror /tmp/e11_toolbar_semantic_fixture.cpp -o /tmp/e11_toolbar_gcc
/tmp/e11_toolbar_gcc
=> toolbar semantic binding fixture: PASS

clang version 17.0.0
a clang++ C++17 -Wall -Wextra -Werror ASan+UBSan build of the same fixture
=> toolbar semantic binding fixture: PASS
```

The leading `a` in the prose above is not a command; the exact executed sanitizer command was `clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /tmp/e11_toolbar_semantic_fixture.cpp -o /tmp/e11_toolbar_clang`, followed by `ASAN_OPTIONS=detect_leaks=1 /tmp/e11_toolbar_clang`. This fixture is source-logic evidence only, not Win32 GUI execution.

## Hosted candidate verification

GitHub Actions triggered for exact code candidate `81e7052f47cab06060ea69c9f9d4f25ec42145e4`:

- Windows build and deterministic tests run `35810233080`, job `107019946669`: in progress at this receipt write;
- profiling capture portability run `35810232976`: in progress at this receipt write;
- release manifest integrity run `35810233015`: in progress at this receipt write.

No pending workflow is counted as passed. Hosted deterministic CTest intentionally excludes tests whose names end in `RuntimeSmoke`, so even a green hosted build will not be native editor GUI evidence.

## Retained acceptance surface

All established E11 checks remain required: one stable visible/enabled process-owned top-level editor; original 12-child HWND/class continuity; bound semantic Static controls; bound semantic toolbar Button controls; exact five ordered Outliner rows and four ordered Assets rows; Outliner `LBS_NOTIFY`; exact Scene Root/Cube Inspector fixtures; post-notification Cube synchronization; truthful disabled pending tools; bounded 800x600 and 420x260 resizes with complete-state and containment checks; bounded cross-process messages; clean process-owned normal shutdown; worker-local cleanup; and supervisor-level process-tree cleanup verification.

`native_evidence` remains empty. Issue #7 is still open, so the historical R0 runner is blocked and was not invoked. E11 remains partial and is not UE5/Unity parity.

## Registered native handoff

After the current candidate has completed hosted verification and receives clean independent review, the registered Windows executor should run the exact reviewed branch head on one owned interactive desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, commands, full stdout/stderr, exit codes, UTC timestamps, normal plus narrow-window screenshots, and proof that any failure/interruption leaves zero owned contained processes.

## Single next useful action

Finish exact-candidate hosted verification, then obtain fresh independent review of the toolbar semantic-binding diff. If clean, execute the registered Windows Debug/Release GUI smoke and preserve the complete native receipt set. Do not rebase onto the separately moving `main` within this packet.