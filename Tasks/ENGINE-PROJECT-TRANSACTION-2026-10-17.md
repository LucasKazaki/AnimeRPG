# Apply project settings and scene transactionally — October 17, 2026

## Current integration preparation — October18

Ownership transferred to `/root/demo_runtime`. Lucas's live demonstration
deadline is October18 noon Eastern (16:00Z), and desktop/GPU access was released.
The coordinator alone schedules every build/test/native/GPU slot.

The original source/proof worktree `work/astral-project-transaction` remains
untouched at base5245a3d, with source/test patchSHA256
`0824ff6f50811b77e451ce6245461c5d2ccacaf852d07201c8b240d0c68f8fed`.
This new owned preparation is `work/astral-project-demo`, branch
`engine/project-transaction-demo-20261018`, deliberate combined base
`3f6371c510a2bc3f7e969acded0ceff135bb153b` (reviewed CVar63d7230 and
Physicsba89029, with complete original fixture ancestry and base2c698887).

The same eight approved paths are adopted here. Production transaction source
is unchanged from the independently reviewed original; Console's batch methods
are added after the reviewed numeric ingress/formatter. The only manual source
conflict was the Services test footer: all25 CVar/base cases and all3 Project
batch cases are preserved, with finite out-of-destination Float/Int candidates
added to batch rejection controls. World production/tests are byte-identical to
the reviewed Physics branch. Original Sample fixtures and assertions remain.

Root independently reviewed the integrated source clean, then granted an
exclusive CPU slot. Configure returned0 (3.860s), two-job Debug/Release builds
returned0 (43.750/52.391s), and World31/Services28/Framework21/Sample25 cases all
passed (105 cases across4/4 suites per configuration;58.687/1.203s). Safety3/3
passed (5.359s); milestone1/2/3 each returned0. Source-before/after patchSHA256
`a288912bd0996920ec9fdb1e9c904f48561be6c66498e3ef697367da8576d37a`
matched. Evidence is in `work/project-demo-final-evidence`; the CPU slot was
released and no affected compiler/test process remained. Final independent
logs/docs/immutable commit binding is pending. No native/GPU or package result
is claimed. Separate stale Ready/Error project caching and
Host ConfigValue numeric formatting findings remain unmodified hypotheses;
this packet does not claim to repair those paths.

## Original packet and preserved OLD evidence

Owner `/root/candidate_review`; independent reviewer `/root`.
Dedicated worktree `work/astral-project-transaction`, branch
`engine/project-transaction-20261017`, exact base
`5245a3dd079083a534b4afd16924a6a5496d1a0d`.
Lucas's October 17 engine-only assignment authorizes this bounded repair;
technical, independent review and serialized execution gates remain required.

## Problem and preparation gates

The original `GameHost::ApplyProject` committed CVars, input and project metadata
before its startup scene is loaded. A failed scene leaves the old world running
with the new project's settings; a late config error leaves earlier CVars and
their captured callbacks applied. Both unchanged-production old filters failed
with actual child exit 3 before the coordinator cleared the fix. Never treat restoring values afterward as
undoing arbitrary callback effects.

Task/old tests preceded production. Both failures ran under an explicit slot
and were reported before the coordinator cleared production. Source remains
frozen during every scheduled build/test.

## Ownership and approved scope

Own `Engine/Framework/GameHost.{h,cpp}`, `Tests/EngineSampleTests.cpp`, this task
and `Docs/QA/ENGINE-PROJECT-TRANSACTION-2026-10-17.md`. Editor owner explicitly
transferred narrow Host ownership; its native/core editor paths remain frozen
to this owner. Preserve the standalone framework base; do not require adoption
of the unreviewed editor branch.

The coordinator approved extra paths `Engine/Core/Console.{h,cpp}` and
`Tests/EngineServicesTests.cpp`, solely for
callback-free CVar preparation and atomic batch application. No general undo,
command-execution transaction, new framework, CMake/policy/dependency, asset,
scene parser, UI, native-adapter, math/physics, audio or game-content change.

## Implemented candidate contract

1. Stage an InputSystem and copied ProjectDesc; validate all config names,
   permissions and typed/ranged values without invoking captured callbacks.
   Existing ConsoleRegistry::LoadConfig already uses callback-free variable
   probes; copying a whole registry or retaining callbacks in probes is unsafe.
2. For ApplyProject, load/parse its startup SceneDocument and resolve required
   dependencies before any live settings mutation. Keep existing C/D contained
   reads, failure diagnostics, strict image policy and scene activation behavior.
3. ValidateBatch and SetBatch share private callback-free preparation. SetBatch
   repeats validation before any write. Apply prepared CVar values as a batch, notify
   changed variables, install input/metadata and activate the staged scene.
   Do not introduce a normal validation-failure return after mutation starts.
   Preserve callback-driven renderer/stat settings. Arbitrary exceptions or
   reentrant registry-lifetime/host mutation inside user callbacks are outside this bounded
   validation-failure guarantee; no speculative rollback is promised.
4. Preserve the editor's new ApplyProjectSettings adapter signature (observed
   at editor baseline d5dc908): use the same atomic settings preparation and
   commit while leaving the loaded scene/world alone. This packet can provide
   the shared hook without requiring editor source adoption. Old regressions
   must use only APIs present at the unchanged base.
5. On input/config/missing-file/syntax/schema/dependency scene failure, retain
   prior live world/document/path, project metadata, input bindings and action
   state, all CVar values and callback effects. Active error/log diagnostics and
   asset-cache observations may update as existing failed loads require.
6. Success and recovery must use new settings, resolve valid scenes and notify
   actual changed CVars. Retain borrowed public service objects and registered
   callback identities; do not replace the ConsoleRegistry with a clone.

## Acceptance and scheduled commands

Original disposable scenes/projects only. Two baseline-compatible real-host
regressions: failed startup scene after valid settings; late invalid config
after an earlier valid setting. Require no mutation and zero callback
notifications, preserve current input action state, then verify valid recovery.
After production clearance, add direct settings-hook and CVar-batch tests for
success, unchanged values, duplicate aliases, types/ranges and permissions.

Use installed VS2022 x64 bundled CMake/CTest, two jobs and external
`work/build-project-transaction`. Retain exact argv/cwd, outputs, child exits,
machine/time and source patches in `work/project-transaction-evidence`.

```powershell
$projectCmakeBin = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin'
& "$projectCmakeBin/cmake.exe" -S . -B ../build-project-transaction -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON
& "$projectCmakeBin/cmake.exe" --build ../build-project-transaction --config Debug --target EngineSampleTests --parallel 2
& '../build-project-transaction/Debug/EngineSampleTests.exe' FailedProjectScenePreservesRunningProject
& '../build-project-transaction/Debug/EngineSampleTests.exe' FailedProjectConfigPreservesRunningProject
```

Fixed Debug/Release affected Sample/Services suites, any adjacent suites,
unchanged safety and milestone checks require explicit coordinator slots.
Independent source/docs/log/final-commit review precedes adoption. No push,
merge, full all-target build, native window, GPU, package or soak execution by
this packet unless separately scheduled. Audio/capture/hierarchy fixes are
independent packets, never silently added here.

## Current execution boundary

Source/tests are ready for read-only review; no fixed build or test has run.
Lucas explicitly requested that the desktop/GPU remain free, and the coordinator
paused all new local build/test slots for RAM headroom at 19:19 UTC. Continue
source-only preparation; fixed gates require release of the resource hold and
a new coordinator slot.
