# Diagnose and repair loaded-scene Save As

Owner `/root/demo_editor`; independent reviewer/coordinator `/root`.
Created before production edits at 2026-10-18 04:50 UTC. This packet follows
Lucas's engine-only implementation authority and noon demonstration deadline.
Dedicated worktree `work/astral-editor-saveas`, branch
`engine/editor-saveas-20261018`, deliberately based on reviewed editor
`1429312a04d21f706caa78a2248f8fcfda43d360`. The old worktree, preserved baseline,
tested binaries, original model fixture and demonstration project stay intact.

## Problem and actual evidence

Root used the tested Release editor (SHA256
`462056871f9e14aa04411416961bbd52b563479d4464704a11d7d3152ebcfa49`)
to open generated NeutralBox's `scenes/main.scene.json`. Assets double-click
created the original FacetedMarker model, its two colored primitives rendered,
Position X changed 1.5 to 2, and Undo/Redo restored 1.5/2. However Ctrl+Shift+S
and actual File > Save As repeatedly produced no picker and no diagnostic;
fresh native window inventory contained only the editor. Untitled Save As had
worked in the earlier native rehearsal, and Ctrl+O now opened a real picker.
Root preserved the original starter scene outside the project before Ctrl+S
saved the authored import, then verified Play/Stop with the imported geometry.
These are root's actual UI observations, without a fabricated per-action time.
Root subsequently supplied the verified common interval 04:46:00-04:50:24 UTC;
the supervised editor exited zero and its owned process was reaped. Ctrl+O
reopened the actually saved model scene before clean close. The saved scene
SHA256 is `b1c468039c825019d48dba376a00ade32106a2a578461ba3fddf82922b46c7a9`.

Source BrowseFile seeds lpstrFile with the forward-slash, content-relative
ScenePath; its false return currently conflates user cancellation with dialog
failure. That initial filename is a cause hypothesis until the actual common
dialog error is captured. Do not claim its diagnosis from the UI symptom alone.

## Allowed paths and boundaries

- `Tools/AstralEditorMain.cpp`
- this Task
- `Docs/QA/ENGINE-EDITOR-SAVEAS-DIALOG-2026-10-18.md`

No CMake, Host, core save resolver, Game, fixtures, policy, dependencies or
system settings changes. Root alone owns native UI and all build/runtime slots.
No parallel engine/test/GPU/native execution; admit builds with fresh 8 GiB
free-memory headroom and at most two compiler jobs. Stop on the first unexpected
deterministic failure; retain exact argv, exits, logs and source bindings.

## Approach

1. Preserve exact initial dialog inputs. On false, immediately inspect
   CommDlgExtendedError. Zero remains quiet cancellation; nonzero reports the
   operation, initial filename and symbolic/hex error. This diagnostic is the
   first bounded source increment, frozen for review before any granted build.
2. Root retries the loaded-scene Save As on the diagnostic binary. Record the
   actual error before choosing a behavior correction. If invalid initial name
   is confirmed, seed an absolute native-separator path under ContentRoot (or a
   leaf filename and scene parent directory), retaining the same core SaveAs
   containment and pending-inspector/cancel/close guards. Reject an overlong
   initial buffer explicitly instead of silently truncating. No broad Unicode
   or long-path support claim follows from this packet.
3. Independently review the final narrow diff. Verify both loaded and untitled
   Save As, menu and shortcut, cancellation/close cancellation, saved bytes,
   reopen, and Play/Stop with the real original model through root's native UI.

Microsoft's documented false-result distinction and error codes:
https://learn.microsoft.com/en-us/windows/win32/api/commdlg/nf-commdlg-commdlgextendederror
The initial filename/directory contract is documented at:
https://learn.microsoft.com/en-us/windows/win32/api/commdlg/ns-commdlg-openfilenamew

## Actual diagnostic and authorized correction

The source-only diagnostic was independently reviewed before its exclusive
Debug editor-only build. Configure/build exited zero in 3.656/40.250 seconds;
fresh free memory was 12.616 GiB. All 268 production/test/Tool source records
were physically identical before/after (JSON SHA256
`1415297f5febd3319b99b530b71b87a23fa56db02b80c0773b2166f9eee18345`).
The diagnostic Tool SHA256 was
`bd4381740af3df9205fddfd07aca82b78d805d9395504ac53bea6e1ba0a13445`.

Root then actually ran diagnostic Debug editor SHA256
`e6c9343c50adf88e609ffa00fa7c8042beb447fd4fad4aa787078d56608a86c6`
on the same saved NeutralBox scene. Ctrl+Shift+S reported visibly:
`Error: Save As dialog failed for 'scenes/main.scene.json': FNERR_INVALIDFILENAME (0x3002)`.
No picker appeared. The unchanged initial inputs now have a real invalid-name
error witness, not merely a path-format hypothesis. Common native interval:
2026-10-18 04:55:44.497266-04:56:08.528701 UTC. Owned process 25036 exited zero,
was reaped, did not time out, and root's fresh window/process inventories were
empty. External receipt:
`work/editor-saveas-evidence/diagnostic-native-first/exit.receipt.json`.

Before the next production edit, root authorized only the absolute native-path
seed under the existing content root, explicit initial-buffer capacity failure,
and retained extended-error handling in the same three paths. Untitled's empty
seed, pending inspector guards, core containment and cancellation semantics must
remain. Final Debug/Release and native acceptance were pending at that stage;
their subsequent actual results are recorded below.

## Exact planned local commands and acceptance

Use the installed VS2022 CMake bin and bundled Python, with absolute paths in
the external retained runner. External build is
`C:/AI/astral-build/editor-saveas-first`; verify its CMAKE_HOME_DIRECTORY before
reuse. Root grants diagnostic build/native observation separately from final
acceptance. Planned commands (D and R mean Debug and Release, run sequentially):

```text
cmake -S <work/astral-editor-saveas> -B C:/AI/astral-build/editor-saveas-first -G "Visual Studio 17 2022" -A x64
cmake --build C:/AI/astral-build/editor-saveas-first --config Debug --target AstralEditor --parallel 2
cmake --build C:/AI/astral-build/editor-saveas-first --config <D/R> --target AstralEditor EngineEditorTests EditorLayoutTests EngineFrameworkTests EngineSampleTests --parallel 2
ctest --test-dir C:/AI/astral-build/editor-saveas-first -C <D/R> -V --output-on-failure -R "^(EngineEditorTests|EditorLayoutTests|EngineFrameworkTests|EngineSampleTests)$" --stop-on-failure -j1 --no-tests=error
python Scripts/test_test_safety.py
python Scripts/verify_milestone1.py
python Scripts/verify_milestone2.py
python Scripts/verify_milestone3.py
git diff --check
```

Prepend CMake's directory only to the safety runner's process-local PATH. Record
pre/post production/test/Tool source manifests, actual native outcomes and disk
hashes. A diagnostic-only build is not a repaired workflow or final acceptance.
Commit only independently reviewed, verified changes in these three paths.

## Final actual verification

Root independently reviewed the corrected source diff
`38bdb0a6fc828d93e0a7d2c878a94441976a63d69d9180170e62171e6296397a`
and the corrected acceptance runner
`a459c4fe5ff3fef9fa5683a7d4ddcd3afe42afa5410705d69bf8f1adfda5331f`.
Tool SHA256 remains
`f8f0a83033461482ff842045843d4143717f2a9b12a990c155df2d9abfc30e75`.
Both configurations built all five affected targets successfully, with fresh
13.446/13.287 GiB free immediately before Debug/Release builds. Build child exits
zero in 11.031/54.375s; four anchored suites passed 4/4 in each configuration,
including 24 editor, 14 framework and 20 sample cases plus Layout, with no skipped link tests.
CTest elapsed 5.58/0.82s (owned wrappers 5.625/0.843s). Safety passed 3/3;
milestones 1-3 and both diff checks exited zero. All 268 production/test/Tool
physical records matched before/after, JSON SHA256
`601bb577e74282ba0d90f2134cf3315a52cf55f1bbc2cc521538ec1e2ded7bb9`.
Exact argv/cwd/PID/exits, per-build memory records and source/binary bindings:
`work/editor-saveas-evidence/fixed-first`. Fresh cleanup found zero relevant
compiler/test/editor processes before releasing the slot; this agent launched
no native UI. Subsequent edits are only final Task/QA evidence.

Root's fixed Release binary SHA256
`77f0a2ba48ad4fe0fdcaad91394ab41981933b4b341af3be25ea81b5cac81e2e`
passed actual loaded-scene Save As: Ctrl+Shift+S opened the real picker with
visible seed `main.scene`; Escape retained the clean scene. A second Save As
saved new `scenes/imported-demo.scene.json`. Play showed the colored original
marker, box and ground; Stop returned authoring; Ctrl+O reopened the saved scene.
Disk SHA256 `b1c468039c825019d48dba376a00ade32106a2a578461ba3fddf82922b46c7a9`
retains `models/FacetedMarker.gltf` and transform `[2,0,1]`.

Root then created an untitled scene and renamed Ground to
`UntitledSaveAsControl`. Alt+F4/Yes opened a real picker with empty filename;
Escape retained the dirty document/entity/window. Repeating Alt+F4/Yes and
saving new `scenes/untitled-saveas-control.scene.json` completed normal exit.
Its disk SHA256 is
`25bfbd5102985a0b309972aa74c20044357cf9f4e93d57d5492e9645fff0e0c3`,
and the entity name was confirmed from actual JSON.

The fixed native interval was 2026-10-18 05:06:48.037525-05:14:10.030426 UTC.
Supervisor process 46672 actual exit zero, reaped, no timeout, elapsed 441.984s;
receipt `work/editor-saveas-evidence/fixed-native-first/exit.receipt.json`.
Root's fresh window/process checks found zero owned editor/player/build
processes. Actual UI images are in chat; no saved screenshot file is claimed.

This bounded loaded/untitled Save As correction is locally verified; final
documentation review/commit and integration into the noon candidate remain.
Its capacity guard does not establish general long-path/Unicode support.
Native focus/capture cancellation is a separate unresolved packet. No combined
candidate/package, clean-machine, GPU presentation or 24-hour-soak acceptance
is inferred, and the complete engine workflow gate remains separately owned.
