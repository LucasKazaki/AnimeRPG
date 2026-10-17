# Engine numeric validation repair — October 17, 2026

## Problem and evidence

Owned branch `engine/numeric-repair-20261017`, worktree
`C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/astral-numeric`,
base `c644eec79752b9dc101aeca6b03141cf26854c96`.
Lucas's October 17 pasted engine assignment is the current authority; it
supersedes older local-only and per-change approval restrictions within its
engine scope while preserving review, test and native acceptance gates.

The coordinator cleared source edits after the exact baseline passed the full
native Debug build and 25/25 deterministic CTest suites. Before implementation,
a disposable external harness must reproduce these actual-source failures:

- Member and free `Vec3` length overflow for `{1e20f,1e20f,0}`; normalization
  loses the finite direction.
- Perspective projection accepts NaN and publishes nonfinite coordinates.
- An idle controller updated with NaN time corrupts its position.
- Finite parent/child translations overflow during recursive world resolution.
  The recursive implementation also lacks cycle protection and uses borrowed
  raw parent pointers.

Inspecting merge-base `2958741188279a0b438dd489ad00cac546012c25` through PR #76 tip
`fa76e04f0cf3c88471bd266677589e39e26f8b01` includes three commits. Adapt its useful
numeric contracts, not its combat changes, workflow, isolated test framework or
stale source assumptions. Main already defines by-value `IsFinite(Vec3)`;
preserve one signature rather than adding an ambiguous const-reference overload.

## Allowed paths and dependencies

- `Engine/Math/{Math.h,VectorMath.h}`
- `Engine/Scene/{Camera,PlayerController,Transform}.{h,cpp}`
- `Tests/{MathTests,EngineMathTests,SceneTests}.cpp`
- This task and `Docs/QA/ENGINE-NUMERIC-2026-10-17.md`
- `Scripts/verify_milestone2.py` only if its obsolete recursion-presence marker
  fails after the correct iterative cycle repair; behavioral regressions stay
  in the existing registered test suites.

One implementation owner edits this worktree. The coordinator owns full builds,
the external `work/build-numeric` build root, interactive desktop/runtime checks
and integration. An independent reviewer must review the resulting diff/commit.
No game content, policy changes, dependencies, R0 invocation or parallel native
tests belong to this packet. The implementation owner may commit, not push/merge.

## Approach and contracts

1. Use C++17 `std::hypot` for member/free Vec3 length. Normalize with a double
   norm so even a finite vector whose norm exceeds float range retains its
   direction. Keep the existing `kEpsilon` cutoff: zero/near-zero directions
   still use the caller's fallback; nonfinite input also uses that fallback.
   `LengthSquared` remains the ordinary float dot product and may overflow.
2. Consolidate by-value `IsFinite(Vec3)` in `Math.h`, available to both math
   headers and Scene clients.
3. Checked camera projection validates inputs and computes in double. Failure
   leaves output unchanged. Keep finite offscreen coordinates valid up to the
   actual GDI consumer's signed 32-bit raster range, with rounding/HUD offset
   headroom. Add checked orthographic projection; its existing wrapper returns
   `{0,0}` on failure. Invalid follow operations preserve the last good camera.
   `Depth` returns NaN when its float result is invalid/unrepresentable.
4. Reject invalid speed/bounds at controller construction with
   `std::invalid_argument`; accept zero speed and finite degenerate bounds.
   Invalid time/position updates do nothing. Compute midpoint and finite
   displacement in double before clamping to finite bounds.
5. Add checked translation-only world-position resolution. Detect cycles without
   allocation, traverse iteratively, sum in double, reject nonfinite data or an
   unrepresentable final float result, and leave output unchanged on failure.
   The compatibility wrapper returns NaNs on failure. Parents remain borrowed
   pointers and must stay alive throughout traversal; this packet cannot
   validate dangling references or concurrent hierarchy mutation.

Primary references retrieved October 17:
[C++17 draft 29.9.3](https://timsong-cpp.github.io/cppwp/n4659/c.math.hypot3),
[Microsoft `<cmath>`](https://learn.microsoft.com/en-us/cpp/standard-library/cmath?view=msvc-170),
[Microsoft `isfinite`](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/finite-finitef?view=msvc-170).
The draft and Microsoft document the three-argument hypot overload; finite
classification includes subnormal/zero values and excludes NaN/infinity.
Firecrawl retrieval was unavailable due to exhausted connector credits; the
built-in web tool retrieved these primary documentation pages.

## Acceptance and commands

- Retain the external harness command/output showing each old failure, then its
  repaired result. No cyclic baseline traversal is executed in-process.
- Existing `AstralMathTests`, `EngineMathTests`, and `AstralSceneTests` exercise
  ordinary behavior, huge/tiny norms, nonfinite inputs, unchanged failed outputs,
  valid large movement, hierarchy cancellation/overflow, cycles and a deep chain.
- Tests remain registered through root `astral_add_test` so Release assertions
  stay active; portable harness checks also remain independent of `NDEBUG`.
- Coordinator runs Debug/Release full builds and deterministic suites from the
  root project, then serializes applicable native smokes. Independent review
  and integrated-main verification remain separate gates.

Resolved full-build commands (coordinator owner, external build directory):

```text
cmake -S C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/astral-numeric -B C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/build-numeric -G "Visual Studio 17 2022" -A x64
cmake --build C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/build-numeric --config Debug --parallel 4
ctest --test-dir C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/build-numeric -C Debug --output-on-failure -E RuntimeSmoke --no-tests=error
cmake --build C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/build-numeric --config Release --parallel 4
ctest --test-dir C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/build-numeric -C Release --output-on-failure -E RuntimeSmoke --no-tests=error
python Scripts/test_test_safety.py
python Scripts/verify_milestone1.py
python Scripts/verify_milestone2.py
python Scripts/verify_milestone3.py
git diff --check
git diff --name-only c644eec79752b9dc101aeca6b03141cf26854c96
```

Use the installed Visual Studio CMake and bundled Python executables; the QA
record retains exact invocations, compiler version, results and logs. Stop at
the first deterministic blocker, preserve its evidence and repair only within
this packet's scope. Native smoke, sanitizer availability, performance and the
24-hour soak must be reported separately and never inferred from these tests.

## Risks and bounded stop

Invalid constructor inputs now throw instead of constructing corrupt state;
finite valid caller behavior remains supported. Checked APIs permit callers to
distinguish failure; old wrappers retain documented sentinels. Hierarchy lookup
is stack safe but remains O(depth) and does not establish scene ownership.
The raster bound follows the existing Win32 consumer, not viewport clipping.
This packet ends at a scoped commit ready for independent review and coordinator
verification; it does not establish whole-engine workflow or soak acceptance.
