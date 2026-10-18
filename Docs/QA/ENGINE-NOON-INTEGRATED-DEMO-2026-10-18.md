# Noon integrated candidate QA

## Current delivery record after the Release rehearsal

Current static Release CPU delivery and the fresh-project workflow passed their
bounded independent reviews: static `independent-static-off-actual-review.json`
(ffc296a5), then rehearsal completion 88591bede61a300b18b45c5c1604cbf415a9a13fbfd834abbad559cd30744e45
and `independent-actual-rehearsal-review.json` (b74c007d, 122 sealed evidence
files/42 children). The whole-engine gate is **NOT PASSED**. This establishes
the recorded CPU demonstration on this machine, with the limits below.

The four actual static x64 Release/MT tools and 13-file primary Package were
built from clean commit `4f3c6cef564e94980b333daf35d9870202be8dd5`, tree
`1fb67a3132684bdad419383eead84aaf7f72f71d`, physical manifest 0a009fd1.
All 285 production/test build inputs are unchanged from accepted ON validation
7f6b0e05/61c83e96; ON binaries retain that original provenance. The static cache
663652f0 and owner f4ad996e retain value MultiThreaded: the helper's untyped
configure changed only STRING to UNINITIALIZED. Outer session 43433 exit 1
is preserved; actual package child 11280 exited 0. The independent variance
proof bc2/0662 admitted this metadata-only difference without an engine rerun.

Fresh Basic3D creation/file-copy original-model import, Assets instantiate,
Frame Selection, X1.5-to-2 Undo/Redo, startup save, Save As cancel with no
Content changes, new demo save/reopen X2, CPU Play with settled Box and Stop
restoring authored state passed. Editor PID 2468 exited 0/reaped/no timeout in
309.531 seconds. Headless simulated 120 frames and produced one final PNG
(d45e47f7), with tracked Box Y0.490. Fresh package PID 38900 exited 0/reaped in
4.562 seconds; all ten internal steps passed. Its no-argument player PID 59216
ran from an unrelated outside cwd, showed the saved geometry, maximized and
restored, then closed normally: exit 0/reaped/no timeout, 103.594 seconds.
F5 was sent without a distinct reset oracle; no audio fixture was verified.
Root UI observations have no saved trace. The original editor capture-height
record 1430 has an explicit supplement correcting it to 1429 (width 2378);
this is window-capture metadata, without a render-size/performance inference.

Source 627, old recovery 33 and primary Package 13 files remained exact.
Final documentation scope is this Task/QA pair plus the canonical weekend
checkpoint and capability map (four paths; nineteen candidate-v-main paths).
Before applying/committing this final docs-only reconciliation, independently
bind its new 627-file/tree identity, verify the other 623 files/all 285 inputs
unchanged, and preserve the 4f3c package BuildInfo/manifest identity. Do not
relabel packaged binaries as compiled from a later documentation commit.
Current candidate hosted CI, merge and immutable final guide/delivery seal
remain pending. Full 2D, full GPU/material/skinning parity, remaining native
controls, clean-machine compatibility and the required soak remain unaccepted.
A separate current static CPU animation-player check passed (completion
cb9d146d, independent c38fb86b): same packaged player PID 22844, normal exit 0/
reaped/no timeout, 36.156 seconds, 11 children/26 sealed evidence files. Original
and copied six-file Content stayed exact. Root observed two distinct Bend poses
at the same camera/window; there is no saved visual trace or independent viewing
claim. GPU animation, audio and full animation breadth remain unaccepted.

## Earlier integration and validation record (chronological)

The record below predates the static-package/fresh-rehearsal results above.
Its then-pending package, rehearsal and clean-commit steps are retained history;
the current delivery record above governs status. Initial 61c83 and subsequent
0a009 manifest/patch/receipt history remain immutable.

Status: current CPU/hardware and bounded Release player accepted. Later
editor/delivery/rehearsal/CI gates remain pending. Root created candidate 7f6b0e05, tree
ba17c501, from exact main bfbe3a1 and unchanged reviewed GPU bf8d705. Input is
excluded. Only this new QA and its matching Task are authorized edits. Existing
component/older assembly records remain untouched historical evidence.

All fifteen adopted GPU Git blobs equal bf8d705. The current physical-byte
freeze is authoritative for this checkout: ten GPU paths match prior manifest
31ff8370 raw hashes; five differ only by checkout CRLF conversion (GPU Task/QA,
D3D11Renderer.cpp and D3D11Viewport.cpp/.h). Strict CRLF-to-LF comparison is equal
for those five. The external EOL table records old and current raw hashes.
No source/settings change was made to force a prior raw hash; old evidence
remains evidence for its original bytes and binaries.

## Prior evidence and limits

Main PR107 includes imported-animation authoring and its minimal test initializer
repair. Local D/R CPU/native acceptance, then all current exact main-push checks,
were independently accepted; the external root main audit is 37c1eb46. Those are
component/current-main facts, not combined GPU binary acceptance.

GPU commit bf8d705 has independently accepted bounded optional opaque-Unlit
consumers and retained hardware/native evidence. Open controls, broader material/
post/skinning parity and original native timeouts remain open. Input has twelve
default plus four Debug focused passes and a later failed control oracle; its
unpublished source is not adopted. No full input pass is inferred.

## Accepted current CPU gate

- [x] Independent current 627-file/285-input/17-path/consumer and command review.
- [x] Owned VS2022/x64 ON/testing/hardware-registration cache; fresh RAM gates,
  maximum two build jobs and node reuse disabled.
- [x] Debug/Release ALL_BUILD exit 0; 40 registered, 33 selected and passed
  deterministic suites per configuration; RuntimeSmoke/GPU excluded.
- [x] CPU filters 3/1/1; safety 3, M1-M3, package-helper 19; Release assertions.
- [x] All 17 imported cases and 18 cleaned private roots per configuration;
  source/cache/owner/output seals matched; 47 children exit 0/reaped/no timeout.

Independent `independent-actual-cpu-review.json` SHA prefix 025f7cd2 accepted
completion fa2dc7c5. Its retained root parser failure was a harness-whitespace
postprocessing error; actual CTest passed and was not rerun. Final root-owned
process evidence recorded zero blockers; no fresh reviewer process query is
claimed.

## Accepted current bounded hardware gate

- [x] Current D/R EXE and resolved link-input identities sealed against CPU
  proof; these are current measurements, with no old p95 carry-forward.
- [x] Separately admitted current D/R 15-case suite; 13 artifacts/configuration,
  120 matching native measured/completed frame IDs and 20 debug-layer cycles.
- [x] Original opaque-Unlit fixture, 1280x720 native workload, 4/76/22/1 counts,
  30 warm/120 measured frames, image and equivalent-live-map assertions retained.

Independent `independent-actual-hardware-review.json` SHA prefix 48ac12f3
accepted completion 8a28968d. Current native GPU/full-frame p95 milliseconds:

| Configuration | GPU | Full frame |
| --- | ---: | ---: |
| Debug | 0.146048 | 12.6961 |
| Release | 4.313472 | 12.7855 |

Both native p95 limits remain 16.67 ms. The 29 retained child receipts include
supplemental analyzer exit 1, followed by corrected analyzer v2 exit 0. That
literal parsing correction did not rerun engine tests. Source, fixture, cache,
binaries and link inputs remained unchanged; final root-owned process evidence
had no blockers. This is bounded opaque-Unlit hardware acceptance, not a full
renderer, general GPU UI/skinning or universal lifetime claim.

## Accepted current Release player observation

Root's `native-release-player-first/root-observation.receipt.json` and actual
player receipt bind PID 23928, 10:57:49.985624-11:00:17.938494 UTC, 147.953 seconds,
normal exit 0/reaped/no timeout. CPU default, native-menu GPU geometry/backend,
literal renderer.info submission, console open/close, checked GPU via Alt+R,
CPU return and normal close were observed. Visible diagnostics were RTX 5080,
LUID 73259, hardware/debug layer 1, backbuffer 1280x720, sync 1, DISPLAY2 desktop
2560x1440; the suffix was clipped. F5 retained geometry/backend without a
distinct world-reset oracle. Root's settled list contained no owned player
windows; retained scoped process evidence had zero blockers. No saved UI PNG or
independently inspectable UI trace is claimed.

Pointer first-click, active hot reload, specified resize diagnostics and broader
controls remain open. This does not close the current editor or full native gate.

## Current bounded Debug editor root observation

Root observation 764f6670 and its actual editor receipt record PID 17572,
11:02:06.987516-11:08:20.657458 UTC, 373.672 seconds, normal exit 0/reaped/no
timeout. CPU editing showed the original static geometry; GPU was disabled
before Play. CPU Play then explicit GPU selection showed current geometry.
Literal renderer.info was visible before Return; actual response showed
RTX 5080/LUID 73259, debug/hardware 1, backbuffer 1104x655, sync 1 and DISPLAY2
desktop 2560x1440, with suffix clipped. Stop restored clean CPU editing.

Same-process File > Open Project loaded the owned animation copy. Inspector
Bend/.25 and identity Transform matched; CPU Play showed two distinct Bend
poses, then Stop restored identity/Bend/.25 and clean edit state. Normal close
left no owned editor windows. Original/copied static 5 and animation 6 files
stayed exact. These are root live-frame observations, without a saved UI image;
the independent bounded-evidence review is recorded separately. The original
single-sided model's default edit-camera backface limit remains explicit;
front-camera CPU Play showed the motion.

This does not establish full authoring/history/save/reopen, the Release CPU
editor workflow, complete native matrix, pointer first-click, active reload or
specified resize diagnostic controls. Those gates remain pending/open.

## Pending native and delivery gate

- [ ] Current combined editor full Release create/import/history/Save As cancel/
  save/reopen/PlayStop, admitted Debug focus/lifetime checks and normal closes.
- [ ] Original two-joint CPU fixture + declared front-camera metadata, real
  model/Animator authoring and visible motion, unchanged originals/save state.
- [ ] Current ON player/editor static Unlit GPU choice, geometry, actual
  renderer.info/backend/menu state, CPU return and normal close; remaining
  component pointer/resize/timeout limits stay explicit.
- [ ] Static Release OFF delivery in its separate cache; unchanged strict PE
  policy and four tools, mandatory notice, content/manifest byte verification.
  ON distribution remains outside this packet.
- [ ] Existing capture folder before 120 frames; actual outside-repository
  noargs relocated sealed-player launch and fresh-project rehearsal.
- [ ] Actual independent source/receipts/binaries/package review, immutable
  commit/tree links and all exact current hosted head/merge/main checks before
  root's final SHA-guarded merge decision. No old output is overwritten.

## Freeze, clean-commit and future delivery binding

The actual validation was bound to initial freeze 627/61c83e96, 285 build inputs,
HEAD 7f6b0e05 and index tree 47f2a0d4, with only these two new docs staged.
The documentation reconciliation/apply receipt records its own resulting tree;
it does not replace the tested source identity.
Preserve the complete initial manifest, 17-path patch and all failure history.
After authorized application, bind new complete physical/doc/index identities
and independently review the docs-only delta; the accepted production/test
285-input binding must remain exact. Preparing the external reconciliation proposal performed no source
application, Git mutation or engine execution.

Future static OFF commands are in `static-off-delivery-plan-final.json`
(27695c47). The unchanged packager requires clean committed source. Record the
actual admitted commit/tree and refreshed source manifest before packaging;
use separate noon-int-store, testing/D3D11/hardware OFF, explicit typed
MultiThreaded request, four tools, mandatory notice and actual PE/content
verification. Current static package, captures/outside launch, rehearsal and
candidate CI remain pending. No clean-commit/package identity is invented.

## Exact tree and delivery provenance gates

The accepted ON compilation and native runs used HEAD 7f6b0e05 (tree ba17c501),
index tree 47f2a0d4, physical freeze 627/61c83e96 and the exact 285-input map.
Reconciliation changes only these two new docs. Before commit, independently
verify all other 625 physical files and the complete 285-input map remain
byte-identical, bind the applied full 627-file/doc hashes and actual index tree,
and compare the readable two-doc delta. The applied index tree is not invented
by this external proposal.

After an authorized scoped commit, verify its actual parent is 7f6b0e05, its
tree equals that reviewed applied index tree, and its delta from 7f6b0e05 is
exactly these two new docs (seventeen adopted/new paths relative to bfbe3a1).
Record the actual clean commit/tree; ON binaries retain their pre-doc-commit
build provenance with explicit unchanged-input equivalence, rather than being
relabeled as compiled from a later documentation commit.

Only then refresh the static OFF plan's source manifest/actual commit fields.
Build its four Release tools in separate noon-int-store with explicit OFF and
MultiThreaded settings; record their new actual hashes. Package BuildInfo and
release manifest must identify that actual clean commit/tree and exact staged
content/EXEs. No package commit or result exists yet. Capture/outside launch,
fresh Release workflow/rehearsal, independent package review and exact current
candidate hosted CI remain subsequent gates. Preserve the old 8056 delivery,
initial 61c83 freeze, seventeen-path patch and all actual failure receipts.

Use supported UI controls and only actual observed frames/receipts. Report
query-console exemptions precisely, geometry and UI rendering scopes separately,
and declared dimensions separately from captured pixels. No GPU animation/full
parity/4K/universal-60Hz/clean-machine/soak claim follows from a bounded Unlit demonstration.

Stop the first unexpected result; preserve source/argv/log/PID/exit/owned files.
Do not weaken a test, retry identically, rewrite history, broaden cleanup or
introduce a production fix without a separately reviewed bounded packet.
