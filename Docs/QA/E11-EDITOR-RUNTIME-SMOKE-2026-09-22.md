# E11 Editor Runtime Smoke evidence, 2026-09-23

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. This packet verifies the already-integrated Win32 `AstralEditor`; it does not authorize scene mutation/serialization, gizmos, Play-in-Editor, asset import, graphics/API changes, dependencies, game content, scheduler operations, deployment, release, merge, rebase, or R0 execution.

Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed: `977afadb2630bb3d25755d4407992bfab10018f8`.
Current code candidate: `81e7052f47cab06060ea69c9f9d4f25ec42145e4`.
Exact source/evidence tree reviewed this pass: `727dcf7cef2a01c4be13931db0551247edf929bb`.
`CMakeLists.txt` blob: `ed6a7f44d87241560faf32a57465befd536b59f9`.
`Tests/EditorRuntimeSmoke.cpp` blob: `117c101acc9d65e297c3e0f948a6c3724ff2416d`.
Production editor source is unchanged by this evidence repair.

## False-pass repaired by the current code candidate

Before candidate `81e7052...`, the smoke verified the five pending toolbar controls as an unordered caption set. The original HWND inventory itself is intentionally order-independent. Consequently, two original Button HWNDs could swap semantic captions/roles while all five expected captions still existed and the smoke would continue to pass.

Candidate `81e7052f47cab06060ea69c9f9d4f25ec42145e4` adds explicit semantic toolbar binding:

- initial five Button HWNDs must be process-owned direct visible children and disabled;
- their screen rectangles are mapped into editor-client coordinates and ordered left-to-right;
- zero-width and overlapping slots are rejected;
- the initial ordered slots must be exactly Select, Move, Rotate, Scale, Play;
- each semantic slot retains its exact original HWND; and
- every later shell validation rechecks that retained HWND's ownership, parent, Button class, visibility, disabled state, caption, positive width, and left-to-right non-overlap/order.

The original 12-child HWND/class continuity, semantic Static binding, Outliner/assets/Inspector checks, selection synchronization, resize containment, time budgets, worker cleanup, and supervisor Job Object containment checks remain required.

## Primary research, rechecked 2026-09-23 UTC

- Epic Games, UE 5.8 Viewport Toolbar: https://dev.epicgames.com/documentation/unreal-engine/viewport-toolbar
  - comparison relevance: semantically distinct Select/Move/Rotate/Scale tools and consistent logical toolbar placement.
- Unity Technologies, Unity 6 `Tool`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/Tool.html
  - comparison relevance: semantically distinct Move/Rotate/Scale editor tools.
- Microsoft Learn, `GetWindowRect`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowrect
  - API relevance: screen-coordinate control rectangles used for semantic slot checks.
- Git project, core data model: https://git-scm.com/docs/gitdatamodel
  - evidence relevance: Git objects are immutable and object IDs are content-derived. A newly created evidence commit cannot embed its own not-yet-created commit ID without producing another commit, so this receipt anchors the exact reviewed predecessor tree while PR/checkpoint metadata records the post-write receipt head.

Behavior/API/evidence-model references only. No proprietary source was copied and no dependency was added.

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

## Exact-head hosted verification

Exact source/evidence tree `727dcf7cef2a01c4be13931db0551247edf929bb` is fully green at the hosted level:

- Windows build and deterministic tests `35810545499`, job `107020923389`: `completed/success`, exact `head_sha` `727dcf7...`, completed `2026-09-23T02:31:22Z` on `windows-2022`.
  - passed repository/R0 safety contracts;
  - passed Release assertion and CTest safety contracts;
  - configured Visual Studio 2022 x64;
  - built and ran deterministic Debug tests;
  - built and ran deterministic Release tests;
  - passed runtime dependency/prerequisite policy checks;
  - passed static milestone verifiers; and
  - confirmed a clean tracked tree.
- profiling capture portability `35810545487`: `completed/success`.
- release manifest integrity `35810545457`: `completed/success`.

The intermediate candidate-head Windows run `35810233080` was cancelled after a newer evidence commit superseded it and is not counted as a pass. The exact `727dcf7...` runs above supersede that hosted-evidence gap.

Hosted deterministic CTest intentionally excludes tests whose names end in `RuntimeSmoke`; therefore no hosted result is interactive editor GUI evidence.

## Independent review state

Fresh Codex review of exact source/evidence tree `727dcf7...` was submitted at `2026-09-23T02:34:44Z`. It reported one P2 evidence-traceability finding: this QA receipt, the task packet, and the capability map omitted `727dcf7...` and its exact-head successful workflows, leaving the native handoff tied to intermediate evidence. The review did not report a new runtime-smoke implementation defect.

This commit is part of the evidence-only remediation of that finding. The source candidate and both CMake/smoke blobs remain unchanged. Fresh independent re-review of the post-repair branch head is required before `independent_acceptance` may become true.

## Acceptance state and limitations

`native_evidence` remains empty. No registered interactive Windows desktop execution, screenshots, actual GPU behavior, clean-machine packaging, measured comparative performance, broader stress/recovery, or 24-hour soak was executed by this coordinator.

Issue #7 remains open. The historical R0 runner was not invoked.

Status: **toolbar semantic HWND/slot binding is implemented; exact source/evidence tree `727dcf7...` is hosted-green and independently reviewed with one evidence-only finding now remediated in the durable records. Native Debug/Release GUI evidence and clean re-review of the repaired receipt head remain pending. No UE5/Unity parity claim is made.**

## Registered native handoff

After clean independent re-review of the evidence-repaired branch head, run that exact reviewed head on one owned interactive Windows desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine/Windows identity, MSVC/CMake and GPU/driver versions, exact commands, complete stdout/stderr, exit codes, UTC timestamps, normal and narrow-window screenshots, and a process inspection showing zero owned contained processes after any failure or interruption.

## Single next action

Obtain clean independent re-review of the evidence-repaired head. If clean, execute the registered native Debug/Release GUI smoke and preserve the complete receipt set.