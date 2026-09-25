# RA-001: repository audit, numerical repairs and research corrections

Operator: Lucas, September 25, 2026. Inspected upstream main:
`2958741188279a0b438dd489ad00cac546012c25`. Dedicated branch:
`repair/repo-audit-20260925`; source-slice worktree `repo_audit_20260925/work`.
Read AGENTS.md and GAME_DEVELOPMENT_CONTROL.md at that upstream revision.
The explicit request authorizes this independent audit/repair, not a migration,
release, native-host execution, purchase, installation or unreviewed merge.

## Admitted implementation
Reproduce and repair numerical input/overflow defects in Math::Vec3::Length,
Camera projection/follow, cycle-safe translation hierarchy, PlayerController configuration/update/position and
CombatSandbox enum validation, clock conversion, finite DPS and posture recovery. Preserve
C++17, existing ordinary XY-plane combat, tuning, public caller compatibility,
score and event ownership. Add regression coverage and a dedicated standalone
CI workflow that actually runs it. Do not modify root CMake, active R0 workflow,
ShadowCryptSkirmish, ThoughtCommands or the hourly state/other workers' branches.

Allowed paths: Engine/Math/Math.h; Engine/Scene/Camera.{h,cpp};
Engine/Scene/PlayerController.{h,cpp}; Engine/Scene/Transform.{h,cpp}; Engine/Scene/CombatSandbox.{h,cpp};
Tests/NumericAudit/{NumericAuditTests.cpp,CMakeLists.txt};
.github/workflows/numeric-audit.yml; README.md; this task;
Docs/QA/REPO-AUDIT-2026-09-25.md;
Docs/Research/NUMERIC-AND-ASSET-PIPELINE-2026-09-25.md;
Docs/Research/research-source-ledger-2026-09-25.json.

## Reproduction and validation
Direct clone/archive access failed; reconstructed files must match their exact
GitHub blob hashes before edits. The local source slice is NOT a full repository
checkout. Retain baseline-failing and repaired-passing regression results, GCC
Debug/Release and Clang ASan/UBSan/float-cast-overflow results, commands and exits.
Use astral_add_test for every CMake test executable. Existing full Windows CI
and native interactive tests remain separate acceptance gates.

Run `cmake -S Tests/NumericAudit -B <external-build> -DCMAKE_BUILD_TYPE=Debug`,
`cmake --build <external-build>`, and `ctest --test-dir <external-build>
--output-on-failure --no-tests=error`; repeat for Release. The sanitizer target is
opt-in with `-DASTRAL_AUDIT_SANITIZERS=ON` and a supported Clang/GCC toolchain.
No dependencies are installed. Stop at a tested draft PR, fresh independent
review request and explicit coverage/remaining-gaps report; do not claim a full
line-by-line, native or security certification of all repository history.
