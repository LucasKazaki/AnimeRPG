# Parented physics synchronization evidence — October17, 2026

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
