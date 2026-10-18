# Parented physics synchronization evidence — October17, 2026

## Current local status — October18, 03:51Z onward

Owner transferred to `/root/demo_runtime`. Lucas released desktop/GPU access;
the coordinator still schedules every compiler/test/native/GPU slot. Root ran
the three original filters from assembled `afd4a21`, verified Engine bytes
equal2c698887 and original World fixture hash15dd0d17f403d56952446daee81e55fb4bf8575580ebbc4c13419d7d6a41aed6.
Configure0 (5.0s), Debug Services/World build0 with2jobs (34.344s). Actual child
exits: child-created-first3, unbodied-bridge3, parent-created-first0. Exact
commands/cwd/time and stdout/stderr are in `work/cpu-repro-evidence/receipts.json`
and its per-filter files. These are two observed failures, not observations of
every later assertion in each aborting filter.

After root accepted these receipts, the correction traverses root subtrees
parent-first and refreshes every entity before its dynamic body pull, ensuring
non-bodied ancestors represent this step. It reuses existing pose conversion,
retains world scale, body handles and kinematic bookkeeping, and does not edit
the solver. Original tests are unchanged. One new case covers both creation
orders with rotated bodies/bridge, public off-center impulse angular motion,
body/cache/matrix agreement, stable velocities/handles/scale and descendants.
Four parented cases are now verified. Root independently reviewed the source
clean, then granted a serialized CPU slot after the final CVar matrix. Fresh
available RAM was12.928GiB. Configure returned0 (3.813s), two-job Debug/Release
World/Framework/Sample builds returned0 (43.390/56.812s). Affected suites passed
3/3 in83.641/1.141s: World31, Framework21, Sample20, totaling72 cases per config.
Both creation orders and the non-bodied bridge match body/cache positions
through fixed steps1,2,5; the nonidentity/angular case also passed in both configs.
Safety3/3 passed (5.297s), and milestone1/2/3 each returned0. Exact commands,
times/exits, separate outputs and unchanged before/after source binding are in
`work/parented-physics-final-evidence`. Source/test patchSHA256:
`579f631e829c9eb85808a5dd9aeacc95e4fb8f1a5530ffdf7ff2fd71a7eb7213`.

The CPU slot was explicitly released after the wrapper completed0; read-only
process inventory found no cl/cmake/ctest or affected test process. Final
independent logs/docs/immutable commit binding is pending. No native GUI, GPU,
performance, package, broad workflow or demo acceptance is claimed. The
historical preparation below is superseded where it says no OLD execution,
no production edit or no fixed execution.

## Historical fixture preparation

Status: **source hypothesis and safe regression preparation only**. No new
production edit, configure, build, test, GUI or GPU execution has occurred.
The user's desktop/GPU hold and coordinator RAM/compiler hold remain active.
Actual failures, fixes, independent acceptance and final commit are pending.

Owner `/root/containment_implementation`; worktree `work/astral-parented-physics`,
branch `engine/parented-physics-20261017`, deliberate assembled base
`2c69888774c074a69ec12c5f659e3632280db420`. Allowed five files, proposed contract,
stop conditions and exact unexecuted commands are in the matching task packet.
Coordinator `/root` is the independent reviewer and sole compiler scheduler.

## Source facts and test intent

`FixedStep` updates transforms, pushes physics, steps the solver, pulls physics
and dispatches contacts. `SyncFromPhysics` currently pulls dynamic Colliders in
sorted entity order. `WriteWorldPose` reads the cached parent WorldTransform to
derive a local pose, then refreshes only that entity. Later Tick phases perform
a full transform propagation. These source facts suggest order-dependent
parent motion may be applied twice; they do not establish an observed failure.

Prepare three separate public-API filters with original neutral tiny boxes,
gravity0, damping0, sleep disabled and noncolliding world positions. Parent
velocity1 and child velocity2 remain independent physics world velocities.
SetParent must preserve the child's initial world pose. Child-created-first
is the primary suspected failure; parent-created-first is the direct positive
control. The third fixture includes a non-bodied intermediate between the
simulated parent and child, whose cached pose may otherwise remain stale.

Each checks real body/cached-world and matrix agreement, unchanged handles,
body velocities and expected independent world motion over repeated fixed
steps. No raw registry mutations, random data, game content, audio playback,
visible native application, external assets or expensive benchmark is needed.
The original lifecycle, audio and RigidBody regression source remains intact.

No filter has executed yet. No expected failure is reported as an actual one.
Retain actual old receipts before any production correction, then report and
release the granted slot. The final fixed matrix, safety/milestone checks and
independent review will be recorded with exact SHAs, commands, times and limits.

The coordinator independently pre-reviewed the three frozen source fixtures
and authorized a separate **hosted-only** old reproduction assembly while the
local RAM and human desktop/GPU holds persist. Hosted execution is pending;
local reproduction remains pending. This source-only WIP commit contains only
the three-path fixture/task/QA packet, not a physics correction or acceptance.
GameWorld.h/.cpp are unchanged. Test SHA256:
`15dd0d17f403d56952446daee81e55fb4bf8575580ebbc4c13419d7d6a41aed6`.
