# glTF import containment evidence — October 17, 2026

Status: focused implementation verified locally in native Debug and Release;
independent review and coordinator integration gates remain separate.
Branch: `engine/framework-containment-20261017`.
Base: **unmerged PR #80** at `efac9db54e8f2781c20fbb1bb9be9a0ab85f4f5c`,
not current main. Test source is that base plus this commit's engine/test diff.
Owner: sole containment implementation agent; the coordinator serialized all
compile and native test slots. No GUI, GPU, or competing build was started.

## Reproduction and resulting behavior

Before any engine edits, added `AssetManagerGltfRejectsOutsideDependency`.
It creates a disposable `content/models/external.gltf` with the URI
`../../outside.bin` and four original fixture bytes alongside `content`.
The direct `../outside.bin` request failed as expected, while the model imported
successfully, proving the dependency file was read beyond the declared root.
The regression's expected-failure assertion then aborted with exit **3**.

The repaired built-in glTF loader passes the manager's content root through
`GltfImportOptions::approvedRoot`. Both the source model and each external
dependency must resolve under that canonical directory. Relative dependencies
are percent-decoded once, both slash forms are normalized, and malformed escapes,
NUL, absolute/rooted, drive, UNC and scheme paths are rejected before opening.
The default direct file import approves only the source directory; memory
imports approve their explicit base directory. Empty memory bases still disable
external files. Callers approve a larger project root explicitly when needed.

Buffer violations fail without publishing a partial document. Image violations
preserve the existing nonfatal warning and empty image; the path check runs
before opening the outside image. Embedded data URIs and GLB remain supported.
The manager also rejects NUL in direct content paths.

## Native test evidence

MSVC **19.44.35222.0**, VS2022 Community tools **14.44.35207**, x64;
CMake **3.31.6-msvc6**, generator `Visual Studio 17 2022`.
The cache's source directory is the owned `work/astral-framework` checkout.
Both configurations use the coordinator-approved external multi-config cache.
`EngineAssetsTests` retains its existing `astral_add_test` registration and
`Tests/AssertionsEnabled.cpp`. Release output shows `/UNDEBUG` overriding
`/DNDEBUG` (D9025), preserving checks; no other build warning was recorded.

| Step | Exit/result |
|---|---|
| Unchanged-engine Debug target plus new reproduction test | 0 |
| Filtered old-importer regression | **3**, expected reproduction |
| Fixed Debug `EngineAssetsTests` build | 0 |
| Fixed Debug CTest, full assets suite | 0; **14/14 cases**, 0.66 seconds |
| Fixed Release `EngineAssetsTests` build and runtime/showcase dependencies | 0 |
| Fixed Release CTest, full assets suite | 0; **14/14 cases**, 0.13 seconds |
| `git diff --check` | 0 |

Old-failure slot was captured at `2026-10-17T13:36:47.1018710-04:00` through
`13:36:48.8488387-04:00`. The post-fix build slot began at
`2026-10-17T13:39:46.7123268-04:00` and was released after the final test at
`2026-10-17T13:41:02.4154560-04:00`. The slot was returned to the coordinator
between reproduction and post-fix builds while source/tests were edited.

Four added behavioral cases cover:

- The exact AssetManager-rooted outside-buffer reproduction.
- Real mesh positions and PNG pixels from in-root `../shared` paths, percent
  spaces and mixed slash input; asynchronous manager load; direct file/memory
  default boundaries; explicit wider roots; empty memory bases; exactly one
  URI decode; embedded data; failed reload retention and recovery; direct NUL.
- **23 buffer and 23 image URI variants**, including raw and encoded traversal,
  mixed separators, same-prefix sibling directory, absolute, drive-relative,
  drive-absolute, UNC, scheme, raw/encoded NUL and malformed percent escapes.
  Outside PNG fixtures are valid images, so an accidental read would decode and
  fail the empty-image assertions. Existing malformed/GLB/skin/material/image,
  cache, async and hot-reload cases also pass unchanged.
- File symlink source escape, directory symlink source/buffer/image escapes,
  and Windows junction source/buffer/image escapes. **All fixtures executed on
  this Windows host; no link test was skipped.** Junction creation is test-only
  through Windows filesystem APIs and requires no added dependency.

## Retained commands and logs

All paths below are literal PowerShell values. Logs remain outside Git source
in the task workspace; no binary or large build artifact is committed.

```powershell
$worktree = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/astral-framework'
$build = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/build-framework-baseline'
$evidence = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/import-containment-evidence'
$cmake = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
$ctest = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/ctest.exe'

# Run before engine edits; expected old-regression failure retained.
& $cmake --build $build --config Debug --target EngineAssetsTests --parallel 2 *> "$evidence/reproduction-build-debug.log"
& "$build/Debug/EngineAssetsTests.exe" AssetManagerGltfRejectsOutsideDependency *> "$evidence/reproduction-old-importer.log"

# Run after the source/test implementation below; each exit checked separately.
& $cmake --build $build --config Debug --target EngineAssetsTests --parallel 2 *> "$evidence/fixed-build-debug.log"
& $ctest --test-dir $build -C Debug --output-on-failure -V -R '^EngineAssetsTests$' --no-tests=error *> "$evidence/fixed-tests-debug.log"
& $cmake --build $build --config Release --target EngineAssetsTests --parallel 2 *> "$evidence/fixed-build-release.log"
& $ctest --test-dir $build -C Release --output-on-failure -V -R '^EngineAssetsTests$' --no-tests=error *> "$evidence/fixed-tests-release.log"
git diff --check
git diff --name-only
```

The original coordinator baseline logs are untouched. The coordinator reported
a full untouched PR80 native Debug build and **31/31 deterministic CTest** passes
before clearing this packet. That receipt is baseline context, not evidence of
post-fix full-all or integrated-main acceptance.

## Scope, review and limits

Changed paths are exactly `Engine/Assets/AssetManager.{h,cpp}`,
`Engine/Assets/Gltf.{h,cpp}`, `Tests/EngineAssetsTests.cpp`, the task packet and
this evidence record. No CMake, workflow, numeric, game-content or R0 edits.

Independent review is pending coordinator assignment; no review approval is
inferred from these tests. No push/PR/merge was performed by this owner.
Post-fix full-all engine/editor/player builds, broad deterministic suites,
portable sanitizers, native GUI, performance, GPU, packaging and soak gates
were not executed by this focused packet. The coordinator owns applicable
integration validation. The full engine workflow gate remains unclaimed.

Canonical-check/open races under adversarial concurrent filesystem mutation
are outside this bounded path contract. Custom user-supplied AssetManager
loaders remain responsible for their own dependencies; only the built-in glTF
loader receives this approved-root policy. Existing relative content-root/CWD
semantics of the manager are unchanged.

Primary research: [Khronos glTF URIs](https://github.com/KhronosGroup/glTF/blob/main/specification/2.0/Specification.adoc#uris),
[C++ canonical specification](https://eel.is/c++draft/fs.op.canonical), and
[MSVC canonical documentation](https://learn.microsoft.com/en-us/cpp/standard-library/filesystem-functions?view=msvc-170#canonical).
The registry HTML request timed out; Khronos's own specification source was
read successfully instead. No dependency, asset download or installation.
