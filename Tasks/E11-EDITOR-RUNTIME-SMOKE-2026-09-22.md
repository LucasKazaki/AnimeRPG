# E11 Editor Runtime Smoke task, 2026-09-23

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. This packet may harden the native Windows smoke, its recovery supervisor, deterministic containment tests, and evidence. It must not add scene mutation/serialization, gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, merge, rebase, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Admitted baseline from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest observed `main`: `7dfaeeb340e57d1024a8bc818c65c82cd391d4ae`. Separate game-worker work was not absorbed or rebased into this engine branch.
Current multi-size verification candidate: `d71f5446bfbc7360778101f376a5d6deeaec17a8`.
`Tests/EditorRuntimeSmoke.cpp` blob: `dff48af572093bc99b45ef11a0f1d60390f3d425`.
`CMakeLists.txt` remains blob `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.

Allowed paths only: `CMakeLists.txt`, `Tests/EditorRuntimeSmoke.cpp`, this task, `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`, and `Docs/Research/ENGINE-CAPABILITIES.json`. One active writer only. Do not rebase, merge, force-push, or absorb unrelated work.

## Selected verification coverage gap

The integrated E11 editor-shell packet requires interactive resize acceptance at 800x600, 1280x720, 1440x900, maximized desktop size, and one deliberately short/narrow size if Windows permits it. The later automated runtime smoke had narrowed its executable resize sequence to 800x600 and 420x260 only.

That was an acceptance-coverage regression. A later verification packet must not silently reduce an already-admitted editor-shell acceptance matrix. This pass restores deterministic automated checks for the two omitted fixed resolutions while keeping maximized-desktop validation as native interactive evidence because the available hosted suite does not provide the owned desktop acceptance context required by project controls.

This is verification hardening, not a new editor feature. It does not enable any pending toolbar action or expand engine architecture.

## Bounded implementation

Commit `d71f5446bfbc7360778101f376a5d6deeaec17a8` changes only `Tests/EditorRuntimeSmoke.cpp`:

- retains startup containment before any resize;
- retains the existing 800x600 check;
- adds the previously omitted 1280x720 check using the existing bounded asynchronous `ResizeAndCheck` path;
- adds the previously omitted 1440x900 check using the same path;
- retains the deliberately narrow 420x260 check;
- every fixed-size check still requires the same original 12 child HWND identities, semantic Static/Button bindings, selection/Inspector state, enabled/disabled state, positive child area, and client containment;
- updates the PASS text so a retained receipt states all four automated resize dimensions explicitly.

GitHub commit inspection reports only this file changed. Production editor source, CMake registration, workflows, dependencies, graphics API, game content, scheduler configuration, release state, and architecture are unchanged.

The prior shutdown hardening remains intact: PID/HWND checks require the retained launched-process handle to be live, final close pins the original launch PID/TID, requires a successful suspended-thread `GetThreadContext(CONTEXT_CONTROL)` barrier, posts asynchronous `WM_CLOSE`, and verifies `ResumeThread` returned previous suspend count one before waiting for process exit.

## Primary research basis, rechecked 2026-09-23 UTC

- Epic, Unreal Engine 5.8, Viewport Toolbar: https://dev.epicgames.com/documentation/en-us/unreal-engine/viewport-toolbar
  - documents viewport layout/sizing controls and improved overflow management for smaller viewports. Applicability: editor verification must cover materially different viewport sizes rather than a single convenient size.
- Epic, Unreal Engine 5.8, Unreal Editor Interface: https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-editor-interface
  - documents the Level Viewport, Outliner, Details and Content surfaces that Astral's current E11 shell is intentionally approximating at a much smaller scope.
- Unity 6.0, `EditorWindow.maximized`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/EditorWindow-maximized.html
  - documents maximized editor-window state. Applicability: the original E11 native acceptance requirement for a maximized desktop state remains valid and is not replaced by fixed pixel sizes.
- Unity 6.1, Scene view navigation: https://docs.unity3d.com/Manual/SceneViewNavigation.html
  - documents the Scene view as an interactive authoring camera/view. Applicability: Astral's editor viewport must remain usable across layout changes, even though Astral does not yet claim Unity-equivalent scene tooling.
- Microsoft Learn `SetWindowPos`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowpos
  - `SWP_ASYNCWINDOWPOS` posts the resize request when the caller and target input queues differ. Applicability: the smoke uses this bounded asynchronous path for all four fixed sizes.
- Microsoft Learn `GetWindowRect`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowrect
  - returns screen-coordinate window bounds with exclusive right/bottom edges. Applicability: the smoke polls for exact completion of each requested outer-window size before validating shell state and containment.
- Retained Win32 basis: `GetThreadContext`, `SuspendThread`, `ResumeThread`, `PROCESS_INFORMATION`, `PostMessageW`, `GetWindowThreadProcessId`, `WaitForSingleObject`, `TerminateProcess`, Job Objects, and child-window APIs.

Public documentation is used for behavioral/API comparison only. No proprietary Unreal Engine or Unity source was copied and no dependency was imported.

## Verification state

Previous exact receipt `6c0bd849c2a5feeb1400d7fc263e155f48ea17b7` completed all three hosted workflows successfully:

- Windows build and deterministic tests `35873639886`: PASS;
- profiling capture portability `35873639870`: PASS;
- release manifest integrity `35873639882`: PASS.

Fresh Codex review of exact `6c0bd849...` completed at `2026-09-23T14:29:04.697315Z` with no new inline finding surfaced. That clean review predates the multi-size source change and therefore is not acceptance of `d71f5446...`.

For source candidate `d71f5446...`:

- profiling capture portability `35881476543`: PASS;
- Windows build and deterministic tests `35881476517`: running at this checkpoint;
- release manifest integrity `35881476562`: running at this checkpoint;
- fresh independent review: required after the exact post-evidence receipt head is pinned.

Hosted deterministic suites do not execute interactive `EditorRuntimeSmoke`; a hosted compile/test pass is not native GUI acceptance.

## Retained E11 hardening and gates

1. `EditorContainmentTests` executes in hosted deterministic suites while interactive `EditorRuntimeSmoke` remains separate.
2. Containment coverage exercises worker-local `CleanupProcess`, supervisor whole-job cleanup to zero active processes, and rejection of a zero-exit worker that leaves a descendant.
3. Shell verification retains stable single top-level identity, the original 12 child HWND/class inventory, semantic Static/Button binding, exact Outliner/assets rows, selection/Inspector synchronization, `LBS_NOTIFY`, bounded cross-process messages, positive-area startup containment, and final stable-window revalidation.
4. Automated fixed-size runtime checks now cover 800x600, 1280x720, 1440x900 and 420x260.
5. Maximized-desktop layout, screenshots and human-visible usability remain native interactive acceptance requirements.
6. PID-based HWND checks require retained launched-process-handle liveness. Final close requires exact original PID/TID ownership plus a successful suspended-thread context barrier, followed by verified resume before any process wait.
7. `native_evidence` remains empty. Independent final acceptance remains false until the exact current source/receipt receives fresh independent review and registered native Windows acceptance is retained.
8. Issue #7 remains open, so the historical R0 runner is blocked and must not be invoked.

## Registered native handoff

Only after green hosted checks and a fresh clean independent review of the exact receipt tree, the registered Windows executor should use one owned interactive desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact reviewed source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, and process inspection proving zero owned contained processes after any failure or interruption.

The interactive evidence must preserve the original E11 shell matrix rather than shrinking to the automated subset:

- untouched default startup layout;
- 800x600;
- 1280x720;
- 1440x900;
- maximized desktop size;
- 420x260 as the deliberately short/narrow case, or the closest OS-permitted narrow size if window minimum constraints intervene.

Capture screenshots for each state. Confirm panels/controls remain contained, viewport paint does not bleed into adjacent surfaces, selection and Inspector state remain synchronized, pending toolbar actions remain disabled, and final close leaves the launch thread resumed without acting on an unrelated/recycled HWND. Separately launch `AstralGame` from the same exact source/build and record that it remains a distinct executable with no behavior change attributable to this verification packet. That separate launch is a regression check, not a resumption of game-content testing.

## Rollback and stop conditions

Rollback only the multi-size additions if native evidence shows an admitted fixed size cannot be established reliably by the existing bounded asynchronous resize contract, or if the added checks introduce a reproducible regression. Do not remove an original E11 acceptance size merely to make the gate pass; preserve the failure and repair the verification path instead.

Stop before production-runtime change, workflow edit outside packet authority, rebase, merge, R0 execution, scheduler operation, dependency addition, graphics/API change, or game-content work. Never weaken a native acceptance assertion to make the gate green.

## Single next useful action

Finish hosted verification for `d71f5446...`, reconcile the exact source/merge/base and workflow results into the receipt/capability records, obtain fresh independent review of that exact receipt tree, then hand only that reviewed tree to the registered Windows executor for Debug/Release containment plus the full interactive size matrix.