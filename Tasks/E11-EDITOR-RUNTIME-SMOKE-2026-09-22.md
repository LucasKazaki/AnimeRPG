# E11 Editor Runtime Smoke task, 2026-09-23

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. It may harden the native Windows smoke, its recovery supervisor, deterministic containment tests, and evidence. It must not add scene mutation/serialization, gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed during this packet: `977afadb2630bb3d25755d4407992bfab10018f8`.
Current code candidate: `81e7052f47cab06060ea69c9f9d4f25ec42145e4`.
Final source/evidence tree reviewed this pass: `727dcf7cef2a01c4be13931db0551247edf929bb`.
`CMakeLists.txt` blob: `ed6a7f44d87241560faf32a57465befd536b59f9`.
`Tests/EditorRuntimeSmoke.cpp` blob: `117c101acc9d65e297c3e0f948a6c3724ff2416d`.
Production editor source is unchanged by this pass.

Allowed paths only: `CMakeLists.txt`, `Tests/EditorRuntimeSmoke.cpp`, this task, `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`, and `Docs/Research/ENGINE-CAPABILITIES.json`. One active writer only. Do not rebase, merge, force-push, or absorb unrelated work.

## Selected verification gap and implementation

The E11 smoke previously treated the five pending toolbar buttons as an unordered caption set. `ValidateShellState` searched the current child inventory for `Select (pending)`, `Move (pending)`, `Rotate (pending)`, `Scale (pending)`, and `Play (pending)`, then checked only visibility and disabled state. Because the original child-HWND inventory check is order-independent, two original Button HWNDs could exchange captions/semantic roles while all expected captions still existed and the smoke would pass.

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
  - applicability: workflow comparison for semantically distinct Select/Move/Rotate/Scale controls and consistent logical toolbar locations.
- Unity Technologies, Unity 6 `Tool`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/Tool.html
  - applicability: workflow comparison for distinct Move/Rotate/Scale editor tools.
- Microsoft Learn, `GetWindowRect`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowrect
  - applicability: obtains window/control bounding rectangles used by the semantic toolbar slot checks.
- Git project, core data model: https://git-scm.com/docs/gitdatamodel
  - applicability: Git objects are immutable and their object IDs hash type plus contents. Evidence commits therefore cannot embed their own not-yet-created commit ID without changing the commit again; the post-write exact head belongs in PR/checkpoint metadata, while durable receipts can anchor the reviewed predecessor tree and its exact workflows.

Behavior/API/evidence-model references only. No proprietary source was copied and no dependency was added.

## Portable mutation fixture

A disposable C++17 source-logic fixture modeled five stable toolbar handles and semantic slots. It passes the correct Select/Move/Rotate/Scale/Play arrangement and rejects caption swaps, positional swaps, enabled/hidden controls, and overlapping slots.

Fixture SHA-256: `23ae0ee31850aab8df8ddcf68ca4a11137c1b3529ca9c9ced1d589cf6606b210`.

Executed commands/results:

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

This fixture is source-logic evidence only, not Win32 GUI execution.

## Exact final-tree hosted evidence and independent review

Exact source/evidence tree `727dcf7cef2a01c4be13931db0551247edf929bb` contains the toolbar candidate above and completed all hosted workflows successfully:

- Windows build and deterministic tests `35810545499`, job `107020923389`: `completed/success` on exact head `727dcf7...`, completed `2026-09-23T02:31:22Z`. Repository/R0 safety contracts, Release assertion/CTest safety, VS2022 x64 configure, Debug build/tests, Release build/tests, dependency/prerequisite checks, static verifiers, and clean-tree verification passed.
- profiling capture portability `35810545487`: `completed/success`.
- release manifest integrity `35810545457`: `completed/success`.

Fresh Codex review of exact head `727dcf7...` was submitted at `2026-09-23T02:34:44Z`. It identified one P2 evidence-traceability defect only: the task, QA receipt, and capability map still named the intermediate candidate and cancelled candidate-head Windows run instead of anchoring `727dcf7...` and its successful exact-head workflows. No new runtime-smoke implementation defect was reported in that review. This evidence-only repair updates all three durable records; a re-review of the post-repair head is still required before independent acceptance.

Hosted deterministic CTest intentionally excludes tests whose names end in `RuntimeSmoke`, so these green hosted workflows are not native editor GUI evidence.

## Retained acceptance surface

All established E11 checks remain required: one stable visible/enabled process-owned top-level editor; original 12-child HWND/class continuity; bound semantic Static controls; bound semantic toolbar Button controls; exact five ordered Outliner rows and four ordered Assets rows; Outliner `LBS_NOTIFY`; exact Scene Root/Cube Inspector fixtures; post-notification Cube synchronization; truthful disabled pending tools; bounded 800x600 and 420x260 resizes with complete-state and containment checks; bounded cross-process messages; clean process-owned normal shutdown; worker-local cleanup; and supervisor-level process-tree cleanup verification.

`native_evidence` remains empty. Issue #7 is still open, so the historical R0 runner is blocked and was not invoked. E11 remains partial and is not UE5/Unity parity.

## Registered native handoff

After this evidence-only repair receives clean independent re-review, the registered Windows executor should run the exact reviewed branch head on one owned interactive desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, commands, full stdout/stderr, exit codes, UTC timestamps, normal plus narrow-window screenshots, and proof that any failure/interruption leaves zero owned contained processes.

## Rollback and stop conditions

Rollback only this evidence-only commit if it misstates the reviewed tree or hosted run association. Stop before any runtime code change, rebase, merge, R0 execution, local scheduler operation, dependency addition, graphics/API change, or game-content work. If the review produces a new runtime finding, admit that finding as the next bounded repair instead of weakening the smoke.

## Single next useful action

Obtain fresh independent re-review of the evidence-repaired head. If clean, the next substantive gate is the registered Windows Debug/Release GUI smoke with the complete native receipt set.