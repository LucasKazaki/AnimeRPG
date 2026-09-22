# E0 asset-loader repair and engine comparison baseline

Authorized by Lucas's September 20 request to research UE5/Unity gaps, implement
engine improvements, and validate online/in the sandbox with native tests local.
This is a repair of the existing mesh loader, not a new renderer or content task.
Base: `e12e6c559bf776ffc9c715c809a517f8e02ce5d5` (PR #6, still unmerged).
Keep that PR's acceptance gate. No merge, deployment, local runtime scheduling,
SDK installation, UE migration, or use of the unsafe historical R0 runner.

## Reproduced failure

The existing `StaticMesh::LoadFromFile` accepts a vertex record BEFORE its
`ASTRAL_MESH 1` header. A hash-matched C++ source fixture reproduced this in an
optimized build, returning 1 from the rejecting regression on September 20.
Existing parsing also has no file-byte, vertex-count, or edge-count budgets.

## Allowed paths

- `Engine/Assets/StaticMesh.cpp` and `.h`
- `Tests/AssetValidationTests.cpp`, `Tests/AssetValidation/CMakeLists.txt`
- `CMakeLists.txt` (register only the new native test)
- `.github/workflows/asset-validation.yml`
- `README.md` (test count and research/evidence pointers only)
- this task, `Docs/Research/ENGINE-CAPABILITIES-2026-09-20.md`,
  `Docs/Research/ENGINE-CAPABILITIES.json`, `Docs/QA/E0-ASSET-VALIDATION-2026-09-20.md`

Implementation must preserve the one-argument loader entry point and existing
clear-on-failure behavior. Require a header first, complete numeric tokens, finite
coordinates, valid unsigned edge indices, bounded file/record resources, and
repeatable valid/error/valid reloads. Do not change Game/, rendering, or combat.
Procedural wireframe fixtures only, with temporary files owned by each test run.

## Worktree and execution

Sandbox: dedicated task worktree from a deliberately partial, hash-verified
fixture. It is NOT a remote clone and cannot run AstralGame. The container cannot
resolve github.com; GitHub reads/writes use the connected tool. Compile the actual
loader, not a replacement mock. Build output must be outside the source root.
Run the failing baseline, Debug and optimized Release regressions, Clang ASan/UBSan,
and deterministic mutation/invariant checks. Preserve exact receipts.

GitHub: a dedicated stacked draft PR depending on #6, with hosted Linux sanitizer
checks and the existing Windows full build/domain gates. Native GUI, package,
Windows filesystem behavior, local performance, soak, and independent review
remain local acceptance requirements. Only the registered Company Runtime may
admit the local test job; local models must run commands and attach logs, not
approve from prose. This packet does not activate a 15-minute or hourly scheduler.
