# Reject failed cached projects and preserve project numeric boundaries

Owner `/root/demo_runtime`; independent reviewer and sole execution scheduler
`/root`. Worktree `work/astral-project-ingress`, branch
`engine/project-ingress-20261018`, deliberate base
`1804a3cba77dace7eb04e7f14c052e796adb5128` (reviewed project transactions above
complete numeric and physics branches). Deadline: October18 noon Eastern.

## Problem and bounded ownership

Source inspection finds two remaining project ingress hypotheses:

- AssetManager intentionally retains Ready last-good JSON after a failed reload,
  with nonempty Error and a new Attempt. GameHost::LoadProject checks only Ready,
  unlike LoadScene/ApplyProject, so it may apply stale data after syntax/deletion.
- ConfigValue formats JSON numbers with nine significant digits. Exact finite
  Float extrema can round into out-of-Float-range text before guarded CVars see
  them, rejecting a valid project. Core::WriteJson already emits numeric shortest
  round-trip text and is a candidate reuse after actual reproduction.

Exactly four paths: `Engine/Framework/GameHost.cpp`,
`Tests/EngineSampleTests.cpp`, this Task and the matching
`Docs/QA/ENGINE-PROJECT-INGRESS-2026-10-18.md`. Do not change Assets, Console,
input, scene parsing, physics, audio, editor, renderer, game, CMake, policies or
dependencies. Preserve independently reviewed project transaction semantics,
numeric clamp/rounding/underflow, callback identity and last-good asset retention.

No production edit before accepted actual OLD receipts. The coordinator approved
the plans, an isolated worktree, and an OLD-only CPU slot after package review:
fresh available RAM at least8GiB, two jobs, Debug Sample target only, no GUI/GPU.
Stop/report on unexpected outcomes; release the slot after all original filters.

The approved OLD slot has now completed: configure0 (3.703s), two-job Debug
Sample build0 (32.609s), four intended failure filters each actual child exit3,
and all three controls0. The syntax/deletion cases each observed Ready last-good
Version1 with Attempt2/nonempty Error but accepted stale Candidate application.
Both Float extrema were rejected before a matching getter. Production remained
unchanged1804a3c; exact fixture/source bindings and logs are in the QA record.
The CPU slot was explicitly released with no affected process remaining.
Root read and accepted the actual receipts, then cleared the bounded correction:
LoadProject rejects a nonempty current Error while retaining the underlying
last-good asset; ConfigValue reuses Core::WriteJson for numeric shortest-round-
trip text. All seven fixtures remain, with extra preserved Project metadata
assertions after the safe failed-load predicate. No numeric bound, cache policy,
Assets or Console code changes. After independent source review, the first fixed
Debug attempt exposed a real unchanged parser regression at Sample line204:
WriteJson's default pretty mode appended a newline to numeric config text.
Services28/Framework21 passed, Sample stopped, and no Release/safety gates ran.
Root read the retained actual failure, then cleared only compact
JsonWriteOptions{false} for numeric ConfigValue. All original assertions and
seven fixtures remain. The corrected source is frozen for review and a new
separate fixed attempt; failed logs remain untouched.

Root independently reviewed that compact correction and granted the next
exclusive CPU slot. The corrected attempt completed0 in its own
`work/project-ingress-corrected-evidence` directory. Debug and Release each
passed all81 cases (Sample32, Services28, Framework21), including the unchanged
numeric spelling assertion and all seven ingress fixtures. The safety harness
passed3/3 and milestone1/2/3 each returned0. Source binding remained unchanged
before/after. The slot was released with no affected compiler/test process
remaining. Final Task/QA and the four-path diff await coordinator review before
any commit or candidate adoption.

## Safe original-consumer checks

Use fresh uniquely reserved temporary roots without clearing existing paths.
Only remove the exact canonical directory created by the fixture. Real public
GameHost, AssetManager and JSON APIs are required; no mocks or game assets.

1. Run an Original project with held input and observable CVar callbacks. Cache
   another valid project through AssetManager, then corrupt or delete its file
   and synchronously Reload. Assert Ready, nonempty Error, unchanged good Version
   and increased Attempt before calling LoadProject. Separate syntax/deletion
   filters should expose an incorrect successful stale application. After a fix,
   require the original world, document, project, input state, CVars and callback
   count to survive, followed by valid recovery. A separate valid-cache/recovery
   control establishes that current good data still loads.
2. Write actual project JSON through Core::WriteJson with positive and negative
   exact FLT_MAX, register the matching Float setting, then LoadProject against a
   neutral valid scene. Separate filters assert load success BEFORE a matching
   Float getter. On OLD formatting they should fail safely at load rejection.
   Positive controls cover ordinary values, subnormals/underflow and useful range
   clamps; genuinely out-of-range values must still reject without mutation.

Exact OLD filters: `ProjectLoadRejectsCachedSyntaxFailure`,
`ProjectLoadRejectsCachedDeletion`, `ProjectPositiveFloatBoundaryLoads`,
`ProjectNegativeFloatBoundaryLoads` (each expected child exit3), and controls
`ProjectCurrentCachedDataAndRecoveryLoad`,
`ProjectOrdinaryFloatUnderflowAndClampingLoad`,
`ProjectOutOfRangeFloatRejectsWithoutMutation` (expected0).

Retain source/fixture patch, exact argv/cwd, installed-tool/cache identity,
stdout/stderr, actual child exits and times in `work/project-ingress-evidence`.
Neither preparation, OLD failures nor this focused fixed matrix establishes
candidate adoption, a package, native/GPU behavior or demo acceptance. The
actual failures, bounded correction and scheduled Debug/Release gates are
recorded separately so their limits remain explicit.
