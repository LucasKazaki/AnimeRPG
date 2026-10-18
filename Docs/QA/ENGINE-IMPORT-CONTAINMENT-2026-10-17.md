# glTF import containment evidence — October 17, 2026

Status: implementation and independent re-review complete; full native
Debug/Release builds, deterministic suites and retained GDI smokes pass on this
branch. PR 80 adoption and hosted/proposed-main gates remain separate.
Branch: `engine/framework-containment-20261017`.
Base: **unmerged PR #80** at `efac9db54e8f2781c20fbb1bb9be9a0ab85f4f5c`,
not current main. Test source is that base plus the committed implementation
and review-follow-up engine/test diffs on this branch.
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

## Independent review follow-up: invalid UTF-8 paths

The independent reviewer identified an error-path gap in initial commit
`275b61ee31598df6899f9507cfdbec6ff5043048`: `std::filesystem::u8path` can throw
before the canonical operation's `error_code` is reached. Before changing
production source, added two direct-import regressions and rebuilt Debug.
Both failed with exit **3**: `%FF.bin` escaped the buffer API's bool/diagnostic
contract, and `%FF.png` escaped the image warning contract. Both recorded
the exception text `No mapping for the Unicode character exists in the target
multi-byte code page.` The initial containment test matrix had missed this.

The follow-up catches only `std::system_error` at external path resolution
and the direct file entrypoint. This includes filesystem/conversion errors,
without swallowing allocation failures. On this MSVC implementation the
UTF-8 converter throws `std::system_error`, as confirmed in its local standard
library header. Source/base/root strings retain native filesystem encoding;
they are not interpreted as percent-encoded URIs.

Added behavior coverage for invalid byte, overlong, truncated, surrogate and
out-of-range UTF-8 buffer references; invalid/overlong image references through
both direct import APIs; invalid/missing native source/base/root paths; and
successful percent-encoded accented mesh/image filenames. Buffer failures
retain the last good document; images retain their warning and empty pixels.

| Follow-up step | Exit/result |
|---|---|
| Initial implementation plus new Debug reproduction tests | 0 |
| Filtered malformed UTF-8 buffer regression | **3**, expected reproduction |
| Filtered malformed UTF-8 image regression | **3**, expected reproduction |
| Corrected Debug target and full assets CTest | 0; **16/16 cases**, 0.66 seconds |
| Corrected Release target and full assets CTest | 0; **16/16 cases**, 0.15 seconds |

The follow-up slot started at `2026-10-17T13:44:15.3085462-04:00`; code/tests
were frozen and the slot released at `2026-10-17T13:45:25.6703161-04:00`.
All link fixtures again executed without skips. Independent re-review is
pending; fixing a finding does not itself establish review approval.

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

# Review regression against initial 275b61e engine source, then corrected source.
& $cmake --build $build --config Debug --target EngineAssetsTests --parallel 2 *> "$evidence/review-reproduction-build-debug.log"
& "$build/Debug/EngineAssetsTests.exe" GltfMalformedUtf8BufferUris *> "$evidence/review-reproduction-buffer.log"
& "$build/Debug/EngineAssetsTests.exe" GltfMalformedUtf8ImageUris *> "$evidence/review-reproduction-image.log"
& $cmake --build $build --config Debug --target EngineAssetsTests --parallel 2 *> "$evidence/review-fixed-build-debug.log"
& $ctest --test-dir $build -C Debug --output-on-failure -V -R '^EngineAssetsTests$' --no-tests=error *> "$evidence/review-fixed-tests-debug.log"
& $cmake --build $build --config Release --target EngineAssetsTests --parallel 2 *> "$evidence/review-fixed-build-release.log"
& $ctest --test-dir $build -C Release --output-on-failure -V -R '^EngineAssetsTests$' --no-tests=error *> "$evidence/review-fixed-tests-release.log"
```

The original coordinator baseline logs are untouched. The coordinator reported
a full untouched PR80 native Debug build and **31/31 deterministic CTest** passes
before clearing this packet. That receipt is baseline context, not evidence of
post-fix full-all or integrated-main acceptance.

## Scope, review and limits

Changed paths are exactly `Engine/Assets/AssetManager.{h,cpp}`,
`Engine/Assets/Gltf.{h,cpp}`, `Tests/EngineAssetsTests.cpp`, the task packet and
this evidence record. No CMake, workflow, numeric, game-content or R0 edits.

Independent review found the conversion error above. Reviewer
`/root/numeric_implementation` (not this packet's implementer) re-reviewed
`efac9db..e4dac29a379dcd4cc620dd4f46eb8bca2d974419` after the retained
reproduction and repair and reported no remaining actionable findings. The
review covered actual root propagation, decoding, canonical comparison, file
and directory links/junctions, source/dependency containment, data URIs, failed
reload preservation, UTF-8 diagnostic/warning semantics and regressions. Review
was read-only; it executed no tests. No push/PR/merge was performed by the
implementation owner.

Coordinator full validation on that frozen source, using the existing external
`work/build-framework-baseline` cache:

| Command | Exit/result | Log under `<session>/work/` |
|---|---|---|
| `cmake --build work/build-framework-baseline --config Debug --parallel 2` | 0, all configured engine/editor/player/test targets | `framework-fixed-Debug-build.log` |
| `ctest --test-dir work/build-framework-baseline -C Debug --output-on-failure -E RuntimeSmoke --no-tests=error --stop-on-failure` | 0, 31/31, 55.42 s | `framework-fixed-Debug-test.log` |
| Same build/test commands with Release | 0, 31/31, 5.78 s | `framework-fixed-Release-{build,test}.log` |
| `ctest --test-dir work/build-framework-baseline -C Debug --output-on-failure -R RuntimeSmoke --no-tests=error --stop-on-failure` | 0, 6/6, 19.47 s | `framework-fixed-Debug-smoke.log` |
| Same smoke command with Release | 0, 6/6, 19.27 s | `framework-fixed-Release-smoke.log` |
| `python Scripts/test_test_safety.py` with installed CMake on process PATH | 0, 3/3, 5.652 s | `framework-fixed-test-safety.log` |

Native smokes were serialized and exercise the retained GDI prototype. They do
not establish interactive editor or Win32-player acceptance. A separate real
headless Release player run also succeeds (exit 0):

```text
work/build-framework-baseline/Release/AstralPlayer.exe work/astral-framework/Content/Samples/Playground project.json --script work/astral-framework/Content/Samples/Playground/demo.play --frames 372 --size 320x180 --capture work/framework-player-captures --every 120
```

It loads 35 entities, runs 372 frames and emits four PNG captures; final hash
`f0ee817693b4394b`. Log: `work/framework-fixed-player.log`. This reuses the
existing engine sample, without authoring game content. Update/render figures
printed by that tool are diagnostic observations, not performance acceptance.

Hosted checks, proposed-main integration, portable sanitizer, interactive
editor/player, GPU, distribution and soak evidence remain separate. This
bounded root-path repair does **not** approve PR 80's unresolved input ownership,
dependency propagation or reload diagnostics. The full workflow gate remains
unclaimed. The coordinator will publish this scoped patch against its unmerged
framework base, without merging the entire original PR stack.

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
