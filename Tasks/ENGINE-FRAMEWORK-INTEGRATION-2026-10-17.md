# Repaired framework integration candidate, October 17

Owner: weekend coordinator `/root`, dedicated branch
`engine/framework-integration-20261017`. Start: reviewed containment candidate
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
