# glTF dependency containment — October 17, 2026

Owner: containment implementation agent, coordinated by the weekend engine lead.
Worktree: `C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/astral-framework`.
Branch: `engine/framework-containment-20261017`.
Deliberate base: unmerged PR #80, `efac9db54e8f2781c20fbb1bb9be9a0ab85f4f5c`.
Authority: Lucas's October 17 pasted weekend engine assignment, section 3E;
it supersedes older per-change/local-only limits within this engine scope.
It does not waive independent review, applicable technical gates, or the
engine-only boundary. No game-content or historical R0 work is admitted.

## Problem and evidence

`AssetManager::ValidPath` rejects a direct `../outside.bin` request, but its
built-in glTF loader passes no content root to `ImportGltf`. `Importer::LoadUri`
percent-decodes and concatenates a dependency with the model directory. Thus
`content/models/external.gltf` can read disposable `../../outside.bin` beyond
the manager's declared root. Existing malformed-file coverage uses an empty
base directory and does not exercise this path. Reproduction must use only
original disposable fixtures; no unrelated files are read.

The coordinator cleared the untouched PR80 baseline after a full native MSVC
Debug build and 31/31 deterministic CTest suites passed. All compile/test runs
for this packet require the coordinator's serialized build slot.

## Allowed paths and dependencies

- `Engine/Assets/{Gltf.h,Gltf.cpp,AssetManager.h,AssetManager.cpp}`
- `Tests/EngineAssetsTests.cpp`
- This packet and `Docs/QA/ENGINE-IMPORT-CONTAINMENT-2026-10-17.md`

Preserve C++17, native Win32 build conventions, the current public import APIs,
the existing `astral_add_test(EngineAssetsTests)` registration, and all checks.
No dependencies, CMake/workflow, numeric, gameplay, or policy edits.

## Contract and implementation

Append `approvedRoot` to `GltfImportOptions`. The manager's built-in glTF loader
passes its content root. A direct file import without this option approves
only that source file's directory; a memory import approves its explicit
`baseDirectory`. Empty memory base directories continue to disallow external
files. An explicit approved root also constrains the source model itself.

Decode each external URI exactly once; reject malformed percent escapes,
embedded NUL, absolute/rooted, drive, UNC and scheme references. Normalize both
slash forms defensively, canonicalize existing root and target, and require
the target to descend from the approved directory before reading. Permit
`../shared` and percent-encoded names when their final target remains within
that root. Preserve data URIs and GLB buffers. Missing or prohibited images
retain the current nonfatal warning/empty-image behavior, with no outside bytes
read; prohibited buffer dependencies fail transactionally.

Primary references reviewed October 17:
- [Khronos glTF URI contract](https://github.com/KhronosGroup/glTF/blob/main/specification/2.0/Specification.adoc#uris): data URIs and relative resource paths, with optional additional URI forms left to clients.
- [C++ canonical operation](https://eel.is/c++draft/fs.op.canonical) and [MSVC filesystem functions](https://learn.microsoft.com/en-us/cpp/standard-library/filesystem-functions?view=msvc-170#canonical): canonicalization resolves existing symbolic links and dot segments.

## Acceptance and bounded stop

1. Record the old failure using a rooted model and original outside fixture.
2. Reject raw/encoded traversal, mixed separators, sibling-prefix escapes,
   absolute/drive/UNC/scheme and malformed/NUL references with diagnostics.
3. Exercise real mesh data and PNG decoding through in-root parent references,
   memory/file defaults, data URIs and the built-in manager loader, including
   asynchronous loading and failed reload preservation where relevant.
4. Check source-model and dependency symlink/junction escapes when OS privileges
   permit creation; explicitly record unavailable fixture types.
5. Debug and Release affected targets/tests pass with assertions active, followed
   by substantive independent review. Stop at the first deterministic blocker;
   retain failed attempts and resolve it before accepting the packet.

This is an existing-file path-containment contract, not a race-proof filesystem
sandbox: an adversarial concurrent link replacement between canonicalization
and opening is outside this bounded repair. No native GUI/GPU/soak acceptance
is implied. No push or merge is performed by this implementation owner.

## Exact command shapes

Run from the worktree with a coordinator-granted build slot. CMake is
`C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe`;
CTest is beside it. The coordinator explicitly authorized reuse of its existing
external multi-configuration Debug/Release build root at
`C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/build-framework-baseline`.
Its cache confirms this owned source worktree. Do not reconfigure or build there
without renewing shared ownership. Configure was already completed by the
coordinator; this owner ran only the focused builds and tests below.

```powershell
& $cmake --build $build --config Debug --target EngineAssetsTests --parallel 2
& "$build/Debug/EngineAssetsTests.exe" AssetManagerGltfRejectsOutsideDependency
& $ctest --test-dir $build -C Debug --output-on-failure -V -R "^EngineAssetsTests$" --no-tests=error
& $cmake --build $build --config Release --target EngineAssetsTests --parallel 2
& $ctest --test-dir $build -C Release --output-on-failure -V -R "^EngineAssetsTests$" --no-tests=error
git diff --check
git diff --name-only
```

Retain concrete paths, exact invocations, exits and source revisions in the QA
record. Coordinator-owned broader integration checks are recorded separately.
