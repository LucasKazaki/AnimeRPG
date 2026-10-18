# Noon demonstration assembly evidence

Owner `/root`; Task ENGINE-NOON-DEMO-ASSEMBLY-2026-10-18.
Status: source assembly only; no combined configure/build/test/native/package
acceptance yet. Every input's earlier evidence remains limited to that input.

The branch deliberately started at reviewed Project1804a3c. Task commit10d4e59
preceded source integration. Creator5c9fb44 merged normally without conflicts.
The next normal editor1429312 merge conflicted in five approved paths:
GameHost.h/cpp, root/portable CMake and cmake/AstralEngineSources.cmake.

The initial resolution retained Project's exact Host.h/cpp bytes, including the existing
ApplyProjectSettings signature consumed by Editor Play; the superseded editor
hook's sequential config publication is not restored. CMake preserves Creator's
shared Runtime helper and Runtime-only ProjectTests, adds EditorCore and keeps
both real Project/Editor suites registered. Portable suites preserve their
existing warning/sanitizer/test options; Editor adds its core dependency after
the Runtime-only Project exception. Source list contains both suites once.

Independent editor-owner review identified lost unnamed-scene restart behavior
from the editor parent. Root reproduced it before changing production: fresh
native Debug EditorTests compiled, then existing unchanged
PlayRunsTheEditedSceneAndRestartsIt aborted with actual child exit3 at line513,
sceneLoads/LastError assertion. The exact-argv bounded runner and six physical
source hashes before/after are retained in work/noon-editor-old-evidence.

The corrected resolution preserves Project's settings transaction while
restoring only the editor parent's document-copy Activate branch for unnamed
scene requests and its matching restart command/LoadSceneJson comment. This is
an intentional combined Host, not byte-identical to either full parent.
Independent editor-owner re-review of the corrected Host/doc delta was clean;
the staged CMake/suite resolution also remained clean. Root rebuilt native Debug
EditorTests and ran its complete registered suite:24 cases,1/1 CTest pass in
2.97s, including the formerly failing unchanged restart case and real filesystem
controls. Host/header/test physical hashes matched before/after. Fixed receipts
are separate in work/noon-editor-fixed-evidence. Full combined Debug/Release
all-target acceptance remains pending; this focused result authorizes only the
reviewed merge resolution.
Native-player, Host-ingress and packaging packets remain separately owned.
No R0, GPU presentation, clean-machine or24hour-soak result exists here.

Next, normal D3D11 backend1c370c4 merge conflicted only at root Runtime
construction. Resolution keeps Creator's shared Runtime helper, followed by the
unchanged opt-in backend targets/options from the reviewed GPU parent; the
duplicate old Runtime construction is removed. Backend sources, tests, workflow
and packet records are unchanged from the GPU commit. Independent runtime-owner
resolution review is clean: exact incoming GPU sources/options/tests and one
shared Runtime helper invocation, with creator/editor consumers retained.
Combined compile remains pending. Offscreen hardware acceptance still
does not establish editor/player GPU presentation.

## Reviewed repair and packaging inputs

Packager8c6d68f merged normally as2f56dcb, preserving all five reviewed paths.
Its16 unit cases and safety/milestones pass on its own packet; a real engine
package remains pending the reusable native-player target. Independent runtime
review of immutable2f56dcb found creator/editor/backend/packager blobs unchanged,
transactional Console/Host/settings intact, and only the intentional unnamed
restart branches differing from Project1804. Its CPU-only CTest admission note
requires excluding the GPU label if hardware tests are enabled; RUN_SERIAL is
not ownership across CTest processes.

Reviewed ingress95834bf merged normally as e7f6ceb without conflicts. Root
checked the resulting Host against958: only the retained unnamed-scene restart
branches differ. Its four-path fixture history includes four actual old child3
failures and three passing controls. The first fixed spelling regression remains
in evidence; compact JSON formatting repaired it while preserving the existing
assertion. Final component Debug/Release Sample32/Services28/Framework21 all
passed, safety3/3 and M1-M3, with source65e25... unchanged.

Reviewed SaveAs4ca4a31 merged normally as c4b2a3e without conflicts. Root's final
three-path patch hash c0db390d6f9c0b79d6a1dae2e8075aaa6d2678f05e2b222a109909213a68743e
matches the committed input; Toolf8f0a830... remains the tested source. Actual
component Debug/Release five-target builds/four suites, safety3/3/M1-M3 and
Release native loaded/untitled save/cancel/reopen/Play/Stop controls pass. The
supervised actual child exit0/reaped/noTimeout receipt is
work/editor-saveas-evidence/fixed-native-first/exit.receipt.json. Saved imported
model sceneSHA b1c468... retains model/[2,0,1]; untitledSHA25bfbd... retains the
control entity. Exact full receipts are in the input QA packet, not inferred
combined-candidate acceptance. DraftPR100/101/102 are published and attached;
all remain separate from main.

The reviewer also identified an unexecuted editor OpenScene/OpenProject cached
Ready/current-Error boundary hypothesis. A separate owner prepares safe old
fixtures in a new isolated packet; no result is claimed yet. The full combined
all-target/native/package acceptance is still pending, and full workflow gate
remains NOT PASSED. No clean-machine or24hour-soak claim is made.
