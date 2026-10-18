# Build and stage a reusable Windows engine project

Owner `/root`, independent source reviewer `/root/demo_runtime` or
`/root/demo_gpu`. Isolated worktree `work/astral-project-package`, branch
`engine/project-package-20261018`, deliberately starts at reviewed assembly
`2c69888774c074a69ec12c5f659e3632280db420`. Source preparation can precede the
creator/native-player commits; actual packaging requires the combined candidate.
Deadline remains October18 noon Eastern. Root serializes all execution.

## Problem and bounded scope

The existing release manifest verifies bytes, while existing R0 package runners
exercise the game-specific M10 shell. Neither builds/stages a reusable GameHost
project. Preserve those tools; do not invoke historical R0. The creator packet
supplies AstralProject and AstralEnginePlayer, and the separate native extraction
will supply AstralEnginePlayerWin32 with EXE-adjacent Content/project.json.

Allowed five new paths: Scripts/package_engine_project.py,
Scripts/test_package_engine_project.py, this Task, its matching Docs/QA record,
and Docs/Engine/PROJECT-PACKAGING.md. Reuse Scripts/release_manifest.py unchanged.
No CMake, engine, player, game content, policy, dependencies or system settings.

## Contract

- A standard-library CLI accepts explicit clean source checkout, external build
  directory, selected Content root, absent package destination, evidence root,
  installed CMake and dumpbin paths. Release x64 only, maximum two jobs. Verify
  existing cache belongs to the exact source before reusing it. Configure a
  dedicated static MSVC runtime build and actually build the four real engine
  tools: native player, headless player, project creator and editor. Source head,
  tree and clean state must match before and after the build/staging process.
- Preflight all inputs before reserving output. Reject destination/source/build/
  content overlap and existing output (including broken links). Require an
  existing selected parent. Inventory Content recursively with deterministic
  order, finite file/byte budgets and Windows-case collision checks. Reject
  internal symlink/junction/reparse entries, special files and hard-linked files;
  no source modification or external dependency guessing. Copy every approved
  file unchanged so glTF buffers/textures/prefabs/audio and project notices stay
  available. Explicit project must be a contained regular .json file.
- Run actual dumpbin /dependents for every copied executable; require a bounded
  explicit Windows system-DLL allowlist and reject debug/non-system runtime
  dependencies. Keep complete output in external evidence. A static runtime
  choice is validated by actual PE dependencies, not assumed from a flag.
- Exclusively reserve the absent destination. Stage actual Release executables,
  complete Content, available repository license/notice files and any explicit
  extra notices. Add concise launch instructions and build/PE/source metadata.
  Compare hashes of each source/copy, reject files changed while staging. Do not
  overwrite output or recursively clean a partial package; return nonzero and
  preserve evidence/partial files on failure.
- Reuse the existing manifest to seal and verify staged bytes last. A manifest
  is only integrity evidence. Actual root-owned GUI launch outside the repo,
  headless capture/scene checks, restart/resize/cleanup and independently
  inspected output are separate required acceptance. No clean-machine, GPU
  presentation, license completeness or24-hour-soak inference.

## Verification

Use harmless disposable fixtures for absent/existing destination preservation,
overlap, linked-entry escape, case collisions, missing project/dependency and
changed-source rejection, plus an actual original Basic3D/glTF package. Test
failure paths before any publication. Preserve all failures and exact command,
source/binary hashes, timestamps and exits. Tests must verify filesystem effects
and real manifest integrity; do not replace real builds/runtime with mock success
as acceptance. After substantive independent review, run the affected Python
checks, safety/milestones, and the actual clean-candidate packaging command in an
exclusive slot. Stage under session outputs only after those gates pass.
