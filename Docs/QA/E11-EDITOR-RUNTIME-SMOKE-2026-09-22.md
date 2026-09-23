# E11 Editor Runtime Smoke evidence, 2026-09-23

## Current checkpoint

Branch: `engine/2026-09-22-editor-runtime-smoke`.
Current multi-size source candidate: `d71f5446bfbc7360778101f376a5d6deeaec17a8`.
Last fully hosted integration receipt before this evidence repair: source/receipt head `0d2bc2f89171282cdaa14f8291e2d37e63a301ad`, synthetic PR merge `3d260d2e1baa726d36993deaf85d6b47f5919f00`, tested base `7dfaeeb340e57d1024a8bc818c65c82cd391d4ae`.
`Tests/EditorRuntimeSmoke.cpp` blob: `dff48af572093bc99b45ef11a0f1d60390f3d425`.
`CMakeLists.txt` blob: `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.
Production editor source is unchanged.

The previous clean receipt `6c0bd849c2a5feeb1400d7fc263e155f48ea17b7` completed all three hosted workflows successfully and received a fresh Codex review at `2026-09-23T14:29:04.697315Z` with no new finding. That review predates the multi-size source change.

## Coverage finding and repair

The original integrated E11 shell packet requires native resize acceptance at 800x600, 1280x720, 1440x900, maximized desktop size, and one deliberately short/narrow size if Windows permits it. The later `EditorRuntimeSmoke` executable sequence covered only startup, 800x600 and 420x260.

Commit `d71f5446bfbc7360778101f376a5d6deeaec17a8` restores deterministic automated checks for 1280x720 and 1440x900 using the existing `ResizeAndCheck` implementation. The smoke now checks startup plus 800x600, 1280x720, 1440x900 and 420x260 before final stable-window revalidation and shutdown.

GitHub commit inspection shows only `Tests/EditorRuntimeSmoke.cpp` changed. The functional diff adds two `ResizeAndCheck` calls and updates PASS text to state all four tested dimensions. It does not modify production editor code, CMake registration, workflows, dependencies, graphics API, scheduler configuration, game content, merge state, release state or deployment state.

Every added size reuses the existing bounded asynchronous resize path: `SetWindowPos(..., SWP_ASYNCWINDOWPOS)` requests the outer-window size, `GetWindowRect` is polled under the existing 1.5-second resize deadline, then `DirectChildrenContained` and `ValidateShellState` recheck the original child HWND inventory, positive geometry, containment, semantic bindings, list contents, selection/Inspector state and pending-tool state.

The prior final-close protections remain unchanged: retained process-handle liveness around HWND ownership checks, exact original PID/TID validation, `SuspendThread`, `GetThreadContext(CONTEXT_CONTROL)` as the post-suspend barrier, asynchronous `PostMessageW(WM_CLOSE)`, verified `ResumeThread` previous count one, and only then a bounded process wait. Failure cleanup remains owned-process/job scoped.

## Primary-source research

Accessed 2026-09-23 UTC:

- Epic, Unreal Engine 5.8, Viewport Toolbar: https://dev.epicgames.com/documentation/en-us/unreal-engine/viewport-toolbar
- Epic, Unreal Engine 5.8, Unreal Editor Interface: https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-editor-interface
- Unity 6.0, `EditorWindow.maximized`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/EditorWindow-maximized.html
- Unity 6.1, Scene view navigation: https://docs.unity3d.com/Manual/SceneViewNavigation.html
- Microsoft Learn `SetWindowPos`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowpos
- Microsoft Learn `GetWindowRect`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowrect
- Retained Win32 basis: `GetThreadContext`, `SuspendThread`, `ResumeThread`, `PROCESS_INFORMATION`, `PostMessageW`, `GetWindowThreadProcessId`, `WaitForSingleObject`, `TerminateProcess`, and Job Object documentation.

These sources support materially different editor-size verification, retention of maximized-desktop acceptance, and the bounded asynchronous resize/query approach. No proprietary Unreal Engine or Unity source was copied, and no dependency was added.

## Verification

Previous clean receipt `6c0bd849...`:

- Windows `35873639886`: PASS;
- profiling `35873639870`: PASS;
- release manifest `35873639882`: PASS;
- fresh Codex review completed `2026-09-23T14:29:04.697315Z`, no new finding.

First workflow set on source candidate `d71f5446...`:

- commit inspection: PASS, only `Tests/EditorRuntimeSmoke.cpp` changed;
- profiling `35881476543`: PASS;
- Windows `35881476517`: CANCELLED after newer evidence commits superseded the head;
- release manifest `35881476562`: CANCELLED after newer evidence commits superseded the head.

The cancelled runs are not counted as pass or failure evidence for the final integration receipt.

Exact integration receipt `0d2bc2f89171282cdaa14f8291e2d37e63a301ad` was tested through synthetic PR merge `3d260d2e1baa726d36993deaf85d6b47f5919f00` into base `7dfaeeb340e57d1024a8bc818c65c82cd391d4ae`:

- Windows build and deterministic tests `35881865946`, job `107252459186`: PASS, completed `2026-09-23T15:32:02Z`; all reported repository/R0 safety contracts, prerequisite/runtime checks, Release assertion/CTest safety, VS2022 x64 configure, Debug build/tests, Release build/tests, dependency checks, static milestone verifiers and clean-tree checks passed;
- profiling capture portability `35881866081`: PASS;
- release manifest integrity `35881866038`: PASS.

Fresh Codex review of exact `0d2bc2f8...` completed `2026-09-23T15:33:36.727690Z` and produced one evidence-only P2: the durable records still called the superseded source-candidate Windows/release runs in progress and omitted the exact `0d2bc2f8...` replacement workflows. No new runtime-code defect was reported. This evidence repair corrects all three records. Its post-write content-addressed head is pinned in PR metadata/checkpoint because a Git commit cannot contain its own not-yet-computed SHA without creating another commit.

Hosted deterministic suites do not execute interactive `EditorRuntimeSmoke`. Native Windows interactive acceptance remains pending. The sandbox did not provide the owned Windows interactive desktop required for this test, so no sandbox native GUI, GPU, packaging, performance or soak result is claimed.

## Retained hardening and acceptance state

- `EditorContainmentTests` remains hosted and deterministic; interactive `EditorRuntimeSmoke` remains a separate native gate.
- Containment still exercises worker-local cleanup, whole-job zero-active-process cleanup, and rejection of a successful worker that leaves descendants.
- Shell smoke requires one stable process-owned top-level editor window, original 12 child HWND/class identities, bound semantic Static/Button identities, disabled pending toolbar tools, exact Outliner/assets rows, `LBS_NOTIFY`, selection/Inspector synchronization, bounded cross-process messages, startup containment and positive child area.
- Automated fixed-size checks cover 800x600, 1280x720, 1440x900 and 420x260.
- Maximized-desktop behavior remains a native interactive requirement, not inferred from fixed-size automation.
- Final close requires original launch process/thread identity and a successful suspended-thread context barrier before enqueue, then verified thread resume before any process wait.

`native_evidence`: empty.
Independent final acceptance: false.
UE5/Unity parity claim: false.
Issue #7: open, historical R0 runner not invoked.

## Native handoff

After the exact post-repair receipt has green hosted checks and fresh clean independent review, the registered Windows executor must run Debug and Release `EditorContainmentTests` and `EditorRuntimeSmoke` on one owned interactive desktop.

Retain exact reviewed source SHA, Windows/machine identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, and zero-contained-process proof after any failure or interruption.

Preserve the complete E11 editor-shell size matrix in visual/native evidence:

- untouched default startup;
- 800x600;
- 1280x720;
- 1440x900;
- maximized desktop size;
- 420x260 narrow state, or the closest OS-permitted narrow size if minimum-window constraints intervene.

Capture a screenshot at each state. Verify panel/control containment and positive area, viewport-paint clipping, semantic shell continuity, Cube selection/Inspector synchronization, pending toolbar disabled state, and safe final close. Separately launch `AstralGame` from the same exact source/build and retain a no-regression receipt showing this verification packet did not change its behavior.

Clean-machine packaging, comparative frame-time/RAM/VRAM measurement, broader stress/recovery, the remaining engine capability catalogue and the required 24-hour soak remain unresolved.

## Single next action

Pin the post-write evidence-repair head in PR metadata, let its replacement hosted workflows complete, obtain fresh independent review of that exact post-repair tree, then hand only that reviewed tree to the registered Windows executor.