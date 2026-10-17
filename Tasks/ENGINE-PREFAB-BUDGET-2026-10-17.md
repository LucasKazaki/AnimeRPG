# Bound expanded scene and prefab storage

Owner: `/root/numeric_implementation`; independent reviewer and compiler/native
scheduler: `/root`. Worktree `work/astral-prefab-budget`, branch
`engine/prefab-budget-20261017`, deliberate reviewed C/D base
`4552a31b1669ed34ebb23f05e502af6b425ce696`. Lucas's October 17 engine-only
weekend instruction applies and supersedes older local-only/per-change approval
language within that scope. Local time checked October 17, 14:50 EDT.

## Problem, evidence and ownership

Source review found that ParseEntity charges syntax nodes before copying an
already expanded prefab. Three copies of a root with two children can exceed
maxEntities while parsing succeeds. Cached prefab roots reset depth to zero, so
copying a subtree into a deeper parent bypasses maxDepth. A small acyclic chain
of doubling prefab definitions can allocate many expanded nodes before the
syntax-node counter notices. Model expansion also needs to count its generated
primitive child entities and compose its depth with its insertion point.

All three bounded native old-source regressions reproduced assertion exit 3 on
unchanged production, with 9 entities, three hierarchy levels, and 511 entities
accepted respectively. Configure/build exited 0. The old doubling fixture has
only nine definitions and at most 511 output nodes; it deliberately avoids a
resource-exhaustion reproduction. See QA and external old-* receipts.

Own only `Engine/Framework/Scene.h`, `Engine/Framework/Scene.cpp`,
`Tests/EngineFrameworkTests.cpp`, this packet and
`Docs/QA/ENGINE-PREFAB-BUDGET-2026-10-17.md`. C/D explicitly transferred Scene
ownership after its freeze/review. Preserve frozen C/D branches and every other
worktree. No Assets, GameHost, UI, Math, game content, CMake, policy or dependency
edits. Root owns full integration/native checks and push/PR/merge actions.

## Approved counting and admission contract

- maxEntities independently bounds the sum of fully expanded cached definition
  nodes and the sum of fully expanded scene-output nodes. Each stored definition
  is charged once; each instance is charged in full in its own domain. Do not
  add or multiply the two limits in size_t arithmetic. Unused inline definitions
  remain validated and count toward the cache domain.
- maxDepth uses root depth zero and the final expanded hierarchy. Cached
  definitions validate at zero, and each copy must fit at its insertion depth.
  Models and generated extra-primitive children participate in both metrics.
  The same maxDepth separately bounds active prefab-resolution indirection, with the
  first definition at resolution depth zero, preventing unbounded alias recursion.
- Admit whole cached-subtree copies and generated model expansions before
  materializing them. Compare counts/heights by subtraction after validating
  current values; never increment/add first and test an already wrapped value.
  Zero/one/exact boundaries and SIZE_MAX arithmetic remain meaningful.
- Preserve existing overrides, appended children/behaviours, inline/file/nested
  prefabs, models, assets and dependency collectors. Failed parses retain the
  caller's SceneDocument while returning reached attempted watches and useful
  path diagnostics. Do not redesign serialization or add public test-only APIs.

This bounds EntitySpec node/depth expansion, not JSON/string bytes, asset memory,
arbitrary caller-modified SceneDocuments, or the number of later runtime spawns.
It is not a promise that every caller-selected enormous limit fits host memory.

## Approach, acceptance and stop condition

Prepare the task and three old regressions with Engine unchanged. Obtain the
coordinator's focused compiler slot, retain old failures, and stop/report before
production changes. After clearance, use private per-parse budget state and
cached subtree extents to check copies; preflight model node/primitive expansion
before generation. Preserve C/D dependency and strict-reload state propagation.

After repair, cover legal exact/zero/one limits, repeated instances, cache-domain
limits, depth composition, bounded doubling, alias resolution, model primitives
and insertion depth, file/nested prefabs, transactionality and attempted watches.
Run existing Framework plus affected Sample suites in native Debug and Release
with assertions active, test safety and static milestones. Root independently
reviews the actual final diff/commit before integration. Stop at an unexpected
deterministic failure, retain its logs, and diagnose before rerunning a changed
case. No full-all builds, GUI, push or merge by this implementation owner.

## Exact command plan

All compiler/native commands require the coordinator's explicit slot. External
cache `work/build-prefab-budget`; logs `work/prefab-budget-evidence`, beneath the
shared session directory. Use installed VS2022 x64 CMake/CTest, two compiler jobs,
and the existing bundled Python executable; no installs or historical R0.

```powershell
$source = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/astral-prefab-budget'
$build = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/build-prefab-budget'
$evidence = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/prefab-budget-evidence'
$cmakeDir = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin'
$cmake = "$cmakeDir/cmake.exe"
$ctest = "$cmakeDir/ctest.exe"
$python = 'C:/Users/lucas/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
& $cmake -S $source -B $build -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON
& $cmake --build $build --config Debug --target EngineFrameworkTests --parallel 2
& "$build/Debug/EngineFrameworkTests.exe" PrefabCopiesRespectExpandedEntityBudget
& "$build/Debug/EngineFrameworkTests.exe" PrefabCopiesComposeInsertionDepth
& "$build/Debug/EngineFrameworkTests.exe" AcyclicPrefabDoublingRespectsExpandedCacheBudget
# Stop/report old results before any production repair.
# After separate production clearance:
& $cmake --build $build --config Debug --target EngineFrameworkTests EngineSampleTests --parallel 2
& $ctest --test-dir $build -C Debug --output-on-failure -V -R '^Engine(Framework|Sample)Tests$' --no-tests=error --stop-on-failure
& $cmake --build $build --config Release --target EngineFrameworkTests EngineSampleTests --parallel 2
& $ctest --test-dir $build -C Release --output-on-failure -V -R '^Engine(Framework|Sample)Tests$' --no-tests=error --stop-on-failure
$env:PATH = "$cmakeDir;" + $env:PATH
& $python "$source/Scripts/test_test_safety.py"
& $python "$source/Scripts/verify_milestone1.py"
& $python "$source/Scripts/verify_milestone2.py"
& $python "$source/Scripts/verify_milestone3.py"
git diff --check
git diff --name-only
```

Primary references read October 17: the C++ draft's
[unsigned arithmetic rules](https://eel.is/c++draft/basic.fundamental) explain
why size_t addition can wrap; [vector construction](https://eel.is/c++draft/vector.cons)
supports checking before recursive vector copying/materialization. The engine
remains C++17; no newer language features are required. These references support
the implementation approach, not execution or workflow acceptance claims.

## Focused outcome

All three bounded old regressions failed exit3 before production changes. The
approved repair and seven added cases pass native Framework21+Sample20 cases in
both configurations (2/2 suites; Debug2.48s, Release0.37s), safety3/3 and static
M1-3. Root's source pre-review is clean at tested patch B427D16A; final committed
source/docs binding and full combined/native integration remain separate gates.
See QA for exact logs, times, source identity and retained limitations.
