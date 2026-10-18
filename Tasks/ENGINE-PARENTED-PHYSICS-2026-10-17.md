# Synchronize parented physics poses in hierarchy order

## Current authorized status — October18, 03:51Z onward

Ownership transferred to `/root/demo_runtime`. Lucas released the desktop/GPU
hold and set the live demonstration deadline to October18 noon Eastern (16:00Z).
Root accepted actual local OLD receipts from assembled `afd4a21`: child-created-
first and unbodied-bridge filters each aborted with child exit3; parent-created-
first returned0. Configure0 (5.0s), two-job Debug Services/World build0 (34.344s),
original Engine bytes2c698887 and original World fixture file hash15dd0d17f403d56952446daee81e55fb4bf8575580ebbc4c13419d7d6a41aed6 verified.
Receipts/outputs are in `work/cpu-repro-evidence`. Root then authorized the
bounded production correction: collect sorted-root subtrees parent-first,
refresh every node (including unbodied bridges), and pull dynamic bodies using
existing pose conversion. Kinematic bookkeeping and solver/body state stay
unchanged. Original translation assertions remain unchanged; a new regression
uses nonidentity rotations, public off-center impulses, stable body handles,
unchanged velocities/world scale and body/cache/matrix checks across both
creation orders and repeated steps. Root independently reviewed this source
clean, and final Debug/Release World31, Framework21, Sample20 cases passed:
72 cases across3/3 suites per configuration (83.641/1.141s). Two-job builds
returned0 (43.390/56.812s), safety3/3 passed (5.297s), M1-3 returned0. Source
patchSHA256 remained
`579f631e829c9eb85808a5dd9aeacc95e4fb8f1a5530ffdf7ff2fd71a7eb7213`.
Evidence: `work/parented-physics-final-evidence`. Final independent logs/docs/
commit binding remains pending. The CPU slot was explicitly released after
completion; no affected compiler/test process remained. Root schedules all slots.

The original preparation/contract below is retained as historical context;
its hold, hypothesis-only and pending OLD execution statuses are superseded.

## Original preparation and agreed scope

Owner: `/root/containment_implementation`, sole implementation owner in
`work/astral-parented-physics`, branch `engine/parented-physics-20261017`.
Deliberate base: `2c69888774c074a69ec12c5f659e3632280db420`, the coordinator's
assembled candidate including independently reviewed audio, lifecycle and
RigidBody synchronization repairs. It is distinct from main. Independent
reviewer and compiler/native scheduler: coordinator `/root`.
Local time checked October17, 2026, 15:21-04:00, inside Lucas's engine-only
weekend authority. Current user instruction keeps the desktop/GPU free. All
new local builds/tests are also paused by the coordinator for RAM headroom.
Source preparation and independent review may proceed; no production edit or
execution before an explicit old-regression slot and accepted failure receipts.
After independent source pre-review, the coordinator authorized a separate
hosted-only old reproduction assembly. Its execution is pending, local execution
is still held, and this fixture-only WIP commit contains no production change.

## Evidence and allowed scope

Source hypothesis: `SyncFromPhysics` iterates Colliders in entity order, while
`WriteWorldPose` converts a physics world pose into local space using the cached
parent WorldTransform and refreshes only the current entity. A child created
before its dynamic parent can therefore use the parent's previous-step pose.
The later full hierarchy update may apply the parent's motion a second time.
A non-bodied intermediate transform can remain stale even when its simulated
ancestor was processed first. These are source findings, not executed results.

Exactly five allowed paths:

- `Engine/Framework/GameWorld.h`, `Engine/Framework/GameWorld.cpp`
- `Tests/EngineWorldTests.cpp`
- This task and `Docs/QA/ENGINE-PARENTED-PHYSICS-2026-10-17.md`

Preserve reviewed Audio, behaviour lifecycle, component replacement, Collider
invalidation and CharacterMover/resource behavior. No Behaviour, AudioMixer,
Host, Scene, Assets, UI, Math, renderer, game, CMake, policy or dependency edits.
No project creation or framework replacement. No dependencies or installations.

## Proposed bounded synchronization contract — awaiting agreement

After each executed fixed physics step, every dynamic Collider's cached world
position/rotation must match its Physics body pose, independently of entity
creation order. Derive its local pose from the current-step world pose of its
parent, including non-bodied intermediate ancestors. Hierarchy propagation
must leave unbodied descendants consistent with their local transforms and
must not feed extra ancestor motion back as a teleport on the next fixed step.
Keep world scale, physics body handles/type/mass/velocities/forces, static and
kinematic targets, contacts, mover ownership and fixed-step timing unchanged.

The likely bounded approach is a deterministic parent-before-child pull with
intermediate cache refresh, reusing the existing hierarchy and pose conversion
helpers. Do not repair by changing the solver, removing parenting, flattening
local transforms or forcing every body to world-root space. Public SetParent
keeps its existing worldPositionStays behavior. Direct Registry corruption,
arbitrary direct Get edits, scale/shear redesign and engine-wide eager transform
refresh are outside this packet. Implementation follows only after coordinator
agreement and actual old native failures.

## Reproduction and acceptance

Use only public GameWorld/physics APIs and neutral tiny boxes far apart, zero
gravity, sleeping disabled and damping0. Give the parent velocity[1,0,0] and
child velocity[2,0,0], SetParent with world pose retained, then execute a fixed
Tick. Compare actual body pose to cached world pose/matrix, not just an expected
component value. Keep separate filters for child-created-first failure,
parent-created-first control, and an unbodied intermediate. Initial native
results remain unexecuted until a granted slot. Stop on unexpected failure and
retain every actual stdout/stderr/exit and exact source/test binding.

After accepted old failures and a reviewed implementation, focused native
Debug/Release World and Framework suites must pass with always-active checks,
including existing lifecycle/audio/RigidBody cases. Retain repeated fixed-step
and multi-step/no-fixed-step behavior, body identity/velocity and hierarchy
controls. Run test-safety and milestone checks under an explicit slot. A source
review alone is not local runtime evidence; broad desktop/GPU/soak/R0 acceptance
is not claimed. No push/merge by this owner; commit scoped work after gates and
independent review, with exact source/docs freeze.

## Exact planned commands — not yet executed

External cache: `work/build-parented-physics`; evidence:
`work/parented-physics-evidence`. Use installed VS2022 CMake/CTest and bundled
Python, bounded parallel2. Coordinator may explicitly select a compatible cache
instead; record its actual path and source binding if so.

```powershell
$worktree = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/astral-parented-physics'
$build = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/build-parented-physics'
$cmakeDir = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin'
$python = 'C:/Users/lucas/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
& "$cmakeDir/cmake.exe" -S $worktree -B $build -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON
& "$cmakeDir/cmake.exe" --build $build --config Debug --target EngineWorldTests --parallel 2
& "$build/Debug/EngineWorldTests.exe" 'ParentedDynamicChildCreatedBeforeParentMatchesBodyPose'
& "$build/Debug/EngineWorldTests.exe" 'ParentedDynamicChildCreatedAfterParentMatchesBodyPose'
& "$build/Debug/EngineWorldTests.exe" 'ParentedDynamicChildThroughUnbodiedAncestorMatchesBodyPose'
# Stop/report old receipts before any production fix; later fixed-slot commands:
& "$cmakeDir/cmake.exe" --build $build --config Debug --target EngineWorldTests EngineFrameworkTests --parallel 2
& "$cmakeDir/ctest.exe" --test-dir $build -C Debug -V --output-on-failure --stop-on-failure -R '^(EngineWorldTests|EngineFrameworkTests)$' --no-tests=error
& "$cmakeDir/cmake.exe" --build $build --config Release --target EngineWorldTests EngineFrameworkTests --parallel 2
& "$cmakeDir/ctest.exe" --test-dir $build -C Release -V --output-on-failure --stop-on-failure -R '^(EngineWorldTests|EngineFrameworkTests)$' --no-tests=error
& $python Scripts/test_test_safety.py
& $python Scripts/verify_milestone1.py
& $python Scripts/verify_milestone2.py
& $python Scripts/verify_milestone3.py
git diff --check
git diff --name-only
```
