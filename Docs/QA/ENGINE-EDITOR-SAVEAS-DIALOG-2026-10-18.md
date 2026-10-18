# Loaded-scene Save As evidence

Owner `/root/demo_editor`; reviewer and native operator `/root`.
Task: `Tasks/ENGINE-EDITOR-SAVEAS-DIALOG-2026-10-18.md`.
Base editor: `1429312a04d21f706caa78a2248f8fcfda43d360`.
Status: loaded/untitled Save As locally verified in Debug/Release regressions
and actual Release native controls; final documentation review/commit pending.

## Root's actual old observation

Root's tested Release editor SHA256
`462056871f9e14aa04411416961bbd52b563479d4464704a11d7d3152ebcfa49`
opened the generated NeutralBox project and loaded `scenes/main.scene.json`.
Assets double-click created `FacetedMarker [model]`; Frame Selection displayed
the original blue upper/orange lower geometry. Inspector X 1.5 to 2 committed,
and Undo/Redo restored 1.5/2. Both Ctrl+Shift+S and actual File > Save As then
repeatedly dismissed the menu without a picker or error; the fresh native
window list had only the editor. Ctrl+O opened a real picker. At that stage no
common-dialog error code had been observed; the relative forward-slash initial
filename was an unconfirmed hypothesis until the later diagnostic below.

Root exclusively backed up the original starter scene (historical SHA256
`de4437822b08d008086e883971fbc1deb9e72bad6390e7caf256e8d80d359c52`)
outside the project before Ctrl+S saved the authored import. Play rendered the
colored marker with the box/ground, and Stop retained X2. The disk scene retains
`models/FacetedMarker.gltf` and transform `[2,0,1]`, with actual saved SHA256
`b1c468039c825019d48dba376a00ade32106a2a578461ba3fddf82922b46c7a9`.
Ctrl+O reopened `scenes\main.scene.json` through the real picker and displayed
the model. Root supplied the common native interval 04:46:00-04:50:24 UTC and
the supervisor's actual exit zero/reaped process 48932 receipt. Its fresh window
and process inventories were empty. The original backup is
`work/editor-import-native-evidence/starter-before-native.scene.json`.
No saved screenshot file, precise per-action timestamp, clean-machine or GPU
presentation result is claimed here; actual UI images remain in the chat.

## Actual diagnostic build and native witness

Root independently reviewed the diagnostic three-path diff SHA256
`85cdad82c8a5146514169a16a378e4116b8e9d7c389c3ffb0e74a76e0b9c6c3e`.
The exact common-dialog inputs remained unchanged. The separately granted
runner used the checksum-bound reviewed run_owned helper, fresh 12.616 GiB free
memory, no MSBuild node reuse and two jobs. Configure exited zero in 3.656s;
Debug AstralEditor-only build exited zero in 40.250s. No suites or native UI ran
from this agent. All 268 physical source records matched before/after; JSON
SHA256 `1415297f5febd3319b99b530b71b87a23fa56db02b80c0773b2166f9eee18345`.
Exact argv/cwd/PID/exits, source manifests, binary hash and the fresh zero-process
cleanup inventory are in `work/editor-saveas-evidence/diagnostic-first`.

Root launched the actual diagnostic Debug binary, SHA256
`e6c9343c50adf88e609ffa00fa7c8042beb447fd4fad4aa787078d56608a86c6`,
on the same loaded NeutralBox scene. Ctrl+Shift+S visibly reported exactly
`Error: Save As dialog failed for 'scenes/main.scene.json': FNERR_INVALIDFILENAME (0x3002)`;
no picker appeared. This confirms the invalid initial-name diagnosis. Native
interval 2026-10-18 04:55:44.497266-04:56:08.528701 UTC; owned process 25036 actual
exit zero, reaped, no timeout. Root found no remaining owned window/process.
External supervised receipt is
`work/editor-saveas-evidence/diagnostic-native-first/exit.receipt.json`.

The narrow authorized correction seeds loaded-scene Save As with an absolute
native-separator ContentRoot/ScenePath and rejects an initial name that will not
fit the existing buffer. Untitled keeps its empty seed; error/cancel handling,
pending inspector guard and core save containment remain. This is not a broad
long-path, Unicode, GPU presentation or clean-machine claim.

## Final affected-target and regression gates

Root independently reviewed final source diff SHA256
`38bdb0a6fc828d93e0a7d2c878a94441976a63d69d9180170e62171e6296397a`
and acceptance runner SHA256
`a459c4fe5ff3fef9fa5683a7d4ddcd3afe42afa5410705d69bf8f1adfda5331f`.
The granted exclusive CPU run used the attributable short cache, two jobs,
no MSBuild node reuse, and fresh >=8 GiB gates before each configuration build.
Actual free memory before Debug/Release was 13.446/13.287 GiB.

| Gate | Debug | Release |
| --- | --- | --- |
| Five affected targets | child 0, 11.031s | child 0, 54.375s |
| Four anchored suites | 4/4, CTest5.58s | 4/4, CTest0.82s |
| Editor/Framework/Sample cases | 24/14/20 | 24/14/20 |
| Layout guards and link fixtures | pass; no SKIP | pass; no SKIP |

The exact CTest argv included `--stop-on-failure -j1 --no-tests=error` and the
four-suite anchored expression. Safety passed 3/3 (actual child 0, 5.203s wrapper;
unittest 5.107s). Milestones 1-3 and ordinary/cached diff checks all exited zero.
All 268 production/test/Tool physical hashes matched before/after, source JSON
SHA256 `601bb577e74282ba0d90f2134cf3315a52cf55f1bbc2cc521538ec1e2ded7bb9`.
Base 1429312 and all three then-frozen scope hashes remained unchanged throughout.
Exact argv/cwd/PID/exits, source bindings, per-build memory, binary receipts and
zero-process cleanup are in `work/editor-saveas-evidence/fixed-first`.
The agent launched no native UI; its session 16250 closed wrapper 0 and released
the CPU slot. Only final Task/QA evidence changes follow those gates.

Tool SHA256 stays
`f8f0a83033461482ff842045843d4143717f2a9b12a990c155df2d9abfc30e75`.
Debug editor: 7,062,016 bytes, SHA256
`7cf5c814ef88f1e4514ed1036351409b228e0f40b95dfb21c70d4f020f749cdd`.
Release editor: 1,792,512 bytes, SHA256
`77f0a2ba48ad4fe0fdcaad91394ab41981933b4b341af3be25ea81b5cac81e2e`.
The exact older diagnostic binary remains preserved separately at
`work/editor-saveas-evidence/diagnostic-first/AstralEditor-diagnostic.exe`.

## Root's actual fixed native controls

The actual Release binary 77f0a2 above loaded the same NeutralBox model scene.
Ctrl+Shift+S opened a real Save As picker with visible initial name `main.scene`;
Escape retained the clean scene. A second Save As wrote new
`scenes/imported-demo.scene.json`. Play displayed the colored marker, box and
ground; Stop returned authoring; Ctrl+O reopened that same saved model scene.
Actual disk SHA256
`b1c468039c825019d48dba376a00ade32106a2a578461ba3fddf82922b46c7a9`
retains `models/FacetedMarker.gltf` and transform `[2,0,1]`.

Ctrl+N created a new scene; Ground renamed to `UntitledSaveAsControl` and Return
made it dirty. Alt+F4/Yes opened a real Save As picker with empty filename.
Escape kept the dirty untitled entity/document/window. Repeating Alt+F4/Yes and
saving new `scenes/untitled-saveas-control.scene.json` completed actual exit 0.
Disk SHA256 `25bfbd5102985a0b309972aa74c20044357cf9f4e93d57d5492e9645fff0e0c3`
and the entity name were independently checked from the actual saved file.

Common native interval: 2026-10-18 05:06:48.037525-05:14:10.030426 UTC.
Supervisor owned process 46672, actual child/wrapper exit 0, reaped, no timeout,
elapsed 441.984s. Exact launch/exit receipt:
`work/editor-saveas-evidence/fixed-native-first/exit.receipt.json`.
Root's fresh Sky/window and process inventories found zero owned
editor/player/build processes. Actual native images are chat observations;
no saved screenshot file is claimed.

## Scope and remaining acceptance

This locally verifies the narrow loaded/untitled Save As repair and the tested
original model save/reopen/Play path. Final documentation review and the scoped
three-path commit remain; integration into the combined noon candidate still
needs its own acceptance. The explicit existing-buffer rejection is not broad
long-path/Unicode support. Focus/capture cancellation remains a separate source
lead. No combined package, clean-machine, GPU presentation or 24-hour-soak gate
is claimed from this component, and full engine workflow acceptance stays with
the coordinator. Earlier component receipts remain bounded to their own source.
