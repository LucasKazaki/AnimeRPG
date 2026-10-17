# Synchronize public rigid-body component changes

Owner: sole rigid-body synchronization implementation agent; compiler/native
scheduling and integration belong to weekend coordinator `/root`. Worktree
`work/astral-rigidbody-sync`, branch `engine/rigidbody-sync-20261017`, deliberate
base `a537a7f3d761a609529c9eb7f7f633f191d6cdc3` (reviewed behaviour lifecycle
candidate, distinct from main). Independent reviewer: numeric owner. Lucas's
October 17 engine-only weekend authority supersedes older local-only/per-change
approval language; technical/native/review gates remain. Checked local time:
October 17, 2026, 14:30:49 -04:00, inside the authorized window.

## Evidence and scope

`GameWorld::EnsureBody` returns when the Collider already has a valid physics
body. `RigidBody` uses the generic no-op Release path, so first addition,
replacement and removal never invalidate a previously created Collider body.
Reproduce through public GameWorld APIs and real physics, using neutral primitive
fixtures before production edits. Do not infer native results from this source
hypothesis. The reviewed lifecycle head already received coordinator-owned
affected Debug/Release and safety checks; new regression results remain pending.

Allowed files: `Engine/Framework/GameWorld.{h,cpp}`,
`Tests/EngineWorldTests.cpp`, this packet, and
`Docs/QA/ENGINE-RIGIDBODY-SYNC-2026-10-17.md`. No Behaviour, UI, Host, Scene,
Assets, game content, policy, CMake or dependency changes. Preserve existing
behaviour teardown, body, mover and resource behavior. No new dependency.

## Approved bounded contract

After a successful public `GameWorld::Add<RigidBody>` (first addition or
replacement), release only that entity's Collider runtime body and clear its
failed-attempt latch. An actual `Remove<RigidBody>` does the same; removal when
the component is absent is a no-op and preserves a valid existing body.
The next executed fixed physics synchronization recreates the body from the
current transform and collider plus the current RigidBody, or Static defaults
when absent. Tick with no fixed step does not create it. Invalid descriptions
remain rejected by the existing physics creation rules; a later public change
may retry. Removing the Collider still releases its body.

Replacement supplies type, mass, damping, gravity scale, rotation lock and
initial velocity. Retain the existing dynamic-mesh-to-kinematic restriction.
Unchanged bodies retain handles, simulation velocities and pending forces.
Other entities, CharacterMover controllers/bodies and resources are untouched.
Intentional reconfiguration may replace body handles and clear contact caches;
contact/handle continuity is not promised. Direct Registry structural changes
and direct edits through `Get` are outside this packet, as are engine-wide
property editing, collider-rescale tracking and physics solver redesign.

Implementation should invalidate after successful component insertion, preserve
existing lifecycle-specialized Add/Remove paths, and use the existing Collider
Release helper rather than duplicating physics ownership logic.

## Acceptance and stop conditions

1. Retain separate old native failures for first Add after Static creation,
   Remove after Dynamic creation, and replacement with different mass/config.
2. Cover delayed fixed-step recreation, add-before-collider ordering, no-op
   removal, failure/recovery, mesh restriction, stale handles/entity mapping,
   unaffected body forces/velocities and CharacterMover identity, repeated
   changes and entity/Collider cleanup through public APIs.
3. Native Debug/Release existing World suite plus affected Framework suites
   pass with always-active assertions. Run test safety and static milestones.
   Broad/native desktop gates remain coordinator-owned; do not claim them.
4. Stop on an unexpected deterministic failure, preserve logs and debug the
   material cause. No repeated unchanged failing command, weakened checks,
   CMake edits, full-all build, GUI, push or merge by this owner.
5. Commit scoped source/tests/QA, freeze, and request independent review.

Every compiler/native invocation requires the coordinator's explicit slot.
External cache: `work/build-rigidbody-sync`; evidence:
`work/rigidbody-sync-evidence`, beneath the shared projectless workspace.
Use installed VS2022 CMake/CTest and bundled Python; no installs.

```powershell
& $cmake -S $worktree -B $build -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON
& $cmake --build $build --config Debug --target EngineWorldTests --parallel 2
& "$build/Debug/EngineWorldTests.exe" AddingRigidBodyRebuildsExistingStaticCollider
& "$build/Debug/EngineWorldTests.exe" RemovingRigidBodyRestoresStaticCollider
& "$build/Debug/EngineWorldTests.exe" ReplacingRigidBodyAppliesNewPhysicsDescription
# After old failures and approved production edits:
& $cmake --build $build --config Debug --target EngineWorldTests EngineFrameworkTests --parallel 2
& $ctest --test-dir $build -C Debug --output-on-failure -V -R '^(EngineWorldTests|EngineFrameworkTests)$' --no-tests=error
& $cmake --build $build --config Release --target EngineWorldTests EngineFrameworkTests --parallel 2
& $ctest --test-dir $build -C Release --output-on-failure -V -R '^(EngineWorldTests|EngineFrameworkTests)$' --no-tests=error
& $python Scripts/test_test_safety.py
& $python Scripts/verify_milestone1.py
& $python Scripts/verify_milestone2.py
& $python Scripts/verify_milestone3.py
git diff --check
git diff --name-only
```

## Focused outcome

Fresh old-source configure/build succeeded, then the three original fixtures
each exited 3 before production edits: Add stayed Static, Remove stayed Dynamic,
and replacement retained the old inverse mass. The repaired full native focused
Debug and Release runs each pass World 21 cases plus Framework 12 cases; safety
3/3 and static milestones 1/2/3 pass. Existing registration/assertions remain
active. No unexpected post-fix failure occurred. See the matching QA record for
exact commands, times and retained logs. Source pre-review was reported clean;
final independent commit/evidence binding and coordinator integration remain
separate gates. No full-all build, GUI, push or merge by this owner.
