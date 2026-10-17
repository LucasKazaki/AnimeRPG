# UI input ownership evidence — October 17, 2026

Status: focused native Debug/Release verified; independent review/integration
remain pending. Sole implementation owner: UI-input packet agent.
Worktree: `C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/astral-ui-input`.
Branch: `engine/ui-input-ownership-20261017`.
Base: `92e4880489d2478f2fa45fbb690fabd8caca63dd`, the coordinator's deliberately
assembled PR80 + reviewed containment + main numeric + reviewed finite input
candidate. It is distinct from main and original unmerged PR80. The coordinator
reported full baseline Debug/Release all-target builds and 31/31 deterministic
suites each; this receipt is baseline context, not post-fix integration evidence.

## Old failures and implemented ownership

Before production edits, added four neutral fixtures through the **real
GameHost**, existing UIRoot and existing InputSystem. No gameplay behavior or
content fixture was added. Each expected-failure run aborted with exit **3**:

- HUD button pointer-down also made the Attack/MouseLeft action down.
- Focused button Space activation also made Jump down.
- Open console plus outside pointer-down let W/Attack reach the game.
- After an outside click removed text focus, Escape left the console open.

`UIRoot::Update` now returns `UIInputResult`: consumed key bits, keyboard
capture, pointer consumption and modality. The console returns the same result
from preprocessing. GameHost merges both and masks its actual action snapshot.
It latches consumed held keys until an observed raw release. Current-frame
consumption and held latches are distinct, so a release frame cannot make the
next fresh press remain blocked.

UI pointer ownership begins on a hit-tested press and lasts through its release,
including drags/releases outside and widget removal/disable/hide. Cancellation
stops widget callbacks and press feedback but retains ownership through release.
Handled activation/navigation keys are reported individually; ordinary unhandled
keys on focused buttons still reach gameplay. TextBox focus captures keyboard
input through the frame that changes focus. Mouse virtual keys remain a separate
channel; owned left-pointer gestures mask left-pointer input and mouse delta.

The console sets one modal UIRoot subtree. Outside presses cannot activate the
HUD or steal its input focus; modal navigation stays in its subtree. Escape and
the toggle route before focused widgets. Opening/closing frames remain modal,
and closing-frame UI dispatch is suppressed so simultaneous pointer/keyboard
events cannot activate the HUD. Modal capture masks all gameplay keys and mouse
delta. Held keys remain masked after closing until physically released.

## Focused validation

VS2022 Community x64, MSVC 19.44 / tools 14.44; CMake 3.31.6-msvc6,
`Visual Studio 17 2022` generator. External multi-config Debug/Release cache:
`work/build-ui-input-ownership`. Existing `astral_add_test(EngineUITests)` and
`AssertionsEnabled.cpp` remain unchanged; Release logs show `/UNDEBUG`
overriding `/DNDEBUG`. No post-fix build or test failure occurred.

| Step | Exit/result |
|---|---|
| Fresh configure / old-source EngineUITests Debug build | 0 / 0 |
| Four old real-host regressions, separately filtered | **3 each**, expected reproduction |
| Fixed Debug build / full focused CTest | 0 / 0; **15 cases**, 0.05 seconds (0.06 total) |
| Fixed Release build / full focused CTest | 0 / 0; **15 cases**, 0.04 seconds |
| Test-safety fixture suite | 0; **3/3**, 6.643 seconds |
| Static milestone 1 / 2 / 3 | 0 / 0 / 0 |
| `git diff --check` | 0 |

The nine added cases cover the four original leaks, multi-frame Space/MouseLeft
holds, outside pointer release, release then fresh gameplay press, widget
removal/disable/hide, keyboard focus loss/removal/disable inside callbacks,
TextBox capture and cancellation, unhandled W, modal HUD shielding and closing
frames, and actual consumption-result fields. Existing six UI cases pass,
including layout, drawing to the real Canvas, slider capture, navigation,
deferred removal, text entry, console history and completion. Snapshot tests
exercise application focus/cancellation APIs; they do not establish OS focus or
capture-loss delivery. No test was weakened or disabled.

Initial old-reproduction slot: `2026-10-17T14:13:19.5178578-04:00` to
`14:14:31.2507891-04:00`. It was released during source editing. Post-fix slot:
`14:18:36.0753419-04:00` to `14:20:04.4442577-04:00`, then released. All native
compilation/tests were serialized by the coordinator. No GUI/GPU run was made.

## Retained exact commands

Logs remain outside Git source under `work/ui-input-ownership-evidence`.
All actual command exits were checked before continuing dependent work.

```powershell
$worktree = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/astral-ui-input'
$build = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/build-ui-input-ownership'
$evidence = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/ui-input-ownership-evidence'
$cmakeDir = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin'
$cmake = "$cmakeDir/cmake.exe"
$ctest = "$cmakeDir/ctest.exe"
$python = 'C:/Users/lucas/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
& $cmake -S $worktree -B $build -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON *> "$evidence/configure.log"
& $cmake --build $build --config Debug --target EngineUITests --parallel 2 *> "$evidence/reproduction-build-debug.log"
$tests = @('UIHostButtonPointerDoesNotReachGameplay','UIHostButtonKeyboardDoesNotReachGameplay','UIHostConsoleOutsidePointerKeepsModalInput','UIHostConsoleEscapeClosesWithoutTextFocus')
foreach ($testName in $tests) {
    & "$build/Debug/EngineUITests.exe" $testName *> "$evidence/old-$testName.log"
}
# After the approved production implementation:
& $cmake --build $build --config Debug --target EngineUITests --parallel 2 *> "$evidence/fixed-build-debug.log"
& $ctest --test-dir $build -C Debug --output-on-failure -V -R '^EngineUITests$' --no-tests=error *> "$evidence/fixed-tests-debug.log"
& $cmake --build $build --config Release --target EngineUITests --parallel 2 *> "$evidence/fixed-build-release.log"
& $ctest --test-dir $build -C Release --output-on-failure -V -R '^EngineUITests$' --no-tests=error *> "$evidence/fixed-tests-release.log"
$env:PATH = "$cmakeDir;" + $env:PATH
& $python Scripts/test_test_safety.py *> "$evidence/test-safety.log"
foreach ($milestone in 1,2,3) {
    & $python "Scripts/verify_milestone$milestone.py" *> "$evidence/milestone$milestone.log"
}
git diff --check
git diff --name-only
```

## Scope, design guidance and remaining gates

Exactly nine authorized files changed: Widgets/ConsoleOverlay header/source,
GameHost header/source (UI Update/members only), EngineUITests, task and this QA
record. No InputSystem, Assets, Scene, EngineSampleTests, CMake, native platform,
policy, dependency, visual style or game-content edits. No push/merge by this
owner. GameHost remains reserved until the coordinator accepts the final freeze
and any independent-review fixes, before the reload owner begins host wiring.

Read and applied the user's Apple design skill and relevant bundled focus,
keyboard, modality and pointing-device guidance: predictable focus, familiar
Escape cancellation and exclusive modal interaction. Fluid-motion capture
principles informed tracking beyond bounds and cancellation; no motion or
visual redesign was introduced. Platform behavior remains Astral's C++17/Windows
behavior rather than adopting Apple-specific UI controls or shortcuts.

Independent review remains separate from these tests. Full-all post-fix
engine/editor/player builds and broad deterministic tests are coordinator-owned.
Native window focus/capture behavior, modifier-side mapping, Unicode/IME,
clipboard, subframe delivery, DPI, GPU/performance, packaging and soak are not
verified or claimed fixed here. Full engine workflow acceptance remains open.
