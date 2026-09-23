# E11 Editor Runtime Smoke evidence, 2026-09-23

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. This packet verifies the already-integrated Win32 `AstralEditor`; it does not authorize scene mutation/serialization, gizmos, Play-in-Editor, asset import, graphics/API changes, dependencies, game content, scheduler operations, deployment, release, merge, rebase, or R0 execution.

Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed: `1042339ee7052fcbff60b1b8bba6840b43019c4e`.
Current code candidate: `81e7052f47cab06060ea69c9f9d4f25ec42145e4`.
`CMakeLists.txt` blob: `ed6a7f44d87241560faf32a57465befd536b59f9`.
`Tests/EditorRuntimeSmoke.cpp` blob: `117c101acc9d65e297c3e0f948a6c3724ff2416d`.
Production editor source is unchanged by this pass.

## Previous reviewed evidence

Exact receipt-repair head `973403eeaa93313f0ec68f41d962ef6083eab01e` received a clean Codex re-review completed at `2026-09-23T01:28:13Z` with no new finding. Its exact hosted workflows all completed successfully:

- Windows build and deterministic tests `35806022252`: success;
- profiling capture portability `35806022245`: success;
- release manifest integrity `35806022278`: success.

This establishes a clean source/evidence review for that older tree only. It is not review of later source changes and is not native GUI acceptance.

## New false-pass repaired

Before candidate `81e7052...`, the smoke verified the five pending toolbar controls as an unordered caption set. The original HWND inventory itself is intentionally order-independent. Consequently, two original Button HWNDs could swap semantic captions/roles while all five expected captions still existed and the smoke would continue to pass.

Candidate `81e7052f47cab06060ea69c9f9d4f25ec42145e4` adds explicit semantic toolbar binding:

- initial five Button HWNDs must be process-owned direct visible children and disabled;
- their screen rectangles are mapped into editor-client coordinates and ordered left-to-right;
- zero-width and overlapping slots are rejected;
- the initial ordered slots must be exactly Select, Move, Rotate, Scale, Play;
- each semantic slot retains its exact original HWND; and
- every later shell validation rechecks that retained HWND's ownership, parent, Button class, visibility, disabled state, caption, positive width, and left-to-right non-overlap/order.

The original 12-child HWND/class continuity, semantic Static binding, Outliner/assets/Inspector checks, selection synchronization, resize containment, time budgets, worker cleanup, and supervisor Job Object containment checks are unchanged and remain required.

## Primary research, rechecked 2026-09-23 UTC

- Epic Games, UE 5.8 Viewport Toolbar: https://dev.epicgames.com/documentation/unreal-engine/viewport-toolbar
  - comparison relevance: the editor exposes semantically distinct Select/Move/Rotate/Scale transform tools and keeps tools organized in consistent locations/logical categories.
- Microsoft Learn, `GetWindowRect`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowrect
  - API relevance: obtains control bounding rectangles in screen coordinates for semantic toolbar slot checks.
- Microsoft Learn, `WM_GETTEXT`: https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-gettext
  - API relevance: for a Button, window text is its name/caption; the smoke reads it only through the existing bounded ownership-validated helper.
- Unity Technologies, Unity 6 `Tool`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/Tool.html
  - comparison relevance: Unity exposes semantically distinct editor Move/Rotate/Scale tools.

Behavior/API references only. No proprietary source was copied and no dependency was added.

## Coordinator fixture evidence

Disposable C++17 semantic-binding fixture SHA-256:
`23ae0ee31850aab8df8ddcf68ca4a11137c1b3529ca9c9ced1d589cf6606b210`.

It passed the correct toolbar mapping and deterministically rejected caption swaps, position swaps, enabled/hidden controls, and overlapping slots.

```text
g++ (Debian 14.2.0-19) 14.2.0
g++ -std=c++17 -Wall -Wextra -Werror /tmp/e11_toolbar_semantic_fixture.cpp -o /tmp/e11_toolbar_gcc
/tmp/e11_toolbar_gcc
=> toolbar semantic binding fixture: PASS

clang version 17.0.0
clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /tmp/e11_toolbar_semantic_fixture.cpp -o /tmp/e11_toolbar_clang
ASAN_OPTIONS=detect_leaks=1 /tmp/e11_toolbar_clang
=> toolbar semantic binding fixture: PASS
```

This is source-logic evidence only. It is not Win32 GUI execution.

## Hosted verification state

Exact code candidate `81e7052f47cab06060ea69c9f9d4f25ec42145e4`:

- profiling capture portability `35810232976`: `completed/success`;
- release manifest integrity `35810233015`: `completed/success`;
- Windows run `35810233080`, job `107019946669`: overall `cancelled` because a newer evidence commit superseded the branch head. Before cancellation, every substantive step through clean-tree verification completed successfully, including repository/R0 safety contracts, Release assertion/CTest safety, VS2022 x64 configure, Debug build/tests, Release build/tests, dependency/prerequisite checks, and static verifiers. Because the overall run is cancelled, it is not counted as a passing exact-head workflow.

A final evidence head containing the same source must complete its own Windows workflow successfully. Hosted deterministic CTest intentionally excludes test names ending in `RuntimeSmoke`; therefore no hosted result is interactive editor GUI evidence.

## Acceptance state and limitations

`native_evidence` remains empty. No registered interactive Windows desktop execution, screenshots, actual GPU behavior, clean-machine packaging, measured comparative performance, broader stress/recovery, or 24-hour soak was executed by this coordinator.

The new toolbar binding has not yet received a fresh independent review. Author/coordinator inspection and the portable fixture are not independent acceptance.

Issue #7 remains open. The historical R0 runner was not invoked.

Status: **toolbar semantic HWND/slot binding is implemented and partially hosted-verified, but final exact-head Windows CI, fresh independent review, native Debug/Release GUI evidence, and broader engine acceptance remain pending. No UE5/Unity parity claim is made.**

## Registered native handoff

After final exact-head hosted verification and clean independent review, run the exact reviewed branch head on one owned interactive Windows desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine/Windows identity, MSVC/CMake and GPU/driver versions, exact commands, complete stdout/stderr, exit codes, UTC timestamps, normal and narrow-window screenshots, and a process inspection showing zero owned contained processes after any failure or interruption.

## Single next action

Complete Windows hosted verification on the final evidence head and obtain fresh independent review of candidate `81e7052...`. If both are clean, execute the registered native Debug/Release GUI smoke and preserve the complete receipt set.