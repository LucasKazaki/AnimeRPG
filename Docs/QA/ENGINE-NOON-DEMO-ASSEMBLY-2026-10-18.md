# Noon demonstration assembly evidence

Owner `/root`; Task ENGINE-NOON-DEMO-ASSEMBLY-2026-10-18.
Status at2026-10-18 06:15UTC: combined8056 CPU3D package and fresh-project
workflow locally verified; complete native compatibility checks pass. Final
overall hosted/main integration remains pending. The ordered entries below
retain earlier admission/failure history; later actual results supersede their
then-pending status. Full development workflow gate remains NOT PASSED.

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

## Runtime-only native input and matrix admission

The eight-path native-player commit is c381c4049fa241fb02d23ef1e7cffa04d0e35d4c,
tree a2a8e86af2d9463ac3ac13e32eab817e6c64c568, based on creator5c9fb44. Its
whole delta EF03FF00A249F47915B1365CE61A4A6FE57A92FE9E967162A1C20666445F2CDA
matches the independently reviewed exact eight paths; production873F32E8...
and native Tool1C189024... stayed unchanged through all gates. Debug/Release
builds passed Game, old/new native players and four suites68cases each;
safety3/3 and M1-M3 passed. A separate reviewed normal device-status diagnostic
rebuilt old/new native targets in both configurations.

Root staged the exact Debug56B2153F... and ReleaseC2CC35CE... binaries and whole
Content beside each EXE in an exclusively new pilot directory containing spaces.
Four actual supervised launches passed: both configurations no-argument defaults
from unrelated cwd and both explicit cross-configuration Content roots plus the
saved imported scene. Actual marker/box/ground, console device-active and F5
reload were observed; Debug default and Release explicit maximize/restore were
also checked. All four child/wrapper exits0, reaped/noTimeout, final windows and
process inventories empty. Device availability does not establish audible
content, and informal window-title FPS is not a fixed-workload measurement.
The input QA retains exact binary hashes, intervals, PIDs and external receipts.
DraftPR103 is published and attached; packaging/native legacy smokes remain
separate combined gates.

The normal merge encountered only adjacent top-level CMake blocks. Resolution
retains the candidate's exact optional D3D11 block after the shared Runtime
helper, then adds the incoming exact shared Win32Audio block. Incoming Game and
legacy/new-player target definitions apply unchanged. The other seven paths
remain byte-identical to c381c40. Independent resolution review is required
before final merge and acceptance execution.

That independent review completed CLEAN against HEAD60a908f plus incomingc381c40:
exact eight-path staged delta A0B359EB3E3572565BE0FE7318E476F1E4A02197D2BFA182E244DD118131D608,
treef32306f9b72ff9e35f22ea635b4cff686477f138. The reviewer independently rebuilt
the combined CMake text from its candidate blocks plus incoming audio/target
edits and confirmed byte-for-line equality; seven other incoming blobs match.
Its physical CMake SHA256 is CB5FE9F933617C854EED6CC6F9F9C1EE97B2C2D4B03FC7A93631C5687A5490BB.
No source execution occurred in review. Evidence is
work/native-runtime-player-evidence/native-candidate-merge-review.receipt.json.

The reviewed external CPU runner is work/run_noon_combined_cpu.py, SHA256
66e0a72cfccea1609b9e3fde8651e919ac53e4b689b80db369771c69b97ecfc3. It binds the
final clean HEAD/tree and complete source before/after, attributable short
cache, two-job/no-node-reuse build, fresh8GiB per configuration, all targets,
exact33 deterministic suites with RuntimeSmoke/GPU excluded, safety/M1-M3 and
the16 packaging unit cases. Native and hardware execution require later slots.
No complete matrix or package result is asserted by this admission record.

## Actual combined CPU and offscreen hardware acceptance at8867

The admitted immutable revision8867bb39ee76f209c2c87b1089d0d61a9ece5439,
treec98fb623ee88668788ae920819e97db475dd96a3, passed the complete native CPU
matrix. All-target Debug/Release builds returned0 in30.234/70.531s. Both exact
inventories selected33 deterministic CTest suites, all passed in106.828/7.219s.
Seventeen self-registering engine suites totaled299 cases per configuration;
the other16 are legacy suites. Safety3/3 passed in5.726s, milestones1–3 each0,
and the then-current16 packaging controls passed. Fresh RAM was12.791/12.778/
12.561GiB before configure/Debug/Release. All23 bounded recorded commands
returned0/reaped without timeout. Full615-file/56-directory before/after source,
HEAD/tree and clean status matched; canonical binding SHA256
ce6115ca75bdc574e37234c81a822652c1ea2973303c33ea0dab456fee212037.
Exact commands, actual exits, cache and binary hashes are retained under
work/noon-combined-cpu-first. No interactive/RuntimeSmoke/GPU action ran there.

Root then independently read and executed reviewed offscreen hardware runner
deed2bb608f1b7fbdf9d90f4cd401cff8da1ab46cfe3235b9f57701a700494a5
in a separate exclusive slot. Same source8867 stayed clean and byte-identical.
Both configurations passed anchored D3D11 ten +Graphics sixteen cases,2/2 CTests
in31.640/7.672s, including13 invalid comparison children exiting3 and2 finite
controls exiting0. Both affected target builds returned0 in3.047/3.140s.
Actual RTX5080 vendor10de/device2c02/LUID73259/featureb000/debugLayer1 was used.
All unchanged coverage/RGB/depth/recovery/resource/20-cycle budgets passed.

For the explicitly tiny fixed720p workload (2draws,28vertices,14triangles,
1texture,30warm/120measured), Debug/Release GPU p95 was0.020512/0.019648ms;
synchronous total including CPU upload/readback was69.6256/17.3454ms against the
existing100ms bound. Release prepare/upload p95=2.6243ms, completion/readback
p95=14.8589ms. Private bytes at the fixed run were219062272/225542144; logical
count11/40553400bytes. Release DXGI local usage56193024/budget15947792384.
These are reported sample aggregates, not exported per-frame samples or a
general-game/60Hz/native-presentation result. Driver live-object baseline and
physical device-removal behavior remain unverified by this offscreen pass.

Evidence work/noon-combined-hardware-first retains exact command/source/cache/
binary/memory metadata, verbose logs and14 actual PNGs. Root visually inspected
the7 distinct Debug images (CPU/GPU/difference/IDs and3 Graphics references);
the matching7 Release files are byte-identical by actual SHA256 comparison.
Geometry, texture/depth and sparse amplified difference output were coherent.
Every recorded command returned0/reaped without timeout; final owned process
query was empty. The reused cache deliberately now has hardware tests ON, so
subsequent CPU admission must turn that option OFF or exclude the GPU label.

## Actual first package blocker and accepted narrow follow-ups

The real static Release four-tool package attempt at8867 configured0/3.359s and
built all four tools0/57.594s. The actual dumpbin command0 emitted dependency
names followed by SECTION HEADER #3 before Summary; the strict combined-output
parser rejected it. Overall child1/reaped/noTimeout61.953s is retained in
work/noon-package-supervision-first and work/noon-package-first. No destination
was reserved. Root preserved this failure and captured8 separate actual headers/
dependents reports, each0/reaped with unchanged EXE hashes, under
work/noon-package-pe-format-first. A corrected real package has not run yet.

The independently verified editor reopen86cd91a normal-merged as034009c without
conflicts after the8867 hardware source binding completed. Exactly its4 paths
are adopted: two current-Error guards, six meaningful original/recovery fixtures
and Task/QA. Four safe old failures/two controls preceded production. Component
D/R five targets and four suites all passed (Editor30), safety3/M1–3; exact full
patch71a3b7c711ed3485d8e02241a85007b3be86a86fdcf1c427b8b37c0fb5099286.
DraftPR104 is attached. The combined editor and real package still need the
subsequent source-bound checks; earlier8867 acceptance does not validate new code.

The separate four-path PE-format correction was independently reviewed against
actual output. Its OLD unit reproduced the sameSECTION failure; final19 controls,
safety3 and M1–3 passed with the parser/allowlist unchanged. The only production
change makes two real dumpbin calls and concatenates headers before dependents.
Root is integrating that accepted packet before the next real package attempt.
No mocked fixture constitutes a real package or native acceptance. Validation
staging is C:/AI/astral-demo; accepted delivery copies may later go under session
outputs with the exact manifest preserved. Full workflow remains NOT PASSED.

## Actual combined8056 package and complete CPU workflow

The PE-format input12cdbc05dd3a6513a80166a78ada6a7c24856f04 normal-merged
without conflicts as8056ff6a6d04589bffbac84120365c271f2e9e85, tree
7758351dfb56415ea945693db01847a74738226d. This revision includes editor-reopen86
and all earlier reviewed inputs. No Host, native Tool, GPU or CMake code changed
between the fully accepted8867 hardware run and8056; the changed production is
the two editor cached-current-error guards and two actual PE commands. The
previous failed first real package and safe old failures remain retained.

Final all-target Debug/Release builds passed in4.578/4.875s. Both exact33-suite
deterministic CTest inventories passed in72.266/6.047s (CTest72.24/6.01s):
17engine suites305cases each, including Editor30, plus16legacy suites.
Safety3/3, packaging19/19 with zero skips and milestones1-3 all passed. All23
recorded children returned0/reaped/noTimeout. Available RAM before configure/
Debug/Release was12.637/12.659/12.562GiB. Complete619physical files/56directories,
HEAD/tree and clean status matched before/after. Canonical binding SHA256 is
9057c0afa722aecc85dc32528aaad2b9e00fb1bfbb69bb16942e53df351ff7a1.
Evidence: work/noon-combined-cpu-8056-first, reviewed external runner00c6b134
(one expected16-to19 unit-count change from original66e0a72). Hardware tests
were explicitly OFF for this separate CPU admission. The original8867 hardware
evidence is unchanged and its GPU/Host/Tool sources match this8056 production.

The corrected actual static Release package ran05:46:17.447774-
05:46:24.744329Z, child0/reaped/noTimeout7.297s. All four static CRT/x64 tools
have only the actual checked Windows system DLL imports.14sealed files were
byte-verified; raw separate headers/dependents reports and source/toolchain/
memory/EXE hashes are in work/noon-package-fixed-first and
work/noon-package-supervision-fixed-first. The copied source notice is factual
original-fixture provenance, not an invented redistribution license.

Exact static binary hashes:

- Editor da8b4e7cfeb8015538b501cbd4a206d7093d9d734cb6a5bbbf085aeeca3ca6e7.
- Headless cc6a0b18f59d91ad96d9f6fcb5038ec80e6ce0289a088eef205cde1c4cb96760.
- Native player4d03df3d6b7e3457af942615436566702c52409f4d4a0e6809def140bae09227.
- Creator31ff45b9d37b72d63743c333f51ff4733acf0a9545701b4221601096c7fcad4f.

Root's actual no-argument static native launch from unrelated SESSION cwd ran
05:50:39.195360-05:51:43.721543Z/PID3864, child0/reaped/noTimeout64.531s.
Root observed the original blue/orange marker, Box/ground, audio-device-active
console, F5 reload, maximize4K/restore and normal close. A separate headless
120frames/320x180 run passed with BoxY0.490; root inspected the real PNG. The
first headless attempt returned3 because the capture directory did not exist;
it is retained in work/noon-package-headless-first. Creating the output folder
before the corrected run repaired setup, without altering production.

The full fresh-project rehearsal used only the four packaged tools and the
original glTF/bin, in a new owned NoonLive-01 project. The creator returned0.
Actual editorPID24812 ran05:52:38.179992-05:54:54.937014Z,0/reaped136.750s.
Root instantiated the Assets model, framed/selected it, changed X1.5-to2,
observed global Undo1.5 and Redo2 after leaving the field, saved startup main,
saved demo.scene.json via SaveAs, reopened that named file and observed X2.
Play showed the model/Box/ground; Stop restored authoring and close returned0.
Startup and demo JSON hashes both equal
b1c468039c825019d48dba376a00ade32106a2a578461ba3fddf82922b46c7a9.
The separate120frame capture passed and was visually inspected. A new real
source packaging command returned0/reaped4.406s05:55:13-05:55:17Z, producing
NoonLivePackage-01. Its outside-directory no-argument nativePID56580 ran
05:55:23.700570-05:56:19.879525Z,0/reaped/noTimeout56.188s, with correct
Astral Noon Live title/content, F5, maximize/restore and normal close. All exact
commands/receipts are in work/noon-rehearsal-first,
work/noon-rehearsal-package-first and noon-rehearsal-package-supervision-first.

New delivery outputs/astral-noon-cpu-8056ff6 contains the12-file primary Package,
14-file RecoveryPackage, separate verification/addendum and reviewed walkthrough.
Both copies include unchanged release manifests and byte-identical complete
inventories. The3,088,755-byte ZIP SHA256 is
9b6a21790ceefce0c3a7ec938252ae8eb5166c6c1c73070b3c567a3ee4f024aa;
each ZIP entry was read back and compared with the primary package. The actual
relocated primary player also ran from unrelated C:/AI/astral-demo cwd:
06:01:18.048451-06:01:41.531833Z/PID53076,0/reaped/noTimeout23.484s. Root
observed the correct title and model/Box/ground and closed normally. Both package
manifests were reverified afterwards. No nonexistent screenshot file is claimed;
actual native screenshots are in the task conversation. WALKTHROUGH SHA256
67016c98d20a35a64d9c0a3a8ad78f4e291c3c01cb45ed71f7e2dfde206470b3;
its seven PowerShell blocks parsed without execution and sealed folders stayed
unchanged. The walkthrough uses fresh02names, explicit capture-directory creation
and the same factual notice argument. Its approximate seven minutes is a human
presentation estimate, not a measured human-live timing promise.

Scoped PE PR105 is draft/attached against frozen8867. Both Windows head/merge
jobs passed. Root downloaded actual merge job117600238096/run39102081138:
checked-out71b3d2f20f943cb0d0d1f4526574e1d818840efc has ordered8867/12cdbc
parents and treefdfcabe474c09839168ce03d97d8cff1fd7f75c2, exactly the reviewed
PE head tree. Do not confuse the setup action's own f3dc54 commit with the
repository checkout. Editor PR104's seven hosted checks also passed. These
scoped jobs do not substitute for the final overall candidate/main jobs.

The sealed BuildInfo and manifest helper flags describe staging-time acceptance;
later native/CPU proof is recorded separately, never by editing the seal. This
is a locally verified CPU3D workflow. General GPU presentation is still a
separate15-path implementation; no2D/rigged-authoring/full-parity, clean-machine
or24hour-soak completion is asserted. Overall development workflow gate remains
NOT PASSED. Remaining final integration/native checks are recorded separately.


## Retained native compatibility on the same8056 source

The separate direct-harness runner ef6e31ddfe5a191b02fddaa8092d70abe6e1a6cad5d6bfb891627e899741fc2b
ran all six unchanged M4/M5/M7/M8/M9/M10 native executables in each configuration.
All12 returned0/reaped/noTimeout and reported their real controls plus Clean
Escape0. This is12 direct harness passes, not12 registered CTest executions:
only the harness working-directory argument changed to new external empty roots
so astral.log could not alter the frozen source. Default procedural GDI uses
no external assets; each root contains only its generated log. Memory before
every harness was12.358-12.695GiB. All44 recorded helper children returned0,
reaped/noTimeout. Source619files/56directories/cleanHEAD/tree,14 native EXEs and
cache matched before/after/final. Corrected exact-build process queries were
empty before, after every harness and finally. Root found and fixed the runner's
double-separator inventory prefix before execution; retained pure-string actual-
path positive/sibling negative control proves its revised matching. Evidence:
work/noon-runtime-smokes-8056-first; completion06:07:28.923236Z.

Root additionally inspected existing application/player behavior in real windows.
Debug AstralGame's original GDI scene ran normally and closed0. Existing Astral
3D presentation was explicitly enabled using its existing ASTRAL_RENDER_MODE
environment option, then visually inspected in Debug and Release and closed
normally0/reaped/noTimeout (59.688/82.844s). Both actual logs report that opt-in.
Two short Sky F2 inputs did not visibly switch the initial Debug app; one short
Release F2 did not visibly return to GDI. These observations are retained; a
subframe polling/input-tool timing hypothesis remains unproven. No F2 switching
acceptance or new production input fix is claimed from these runs.

The retained legacy AstralPlayerWin32 actually loaded its original Playground
project in both configurations, showing35entities and the existing scene/HUD.
Root opened the console and observed Audio output device active, pressedF5 and
observed a second load plus the reset sample timer, then closed normally. Debug
PID44524 ran06:12:28.373856-06:13:15.143483Z,0/reaped/noTimeout46.781s;
ReleasePID40712 ran06:13:15.629452-06:14:12.530207Z,0/reaped56.890s.
Exact EXE hashes/arguments/logs/receipts are in work/noon-legacy-native-first.
Final native window and owned process inventories are empty. Root then checked
all619physical source files/56directories against the complete8056 CPU proof,
plus all53 EXE/LIB outputs per configuration: every hash and byte count matches.
The two documentation paths updated after that binding do not change compiled
production/test/workflow inputs. No audible-content or title-FPS performance
claim is made. The frozen static CPU delivery remains unchanged.
