# RA-001: repository audit and repair receipt

Date: September 25, 2026. Author verification, not independent acceptance.
Repository: LucasKazaki/AnimeRPG. Main/source baseline:
`2958741188279a0b438dd489ad00cac546012c25`.
Branch: `repair/repo-audit-20260925`.
Task: [REPO-AUDIT-2026-09-25](../../Tasks/REPO-AUDIT-2026-09-25.md).

## Coverage and limits

This is a repository-wide risk triage with deep source repairs in selected shared
subsystems. It is NOT a claim that every file, historical branch, dependency or
native behavior has been inspected and certified. Direct git clone failed DNS;
the sandbox uses a dedicated source-slice worktree reconstructed from connector
reads and verified against upstream Git blob hashes. No Windows host was accessed.

| Area | Actual inspection in this pass | Remaining coverage |
|---|---|---|
| Root/direction | Root tree, current ref, AGENTS, README and prior exact control content; active PR #67 ownership checked. | Full history and all active workers not independently certified. |
| Math/movement/camera/hierarchy | Full source, baseline reproductions, repairs, 45 named new tests and unchanged legacy scene tests. | Native renderer/GUI and all cross-module regressions. |
| CombatSandbox | Full header/implementation; invalid-enum, clock, recovery and DPS repairs. | Every other combat/mission/progression class is not line-by-line audited. |
| Asset loading | Current StaticMesh source reviewed and unchanged legacy scene test executed. | All asset-validator cases, binary packs, glTF/DCC/native rendering and art approval. |
| Core/platform/entry point | SimulationTimeStep and Game/Main source read; prior renderer/editor excerpts and capability evidence considered. | Win32Application, every profiler path, live input, native teardown and stress. |
| Build/CI | Existing test support copied exactly; new standalone CMake and explicit head/merge workflow checked. PR #52 head and failed run rechecked. | Existing full root regression and new hosted jobs still need actual receipts. |
| Research/content | Current README contradictions corrected; eight primary technical sources checked; prior art/voice/free-tool claims treated as proposals. | Exact release/model pins, all citations, acoustic/art quality and every third-party license. |

No separate UE5 repository was changed. Existing damage and voice branches are not
silently merged. Main does not gain their runtime features through this repair.
No hardware, dependency, model, API credit, billing, scheduler or release change.

## Confirmed defects and repairs

All defects below are API-boundary or extreme/malformed-input cases unless a
normal example is stated. They are not claims of a remotely exploitable game.

| ID | Defect in inspected source | Repair and tested boundary |
|---|---|---|
| RA-01 | Vec3 length squared in float, returning infinity for representable large norms and zero for tiny nonzero norms. | Three-argument hypot; normal/large/tiny checks plus 5,000 fixed-seed trials. |
| RA-02 | Nonfinite time/positions could corrupt movement; reversed bounds violate clamp's precondition; finite midpoint/displacement intermediates overflow. | Validate constructor, reject invalid updates/position atomically, use double intermediates and finite bounds. 5,000 movement trials. |
| RA-03 | Camera reports successful NaN projection; invalid FOV/near/dimensions and huge raster coordinates escape checks. | Checked projections and follow, finite parameters, raster bounds and unchanged output on failure; valid offscreen points retained. |
| RA-04 | Parent traversal recursively follows cycles/deep chains. | Checked iterative position calculation with cycle detection; self/multi-node cycles and 50,000-node valid chain tested. |
| RA-05 | Invalid AttackType falls through to heavy attack; unknown affinity triggers reactions. | Reject invalid mutation and return zero invalid attack definition. Valid attacks/reactions retained. |
| RA-06 | Invalid training profile/mode resets health/statistics; invalid assist value overwrites selected assistance. | Validate supported enum values before changing state. |
| RA-07 | Huge finite time reaches out-of-range microsecond conversion; long recovery narrows before limiting; tiny time permits infinite displayed DPS. | Bound clock before commit, clamp recovery before integer conversion and cap DPS to finite float. |
| RA-08 | README has obsolete blanket pause, stale audit-branch wording and fixed test count. | State actual authority boundaries, mark plans separate from runtime and direct readers to configured test discovery. |

The cycle repair does not validate dangling raw pointers or introduce a scene
ownership model. Translation-only semantics stay unchanged. Invalid constructor
configuration now throws invalid_argument, documented in PlayerController.h;
existing valid construction and normal gameplay are retained. Unknown attacks
still update LastAttack as a failed attempt report, not a successful hit.

## Actual baseline and repaired reproduction output

Commands compiled the same small source-level reproductions first against the
exact baseline source and then against the repaired source. Baseline output:
```
large_vector_length=inf
tiny_vector_length=0
idle_nan_delta_within_bounds=0 position_x=nan
nan_point_projected=1 output=nan,300
nan_fov_projected=1 output=nan,nan
invalid_attack_damage=60
invalid_profile_accepted=1 health=100 total_damage=0
invalid_affinity_bonus=20
posture_after_long_recovery=794967374 maximum=80
huge_finite_time_accepted=3.40282e+38
```
Repaired output:
```
large_vector_length=1.41421e+20
tiny_vector_length=1e-30
idle_nan_delta_within_bounds=1 position_x=0
nan_point_projected=0 output=123,456
nan_fov_projected=0 output=123,456
invalid_attack_damage=0
invalid_profile_accepted=0 health=75 total_damage=25
invalid_affinity_bonus=0
posture_after_long_recovery=0 maximum=80
huge_finite_time_accepted=0
```
These are observed Linux outputs, not portable promises about how undefined or
out-of-range conversions must behave on every compiler. Source reproductions,
commands, hashes and logs accompany the conversation audit archive.

## Executed verification

GCC 14.2, Clang 17, CMake 3.31.6 on Linux. Final runs:

| Configuration | Result |
|---|---|
| GCC Debug | 2/2 CTest executables passed. |
| GCC Release | 2/2 CTest executables passed, assertion guard enabled. |
| Clang Debug, ASan + UBSan + float-cast-overflow, leak detection | 2/2 CTest executables passed with no reported diagnostic. |
| New NumericAuditTests executable | 45/45 named cases, including 10,000 fixed-seed trials. |
| Unchanged SceneTests.cpp under the new target | Passed in all three configurations. |

Do not add trial counts to the number of independent CTest executables.
Both executables use astral_add_test. Tests/SceneTests.cpp and StaticMesh.cpp/.h
were copied for local execution, but are NOT changed by this patch. The legacy
scene test uses a fixed temporary filename; it is serialized in this CTest
process but separate concurrent invocations still need separate environments.

Final command pattern, from the worktree:
```
cmake -S Tests/NumericAudit -B ../build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build ../build-debug --parallel 2
ctest --test-dir ../build-debug --output-on-failure --no-tests=error
cmake -S Tests/NumericAudit -B ../build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build ../build-release --parallel 2
ctest --test-dir ../build-release --output-on-failure --no-tests=error
cmake -S Tests/NumericAudit -B ../build-sanitize -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ -DASTRAL_AUDIT_SANITIZERS=ON
cmake --build ../build-sanitize --parallel 2
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir ../build-sanitize --output-on-failure --no-tests=error
```
Initial test-harness compilation found an ambiguous `nan` name and a single-arg
macro failing on braced values. Those harness errors were corrected before the
successful runs; the initial failure log is retained. This was not a passing
production baseline run. YAML structure/content and whitespace checks are local
checks, not proof that GitHub has executed the newly configured workflow.

## Research corrections and open blockers

The [research extension](../Research/NUMERIC-AND-ASSET-PIPELINE-2026-09-25.md)
turns library suggestions into explicit numerical, importer/resource and CI
contracts. It separates Qwen model interfaces, language support from voice
quality, and rendering libraries from a finished engine. Unverified tool release
pins remain unverified, not branded false solely because a page failed to load.

PR #52 was rechecked at head `1b7a80715b33080372405ff357b1dafb3979d178`:
workflow 36026761079 remains failed. Its body still describes older green
`237241a...` evidence. The earlier reported head/merge workflow-test mismatch is
not repaired by this patch, and historical R0 remains outside the audit's execution
scope. Its owner already has the finding. Our new workflow explicitly covers
both refs rather than repeating that gap. PR #67 at `b39e464...` owns separate
ShadowCrypt files and received coordination notice; those files are untouched.

No new progress percentage is claimed. Repaired numeric inputs do not deliver
GPU rendering, imported rigged characters, audio, facial expressions, full editor
authoring or a 24-hour soak. Whole-engine completion and all required tools being
free cannot be certified from metadata or this selected source slice.

## Merge and native acceptance

Publish a draft PR. Require fresh independent review on the final exact source
SHA, the root Windows build/regression and the dedicated audit jobs. Then use the
registered native executor for movement/camera/near-clipping, ordinary combat,
large/invalid settings failure behavior, and all relevant interactive regressions.
Do not weaken the tests, modify another worker's branch or merge based on this
author receipt. No independent/native acceptance is asserted here.
