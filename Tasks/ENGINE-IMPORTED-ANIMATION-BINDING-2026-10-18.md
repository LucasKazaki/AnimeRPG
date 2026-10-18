# Imported animation binding through authored scenes

Owner: demo_editor, coordinated and independently reviewed by root. Dedicated
worktree: `C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/astral-imported-animation-binding`.
Branch: `engine/imported-animation-binding-20261018`. Immutable base:
`8056ff6a6d04589bffbac84120365c271f2e9e85`.

## Current phase

The accepted implementation passed local Debug/Release CPU and actual native
checks and was committed as ad30857e6bc63a2c9472a88c262dd2590279b69a in PR107.
Hosted Windows head/merge checks passed, but all five portable jobs failed at
one test-only partial AnimState initializer under warnings-as-errors. A narrow
source-only repair now default-initializes that state and assigns the same name
and clip. All assertions, fixture bytes and production source remain unchanged.
The correction passed the separately granted local MSVC Framework gate in
Debug and Release: 31 cases, including ten imported cases and nine cleaned
fixture roots per configuration. Independent actual acceptance is recorded in
QA. Only the two Framework test EXEs changed; their old versions are preserved.
Production/native binaries and PR head remain unchanged. Fresh portable GCC/
Clang warnings-as-errors and sanitizer CI is still pending publication; no new
commit, push, merge or engine execution is authorized by this record.

The accepted concrete design is
`work/imported-animation-binding-evidence/OWNERSHIP-AND-SCHEMA-DESIGN.md`,
SHA256 b680dd21ebbea748d4dca3303938b3a8ef5e8ad18c4396cdfcebc910b7879b90.
The chronology below retains the safe OLD witnesses and intervening failures;
the matching QA gives final receipts and limits.

Two explicit refinements: meshBind must be affine with last row [0,0,0,1],
and automatic-first clip selection must support an unnamed first clip. Empty
stays automatic in flattened saves for every clip name, and has no identity
guarantee across clip reorder. An explicit validated name stays pinned.
Explicit named selections still require exactly one nonempty match. No durable
DCC identity is invented. Preserve original OLD fixture bytes/case bodies.

The following preparation paragraph is the historical OLD phase:

Task written before fixture/test changes. Only safe OLD fixture/test source and
an external bounded runner may be prepared now. Production must stay identical
to base. No configure/build/test/native/GPU execution until the coordinator has
reviewed exact source/fixture hashes and grants an exclusive execution slot.
No OLD failure or fixed acceptance is claimed yet. Preserve the working CPU
demonstration package and candidate source.

Source-only proposal:
`C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/editor-reopen-evidence/IMPORTED-ANIMATION-BINDING-TASK-PROPOSAL.md`, SHA256
`10b89434b3d93093ae13f5e69f1c75f40f1fd02a3d3841bb5c5176b23819c0a5`.

## Problem and bounded outcome

At the reproduced baseline, ExpandModel constructs static renderers without using imported
skins/animations. Reflected Animator fields only retain speed/root motion;
runtime skeleton/machine/bind mesh still require code installation. Reuse the
existing importer, animation runtime, serializer and generic editor inspector
to make a model-root Animator an explicit imported-animation opt-in.

Implemented controls: clip by unique name, speed and existing root-motion flag.
Empty clip selects the first animation. Explicit named selection must not
silently change when clips reorder or disappear. The controller drives every
skinned primitive with immutable owned rig/clip/machine data and independent
mutable runtime/output meshes. Code-installed Animators retain their behavior.

Generated flattened `Animator.binding` validates model, node,
skin, mesh, primitive and the actual renderer reference. This schema requires
fixture/source review, completed before implementation. It is distinct from the
authored model-root controller and must retain immutable resource owners.

Required correctness: remap reversed imported skin slots into parent-first
runtime joint indices; convert document-space skin matrices into each source
mesh node's space exactly once; preserve wrapper transforms; isolate multiple
primitives, nodes and instances; keep clips alive across document destruction,
asset collection, save/reopen, Undo/Redo and repeated Play/Stop/restart.

Reject unsupported imported root-motion application, deforming mesh collision,
static LOD on skinned renderers and non-joint animation channels requiring a
different transform pipeline. Preserve static model-only scenes and the
existing code-installed Animator contract.

Model-only editor reimport must retain dirty authored JSON, saved bytes,
selection, camera, both history stacks and a complete last-good candidate
across model syntax or external-bin failures. Acknowledge each attempt once,
retain failed dependencies and recover when only the broken dependency is
repaired. Include untitled authoring; make no unnamed-Play hot-reload claim.

## Allowed paths (exactly eleven)

1. `Engine/Framework/Components.h`
2. `Engine/Framework/Components.cpp`
3. `Engine/Framework/Scene.h`
4. `Engine/Framework/Scene.cpp`
5. `Engine/Framework/GameWorld.cpp`
6. `Engine/Editor/SceneEditor.h`
7. `Engine/Editor/SceneEditor.cpp`
8. `Tests/EngineFrameworkTests.cpp`
9. `Tests/EngineEditorTests.cpp`
10. `Tasks/ENGINE-IMPORTED-ANIMATION-BINDING-2026-10-18.md`
11. `Docs/QA/ENGINE-IMPORTED-ANIMATION-BINDING-2026-10-18.md`

Host, Tools, CMake, D3D, graphics renderer, importer, Animation library and Game
files are excluded. Do not modify production during the OLD phase. Discovery
of a BuildSkeleton/BuildClip defect stops this packet for independent review;
do not weaken a fixture to conceal it.

## OLD witness and independent controls

Prepare an original two-joint, two-primitive fixture with reversed skin joint
slots, a static prefix, nonidentity mesh-node transforms and two mesh-node
instances. Scene JSON has two model instances with different wrapper positions
and speeds. It uses only existing accepted model + Animator.speed fields.
An external `.bin` holds geometry, influences, explicit inverse binds and two
named joint clips. All data is finite and bounded; no game/downloaded content.

The actual scene test must assert successful current import, source structure
and renderers before safely requiring an initialized generated Animator. Its
first OLD failure must occur at that reviewed presence assertion, before any
missing pointer dereference. Later fixed assertions compare independent
numeric vertex positions, primitive/instance runtime ownership and bind-data
immutability, after destroying the parsed document and collecting assets.

A separate passing control must prove import/BuildSkeleton/BuildClip and the
fixture's numeric pose/deformation without new production binding code. Its
explicit slot swap and literal mesh inverse are checked against hard-coded
world/local coordinates. The runner retains exact original fixture bytes and
all OLD evidence; successful controls remove only their own ledgered files.

## Historical OLD execution plan

External runner/evidence parent:
`C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/imported-animation-binding-evidence`.
Fresh OLD cache: `C:/AI/astral-build/imported-animation-old-first`.
Only Debug EngineFrameworkTests is planned for the initial OLD slot. The runner
must bind the base, full physical source, allowed test/doc scope and immutable
fixture proof, then use the reviewed owned-process helper, fresh >=8 GiB before
configure/build, max two build jobs and no MSBuild node reuse. Every child has
exact argv/cwd/PID/log/actual exit/reaped/timeout receipts. Stop unexpected
results immediately, retain original failures and source-before/after equality.
No native/GPU launch is part of the OLD runner.

After actual OLD evidence and a separately authorized correction, require
relevant Debug/Release Framework/Editor/Assets/Animation, Sample/Layout and
safety/M1–3 checks. Complete meaningful native original-fixture observation,
save/reopen, Play/Stop and captures under a separate exclusive slot before
claiming acceptance. Source design, compilation and hosted tests alone do not
prove the native authoring workflow.

Actual OLD cases: `ImportedAnimationFixtureConvertsWithIndependentCoordinates`
(conversion control, expected 0) followed by
`ImportedModelSceneBindsEveryPrimitiveAndOwnsPlayback` (expected OLD abort 3
at the first `bound != nullptr` assertion). Both actual results matched in the
granted OLD slot. Receipts and retained original bytes are under
`work/imported-animation-binding-evidence/old-first`; QA records the evidence.

The original OLD cases/payloads remain unchanged through final acceptance.
The full eleven-path source and variant/literal expectations were independently
reviewed before each separately granted execution step. Final results follow
the preserved normal and selector findings below.

## Historical normal-basis witness and correction

The coordinator and independent reviewer accepted source packet aad7b479 /
manifest bd88880c, receipt `independent-source-review-aad7.json` SHA256
`1af95f98b6b7abc9f22af57ed784d9fc9beb743467d9dd39d04510884e69c2eb`.
Before execution, the coordinator identified an imported normal-basis defect:
SkinMesh uses a vector transform for inverse(meshBind) * document skin matrices,
while the renderer later uses inverse-transpose(entityWorld). Nonuniform
meshBind therefore leaks into normal direction. The separately prepared safe
OLD-normal numeric witness has now reproduced it, as recorded below and in QA.

The authorized additive witnesses used the original model with BodyOne scale
[2,3,4] and unit non-axis engine normal (2,3,6)/7; position coordinates must
remain unchanged, world normal must be (-3,2,6)/7, local direction (4,9,24).
The OLD-normal filtered child is expected to reject at that world-normal
numeric assertion before any unsafe access. Direct helper controls separately
cover identity, two-influence normal blending and singular-animation fallback.
The existing OLD2 cases/payloads and archived source freeze remain unchanged.

The exact numeric OLD-normal rejection was observed in the third bounded
attempt (configure/build0, test3 at line1750), independently accepted by the
coordinator. This is a partial-implementation regression, separate from the
original 8056 missing-binding OLD. Earlier runner-only setup failures are
retained without claiming an engine test ran in either.

The authorized imported helper correction in Components.cpp uses
local normal B^T * sum(weight * inverseTranspose(P) * sourceNormal). Renderer
(W B)^-T cancels B. Singular/nonfinite inverses use a document-space identity
normal fallback before B^T; require finite/nonzero output. Preserve Animation
library, legacy behavior and all positions. All tests/fixture recipes stayed
unchanged while the corrected packet and bounded runner received independent
review before execution. The final numeric regressions passed in both builds.

## Actual unnamed-selector finding and correction

The first corrected matrix configured and built all seven Debug targets with
exit 0. Layout, Animation and Assets suites passed; Framework stopped at the
unchanged unnamed-auto assertion line1630, before Editor/Sample, Release or
safety checks. The importer supplies a generated animation0 display name for
an omitted source name, and the initial binding implementation incorrectly
pinned that display name during flattening. The actual fixture and all seven
Debug binaries are preserved before any rebuild.

Coordinator authorized a bounded Scene.cpp correction: retain `requested`
directly after explicit unique-name validation. Empty is always automatic
first, independent of real/generated display names; explicit selection stays
pinned. No importer/provenance field or prefix guessing. All tests are unchanged.
The reviewed retry used new fixed-second evidence and the existing owned
fixed-first cache only after verifying its exact
cache/options/owner/source/base, all seven baseline/preserved binaries and prior
failed receipt. It incrementally built the same seven targets in Debug/Release,
then passed six suites, 17 imported cases/18 roots per configuration and
safety/M1–3. No configure was repeated. Earlier failure evidence is retained.

## Final acceptance and bounded delivery

The fixed-second run completed all 40 supervised children with actual exit 0,
reaped and no timeout: 137 engine cases plus Layout per configuration, all
17 imported-animation cases and 18 cleaned private fixture roots per build.
Fourteen final binaries were rehashed, the prior seven Debug archives stayed
unchanged, and the full 283 build-input map was identical before/after. Root's
independent CPU acceptance is SHA256
`bda6844c845b47a796e96e9cc8a4c960763f38ddb839306d6a76c1ed99097bcf`.

The first actual native attempt exposed back-facing single-sided verification
triangles, not accepted visible animation. A reviewed independent projection
proof justified a new camera-only fixture at [1,4,7], yaw 180; original geometry,
materials, bin and reference scene stayed exact. Actual Debug then passed real
Assets import, supported temporary front-preview rotation followed by Undo,
visible Raise/Bend motion, clip/speed Undo/Redo, Save As cancel/save, reopen and
three Play/Stop cycles. Actual Release passed saved-scene front preview/Undo,
visible Bend motion, Stop restoration and normal close. Both exited 0/reaped
without timeout. The final saved scene hash and original files were unchanged
after Release; fresh process verification found no relevant processes.

Combined external acceptance:
`work/imported-animation-binding-evidence/native-front-combined-acceptance.json`,
SHA256 `601cb6da634f4be379326e60669d3213004daf78eeac8c40938e0fee82392190`.
Only these Task/QA documents changed after the compiled and native-tested freeze;
production and test bytes remain identical to that accepted source.

This is CPU-renderer support with original unlit verification geometry, not GPU
animation or broad DCC parity. The private editor camera remains independent of
the authored runtime camera; single-sided backs can show bounds only. Empty clip
retains automatic-first semantics across save and has no durable identity after
clip reorder. Controller association is captured at instantiation: runtime
reparenting is not a live controller-rebinding API. Direct C++ clip-string
mutation also does not rebind. Unsupported imported root motion, deforming
collision, static skinned LOD and non-joint selected channels reject. The sealed
8056 CPU demonstration package remains untouched and separate from this packet.

## Actual initializer-repair local acceptance

Root reported outer execution session 84818 closed with exit 0. All 57 individual
owned children exited0, were reaped and had no timeout. The exact reviewed
six-command plan built only Framework Debug/Release with max 2/no node reuse,
then ran one anchored 31-case suite per configuration, including all ten imported
cases and nine cleaned roots. No configure, production/native rebuild, source
change or hosted acceptance was claimed. All 621 tracked source hashes, the
283 build inputs and eleven-file scope matched the compiled repair freeze;
cache/owner, twelve other accepted EXEs, 711 prior evidence files and eleven
native Content files remained exact. The final inventory had blockingCount 0
and only its own direct conhost marked currentQueryConsole/nonblocking.

Independent evidence-only acceptance:
`work/imported-animation-binding-evidence/ci-repair-framework-independent-actual-acceptance.json`,
SHA256 `595c494351fe7536ad225dcdf7d6a85231f0fbcb78506ab6f1401821bc5d0e1e`.
This proves local MSVC semantics of the unchanged default-state initialization;
portable compiler warnings and sanitizer CI still require the new exact head.
Only Task/QA are reconciled after this local pass. Every production/test byte
and prior proof stays frozen. The current PR remains unmerged.
