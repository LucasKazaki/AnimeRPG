# Adopt the committed scene editor — October 17, 2026

Owner: `/root/demo_editor` (transferred from `/root/containment_implementation`), sole implementation owner in
`work/astral-editor-adoption`, branch `engine/editor-adoption-20261017`.
Deliberate base: `964a8c46b8469e549dd57ebeaadd7b224a350c30`; coordinator reports
fresh full native Debug/Release all-target builds and 31/31 suites each
(78.09/4.96 seconds). This is a reviewed assembled candidate, distinct from main.
Lucas's current engine-only weekend authority applies; technical, independent
review and native acceptance gates remain. Local start: October 17, 14:57 -04:00.

## Reuse and exact scope

Selectively reuse committed editor core
`1c3d2ddb732b48d2129a5a0150747dbe3aa55978` and native shell
`96c25cbb6d56094b49a3e60717d150db42a9b873` from the separate local-test
repository. Read those Git objects only. Never edit its dirty checkout or copy
its Canvas/UI modifications or untracked UIDocument files. Do not adopt game
commits `bffe363` / `abadca8`, or optional tooling `087f347` in this packet.

Exactly 15 allowed paths:

- `Engine/Editor/EditorCamera.h`, `Engine/Editor/EditorCamera.cpp`
- `Engine/Editor/SceneEditor.h`, `Engine/Editor/SceneEditor.cpp`
- `Engine/Editor/EditorLayout.cpp`
- `Engine/Framework/GameHost.h`, `Engine/Framework/GameHost.cpp` (editor hooks only)
- `Tools/AstralEditorMain.cpp`
- `Tests/EngineEditorTests.cpp`, `Tests/EditorLayoutTests.cpp`
- `CMakeLists.txt`, `cmake/AstralEngineSources.cmake`, `Tests/EngineRuntime/CMakeLists.txt`
- This packet and `Docs/QA/ENGINE-EDITOR-ADOPTION-2026-10-17.md`

No Scene/Gltf/Assets/UI/Canvas, game, policy, workflow or dependency writes.
Prefab/sparse owners remain disjoint. No NewProject implementation or claim.

## Dependency-preserving adoption first

Reuse the final committed EditorCamera/SceneEditor/native shell and the existing
14 editor tests. Add an actual neutral OnCreate counter plus Behaviour component
count to establish script-free preview creation, including inherited children.
Keep all current host finite-input, UI ownership, dependency-watch/reload,
containment, reflection and lifecycle repairs. Port only ApplyProjectSettings
and unnamed-document restart hooks; use current hasScene_ reset semantics,
never replace the host with the old file or its stale unnamedScene_ lifecycle.
Add AstralEditorCore and EngineEditorTests, retain astral_add_test/assertions;
portable editor-core linking is limited to its own suite. Native editor adds
the existing Windows comdlg32 library; no new dependency or installation.

Freeze and run the adopted original-editor baseline before corrective edits.
Then add old public-core regressions and retain actual failures before fixing:

- Editing commands during Play must reject authoring mutations and preserve
  JSON, selection/history, preview and dirty state; ordinary runtime mutation
  is isolated in the existing GameHost. Explicit document replacement remains
  a separate file operation.
- SaveAs must stay inside its approved content root after canonicalization,
  including link escapes. Failed saves preserve authored state/path/history
  and the last saved file. Keep the existing atomic JSON writer.

Source facts guiding tests: preview recursively strips behaviours before Spawn,
but Parse validates via registered constructors. Play creates a separate host
and Stop does no runtime-to-document copy-back. Core authoring APIs and some
native commands currently remain callable while playing. Existing preview
Stats().behaviours==0 alone is not proof because preview is never Tick()ed.

## Native findings require native reproduction

The coordinator owns the desktop. Prepare exact neutral fixture/launch/steps;
do not fix these source hypotheses before actual reproduction:

1. External/project Open replaces the editor before candidate Open succeeds.
2. Dirty untitled close confirmation calls Save without reaching Save As.
3. A pending Inspector field followed by F5 may be dropped: Play precedes focus
   transfer, and subsequent EN_KILLFOCUS CommitField rejects while Playing.

Apply the user's Apple design skill proportionally: familiar Windows file
dialogs/shortcuts, visible/predictable native focus, preservation of authored
work and cancellable pointer capture. Relevant references: focus-and-selection,
keyboards, file-management and windows; fluid-motion direct manipulation and
cancellation guidance. Preserve current appearance; no unrelated redesign.
Native capture-loss, keyboard, DPI, Unicode/IME, accessibility and project
creation gaps are not claimed fixed without separate evidence.

## Execution gates and commands

All compiler/native/GUI work needs an explicit coordinator slot. Initial cache:
`work/build-editor-adoption`; external logs/fixtures: `work/editor-adoption-evidence`.
Use installed VS2022 x64 CMake/CTest and bundled Python; no installs.

```powershell
& $cmake -S $worktree -B $build -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON
& $cmake --build $build --config Debug --target AstralEditor EngineEditorTests EditorLayoutTests EngineFrameworkTests EngineSampleTests --parallel 2
& $ctest --test-dir $build -C Debug -V --output-on-failure -R '^(EngineEditorTests|EditorLayoutTests|EngineFrameworkTests|EngineSampleTests)$' --no-tests=error
& $cmake --build $build --config Release --target AstralEditor EngineEditorTests EditorLayoutTests EngineFrameworkTests EngineSampleTests --parallel 2
& $ctest --test-dir $build -C Release -V --output-on-failure -R '^(EngineEditorTests|EditorLayoutTests|EngineFrameworkTests|EngineSampleTests)$' --no-tests=error
& $python Scripts/test_test_safety.py
& $python Scripts/verify_milestone1.py
& $python Scripts/verify_milestone2.py
& $python Scripts/verify_milestone3.py
git diff --check
git diff --name-only
```

Stop on unexpected deterministic failure, retain logs and debug its cause. No
weakened checks, skipped assertions, repeated unchanged failed command, full-all
or GUI work outside a granted slot. Root independently reviews the actual
implementation; this owner cannot approve it. Final source/docs commit and
integration follow applicable gates. Full create/import/edit/play/save/package,
NewProject, GPU/performance/soak and broad native acceptance remain unclaimed.

## Adopted baseline outcome

Fresh configure and granted Editor/Layout/Framework builds and full suites pass
in Debug/Release: 3/3 suites, Editor15 (original14 plus real OnCreate/component
proof), Framework14 and retained Layout/process safeguards. No corrective
editor changes or GUI execution preceded these receipts. The native shell is
built for root-owned actual reproduction. See QA for exact commands/times/logs.
Baseline committed clean as `d5dc908ddec40d03d6cb40a2ff1b15f4c8d453f6`.
Its native Debug binary was preserved externally with a SHA/source receipt
before the next build. GameHost ownership transferred separately for narrow
project/settings transactionality, and root CMake transferred to the optional
backend owner; this editor owner will not overlap those reservations.

The subsequent test-only old run built Debug EngineEditorTests and retained six
separate exit3 failures: Play authoring command, direct transform and Undo;
SaveAs junction escape, temporary-file symlink escape and incorrect rejection
of a legitimate `..hidden` filename. Two existing behavior controls passed:
failed writes/rename preserve author state and last saved bytes, and in-root
linked saves reopen. No link skip, GUI or unexpected failure occurred. Slot
released October17 at15:13:42-04:00. See QA for exact filters/receipts; production
corrections were then cleared by the coordinator. Core mutation guards and
canonical destination/temporary-path SaveAs validation are now staged; fixed
acceptance awaits a granted compiler slot. Save success publishes a normalized
root-relative path usable by the existing asset cache; legitimate in-root links
and parent segments remain supported. The shared atomic writer is unchanged.
No race-proof filesystem sandbox claim. The 24-case fixed
editor suite retains original15 and all new regressions/controls. GameHost,
root CMake and native-only hypotheses remain untouched in these corrections.
Independent review identified an existing regular temporary hard link that can
overwrite last-saved bytes before rename failure. Its safe in-root regression
actually failed with native childexit3 at the saved-byte equality on the frozen
proposed fix, after an incremental Debug build0. The author-state checks passed;
the prior saved file's bytes changed. No privilege skip/outside access occurred.
The brief slot was released15:18:44-04:00. The coordinator accepted the receipt
and cleared an existing-regular-temp hard-link-count guard, now staged before
any truncation. No such guard preceded the actual review reproduction. The
coordinator later completed source re-review. The user released the desktop/GPU
hold for the October18 noon Eastern demonstration deadline; all execution remains
serialized under the coordinator. The first fixed Debug matrix built all five
granted targets, then stopped on a real in-root junction/parent-segment path
assertion. No Release or safety check followed the failure. A brief independent
diagnostic slot then confirmed the editor actually wrote to the wrong root
folder and reported that wrong path. The coordinator authorized preserving
component order: canonicalize each existing component before processing its
following parent segment, while permitting missing suffixes and retaining final
physical containment and temporary-file checks. That bounded correction passed
the coordinator's independent source review, then all granted fixed checks:
five target builds and four full suites in each Debug/Release, Editor24 cases
without link skips, safety3/3 and milestones1–3. CTest totals5.45/0.78s. Source
binding before/after is identical. The diagnostic and every assertion remain.
See QA for exact commands/receipts and the preserved earlier failure. Only this
Task/QA prose changed after the matrix. Native GUI findings and full workflow
acceptance remain pending.

## Native failed-open correction — authorized after actual reproduction

The core correction is committed as `2c4d286b27b2fd9bdc9eee6608da89f8b2ea6d42`,
clean before this extension. Root used the untouched d5dc908 native baseline
(exeSHA256 fb227c169c58bd612ddb5d4f1df1d53104edda8f78be734c3d300fb89ece881a)
and actually reproduced native hypothesis1 during the verified October18
04:06:50–04:10:10UTC interval:
opening root-a/keep.scene.json showed Keep Authored Box/Camera/Sun and its cube;
opening root-b/bad.project.json with a missing startup scene then replaced it
with an untitled Sun/Ground/Main Camera scene and status New scene. The prior
authored document/cube disappeared despite the failed candidate. No error modal
was visible. This is the coordinator's native observation, not a source-only
claim or an agent-run GUI.

Root authorized exactly `Tools/AstralEditorMain.cpp` plus this Task/QA for the
narrow native correction (the message called it AstralEditorWin32.cpp; this
repository's real front-end file is AstralEditorMain.cpp). Stage an external
scene/project's replacement SceneEditor separately; replace/stop the current
editor only after the candidate fully opens. On failure retain the prior root,
document, selection/history and window state and leave a useful path-specific
error visible. Same-root OpenScene keeps its existing transactional core load.
Do not touch CMake/Host/Game or core source, native close-save/focus hypotheses
until their actual observations arrive. Root retains the desktop and all
execution slots; no build/test or fixed GUI acceptance has occurred for this
native correction yet.

Root subsequently reproduced the other two hypotheses on that same preserved
baseline before authorizing their corrections. Later in that interval, Ground
Position X showed a typed5 with caret and no Enter/focus loss; F5 entered Play
with disabled X0, and Stop returned X0 without a dirty marker. Finally in that
interval, renaming Ground to Unsaved Native Probe and pressing Return showed the
dirty marker; Close displayed the real Yes/No/Cancel save confirmation, but Yes
produced no Save As picker and left the dirty document with the no-file/use-Save-As
status error. Root supplied these actual Sky observations; no screenshot file
is claimed saved by this agent.
The coordinator corrected its earlier estimated per-action timestamps; only
the verified common interval and action order are recorded. It then preserved
the probe through explicit Save As to root-b/preserved-native-probe.scene.json,
SHA256 `29638753063e53a00ccfc49e1f068215eaca420eaa9289b0fcc9e44b156d7498`,
closed normally and found no remaining owned native process.

The same narrow native file may now flush/validate pending inspector edits while
still authoring before Play and context-replacing/rebuilding actions. Invalid
input must remain editable, retain its text and block transition with a useful
error until corrected or explicitly reverted. Keep the core Play mutation guards.
For dirty untitled close, use the same Save As path as explicit Save, returning
failure on cancellation or failed writes so the window stays open. Root retains
all native and compiler execution; these changes need independent source review,
granted builds and actual fixed native rehearsal before acceptance.

All three native corrections are now staged in source only. Invalid inspector
values remain in their controls with a tracked failed binding; other commits or
context transitions cannot bypass that invalid field. Enter retries, and Escape
explicitly restores the displayed field value. Successful focused edits flush
before Play while authoring guards still allow the commit. Selection, authoring
commands, file actions and layout rebuilds use the same guard; disk reload waits
while an inspector control has focus or a failed field remains. Error status is
kept visible until a successful action or explicit correction. Untitled Yes
returns the actual Save As result, preserving the window on cancel/write failure.
Native acceptance and compiler execution remain pending root review/slot.

A separate native numeric warning was corrected after tracing the actual JSON
helper: JsonValue::AsFloat already clamps finite values to ±FLT_MAX before
conversion. There is no unchecked-conversion claim or undefined-behavior witness
for that path. The proposed rejection-policy harness/expansion was cancelled;
existing supported finite clamping and scalar declared ranges remain unchanged.

The native Tool51815b18... then passed root's independent source review and a
granted Debug/Release five-target/four-suite matrix:4/4 each5.65/0.78s, Editor24
retained, safety3/3 and milestones1–3. No source changed during the run; the
268-file binding including Tools is882b07681533a77b02d7797c8b31ef1a9cea8f560e96bce8af94e37465ad4c41.
Session43362 closed0 and no owned process remained before slot release.
Release native exeSHA256 is462056871f9e14aa04411416961bbd52b563479d4464704a11d7d3152ebcfa49.
Root will now perform the actual fixed native rehearsal; these core suites and
compilation do not establish interaction correctness. Only Task/QA prose changed
after the frozen source matrix, pending final observation/review/commit.

Root completed the actual fixed native rehearsal on the exact Release binary
46205687... from `2026-10-18T04:20:43Z` through close/cleanup `04:24:54Z`.
Focused X0→5 survived F5 Play/Stop; invalid text blocked Play, retained its text
and recovered through Escape. Failed project open preserved the prior scene,
selection/cube/X5 and usable undo/redo history, with a visible useful error.
Untitled Close > Yes opened Save As; cancelling retained the dirty document,
then a repeated save succeeded and closed the window. The actual saved
root-a/fixed-close-save.scene.json contains Fixed Save Probe, SHA256
`73778f0a654bb147bc0c78b54ce4632ea69a52292ad01da8b9bfd73a29cc7d09`.
Root's fresh Sky and owned-process inventories were empty. No actual exit code
was captured, and no saved screenshot PNG is claimed; the screenshots were in
chat. Source51815/core c6b22/tests e04 remain exact. See QA for bounded details;
full import/project/package and broad native coverage remain separate gates.
