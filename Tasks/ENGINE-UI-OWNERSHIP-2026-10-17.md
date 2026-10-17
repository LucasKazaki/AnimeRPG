# UI and console input ownership — October 17, 2026

Owner: sole UI-input implementation agent; weekend coordinator owns compiler
and native desktop scheduling. Worktree `work/astral-ui-input`, branch
`engine/ui-input-ownership-20261017`, deliberate base
`92e4880489d2478f2fa45fbb690fabd8caca63dd` (assembled PR80, reviewed containment,
main numeric repairs and reviewed finite input). The coordinator cleared the
full native Debug baseline and 31/31 deterministic suites (55.99 seconds).
Current authority is Lucas's October 17 weekend engine assignment, repair B;
technical review/integration gates remain. No game-content work.

## Problem and bounded scope

The real host only clears gameplay input when `WantsKeyboard()` recognizes a
focused TextBox. A clicked HUD button can also Attack, focused Space activation
can also Jump, and clicking outside the open console loses keyboard capture
and Escape routing. A one-frame consumed-key flag alone would leak held input
on later frames. Reproduce all four through the real host before source edits.

Allowed paths: `Engine/UI/{Widgets,ConsoleOverlay}.{h,cpp}`;
`Engine/Framework/GameHost.{h,cpp}` for UI Update/members only;
`Tests/EngineUITests.cpp` for existing UI and neutral real-host regressions;
this packet and `Docs/QA/ENGINE-UI-OWNERSHIP-2026-10-17.md`.
Do not edit InputSystem, Assets, Scene, native platform code, other test suites,
CMake, dependencies, policy, visuals or game content. The reload owner must
wait for this owner to freeze GameHost before touching it.

## Consumption contract

`UIRoot::Update` and console preprocessing return an explicit result carrying
consumed key bits, keyboard capture, pointer ownership and modality. The host
uses the result to mask real action input. Existing UI callers may ignore the
return. Unhandled gameplay keys and pointer presses remain available.

Keyboard handlers and navigation report the exact keys they use. Text entry
owns keyboard input through a focus-changing frame. The host latches consumed
held keys until observed physical release, independently of later focus,
removal or disable. Frame masks and held latches are separate so a fresh press
after release is not accidentally blocked. Pointer gestures begun over UI are
owned through drag/outside release. Removing, hiding or disabling their widget
cancels callbacks but retains gameplay suppression through that release.

The console owns a single modal subtree while open, including outside clicks;
the HUD below it cannot activate. Escape/toggle routes before TextBox focus.
Opening and closing frames remain modal. A narrow suppressed-dispatch update
handles the closing frame so simultaneous events do not activate the HUD.
Modal capture masks all gameplay keys/mouse delta; owned pointer interaction
masks left pointer input and mouse delta. Native focus/IME are out of scope.

## Design guidance

Read the user's Apple design and fluid-motion skills proportionally for this
behavior-only repair. Relevant bundled HIG references are focus-and-selection
(predictable focus), keyboards (standard cancellation), modality (parent
interaction is suspended) and pointing-devices (consistent pointer/keyboard
behavior). The fluid-motion capture guidance informs tracking beyond bounds
and cancellation. Preserve Astral's current C++17/Windows conventions, keyboard
navigation, press feedback and appearance; add no animation or visual redesign.

## Acceptance, commands and limits

1. Retain the four pre-fix host failures using neutral original widget/input
   fixtures, without gameplay content or a simulated replacement host.
2. Cover held/released pointer and Space, outside release, cancellation through
   removal/disable/focus change, console outside clicks, focus-independent
   Escape, modal closing-frame consumption and fresh unhandled input afterward.
3. Existing UI behavior and real-host `EngineUITests` pass in native Debug and
   Release with assertions active. Run applicable test-safety/static checks.
4. Freeze GameHost, retain exact logs/commands, commit scoped work and request
   independent review. No push/merge by this owner. Stop/debug deterministic
   failures without weakening tests or repeating unchanged failing commands.

All compiles/native tests require the coordinator's explicit slot. Planned
external multi-config root: `work/build-ui-input-ownership`; evidence:
`work/ui-input-ownership-evidence`. Resolve these beneath
`C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you`.

```powershell
& $cmake -S $worktree -B $build -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON
& $cmake --build $build --config Debug --target EngineUITests --parallel 2
& "$build/Debug/EngineUITests.exe" UIHostButtonPointerDoesNotReachGameplay
& "$build/Debug/EngineUITests.exe" UIHostButtonKeyboardDoesNotReachGameplay
& "$build/Debug/EngineUITests.exe" UIHostConsoleOutsidePointerKeepsModalInput
& "$build/Debug/EngineUITests.exe" UIHostConsoleEscapeClosesWithoutTextFocus
& $ctest --test-dir $build -C Debug --output-on-failure -V -R '^EngineUITests$' --no-tests=error
& $cmake --build $build --config Release --target EngineUITests --parallel 2
& $ctest --test-dir $build -C Release --output-on-failure -V -R '^EngineUITests$' --no-tests=error
git diff --check
& $python Scripts/test_test_safety.py
& $python Scripts/verify_milestone1.py
& $python Scripts/verify_milestone2.py
& $python Scripts/verify_milestone3.py
```

Use installed VS2022 CMake/CTest and bundled Python; no installs. Full-all,
native window/focus loss, Unicode/IME, GPU, performance and soak evidence remain
coordinator-owned/unclaimed. Unit snapshots cannot establish native event
delivery or OS capture behavior.

Focused outcome: all four old host regressions reproduced exit 3 before source
edits; the repaired native Debug/Release suite passes 15 cases each. Test safety
3/3 and static M1/2/3 pass. See the QA record for exact commands/logs and scope.
Independent review and coordinator integration remain separate gates.
