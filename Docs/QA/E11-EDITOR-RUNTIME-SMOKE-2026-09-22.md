# E11 Editor Runtime Smoke evidence, 2026-09-22

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. The packet verifies the already-integrated Win32 editor shell and does not authorize scene mutation/serialization, transform gizmos, Play-in-Editor, asset import, graphics/API changes, dependencies, game content, scheduler operations, deployment, release, or R0 execution.

Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Current independently moving `main` re-read during this pass: `1ac6bf8d54219968effadd17e575aafb4ebd3847`. No rebase, merge, force push or unrelated game-work absorption was performed.

Current code candidate: `7ead712f69b18d4a1be9818ae53fb3e252f06214`.
`Tests/EditorRuntimeSmoke.cpp` blob: `f4e4c1a14bc2d382fbe32849f5b07bee15e55118`.
Integrated editor source fixture: `Tools/AstralEditorMain.cpp` blob `5142e632a79c89d0d0ce3efe87e752456f802185`; not modified by this packet.

## Independent findings repaired

Codex reviewed prior head `9931095a40a32e4ec6c93e35a9fb550f7bd1a243` at `2026-09-22T15:04:25Z` and reported two P2 findings.

First, a cached child HWND could be destroyed/recycled between individual cross-process sends inside a multi-message read. Broad pre-read surface checks were insufficient because `LB_GETTEXTLEN` and `LB_GETTEXT`, or `WM_GETTEXTLENGTH` and `WM_GETTEXT`, were separated by time while still trusting the same numeric handle.

Second, post-resize verification checked geometry containment but not the full required editor state. A hidden or relabeled surface, replaced control, or unexpectedly enabled pending toolbar action could retain an in-bounds rectangle and falsely pass.

Candidate `7ead712...` repairs both:

- all top-level, child-text and list-box read helpers revalidate ownership immediately before a bounded send; child reads additionally require the expected parent, class, visibility and, for Outliner/Assets, exact control ID;
- two-message text reads revalidate again before the second send, directly closing the recycled-HWND interval identified by review;
- direct-child enumeration is restricted to controls currently owned by the launched process;
- a reusable `ValidateShellState` verifies exact top-level identity, the 12-control inventory and classes, exact visible disabled pending buttons, exact visible labels/status, exact Inspector fixture, exact Outliner/Assets identity and contents, and the expected selection;
- both 800x600 and 420x260 asynchronous resizes retain containment checks and then execute the full shell-state validator;
- the Cube selection/notification path and final pre-shutdown state use the same complete validator.

Production editor code was not changed.

## Primary-source research, rechecked 2026-09-22

References are behavioral/API references only. No proprietary source, artwork, asset or dependency was copied/imported.

- Epic Games, Unreal Engine 5.8, Unreal Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
  - Defines Level Viewport, Outliner, Details and Content Browser/Drawer as distinct editor surfaces and documents synchronized actor selection.
- Epic Games, Unreal Engine 5.8, Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
  - Hierarchical selection surface with actor identity.
- Epic Games, Unreal Engine 5.8, Content Browser: https://dev.epicgames.com/documentation/en-us/unreal-engine/content-browser-in-unreal-engine
  - Primary area for viewing/managing identifiable project assets.
- Unity Technologies, Unity 6.0 (6000.0), The Hierarchy window: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html
  - Displays every GameObject in the scene and supports organization/visibility, reinforcing that post-layout surface state is behavioral evidence rather than geometry alone.
- Unity Technologies, Unity 6.0 (6000.0), Project window: https://docs.unity3d.com/6000.0/Documentation/Manual/ProjectView.html
  - Project asset-browsing surface; Astral's four procedural rows remain only a minimum verification fixture.
- Unity Technologies, Unity 6.0 (6000.0), Inspector items: https://docs.unity3d.com/6000.0/Documentation/Manual/InspectorItems.html
  - Inspector content depends on the selected item, matching the selection-synchronization property tested here.
- Microsoft `IsWindow`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindow
  - Explicitly warns that a window can be destroyed after checking and the handle can be recycled to another window.
- Microsoft `GetWindowThreadProcessId`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowthreadprocessid
  - Used to bind a current HWND to the launched PID and fail on invalid handles.
- Microsoft `IsWindowVisible`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindowvisible
  - Used to require the claimed editor surface to remain visible.
- Microsoft `IsWindowEnabled`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindowenabled
  - Used to verify pending toolbar actions remain disabled.

Access date: 2026-09-22.

## Portable reproduction and compiler checks

Disposable C++17 fixture SHA-256:
`47ad65939c672b33f1aad3831d5dcb770dae96865c698ef7c7774720299f3e57`

Commands executed in the coordinator sandbox:

```bash
g++ -std=c++17 -Wall -Wextra -Werror -pedantic /tmp/e11_per_send_resize_fixture.cpp -o /tmp/e11_per_send_resize_fixture
/tmp/e11_per_send_resize_fixture
clang++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer /tmp/e11_per_send_resize_fixture.cpp -o /tmp/e11_per_send_resize_fixture_san
ASAN_OPTIONS=detect_leaks=1 /tmp/e11_per_send_resize_fixture_san
sha256sum /tmp/e11_per_send_resize_fixture.cpp
```

Results:

- GCC warning-clean compile and execution: PASS, printed `per-send identity + post-resize shell-state contract fixture: PASS`.
- Clang ASan+UBSan warning-clean compile and execution with leak detection: PASS, printed the same marker.
- Fixture hash: `47ad65939c672b33f1aad3831d5dcb770dae96865c698ef7c7774720299f3e57`.
- The rewritten smoke was also checked with Clang C++17 `-Wall -Wextra -Werror -pedantic -fsyntax-only` against a disposable minimal Win32 declaration shim: PASS.

The portable fixture models the exact review conditions, including handle recycling between two read phases plus hidden/enabled/relabeled/replaced controls after resize. These are source-logic/compiler checks only. They are not native Win32 editor execution.

## Hosted exact-candidate evidence

Exact code candidate `7ead712f69b18d4a1be9818ae53fb3e252f06214` passed Windows Server 2022 run `35746903272`, job `106810756831`. Successful steps included checkout/external build root, R0 parser-only and runner-safety contracts, PE/prerequisite/runtime-policy contracts, Release assertion and CTest safety contracts, VS2022 x64 configure, MSVC Debug build and deterministic Debug tests, MSVC Release build and deterministic Release tests, Release dependency/prerequisite/runtime checks, static milestone verifiers and clean tracked-tree verification. The historical R0 runner itself was not executed.

Additional exact-candidate checks:

- Profiling capture portability run `35746903276`: PASS.
- Release manifest integrity run `35746903314`: PASS.

Hosted deterministic CTest intentionally excludes every `RuntimeSmoke`. These hosted results therefore prove compilation and deterministic non-runtime regression status, not native editor GUI execution.

## Review state

The prior independent Codex review completed on `9931095...` and its two new P2 findings are the defects repaired by `7ead712...`. Because runtime-smoke code changed, that review is not independent acceptance of the current candidate.

Fresh independent review of the exact current code/evidence head is required. Same-author inspection is not counted as independent acceptance.

## Native evidence and handoff

`native_evidence` remains empty. The coordinator did not access or claim a registered Windows interactive desktop.

Run exact code candidate `7ead712f69b18d4a1be9818ae53fb3e252f06214` on one owned interactive Windows desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain source SHA, machine/Windows identity, MSVC and CMake versions, GPU/driver identity, exact commands, full stdout/stderr, exit codes, UTC timestamps and screenshots at normal and narrow/short sizes. Human-visible acceptance must confirm exact `OUTLINER`, `INSPECTOR`, and `ASSETS / DEFAULT PRIMITIVES` labels; exact four asset rows; truthful E11 status text; Outliner visibly remains on Cube while Inspector shows Cube; viewport `Selected:` text reflects Cube; and panels do not bleed during resize.

## Result

Status: **per-send HWND identity validation and post-resize full shell-state verification implemented; portable warning-clean and sanitizer fixtures pass; exact-candidate hosted Windows Debug/Release, profiling and release-manifest workflows pass; fresh independent review and native Debug/Release RuntimeSmoke remain pending**.

E11 remains a partial editor-shell candidate, not UE5/Unity parity. No native GUI, GPU/performance, clean-machine, stress/recovery, soak, or final independent runtime acceptance claim is made. Issue #7 remains separate and open, and R0 was not invoked.

Single next useful action: obtain fresh independent review of the exact current code/evidence state, then execute Debug and Release `EditorRuntimeSmoke` on the registered Windows desktop with the required receipts/screenshots.
