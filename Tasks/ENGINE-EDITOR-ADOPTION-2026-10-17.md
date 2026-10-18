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
