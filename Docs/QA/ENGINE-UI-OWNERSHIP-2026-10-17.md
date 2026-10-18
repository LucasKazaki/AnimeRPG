# UI input ownership evidence — October 17, 2026

Status: focused native Debug/Release verified after three independently found
callback defects were reproduced and repaired; independent re-review/integration
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

## Independent-review failures and follow-up

The first candidate, `74eed63f3a684e207653499963317b93fce6a4f3`, passed its
15-case focused suite but independent review found three additional dispatch
defects. Added direct UIRoot regressions and executed each against that unchanged
production source before repairing it:

- A previous widget's blur callback set another modal scope, cancelling the
  pointer target. Dispatch then dereferenced null: access violation exit
  **-1073741819** (`0xC0000005`).
- An unhandled Space bubbled past a nested modal root to an outside handler:
  one outside invocation, expected assertion exit **3**.
- Pointer release opened a modal, queued Enter closed it, then queued Space
  reactivated the outside launcher: two launches instead of one, exit **3**.

The follow-up validates the original captured target after focus callbacks.
Focus notifications honor callback-driven replacement/cancellation. Key hooks
are followed by validation before invoking the old widget or continuing its
route; a hook that disables/removes the target or changes modality cancels that
route even if it returns false. Keyboard bubbling ends at the modal root,
inclusive. A modal activation counter records ownership throughout dispatch,
including a modal opened and closed entirely inside one callback. Subsequent
queued keys/text cannot escape its closing frame. There is no GameHost change
in this follow-up; its existing modal-result consumer provides gameplay masking.

## Focused validation

VS2022 Community x64, MSVC 19.44 / tools 14.44; CMake 3.31.6-msvc6,
`Visual Studio 17 2022` generator. External multi-config Debug/Release cache:
`work/build-ui-input-ownership`. Existing `astral_add_test(EngineUITests)` and
`AssertionsEnabled.cpp` remain unchanged; Release logs show `/UNDEBUG`
overriding `/DNDEBUG`. The three review failures above are retained; no failure
occurred in the subsequent follow-up builds/tests.

| Step | Exit/result |
|---|---|
| Fresh configure / old-source EngineUITests Debug build | 0 / 0 |
| Four old real-host regressions, separately filtered | **3 each**, expected reproduction |
| Fixed Debug build / full focused CTest | 0 / 0; **15 cases**, 0.05 seconds (0.06 total) |
| Fixed Release build / full focused CTest | 0 / 0; **15 cases**, 0.04 seconds |
| Test-safety fixture suite | 0; **3/3**, 6.643 seconds |
| Static milestone 1 / 2 / 3 | 0 / 0 / 0 |
| Review old-source Debug build | 0 |
| Three review regressions against unchanged `74eed63` | **-1073741819 / 3 / 3**, expected reproduction |
| Review-fixed Debug build / full focused CTest | 0 / 0; **19 cases**, 0.04 seconds (0.05 total) |
| Review-fixed Release build / full focused CTest | 0 / 0; **19 cases**, 0.03 seconds |
| Review-fixed test safety / static milestones 1, 2, 3 | 0; **3/3**, 5.289 seconds / 0, 0, 0 |
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

Review-reproduction slot: `2026-10-17T14:24:04.3581497-04:00` to
`14:24:08.1225122-04:00`. The coordinator-authorized follow-up focused builds
started at `14:26:29.1877814-04:00`; native suite checks finished at
`14:26:56.9910279-04:00`. Safety/static checks finished and the compiler/native
slot was released at `14:27:13.4009193-04:00`. Four additional cases cover all
three reported defects, same-callback transient modality, and false-return
key-hook removal/disable/modal cancellation. The original 15 cases still pass.

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

The review follow-up used these commands in the same worktree/cache; original
logs were preserved. The three old-source test exits were individually retained
and checked before production edits.

```powershell
& $cmake --build $build --config Debug --target EngineUITests --parallel 2 *> "$evidence/review-reproduction-build-debug.log"
$reviewTests = @('UIFocusCallbackCanCancelPointerTargetBeforeDispatch','UIModalKeyboardBubblingStopsAtModalRoot','UIModalOpenedAndClosedDuringDispatchOwnsRemainingFrame')
foreach ($testName in $reviewTests) {
    & "$build/Debug/EngineUITests.exe" $testName *> "$evidence/review-old-$testName.log"
}
# Only after retaining those failures, apply the production follow-up:
& $cmake --build $build --config Debug --target EngineUITests --parallel 2 *> "$evidence/review-fixed-build-debug.log"
& $ctest --test-dir $build -C Debug --output-on-failure -V -R '^EngineUITests$' --no-tests=error *> "$evidence/review-fixed-tests-debug.log"
& $cmake --build $build --config Release --target EngineUITests --parallel 2 *> "$evidence/review-fixed-build-release.log"
& $ctest --test-dir $build -C Release --output-on-failure -V -R '^EngineUITests$' --no-tests=error *> "$evidence/review-fixed-tests-release.log"
& $python Scripts/test_test_safety.py *> "$evidence/review-test-safety.log"
foreach ($milestone in 1,2,3) {
    & $python "Scripts/verify_milestone$milestone.py" *> "$evidence/review-milestone$milestone.log"
}
git diff --check
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
