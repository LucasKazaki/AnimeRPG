# E11 Editor Runtime Smoke evidence, 2026-09-23

## Current checkpoint

Branch: `engine/2026-09-22-editor-runtime-smoke`.
Current multi-size source candidate: `d71f5446bfbc7360778101f376a5d6deeaec17a8`.
`Tests/EditorRuntimeSmoke.cpp` blob: `dff48af572093bc99b45ef11a0f1d60390f3d425`.
`CMakeLists.txt` blob: `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.
Latest observed `main`: `7dfaeeb340e57d1024a8bc818c65c82cd391d4ae`.
Production editor source is unchanged.

The previous exact receipt `6c0bd849c2a5feeb1400d7fc263e155f48ea17b7` completed all three hosted workflows successfully and received a fresh Codex review completed at `2026-09-23T14:29:04.697315Z` with no new inline finding surfaced. That clean review predates the current source change and is not treated as review of `d71f5446...`.

## Coverage finding and repair

The original integrated E11 shell packet requires native resize acceptance at 800x600, 1280x720, 1440x900, maximized desktop size, and one deliberately short/narrow size if Windows permits it. The later `EditorRuntimeSmoke` executable sequence covered only startup, 800x600 and 420x260.

That mismatch could allow the current verification packet to silently narrow E11 acceptance. Commit `d71f5446bfbc7360778101f376a5d6deeaec17a8` restores deterministic automated checks for 1280x720 and 1440x900 using the existing `ResizeAndCheck` implementation. The smoke now checks startup plus 800x600, 1280x720, 1440x900 and 420x260 before final stable-window revalidation and shutdown.

GitHub commit inspection shows only `Tests/EditorRuntimeSmoke.cpp` changed. The functional diff adds two `ResizeAndCheck` calls and updates PASS text to state all four tested dimensions. It does not modify production editor code, CMake registration, workflows, dependencies, graphics API, scheduler configuration, game content, merge state, release state or deployment state.

Every added size reuses the existing bounded asynchronous resize path: `SetWindowPos(..., SWP_ASYNCWINDOWPOS)` requests the outer-window size, `GetWindowRect` is polled under the existing 1.5-second resize deadline, then `DirectChildrenContained` and `ValidateShellState` recheck the original child HWND inventory, positive geometry, containment, semantic bindings, list contents, selection/Inspector state and pending-tool state.

The prior final-close protections remain unchanged: retained process-handle liveness around HWND ownership checks, exact original PID/TID validation, `SuspendThread`, `GetThreadContext(CONTEXT_CONTROL)` as the post-suspend barrier, asynchronous `PostMessageW(WM_CLOSE)`, verified `ResumeThread` previous count one, and only then a bounded process wait. Failure cleanup remains owned-process/job scoped.

## Primary-source research

Accessed 2026-09-23 UTC:

- Epic, Unreal Engine 5.8, Viewport Toolbar: https://dev.epicgames.com/documentation/en-us/unreal-engine/viewport-toolbar
  - documents viewport layout/sizing controls and smaller-viewport overflow management. This supports testing materially different editor sizes rather than only one convenient layout.
- Epic, Unreal Engine 5.8, Unreal Editor Interface: https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-editor-interface
  - documents the Level Viewport, Outliner, Details and Content surfaces used as behavioral comparison points for Astral's much smaller current editor shell.
- Unity 6.0, `EditorWindow.maximized`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/EditorWindow-maximized.html
  - documents a maximized editor-window state. The original E11 maximized-desktop native check therefore remains explicit instead of being replaced by fixed resolutions.
- Unity 6.1, Scene view navigation: https://docs.unity3d.com/Manual/SceneViewNavigation.html
  - documents the Scene view as an interactive authoring view. Astral does not claim equivalent tooling, but its viewport/panels must remain coherent as layout changes.
- Microsoft Learn `SetWindowPos`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowpos
  - `SWP_ASYNCWINDOWPOS` posts the resize when caller and target input queues differ, which is the bounded cross-process path retained by this smoke.
- Microsoft Learn `GetWindowRect`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowrect
  - returns the window bounds in screen coordinates with exclusive right/bottom edges; the smoke polls those bounds before checking containment/state.
- Retained basis: Microsoft `GetThreadContext`, `SuspendThread`, `ResumeThread`, `PROCESS_INFORMATION`, `PostMessageW`, `GetWindowThreadProcessId`, `WaitForSingleObject`, `TerminateProcess`, and Job Object documentation.

No proprietary Unreal Engine or Unity source was copied, and no dependency was added.

## Verification

Previous exact receipt `6c0bd849...`:

- Windows build and deterministic tests `35873639886`: PASS;
- profiling capture portability `35873639870`: PASS;
- release manifest integrity `35873639882`: PASS;
- fresh Codex review completed `2026-09-23T14:29:04.697315Z`, no new inline finding surfaced.

Current source candidate `d71f5446...`:

- commit inspection: PASS, only `Tests/EditorRuntimeSmoke.cpp` changed;
- profiling capture portability `35881476543`: PASS;
- Windows build and deterministic tests `35881476517`: in progress at this receipt update;
- release manifest integrity `35881476562`: in progress at this receipt update;
- hosted interactive `EditorRuntimeSmoke`: NOT RUN by the deterministic suite;
- native Windows interactive acceptance: pending;
- independent review of the new source: pending.

The sandbox did not provide the owned Windows interactive desktop required for this test. No sandbox native GUI, GPU, packaging, performance or soak result is claimed.

## Retained hardening and acceptance state

- `EditorContainmentTests` remains hosted and deterministic; interactive `EditorRuntimeSmoke` remains a separate native gate.
- Containment still exercises worker-local cleanup, whole-job zero-active-process cleanup, and rejection of a successful worker that leaves descendants.
- Shell smoke requires one stable process-owned top-level editor window, original 12 child HWND/class identities, bound semantic Static/Button identities, disabled pending toolbar tools, exact Outliner/assets rows, `LBS_NOTIFY`, selection/Inspector synchronization, bounded cross-process messages, startup containment and positive child area.
- Automated fixed-size checks now cover 800x600, 1280x720, 1440x900 and 420x260.
- Maximized-desktop behavior is still a native interactive requirement, not inferred from fixed-size automation.
- Final close still requires the original launch process/thread identity and successful suspended-thread context barrier before enqueue, then verified thread resume before any process wait.

`native_evidence`: empty.
Independent final acceptance: false.
UE5/Unity parity claim: false.
Issue #7: open, historical R0 runner not invoked.

## Native handoff

After the exact current receipt has green hosted checks and fresh clean independent review, the registered Windows executor must run Debug and Release `EditorContainmentTests` and `EditorRuntimeSmoke` on one owned interactive desktop.

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

Wait for the two remaining hosted workflows on `d71f5446...`, record their exact conclusions and synthetic merge/base provenance, then obtain fresh independent review of the exact post-evidence receipt before any registered native handoff.