# Editor adoption evidence — October 17, 2026

Status: adopted original editor baseline passed native Debug/Release builds
and all three granted suites, retaining its 14 cases plus one real neutral
preview lifecycle proof. Corrective editor changes and native GUI reproductions
have not begun. This is a frozen baseline, not final editor acceptance.
Owner `/root/containment_implementation`, worktree `work/astral-editor-adoption`,
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

After this baseline, GameHost ownership transfers separately for a bounded
transactional project/settings repair. The editor owner will not modify those
files during that reservation. Old editor core failure regressions and actual
native observations must still precede their respective corrective changes.

## Prepared native reproduction — not yet executed

Coordinator owns all launches/input and must retain actual before/after
evidence. The following creates original fixture JSON under the external
evidence directory, never inside game content. Do not run it until the native
slot and launch are authorized. Retain files rather than cleaning other work.

```powershell
$nativeFixture = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/editor-adoption-evidence/native-fixtures'
$nativeEditorBuild = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/build-editor-adoption'
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
& "$nativeEditorBuild/Debug/AstralEditor.exe" "$nativeFixture/root-a" 'keep.scene.json'
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
