# Release removed/replaced behaviour components

Owner: weekend coordinator `/root`, branch `engine/behaviour-lifecycle-20261017`,
base `92e4880489d2478f2fa45fbb690fabd8caca63dd`. Independent reviewer:
`/root/numeric_implementation`. Lucas's October17 engine-only assignment applies.

## Problem and evidence

Source review found `GameWorld::Add/Remove<Behaviours>` dispatch to a no-op
generic Release. Removing the component skips OnDestroy, leaves its entity-owned
timers active, and allows retained frame snapshots to update removed behaviours.
Unlike direct Registry mutation, these public GameWorld methods explicitly
promise runtime-object release. Reproduce using external shared counters only;
do not deliberately dereference freed memory.

## Allowed paths, approach and dependencies

Own `Engine/Framework/{GameWorld,Behaviour}.{h,cpp}`, `Tests/EngineWorldTests.cpp`,
this packet and matching `Docs/QA/ENGINE-BEHAVIOUR-LIFECYCLE-2026-10-17.md`.
No UI, Assets, Scene/GameHost, game, CMake, policy or workflow edits. Keep separate
from B and C/D owners. Add public-API removal, replacement and same-frame snapshot
regressions before production changes. Release lifecycle once, cancel the removed
behaviours' timers, and suppress callbacks/timer scheduling after destruction.
Handle same-entity teardown reentrancy without retaining invalid component refs;
independent review must check callback ordering and replacement semantics.
During behaviour teardown, same-entity behaviour attachment is rejected (typed
reference-return APIs throw invalid_argument, named API returns an error).
Removal/destruction is idempotent; callbacks may still attach on other entities.
This rule avoids creating and orphaning replacement behaviours during an outer
component removal/replacement. A generation-scoped guard ends after teardown.

## Acceptance, risks and exact commands

Old regressions must fail on unchanged production source. Fixed tests plus
existing World/Framework/System/Sample suites pass Debug and Release with active
checks, followed by full relevant native matrix and safety registration checks.
No new executable/test registration is needed. Portable sanitizer CI supplements
local Windows tests. Raw Registry bypass and exception-throwing callbacks remain
outside this bounded public GameWorld repair.

Use VS2022 x64, two jobs, external `<session>/work/build-behaviour-lifecycle`:

```powershell
& $cmake -S <session>/work/astral-lifecycle -B <session>/work/build-behaviour-lifecycle -G 'Visual Studio 17 2022' -A x64
& $cmake --build <session>/work/build-behaviour-lifecycle --config Debug --target EngineWorldTests --parallel 2
& <session>/work/build-behaviour-lifecycle/Debug/EngineWorldTests.exe RemovingBehaviourComponentReleasesLifecycleAndTimers
& <session>/work/build-behaviour-lifecycle/Debug/EngineWorldTests.exe ReplacingBehaviourComponentReleasesLifecycleAndTimers
& <session>/work/build-behaviour-lifecycle/Debug/EngineWorldTests.exe RemovedBehavioursCannotRunFromAnEarlierFrameSnapshot
& $ctest --test-dir <session>/work/build-behaviour-lifecycle -C Debug --output-on-failure -R '^Engine(World|Framework|Systems|Sample)Tests$' --no-tests=error
```

Repeat affected build/tests in Release. Check each exit, retain all old/fixed
logs in `work/behaviour-lifecycle-evidence`, freeze source before review, and
integrate only reviewed code. Full development workflow remains unaccepted.
