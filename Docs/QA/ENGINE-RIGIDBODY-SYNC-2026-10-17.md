# Rigid-body synchronization evidence — October 17, 2026

Status: three old public-API failures retained; bounded repair passes native
Debug/Release focused suites and safety/static checks. Independent source
pre-review was reported clean by the coordinator; final commit/evidence binding
and broader integration remain pending.
Worktree: `work/astral-rigidbody-sync`, branch
`engine/rigidbody-sync-20261017`, deliberate reviewed lifecycle base
`a537a7f3d761a609529c9eb7f7f633f191d6cdc3`. Scope and exact planned commands:
`Tasks/ENGINE-RIGIDBODY-SYNC-2026-10-17.md`. Logs will be external under
`work/rigidbody-sync-evidence`. Coordinator serializes builds and native tests.

The contract applies only to public GameWorld Add/Remove of RigidBody. Direct
Registry/Get edits, contact identity continuity on reconfiguration, native
desktop/GPU/performance/soak and broader workflow acceptance are not established.

## Old-source native failures

VS2022 Community x64, MSVC 19.44.35222.0, tools 14.44.35207, CMake
3.31.6-msvc6. Fresh external cache `work/build-rigidbody-sync`; unchanged
`astral_add_test(EngineWorldTests)` and `AssertionsEnabled.cpp` registration.
Production remained byte-for-byte at the reviewed base while building these
tests. Configure and focused Debug build both exited **0**. Ran only the three
individually filtered reproductions; did not run the full old World suite.

| Filter | Actual old behavior | Exit |
|---|---|---|
| `AddingRigidBodyRebuildsExistingStaticCollider` | type Static, inverse mass 0 after Dynamic Add | **3**, expected |
| `RemovingRigidBodyRestoresStaticCollider` | type Dynamic, inverse mass 0.5 after Remove | **3**, expected |
| `ReplacingRigidBodyAppliesNewPhysicsDescription` | inverse mass remains 0.5 instead of 0.125 | **3**, expected |

Coordinator-owned slot start `2026-10-17T14:33:41.1957934-04:00`; configure and
build completed by `14:34:20.1221020-04:00`; three failures complete and slot
released at `14:34:28.3141637-04:00`. Logs preserved as `old-configure.log`,
`old-build-debug.log`, and `old-<full-filter-name>.log` beneath the evidence
directory. Every command exit was checked before dependent work continued.

```powershell
$worktree = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/astral-rigidbody-sync'
$build = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/build-rigidbody-sync'
$evidence = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/rigidbody-sync-evidence'
$cmakeDir = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin'
$cmake = "$cmakeDir/cmake.exe"
$ctest = "$cmakeDir/ctest.exe"
$python = 'C:/Users/lucas/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
New-Item -ItemType Directory -Path $evidence -Force | Out-Null
& $cmake -S $worktree -B $build -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON *> "$evidence/old-configure.log"
& $cmake --build $build --config Debug --target EngineWorldTests --parallel 2 *> "$evidence/old-build-debug.log"
$tests = @('AddingRigidBodyRebuildsExistingStaticCollider','RemovingRigidBodyRestoresStaticCollider','ReplacingRigidBodyAppliesNewPhysicsDescription')
foreach ($testName in $tests) {
    & "$build/Debug/EngineWorldTests.exe" $testName *> "$evidence/old-$testName.log"
}
```

## Bounded implementation

Only public Add/Remove branches for RigidBody change. Successful insertion
precedes invalidation; actual removal invalidates and absent removal does not.
A private helper reuses the existing Collider Release path, then clears its
failed-creation latch. Existing fixed synchronization supplies current pose,
collider fields and the new RigidBody configuration. Physics creation rules,
mesh restriction, solver, lifecycle teardown and other component release paths
are unchanged. Tests inspect actual PhysicsWorld body fields, movement, counts,
stale-handle rejection and entity mapping; no replacement physics model or
test-only API was introduced.

## Post-fix native verification

Both focused builds exited **0**, then both full two-suite runs exited **0**:

| Configuration/check | Result | Retained log |
|---|---|---|
| Debug World | **21/21 cases**, 61.47 seconds | `fixed-test-Debug.log` |
| Debug Framework | **12/12 cases**, 0.65 seconds; total 62.13 seconds | same |
| Release World | **21/21 cases**, 0.69 seconds | `fixed-test-Release.log` |
| Release Framework | **12/12 cases**, 0.08 seconds; total 0.79 seconds | same |
| Release assertion safety | `/UNDEBUG` overrides `/DNDEBUG`; `AssertionsEnabled.cpp` built for both targets | `fixed-build-Release.log` |
| Test safety | **3/3**, 5.052 seconds, exit 0 | `test-safety.log` |
| Static milestones 1/2/3 | **0/0/0** exits | `milestone1.log` / `milestone2.log` / `milestone3.log` |
| Diff whitespace/scope check | exit **0**; exactly five allowed paths | checked before commit |

The original three fixtures now report Dynamic/inverse mass 0.25 after Add,
Static/inverse mass 0 after Remove, and Dynamic/inverse mass 0.125 after
replacement, in both configurations. No post-fix build/test failure occurred.
No test was disabled or weakened. The seven new cases additionally verify:

- Replacement fields and initial dynamic velocity, actual movement, stale body
  rejection and entity mapping, without accumulating bodies.
- Add before Collider, fixed-step-only recreation, current pose, kinematic
  replacement, absent Remove preserving its handle, and Collider cleanup.
- Failed mass creation followed by successful replacement or removal; invalid
  shape creation still fails closed.
- Unrelated pending force/velocity and body handle, CharacterMover body and
  controller identity, collider shape/material/filter/trigger fields, repeated
  changes and entity cleanup.
- Shared triangle mesh identity and existing Dynamic-to-Kinematic restriction.

The existing 14 World cases include the reviewed behaviour removal/replacement,
reentrant teardown and pending/snapshot lifecycle regressions. The existing 12
Framework cases include dynamic/contact synchronization, character walking,
jumping/pushing, scene serialization and failure handling, presentation resource
following and deterministic worlds. All pass unchanged.

Post-fix slot began `2026-10-17T14:45:22.4253719-04:00`. Debug suites completed
at `14:46:41.0184689-04:00`; Release build at `14:47:31.6733800-04:00` and suites
at `14:47:37.4256050-04:00`. Safety/static checks finished and slot released at
`14:47:50.4072239-04:00`. No competing builds/native tests or GUI were run by
this owner. Logs remain external; old failure evidence was not overwritten.

```powershell
& $cmake --build $build --config Debug --target EngineWorldTests EngineFrameworkTests --parallel 2 *> "$evidence/fixed-build-Debug.log"
& $ctest --test-dir $build -C Debug --output-on-failure -V -R '^(EngineWorldTests|EngineFrameworkTests)$' --no-tests=error *> "$evidence/fixed-test-Debug.log"
& $cmake --build $build --config Release --target EngineWorldTests EngineFrameworkTests --parallel 2 *> "$evidence/fixed-build-Release.log"
& $ctest --test-dir $build -C Release --output-on-failure -V -R '^(EngineWorldTests|EngineFrameworkTests)$' --no-tests=error *> "$evidence/fixed-test-Release.log"
$env:PATH = "$cmakeDir;" + $env:PATH
& $python Scripts/test_test_safety.py *> "$evidence/test-safety.log"
foreach ($milestone in 1,2,3) {
    & $python "Scripts/verify_milestone$milestone.py" *> "$evidence/milestone$milestone.log"
}
git diff --check
git diff --name-only
```

## Scope and limits

Exactly five paths change: `Engine/Framework/GameWorld.h`,
`Engine/Framework/GameWorld.cpp`, `Tests/EngineWorldTests.cpp`,
`Tasks/ENGINE-RIGIDBODY-SYNC-2026-10-17.md`, and this record. No Behaviour,
UI/Host, Scene/Assets, game content, CMake, policy, dependency or install changes.
No push/merge by this owner. Physics reconfiguration intentionally releases the
old body/contact cache; callback or body-handle continuity is not promised.
Unmodified bodies are preserved. Direct Registry/Get mutations, collider scale
editing, new property-editor behavior, and solver changes remain out of scope.

This is focused native API evidence on a reviewed branch, not main acceptance.
Full all-target integration builds, broad deterministic matrix, players/editor,
native interactive 3D/2D, GPU/performance/soak, packaging and complete workflow
acceptance remain coordinator-owned/unverified by this packet.
