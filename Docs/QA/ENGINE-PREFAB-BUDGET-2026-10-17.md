# Expanded prefab budget evidence

Owner `/root/numeric_implementation`; independent reviewer `/root`.
Worktree `work/astral-prefab-budget`, branch `engine/prefab-budget-20261017`,
reviewed C/D base `4552a31b1669ed34ebb23f05e502af6b425ce696`.
Scope/contract/commands: `Tasks/ENGINE-PREFAB-BUDGET-2026-10-17.md`.

Status: all three bounded old failures reproduced natively. Engine production
repair and seven added regression cases pass focused native Debug/Release and
safety/static checks. Independent source pre-review by `/root` was clean at the
tested patch; final commit/docs binding and broader integration remain pending.

Confirmed failures are repeated expanded copies bypassing maxEntities, insertion
depth bypassing maxDepth, and acyclic doubling bypassing the syntax-node count.
The original fixtures use only neutral in-memory entity hierarchies. The largest
old fixture has nine definitions, 511 output nodes and 1,013 cached nodes, rather
than attempting to exhaust memory. Accepted old parses instantiate the actual
GameWorld solely to print the real entity count before the expected failed check.

## Original native receipts

The coordinator cleared the old-only slot after its combined baseline builds
and 31 deterministic suites passed in both configurations. This owner did not
repeat those full baseline checks. With Engine byte-identical to reviewed
`4552a31b1669ed34ebb23f05e502af6b425ce696`, a fresh VS2022 x64 configure exited
0 (18:56:36–18:56:40 UTC), then Debug EngineFrameworkTests built with two jobs,
exit 0 (18:56:45–18:57:18 UTC). No full old Framework suite was run.

| Original filter | Actual accepted result | Exit |
|---|---|---|
| PrefabCopiesRespectExpandedEntityBudget | Actual GameWorld has 9 entities with maxEntities 7 | 3, expected |
| PrefabCopiesComposeInsertionDepth | Three-level runtime hierarchy, 3 entities, maxDepth 1 | 3, expected |
| AcyclicPrefabDoublingRespectsExpandedCacheBudget | Nine cached definitions and 511 runtime entities, maxEntities 64 | 3, expected |

All three ran sequentially at 18:57:26–18:57:27 UTC. The slot was released
immediately afterward to the coordinator; no production edits preceded these
receipts. Logs and individual JSON command/time/SHA/exit records remain under
`work/prefab-budget-evidence/old-*`. The expected failed checks were not weakened
or disabled. Existing test registration and assertions remain in use.

Actual commands, using the full paths/variables in the task packet:

```powershell
git diff --exit-code 4552a31b1669ed34ebb23f05e502af6b425ce696 -- Engine
& $cmake -S $source -B $build -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON
& $cmake --build $build --config Debug --target EngineFrameworkTests --parallel 2
& "$build/Debug/EngineFrameworkTests.exe" PrefabCopiesRespectExpandedEntityBudget
& "$build/Debug/EngineFrameworkTests.exe" PrefabCopiesComposeInsertionDepth
& "$build/Debug/EngineFrameworkTests.exe" AcyclicPrefabDoublingRespectsExpandedCacheBudget
git diff --check
```

## Repaired contract and source identity

Approved semantics use independent cached-definition and output-node domains,
each bounded by maxEntities, and expanded root-zero depth plus bounded active
prefab-resolution indirection. The two configured limits are never multiplied
or added to compute admission. Private subtree extents are propagated while
parsing and cached alongside expanded definitions. Copies are admitted before
copy assignment; an iterative model preflight admits selected nodes and generated
primitive children at the actual insertion depth before EntitySpec construction.
The original C/D Current/Diagnostic collectors and public Parse signatures remain.

Frozen tested production/tests patch: external `fixed-source.patch`, SHA256
`B427D16A0C011E901341CF450B3AFE8CE187EC0C7B44A78DE5C88B31FC310DF5`.
`running-source.patch` and `verified-source.patch` independently match that hash
before and after the fixed matrix. No source/test edit occurred during checks.
Seven cases cover the old three plus exact
independent output/cache boundaries, zero/one/SIZE_MAX limits, bounded active
alias resolution, real exported/imported model nodes and extra primitives,
model insertion depth, and failed nested file-prefab budgets retaining attempted
watches through Collect/change/recovery. Valid boundary documents instantiate
the real GameWorld with one, seven, four or five entities as applicable. The
metadata is private; no test-only accessor, replacement parser, dependency scan
or public method signature change was introduced.

## Fixed native validation

VS2022 Community x64, MSVC 19.44.35222.0 / tools 14.44.35207, SDK10.0.26100.0;
external multi-config `work/build-prefab-budget`. No installs. Root exclusively
scheduled the compiler/native slot; this owner used two compiler jobs and no GUI.

| Check | Actual result | External log |
|---|---|---|
| Debug build Framework/Sample | exit0, 19:04:17–19:04:25 UTC | fixed-build-Debug.log |
| Debug Framework | 21 cases pass,0.66s | fixed-test-Debug.log |
| Debug Sample | 20 cases pass,1.81s; 2/2 suites,2.48s total | same |
| Release build same targets | exit0,19:04:41–19:05:27 UTC | fixed-build-Release.log |
| Release Framework | 21 cases pass,0.10s | fixed-test-Release.log |
| Release Sample | 20 cases pass,0.27s; 2/2 suites,0.37s total | same |
| Existing test safety | 3/3,4.990s,exit0 | test-safety.log |
| Static milestones1/2/3 | exit0 each | milestone1.log / milestone2.log / milestone3.log |
| Whitespace/scope check | exit0; five allowed paths only | checked before commit |

Both registered suites ran in full, retaining existing C/D collectors, strict
image/reload, file/model/material, serialization and sample-consumer regressions.
The three original failures now reject their candidates without mutating the
supplied document. No post-fix build/test failure occurred. Existing safety
registration is unchanged; Release built AssertionsEnabled.cpp and retained the
expected /UNDEBUG override of /DNDEBUG for both targets. No assertion/test was
disabled or weakened.

Actual commands used the task packet's full source/build/CMake/CTest/Python paths:

```powershell
& $cmake --build $build --config Debug --target EngineFrameworkTests EngineSampleTests --parallel 2
& $ctest --test-dir $build -C Debug --output-on-failure -V -R '^Engine(Framework|Sample)Tests$' --no-tests=error --stop-on-failure
& $cmake --build $build --config Release --target EngineFrameworkTests EngineSampleTests --parallel 2
& $ctest --test-dir $build -C Release --output-on-failure -V -R '^Engine(Framework|Sample)Tests$' --no-tests=error --stop-on-failure
$env:PATH = "$cmakeDir;" + $env:PATH
& $python Scripts/test_test_safety.py
& $python Scripts/verify_milestone1.py
& $python Scripts/verify_milestone2.py
& $python Scripts/verify_milestone3.py
git diff --check
```

Individual JSON receipts record actual UTC start/end, command, exit, source base
and tested patch SHA. A normalized Windows path-prefix cleanup observation
(`verified-cleanup.receipt.json`) found zero process executable paths under the
owned build root after checks; no running tool session remained. It supersedes
the retained earlier receipt's unnormalized separator filter. The slot was
released at 19:05:55 UTC before documentation/commit work resumed. This is a
bounded owned-process observation, not an audit of every machine process.

## Independent review, scope and limits

Root's substantive source/test pre-review was clean at B427D16A: independent
domain admission, cached insertion depth, subtraction bounds, complete model
preflight including primitives, active alias guard, and transactional watches.
This owner did not review its own repair as an independent reviewer. Root's
final commit/docs/log binding remains a separate gate after this record commits.

Exactly five paths change: Scene.h/.cpp, EngineFrameworkTests.cpp and the
dedicated task/QA. No Assets/GameHost/UI/Math, game, policy, CMake, dependency,
native platform or other-owner edits. No push or merge by this owner. Source
branch remains a repaired candidate distinct from main.

Limits apply to parsing expanded EntitySpec nodes/depth, not JSON/string bytes,
asset memory, caller-modified documents, or later repeated runtime spawning.
Active unresolved alias recursion is bounded; already cached definitions need
no further resolution recursion. A caller selecting enormous limits still owns
its host memory/stack capacity choices. Full all-target combined checks, native
window/3D/2D/GPU, packaging, performance/soak and complete workflow acceptance
remain coordinator-owned and unclaimed by these focused results. External logs
remain in `work/prefab-budget-evidence`; no binaries or logs enter the source commit.
