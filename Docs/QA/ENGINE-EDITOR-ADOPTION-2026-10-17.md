# Editor adoption evidence — October 17, 2026

Current status: the corrected editor core passed the granted five-target builds
and four full Editor/Layout/Framework/Sample suites in both Debug and Release,
including all24 editor cases and real junction/symlink/hard-link controls without
skips. Test safety3/3 and milestones1–3 pass. The coordinator independently
reviewed the exact source before this matrix. All earlier failures and diagnosis
are retained below. Native GUI reproductions have not begun; this is bounded
editor-core evidence, not the full demonstration/workflow gate.
Owner `/root/demo_editor` (transferred from `/root/containment_implementation`), worktree `work/astral-editor-adoption`,
branch `engine/editor-adoption-20261017`, deliberate base
`964a8c46b8469e549dd57ebeaadd7b224a350c30`. Coordinator reports its full native
Debug/Release baseline: all targets built and 31/31 suites each, 78.09/4.96s.
This baseline receipt does not establish the adopted editor's correctness.

Only committed core `1c3d2dd` and shell `96c25cb` code may be reused, preserving
current repaired dependencies. The source checkout's dirty UI/Canvas and
untracked UIDocument are excluded. Exact paths, staged reproduction gates,
commands and scope are in `Tasks/ENGINE-EDITOR-ADOPTION-2026-10-17.md`.

No editor correction is accepted from source inspection alone. The adopted
original baseline must execute first; core regressions must fail before fixes,
and native-only findings require coordinator-owned native reproduction. No
NewProject, full workflow, GPU/performance/soak or native accessibility claim.

## Staged reuse and preserved guards

Six files were copied directly from Git blobs at `96c25cb`, incorporating its
core parent, without reading dirty working files as the source. SHA256 before
the new test was appended:

| Path | SHA256 |
|---|---|
| `Engine/Editor/EditorCamera.h` | `1d5728b51595ce4eea7def94b605b2d14c4051087583f5fb1a7c7bb444de4bc9` |
| `Engine/Editor/EditorCamera.cpp` | `dbb43235fc6a5211b69db1a580597c73a3a01b3f82229f765f9d7d72e459e649` |
| `Engine/Editor/SceneEditor.h` | `782e62fb42710b4c0c0f499b3e0875e1bd06918ecf420857707f2e97e67f1647` |
| `Engine/Editor/SceneEditor.cpp` | `ee6800da9052599a363505661725580cf95f7e4f894ca25ac0759a33f452867a` |
| `Tools/AstralEditorMain.cpp` | `b89f04cd8a1cf3f2d3e41f856494319170aeba564c8a140b76748a1d9fa6e0ed` |
| `Tests/EngineEditorTests.cpp` | `34fada14030e789430678b81e8aa667f75a7dea2a8c72ab0bf2e6af4a3a6d9d0` |

EditorLayout and its tests were changed only to enable the five real supported
tools. Existing overflow, bounds/containment, required-control initialization
and GetMessage error/quit regressions remain. The adopted shell retains required
control creation checks, class/window failures and classified edit-mode message
loop results. GameHost was patched narrowly for project settings and unnamed
restart using current hasScene_ state, preserving the finite validation, UI
ownership and dependency-reload code. No old host file was copied.

The new preview proof uses only disposable neutral inline/file prefabs and an
authored behavior with external constructor/OnCreate/OnDestroy counters. It
checks actual Behaviour component count and no OnCreate across open, render,
rename, Undo/Redo, then verifies that Play really calls all five probes and Stop
tears them down without copying runtime state back. This proves lifecycle
suppression, not arbitrary constructor-side-effect isolation. This test now
passes in both configurations. The original 14 cases remain intact.

## Adopted-original baseline execution

On October 17 under one explicit coordinator slot, fresh VS2022 x64 CMake
configuration and the four granted targets succeeded in Debug and Release:
AstralEditor, EngineEditorTests, EditorLayoutTests and EngineFrameworkTests.
Only those targets/suites ran; no GUI, full-all build, Sample suite or safety
fixture run occurred in this baseline slot. No corrective editor change was
made before/during it. The source/CMake 13-file physical-content manifest SHA256
was `7d5c9a02c78e2ecf330f138f093a836ae45b430634629db627c5889ddf3905cc`.

| Step | Exit/result | External log |
|---|---|---|
| Fresh configure | **0** | `adopted-configure.log` |
| Debug four-target build | **0** | `adopted-build-Debug.log` |
| Debug Editor/Layout/Framework CTest | **0**, 3/3, 3.59s | `adopted-test-Debug.log` |
| Release four-target build | **0** | `adopted-build-Release.log` |
| Release same CTest | **0**, 3/3, 0.44s | `adopted-test-Release.log` |
| Scope/whitespace check | **0**, exactly 15 allowed paths | checked before commit |

Each configuration executes EngineEditorTests **15 cases** (original 14 plus
real lifecycle proof), EngineFrameworkTests **14 cases**, and the existing
EditorLayoutTests assertions. Debug suite timings: Layout0.03s, Framework0.64s,
Editor2.90s; Release0.03/0.09/0.32s. Logs confirm the new counter/component test
passes along with every original editor case. Release builds retain the
AssertionsEnabled guard and `/UNDEBUG` overrides. No assertion was weakened.
No unexpected build/test failure occurred. Existing layout overflow/bounds,
containment, required-control and message-loop checks remain unchanged except
the five availability predicates now correspond to real supported tools.

Slot start `2026-10-17T15:06:26.3505141-04:00`; Debug build finished at
`15:07:11.8139029-04:00`, Debug suites at `15:07:41.6238453-04:00`, Release build
at `15:08:36.9670725-04:00`, Release suites and explicit slot release at
`15:08:43.7025843-04:00`. The coordinator accepted these receipts. No process
was launched from the built AstralEditor binary; it remains the original
adopted native shell for actual native-only reproduction.

```powershell
$editorWorktree = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/astral-editor-adoption'
$editorBuild = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/build-editor-adoption'
$editorEvidence = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/editor-adoption-evidence'
$editorCmakeDir = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin'
& "$editorCmakeDir/cmake.exe" -S $editorWorktree -B $editorBuild -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON *> "$editorEvidence/adopted-configure.log"
& "$editorCmakeDir/cmake.exe" --build $editorBuild --config Debug --target AstralEditor EngineEditorTests EditorLayoutTests EngineFrameworkTests --parallel 2 *> "$editorEvidence/adopted-build-Debug.log"
& "$editorCmakeDir/ctest.exe" --test-dir $editorBuild -C Debug -V --output-on-failure --stop-on-failure -R '^(EngineEditorTests|EditorLayoutTests|EngineFrameworkTests)$' --no-tests=error *> "$editorEvidence/adopted-test-Debug.log"
& "$editorCmakeDir/cmake.exe" --build $editorBuild --config Release --target AstralEditor EngineEditorTests EditorLayoutTests EngineFrameworkTests --parallel 2 *> "$editorEvidence/adopted-build-Release.log"
& "$editorCmakeDir/ctest.exe" --test-dir $editorBuild -C Release -V --output-on-failure --stop-on-failure -R '^(EngineEditorTests|EditorLayoutTests|EngineFrameworkTests)$' --no-tests=error *> "$editorEvidence/adopted-test-Release.log"
git diff --check
```

The baseline was committed clean as
`d5dc908ddec40d03d6cb40a2ff1b15f4c8d453f6`. Before any subsequent build, its
Debug `AstralEditor.exe` was copied to external
`work/editor-adoption-evidence/baseline-native/AstralEditor.exe`: 7,034,368
bytes, SHA256 `fb227c169c58bd612ddb5d4f1df1d53104edda8f78be734c3d300fb89ece881a`.
No repository-local Debug runtime DLLs were present. The adjacent
`baseline-binding.receipt.json` binds the preserved binary to that clean
source commit, the source manifest and the baseline test results. Use this
untouched binary for coordinator-owned old native reproductions.

After this baseline, GameHost ownership transfers separately for a bounded
transactional project/settings repair. The editor owner will not modify those
files during that reservation. Root `CMakeLists.txt` is also reserved separately
for the optional backend owner. Neither file set changes in the editor fixes.

## Core old failures retained before corrective changes

Under the next explicit compiler/test slot, only `EngineEditorTests.cpp` changed
from adopted production `d5dc908`. A Debug `EngineEditorTests` build passed
(exit0), starting `2026-10-17T15:13:24.7559785-04:00`, ending
`15:13:28.2979347-04:00`. Eight separate native test children then ran through
the existing filter API, with their stdout/stderr and exact argv/times/exits
retained in `work/editor-adoption-evidence/old-<filter>.*`. The test source
SHA256 was `fad3a5e1fc8c94228e6168e73d27b7cb3e09e38b80ca659eec5fc6e70d63f89a`.
The child runner was external `run-old-core.py`; it stops on an unexpected
deterministic result. Production code was unchanged throughout this run.

| Filter | Actual old exit | Observation |
|---|---|---|
| `PlayingRejectsAuthoringCommandsWithoutChangingPreviewOrHistory` | **3** | Create while playing succeeds instead of rejecting |
| `PlayingRejectsDirectTransformWithoutChangingAuthoredState` | **3** | Direct SetTransform succeeds while playing |
| `PlayingPreservesUndoAndRedoUntilStopped` | **3** | Undo succeeds while playing; the later Redo arm is not reached in this old run |
| `SaveAsRejectsCanonicalDirectoryEscapeWithoutWritingOutside` | **3** | SaveAs returns1 and creates the disposable outside JSON through a junction |
| `SaveAsRejectsCanonicalTemporaryFileEscape` | **3** | SaveAs follows the temporary-file symlink and succeeds |
| `SaveAsAcceptsInRootNormalizationAndRejectsInvalidPathsTransactionally` | **3** | Legitimate `..hidden.scene.json` filename is rejected; later invalid-path checks not reached yet |
| `FailedSavePreservesDocumentSelectionHistoryAndLastSavedBytes` | **0** | Existing atomic writer preserves document, selection, undo/redo and last-saved bytes after write/rename failures |
| `SaveAsAllowsInRootDirectoryLinksAndKeepsAuthoredReferences` | **0** | Legitimate in-root junction SaveAs and reopen work |

All fixtures use unique disposable directories and original neutral content.
Both junction and file-symlink facilities were available; **no link test was
skipped**. Expected aborts leave only these harmless fixture files in the test
temporary directory. No GUI, full suite or corrective source change occurred.
No unexpected failure occurred. Final child ended and the slot was explicitly
released at `2026-10-17T15:13:42.488192-04:00`. Fixed verification is still pending.

## Staged core correction contract — fixed execution pending

The coordinator accepted the old receipts and cleared production corrections.
Authoring commits, direct transforms, active drags and Undo/Redo reject while
Play is active. They retain the document, selection, history, dirty state and
preview; runtime world edits remain isolated in its existing separate host.
Stop makes authoring available again. Explicit document replacement remains a
separate file operation; no new Play framework was added.

SaveAs resolves its existing approved root, destination and fixed `.tmp`
sibling before directory creation or writing. Complete-prefix filesystem
identity handles casing and symlink/junction ancestry. An outside target or
temporary path is rejected; pre-existing temporary file symlinks are rejected
even in-root. Following the independently reproduced review failure, an
existing regular temporary file must also have a successfully inspected hard
link count of at most1 before any truncation; aliases and inspection errors
are rejected. This preserves the shared atomic writer's sibling-file behavior.
Absolute/drive/UNC/scheme references and embedded NUL are errors; Windows path
conversion errors return diagnostics. Parent segments and links are allowed
when their physical resolution stays in-root, including legitimate `..hidden`
filenames. Success publishes the canonical root-relative spelling, so the
existing AssetManager can reopen it without forbidden parent segments. Failed
saves preserve authored state and last-saved bytes. The shared JSON writer is
unchanged. This is a bounded path/temporary-alias contract, not a race-proof
sandbox against concurrent filesystem replacement.

The fixed test set retains original15 and adds9 cases (24 total), covering the
valid mutation families, direct-transform bypass, Undo and Redo, actual runtime
mutation then Stop, outside links and temporary links, malformed/absolute paths,
in-root normalization/reopen, mixed slashes, physical link/parent semantics,
write/rename failure and author-state invariants. The old run reached only the
specific assertions listed above; later arms await fixed verification. Host,
root CMake and all three native-only hypotheses remain untouched by these fixes.

The coordinator's independent source review then identified another promised
failed-save invariant at risk: an existing regular `.tmp` file can be a hard
link to the last-saved authored file, so truncation precedes a failed rename.
A new disposable regression, `FailedSaveRejectsAliasedTemporaryFileAndPreservesSavedBytes`,
saves `author.scene.json`, edits the document, hard-links that saved file to
`blocked.scene.json.tmp` and creates a directory at the final destination to
force rename failure. It checks the existing public author state and last-saved
bytes. Under a separate brief granted slot it **actually failed with child
exit3**, at the last-saved byte comparison. SaveAs had returnedfalse/error and
the public AuthorState checks had passed first; this is a concrete disk-byte
preservation failure, with no out-of-root access or privilege skip. No hard-link
guard preceded that reproduction. Debug incremental build exited0, starting
`2026-10-17T15:18:31.8873412-04:00`, ending`15:18:36.2852273-04:00`; the sole
filtered child ran`15:18:44.387227`–`15:18:44.443430-04:00`, then the slot was
explicitly released. Logs: `old-hardlink-build-Debug.log` and
`old-hardlink.stdout.log`/`.stderr.log`/`.receipt.json`, plus build receipt,
under the same external evidence root. Receipt hashes bind SceneEditor.h
`20635a32bb1b73407fa02a436ab414a37ee51e899b53790ebc881b3a9f0592e7`,
SceneEditor.cpp `d0bcb710aa18764a70412eb53612cf5babfdf0eeb23a4567b44dc87b8df02960`
and tests`4a79b780eff22c771a64b98788083a205a944e917a5d612895a78ecd571dab3f`.
No GUI/visible native application ran. The coordinator accepted that receipt
and cleared the bounded regular-temporary hard-link guard, which is now staged
for independent review. The fixed matrix remains pending. The user explicitly
requested that the desktop/GPU remain free, and the coordinator paused all new
local builds/tests because available RAM was below the requested headroom.
No new build/test/GUI work will run until the coordinator releases those gates.

## First fixed Debug matrix — stopped on a real failure

The user released the desktop/GPU hold for the October18 noon Eastern live
demonstration. Under an exclusive coordinator CPU slot, the existing CMake
cache's source was checked against this exact worktree. No GUI or GPU device
was used. Wrapper `work/editor-adoption-evidence/run-fixed-core.py` records all
commands, times and exits. Its five-target Debug build succeeded (exit0,
7.344s; `2026-10-17T23:52:24.667247-04:00`–`23:52:32.001217-04:00`).
The anchored Editor/Layout/Framework/Sample CTest then failed (exit8, 3/4 suites,
5.79s reported by CTest). Layout, Framework14 and Sample20 passed. Editor passed
its first22 cases, then failed `SaveAsAllowsInRootDirectoryLinksAndKeepsAuthoredReferences`
at `ScenePath() == "scenes/physical-parent.scene.json"` after the public SaveAs
had returned success. The final hard-link case was not reached. No assertion
was weakened and no Release/safety/milestone step ran after the failure.

Logs `fixed-build-Debug.log`, `fixed-test-Debug.log` and adjacent step receipts,
plus `fixed-matrix.receipt.json`, are retained. Source pre/post binding is
identical: manifest264 files, SHA256
`24b58e65975d26b7ba46741997628a4654f827971eb611fe5b10c0e58e7cfaf4`;
SceneEditor.cpp `8c08ce3433a7084cce92fa0c6967c6c0687144c429813f20bdc2fbb3bb5e2af1`,
SceneEditor.h `20635a32bb1b73407fa02a436ab414a37ee51e899b53790ebc881b3a9f0592e7`,
tests `4a79b780eff22c771a64b98788083a205a944e917a5d612895a78ecd571dab3f`.
Wrapper session81272 closed with exit1 and an explicit failure message; no
task-owned compiler/test process remained when the slot was released.

A diagnostic-only print was added after that failure, before the unchanged
assertion, to show the reported path, both possible file locations, full-path
weakly_canonical result and separately canonicalized link parent. Current test
SHA256 is `e04fa86fe11e8ac708582bdaaa59fd06e38c9c54c85daee7680ded4ef209152c`.
The installed MSVC14.44
`include/filesystem` implementation at weakly_canonical shows it tries the
entire path, then lexically_normal() before walking existing components when
the final file is absent. Under a subsequent brief exclusive diagnostic slot,
the Debug EngineEditorTests-only build exited0 (`23:55:22.481644-04:00`–
`23:55:24.947338-04:00`). The sole in-root linked-save filter then actually
exited3 at the unchanged assertion. It printed:
`reported=physical-parent.scene.json expected-dir-file=0 root-file=1`, with
weakly_canonical returning the content-root file while separately canonicalizing
the link first returned its physical parent `content/scenes`. Thus both disk
location and reported path were wrong. The test hash stayed e04fa86f... before
and after; no production change preceded the diagnostic. Logs and command/time/
exit receipts are `path-diagnostic-build-Debug.*` and `path-diagnostic-old.*`.
The slot was released with no remaining task-owned compiler/test processes.

The coordinator accepted the observation and authorized a bounded helper repair:
walk components in order, canonicalize each existing component before a later
`..`, and append missing suffix components while still allowing them to be
cancelled by a subsequent parent segment. Final whole-component filesystem
identity containment and the temporary symlink/hard-link checks remain. This
correction was staged for independent source review before the next fixed
matrix slot. The diagnostic print and every existing assertion remain; there
is no relaxed physical-target expectation. The matrix is recorded below.

## Corrected component-path matrix — passed Debug and Release

The coordinator independently reviewed SceneEditor.cpp c6b22ffa... and the
unchanged24-case diagnostic/assertion source e04fa86f... as clean, then granted
the exclusive CPU slot. `run-component-fixed-core.py` used the existing verified
cache, two build jobs, exact five targets AstralEditor/EngineEditorTests/
EditorLayoutTests/EngineFrameworkTests/EngineSampleTests and anchored four-suite
CTest. The installed CMake bin was prepended only to the command process PATH
so the test-safety harness could find its existing compiler tools. No system
configuration, GUI or GPU operation occurred.

| Step | Actual result | External evidence prefix |
|---|---|---|
| Debug five-target build | exit0, 4.094s | `component-fixed-build-Debug` |
| Debug four-suite CTest | exit0, **4/4**, 5.45s | `component-fixed-test-Debug` |
| Release five-target build | exit0, 10.016s | `component-fixed-build-Release` |
| Release four-suite CTest | exit0, **4/4**, 0.78s | `component-fixed-test-Release` |
| Test safety | exit0, **3/3**, 5.164s harness time | `component-fixed-test_test_safety` |
| Milestones1/2/3 | exit0 each | `component-fixed-verify_milestone1/2/3` |
| Whitespace check | exit0 | `component-fixed-diff-check` |

Both configurations execute Editor24, Framework14 and Sample20 plus the retained
Layout assertions. Logs show no SKIP; actual folder junction, file symlink and
hard-link controls ran. The preserved diagnostic now reports
`scenes/physical-parent.scene.json expected-dir-file=1 root-file=0`, confirming
both intended physical write location and saved-path spelling. The final
temporary-hard-link saved-byte preservation case also passes. The safety
harness confirms Release evaluates assertions and serialized smoke safeguards.

The slot ran `2026-10-17T23:59:23.746150-04:00`–`23:59:49.568184-04:00`.
Wrapper session74489 closed0; an inventory normalized path separators and found
no remaining task-owned compiler/test/native process before explicit release.
Each step's `.log`/`.receipt.json` and complete
`component-fixed-matrix.receipt.json` remain under
`work/editor-adoption-evidence`, alongside the first failed matrix and actual
diagnostic receipts. No unchanged failure was rerun as an acceptance attempt.

The physical264-file Engine/Game/Tests/cmake/root-CMake binding is identical
before and after: SHA256
`23d3a1947b86aa636492f49478bf282a92571016a10b707fd4c5990bc08d29ac`.
Exact tested source SHA256: SceneEditor.cpp
`c6b22ffa16bcff623742820c53a7c2867a5a24ce38aaaf58396227aaf5aecd7e`,
SceneEditor.h `20635a32bb1b73407fa02a436ab414a37ee51e899b53790ebc881b3a9f0592e7`,
EngineEditorTests.cpp
`e04fa86fe11e8ac708582bdaaa59fd06e38c9c54c85daee7680ded4ef209152c`.
Only this Task/QA prose is updated after the matrix. The original preserved
native baseline exe remains hashfb227c169c58bd612ddb5d4f1df1d53104edda8f78be734c3d300fb89ece881a.
Native open/save/focus observations and full creation/import/package/performance
acceptance remain separate pending gates.

## Prepared native reproduction — not yet executed

Coordinator owns all launches/input and must retain actual before/after
evidence. The following creates original fixture JSON under the external
evidence directory, never inside game content. Do not run it until the native
slot and launch are authorized. Retain files rather than cleaning other work.

```powershell
$nativeFixture = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/editor-adoption-evidence/native-fixtures'
$nativeEditorBaseline = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/editor-adoption-evidence/baseline-native'
New-Item -ItemType Directory -Force -Path "$nativeFixture/root-a", "$nativeFixture/root-b" | Out-Null
$nativeUtf8 = [System.Text.UTF8Encoding]::new($false)
$neutralScene = @'
{"format":"astral-scene","version":1,"entities":[
 {"name":"Keep Authored Box","components":{"MeshRenderer":{"mesh":"primitive:cube"}}},
 {"name":"Camera","transform":{"position":[0,2,-5]},"components":{"Camera":{}}},
 {"name":"Sun","components":{"Light":{"type":"Directional"}}}]}
'@
[System.IO.File]::WriteAllText("$nativeFixture/root-a/keep.scene.json", $neutralScene, $nativeUtf8)
$brokenProject = @'
{"format":"astral-project","version":1,"name":"Broken project","startupScene":"missing.scene.json"}
'@
[System.IO.File]::WriteAllText("$nativeFixture/root-b/bad.project.json", $brokenProject, $nativeUtf8)
& "$nativeEditorBaseline/AstralEditor.exe" "$nativeFixture/root-a" 'keep.scene.json'
```

1. **Failed external/project open:** Confirm the named scene, Box outliner row
   and current selection. File > Open Project, choose root-b/bad.project.json.
   Retain the error and resulting window title/outliner. The source hypothesis
   predicts replacement by a new untitled/default scene despite failed open;
   correct behavior retains the prior document/root/selection. Repeat with an
   unsaved rename only if separately needed, explicitly choosing Discard for
   the attempted replacement, then check failed-open state preservation.
2. **Dirty untitled close:** File > New; select Ground, rename it to a distinct
   marker and press Enter so the document is dirty. Close the owned window,
   answer Yes to save. Record whether the real Save As dialog appears. The
   source hypothesis predicts an error saying the scene has no file yet.
   Cancel/keep the editor open; preserve the fixture and avoid losing work.
3. **Pending Inspector F5:** Open root-a/keep.scene.json, select Box. In the
   Position X edit, Ctrl+A then type 5; do not press Enter or move focus. Press
   F5; record Play, then release/press F5 again to Stop. Inspect X and save a
   separate `f5-result.scene.json` through the real menu if needed to inspect
   authored output. The source hypothesis predicts X reverting to 0 because
   focus-loss commit occurs after Playing becomes true. A correction must
   commit/validate the pending field before Play or clearly reject the action.

These are proposed reproduction steps, not native proof. No native-only fix
will be made before the coordinator reports the actual observation. Keyboard
focus/capture loss, DPI and Unicode still require their own bounded evidence.
