# Finite input evidence — October 17, 2026

Status: implementation, independent final re-review, full native Debug/Release
builds, deterministic suites and retained GDI smokes pass. Hosted and eventual
proposed-main adoption gates remain separate.
Owner: sole finite-input implementation agent, with independent read-only
review by the coordinator's assigned reviewer.
Branch/worktree: `engine/input-finite-20261017`,
`C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/astral-input`.
Deliberate base: **unmerged PR #80** at
`efac9db54e8f2781c20fbb1bb9be9a0ab85f4f5c`, not main or the assets branch.
Tested source is that base plus this branch's final engine/test diff. Source
and tests were frozen before documentation/commit at `14:02:38-04:00`.

## Original reproduction and resulting contract

Before any production edits, the neutral real-host test loaded a disposable
empty scene and project with two Axis2D bindings, each `[3e38, 0]`. Both values
parsed unchanged. Simultaneous W/D produced `[-nan(ind), 0]`; the regression
aborted with exit **3**. No game behavior or production content fixture was
added. The fixed test explicitly requires direction `[1, 0]` and a press edge.

The repair retains the input/context framework and defines these boundaries:

- Direct `AddContext` returns bool with an optional diagnostic. It validates
  the entire context before mutation: axis components must be finite; all
  timing values must be finite and nonnegative. Full finite float values are
  accepted, without an arbitrary magnitude cap. Existing callers can ignore
  the return value. Existing context priority and key-consumption rules remain.
- JSON axis components must be numeric, finite and within `[-FLT_MAX, FLT_MAX]`
  before narrowing. Ordinary float rounding, including small values, remains.
  JSON timers retain their existing `[0, 10]` range; integer key/priority
  domains explicitly require finite input. Errors include the field path,
  and unsuccessful parse calls retain prior input/project output values.
- Axis components accumulate in compensated double sums, preserving direction
  and small cancellation residuals. Double `hypot` normalizes only magnitudes
  greater than one; small finite/subnormal contributions retain their size.
- Nonfinite/nonpositive dt continues to mean zero elapsed time. Positive
  elapsed counters saturate at the largest finite float. Private metadata
  prevents first-ever double taps and prevents intervals exceeding every
  representable tap/double-tap window from qualifying after saturation.
- Prior-press history ages exactly once per Update for every stored action,
  including disabled contexts. Disabling does not reset recent history or
  freeze its age. Buffer clearing and hold/key resolution retain existing
  context behavior. No public test-only state accessor was added.
- `ApplyProject` stages validated input before its existing configuration/input
  assignment. Rejected direct contexts report a diagnostic and retain the prior
  project/input/configuration/world. This is a narrow call-site check; broader
  scene-loading/configuration transaction behavior was not rewritten.

## Retained failed attempts and independent findings

The first post-fix Debug compile failed with MSVC C2872/C2882 in new tests:
their local `using namespace Input` made the old `Keys(...)` test helper
ambiguous with the `Input::Keys` namespace. Production runtime code compiled.
The helper was renamed `KeySnapshot` and all test call sites updated without
changing test behavior. The failed build log was retained separately from the
successful retry.

Independent source review then identified two timing gaps before acceptance.
Each was reproduced through existing public APIs against the initial patch:

1. Press/release, disable the only context, update twice with FLT_MAX, re-enable
   and press with dt zero. The prior-press timer was frozen while unresolved,
   so `DoubleTapped` incorrectly remained true. Regression exit **3**.
2. With FLT_MAX tap windows, press at dt zero, hold for one second, then hold
   for FLT_MAX. The overflow test subtracted the small elapsed operand from
   FLT_MAX in double, rounding away that operand. Release and next press
   incorrectly yielded `tapped=1 doubleTapped=1`. Regression exit **3**.

The corrected implementation ages history in the common once-per-action phase,
removing the resolved-only increment. The overflow comparison uses the smaller
operand against `FLT_MAX - larger` near the limit, preserving evidence of the
small positive interval. Additional checks keep short disabled history valid,
avoid double-aging enabled actions and accept exactly FLT_MAX at the boundary.
These findings and failures were fixed before final tests; they were not waived.

## Native results and observation limits

VS2022 Community, MSVC **19.44.35222.0** / tools **14.44.35207**, x64;
CMake **3.31.6-msvc6**, generator `Visual Studio 17 2022`.
External multi-config Debug/Release root: `work/build-finite-input`, configured
against this owned source tree. The coordinator granted every compile/native
slot. Existing `astral_add_test` registrations and `AssertionsEnabled.cpp`
remain unchanged; Release shows the expected `/UNDEBUG` override of `/DNDEBUG`.

| Step | Exit/result |
|---|---|
| Fresh configure / unchanged-engine `EngineSampleTests` Debug build | 0 / 0 |
| Original neutral real-host overflow regression | **3**, NaN reproduced |
| First implementation Debug build | **1**, test-helper ambiguity above |
| Corrected helper Debug build / focused CTest | 0 / 0; Systems 15 + Sample 14 cases |
| Initial Release build | 0; review findings arrived before its CTest run |
| Review reproduction Debug build | 0 |
| Disabled-history / small-interval timing regressions | **3 / 3**, expected reproductions |
| Final Debug build and focused CTest | 0 / 0; **2/2 suites, 17 + 14 cases**, 1.72 seconds |
| Final Release build and focused CTest | 0 / 0; **2/2 suites, 17 + 14 cases**, 0.21 seconds |
| Final separate original host regression | 0; 1 case |
| `Scripts/test_test_safety.py` | 0; **3/3**, 5.538 seconds |
| Static milestone 1 / 2 / 3 verifiers | 0 / 0 / 0 |
| `git diff --check` | 0 |

Coverage includes large/subnormal/cancelling axes and a small residual between
large opposing contributions; all direct nonfinite/negative timing ingress;
out-of-float-range/nonfinite JSON axes and timers; transactional input/project
rejection; ordinary priority/key consumption, press/release, buffered input,
tap/double-tap, rebind and recording behavior. Recording deserialization already
rejects overflow/nonfinite dt/mouse values and retains prior frames on failure;
this was verified without changing the format or implementation. Rebinding
only changes integer key codes and retains axis/timing invariants.

The initial slot ran `13:54:45.4966282-04:00` to the captured old-failure receipt
at `13:55:33.3889644-04:00`, then was released during source editing. The final
slot began `13:58:59.2733950-04:00` and was released at
`14:02:38.4874356-04:00`. Both review failures were captured at
`14:01:32.7141894-04:00`. No concurrent native build, desktop UI or GPU smoke
was started by this owner.

## Exact commands and logs

All retained logs are outside Git source in `work/finite-input-evidence`.
The original baseline and frozen containment worktree were untouched.

```powershell
$worktree = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/astral-input'
$build = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/build-finite-input'
$evidence = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/finite-input-evidence'
$cmakeDir = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin'
$cmake = "$cmakeDir/cmake.exe"
$ctest = "$cmakeDir/ctest.exe"
$python = 'C:/Users/lucas/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'

& $cmake -S $worktree -B $build -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON *> "$evidence/configure.log"
& $cmake --build $build --config Debug --target EngineSampleTests --parallel 2 *> "$evidence/reproduction-build-debug.log"
& "$build/Debug/EngineSampleTests.exe" HostExtremeFiniteAxisBindingsRemainDirectional *> "$evidence/reproduction-host.log"
& $cmake --build $build --config Debug --target EngineSystemsTests EngineSampleTests --parallel 2 *> "$evidence/fixed-build-debug.log"
# After correcting only the test helper's name:
& $cmake --build $build --config Debug --target EngineSystemsTests EngineSampleTests --parallel 2 *> "$evidence/fixed-build-debug-retry.log"
& "$build/Debug/EngineSampleTests.exe" HostExtremeFiniteAxisBindingsRemainDirectional *> "$evidence/fixed-host-reproduction.log"
& $ctest --test-dir $build -C Debug --output-on-failure -V -R '^Engine(Systems|Sample)Tests$' --no-tests=error *> "$evidence/fixed-tests-debug.log"
& $cmake --build $build --config Release --target EngineSystemsTests EngineSampleTests --parallel 2 *> "$evidence/fixed-build-release.log"
# Review reproductions precede the corresponding production fixes:
& $cmake --build $build --config Debug --target EngineSystemsTests --parallel 2 *> "$evidence/review-reproduction-build-debug.log"
& "$build/Debug/EngineSystemsTests.exe" InputDisabledContextsStillAgePriorPressHistory *> "$evidence/review-reproduction-disabled-history.log"
& "$build/Debug/EngineSystemsTests.exe" InputTimerSaturationDetectsLargestDeltaAfterSmallInterval *> "$evidence/review-reproduction-timer-residual.log"
foreach ($configuration in 'Debug','Release') {
    $logSuffix = $configuration.ToLowerInvariant()
    & $cmake --build $build --config $configuration --target EngineSystemsTests EngineSampleTests --parallel 2 *> "$evidence/review-fixed-build-$logSuffix.log"
    & $ctest --test-dir $build -C $configuration --output-on-failure -V -R '^Engine(Systems|Sample)Tests$' --no-tests=error *> "$evidence/review-fixed-tests-$logSuffix.log"
}
& "$build/Debug/EngineSampleTests.exe" HostExtremeFiniteAxisBindingsRemainDirectional *> "$evidence/review-fixed-host-reproduction.log"
$env:PATH = "$cmakeDir;" + $env:PATH
& $python Scripts/test_test_safety.py *> "$evidence/test-safety.log"
foreach ($milestone in 1,2,3) {
    & $python "Scripts/verify_milestone$milestone.py" *> "$evidence/milestone$milestone.log"
}
git diff --check
git diff --name-only
```

Every actual command exit was checked; failures stopped acceptance until the
material source/test correction. The snippets preserve invocation details;
they are not instructions to continue past a failure unchecked.

## Remaining scope and gates

Exactly seven authorized paths changed: InputSystem header/source, GameHost
input parsing plus the explicitly approved narrow ApplyProject check, both
test files, this record and the task packet. No UI, Assets, Math, CMake,
workflow, policy, dependency or game-content edit. No push, PR or merge by this
owner. Final independent re-review and coordinator receipts follow below.

The focused owner tests alone did not establish full-all builds; coordinator
results below add that evidence. Complete native input acceptance,
controller/analog support, focus/subframe/modifier
behavior, clipboard/Unicode/IME/DPI support, GPU/performance, packaging or soak.
The action API here is digital virtual-key contributions; recorded mouse
deltas do not constitute controller/analog action ingress. Those gaps remain.
The coordinator owns broader integration checks. Full engine workflow and
production readiness are not claimed by focused input tests or static markers.

Primary references: [C++ floating conversions](https://eel.is/c++draft/conv.double),
[MSVC hypot](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/hypot-hypotf-hypotl-hypot-hypotf-hypotl?view=msvc-170),
and [MSVC numeric limits](https://learn.microsoft.com/en-us/cpp/standard-library/numeric-limits-class?view=msvc-170).

## Coordinator matrix and final independent review

Frozen source commit `8ecc7d356120c0f70719e5a4d56b18f009cb469d`, clean worktree.
Independent reviewer `/root/numeric_implementation`, who did not implement this
packet, reviewed all seven allowed paths and the actual public API regressions.
Both timing findings were reproduced before the fixes and then re-reviewed:
unresolved actions now age once, and small elapsed time plus FLT_MAX is detected
without rejecting an exact FLT_MAX interval. Axis aggregation, JSON validation,
AddContext rejection/staged ApplyProject and ordinary priority/consumption,
rebind/recording behavior have no remaining actionable review findings. The
reviewer ran no commands/tests and edited no files.

Coordinator validation used the same external `work/build-finite-input` cache,
installed VS2022 CMake/CTest, MSVC19.44, two build jobs and the baseline machine.
Each exit was checked, native smokes ran sequentially, and no source changed
between focused review and these full runs:

| Command | Exit/result | External log under `<session>/work/` |
|---|---|---|
| `cmake --build work/build-finite-input --config Debug --parallel 2` | 0, all configured engine/editor/player/test targets | `finite-input-full-Debug-build.log` |
| `ctest --test-dir work/build-finite-input -C Debug --output-on-failure -E RuntimeSmoke --no-tests=error --stop-on-failure` | 0, 31/31, 56.79 s | `finite-input-full-Debug-test.log` |
| Same build/test commands with Release | 0, 31/31, 4.83 s | `finite-input-full-Release-{build,test}.log` |
| `ctest --test-dir work/build-finite-input -C Debug --output-on-failure -R RuntimeSmoke --no-tests=error --stop-on-failure` | 0, 6/6, 19.45 s | `finite-input-full-Debug-smoke.log` |
| Same smoke command with Release | 0, 6/6, 19.51 s | `finite-input-full-Release-smoke.log` |

This establishes the bounded finite digital-input repair on its unmerged
framework base. The GDI smokes do not establish native Win32 player/editor input
coverage; UI/modal ownership and native Unicode/IME/controller/DPI gaps remain.
No whole-engine performance or distribution/soak acceptance is claimed.
