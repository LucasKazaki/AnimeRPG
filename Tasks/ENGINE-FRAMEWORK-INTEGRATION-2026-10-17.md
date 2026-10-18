# Repaired framework integration candidate, October 17

Owner: weekend coordinator `/root`. Original frozen assembly branch
`engine/framework-integration-20261017`; subsequent bounded combination branch
`engine/framework-stabilized-20261017`. Start: reviewed containment candidate
`8fb54ee7a15a31b4237bb19a75747316037dd3d3`, based on unmerged PR80 efac9db.
Lucas's October17 engine-only assignment authorizes bounded integration and
conflict repair after review/testing. This is a local candidate assembly, not
main adoption or acceptance of remaining PR80 defects.

## Problem and approach

Numeric repair is reviewed, native/hosted verified and merged on current main
126e6cf. Containment is reviewed and native/hosted verified in PR82. Finite input
is independently reviewed and completing its full matrix. Combine exact reviewed
increments without modifying original worker branches. Then give explicit UI
ownership and reload packets separate worktrees and file reservations from the
same assembled base, preserving all existing engine/runtime/sample behavior.

## Allowed paths and ownership

Merge existing reviewed commits only. Resolve conflicts solely in paths already
changed by those commits: Math/Scene numeric contracts, their existing tests and
M2 verifier, asset/Gltf containment, InputSystem and narrow GameHost parsing,
associated task/QA/capability records. No game-content or policy edits. Preserve
both newer contracts and all distinct regression cases. The coordinator owns
assembly and conflict repair; implementation packets own new fixes.

Subsequent stabilization also permits exact reviewed UI Widgets/ConsoleOverlay/
GameHost ownership commits, GameWorld/Behaviour lifecycle commits, and floating
Reflection/Services tests, each within its dedicated packet's reviewed paths.
Do not alter the frozen92e488 original assembly branch: it is the stable base
for scoped drafts85/86/87 and their actual merge-checkout receipts. New source
fixes remain with their separate owners and independent reviewer.

## Gates, commands and limits

Keep exact base/head/tree receipts. Independent review must inspect resulting
conflict resolutions and assembled diff before eventual main adoption. Native
full Debug/Release builds and deterministic suites, test safety, milestone
checks, serialized smokes and real host/player checks must pass on the assembled
source. Source identities do not inherit verification from unrelated branches.
Use external `work/build-framework-integration`, VS2022 x64, two jobs, command
shapes in the weekend coordination packet. GPU/editor/package/soak are separate.

Current PR80 UI ownership, dependent reload and diagnostic defects remain open;
do not push/merge this candidate as ready or claim full development workflow.
No original PR80/52 branch, dirty local-test files, or other scheduler is changed.

## Reviewed combination checkpoint, October17 afternoon

New worktree `<session>/work/astral-stabilized`, branch
`engine/framework-stabilized-20261017`, starts at frozen92e4880. Exact reviewed
commits cherry-picked without conflicts:

- B74eed63/797782e ->7570f41/8cb98b4; final independent review clean; PR86
  has seven passing hosted checks and actual synthetic-merge checkout verified.
- Lifecycle63fe8eb/a537a7f ->15996b4/acc887f; final review clean, affected D/R
  suites pass, PR85 has seven passing hosted checks and actual checkout verified.
- Reflectioncad986d/353dcd5 ->edc59e3/2e3f4b1; final review clean, affected
  native D/R suites pass; scoped PR87 hosted checks are pending.

Combined source head2e3f4b1c08f670eb333f1658cf7ce9f92a42598a has not yet been
built/tested as a whole. This assembly does not inherit complete acceptance.
C/D reload source (based on reviewed B) and Rigidbody sync (based on reviewed
lifecycle) remain in their owners' worktrees pending review/gates; adopt only
their scoped final commits. Prefab expansion limits and sparse integer glTF
accessors remain queued source hypotheses. Whole-framework source review is
incomplete until these findings are reproduced/resolved or explicitly bounded.

Before main adoption refresh main and run new full native all-target builds,
all deterministic suites, serialized smokes, safety and real player checks on
the exact combined head/merge. Preserve existing CPU-rendered sample only for
verification. Editor/GPU/external packaging/immutable24-hour soak remain open.
