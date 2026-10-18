# Imported animation binding QA

Status: local Debug/Release CPU and actual native checks passed at commit
ad30857e6bc63a2c9472a88c262dd2590279b69a. PR107 hosted Windows head/merge
checks passed; five portable builds failed a test-only partial AnimState
initializer under warnings-as-errors. The minimal correction has now passed
local MSVC Framework Debug/Release acceptance, independently verified below.
Portable compiler/sanitizer CI for the corrected head remains pending; the PR
head is unchanged. Production/native binaries stay exact; only the two
Framework test EXEs were rebuilt and their old versions preserved. Earlier
findings and failures remain retained; none is relabeled as a pass.
Base `8056ff6a6d04589bffbac84120365c271f2e9e85`, worktree,
branch and exact eleven-path ownership are recorded in the matching Task.
The sealed CPU demonstration remains separate and unchanged.

## Actual OLD evidence

The coordinator reviewed and granted exactly the frozen `run-old.py` SHA256
`6659da1b5840d39b8b67d23148affcf8d609c3358dd0e3727abcfaab0f0bcb3b`.
Evidence root: `work/imported-animation-binding-evidence/old-first`.
Production was byte-identical to base. The physical test source SHA256 was
`a3d7ecf885a404aa49ddade5bcdacd6102409bff246a21da14adebd96ba8352b`.

| Actual child | PID | Exit | Seconds |
| --- | ---: | ---: | ---: |
| Configure | 58076 | 0 | 3.859 |
| Debug EngineFrameworkTests build | 56528 | 0 | 34.891 |
| ImportedAnimationFixtureConvertsWithIndependentCoordinates | 54696 | 0 | 0.047 |
| ImportedModelSceneBindsEveryPrimitiveAndOwnsPlayback | 44148 | 3 | 0.031 |

The scene child failed at the reviewed first safe presence assertion:
`EngineFrameworkTests.cpp:1454: CHECK failed: bound != nullptr`, before missing
binding access. The passing conversion control exercised the existing importer,
BuildSkeleton and BuildClip against independent coordinates. The later scene
runtime assertions were not reached on OLD. All four children were reaped,
without timeout, between 06:27:27.333638 and 06:28:06.531843 UTC on 2026-10-18.
Fresh memory gates measured 12.6347 and 12.5905 GiB. Cleanup recorded no relevant
owned processes at 06:28:24.6825381 UTC. No native/GPU launch occurred.

The complete 283 Engine/Game/Tests/Tools/cmake/root-CMake build-input binding
was equal before/after; its JSON SHA256 is
`9a55c70a4465f79937300ad9e338afe6c0ade38f12298d77b38124b577d135cc`.
Docs were separately scope-bound. This does not cover every checkout file.
The preserved OLD executable is 5,777,408 bytes, SHA256
`e6a52959bcd11fa9a78643c78a7494a55d2cc81b227191416170e4aab8d66092`.

The failed private fixture and its six exact original/`.initial` files were
retained; exclusive evidence copies and their inventory are under `old-first`.
The successful conversion control removed only its ledgered files and empty
private root. Frozen `fixture-source-proof.json` SHA256:
`5f59a5d3385bc6152867d1c9795553a70f7ea357f5ae1e5f2095b9e9bab6ea25`.

| Original payload | Bytes | SHA256 |
| --- | ---: | --- |
| TwoJointRibbon.gltf | 3457 | cc77ac31625e377b9b88de2476e1b133aa012b9cd1b74be085d2a387c89524f6 |
| TwoJointRibbon.bin | 492 | 93d38e6637d4c445a1bce11140ea760f55fff74fd69fc908582af20f33bf2e1d |
| two-joint.scene.json | 301 | e0a5737eb48b660cb8a32f308095306049db85ec495e6b91fa21bcd0f358343b |

## Implemented correction

Coordinator accepted concrete design SHA256
`b680dd21ebbea748d4dca3303938b3a8ef5e8ad18c4396cdfcebc910b7879b90`,
then refined it to require affine meshBind and unnamed automatic-first clips.
Implementation retains immutable rig/clip/machine owners, remaps skin slots,
applies each mesh-node inverse once, and gives every primitive its own mutable
Animator/output. Constructor-pose skinning publishes geometry without Tick,
audio/particle starts or behaviour OnStart. Code-installed animation is kept.

Authored model-root Animator controls clip/speed; flattened controllers retain
strict source metadata and primitive bindings. An explicit name requires one
nonempty unique match. Automatic selection accepts an unnamed or ambiguously
named first clip and retains empty automatic semantics when flattened, for all
clip names. Only an explicit validated name is pinned. No durable identity across DCC reindexing or
automatic clip reorder is claimed. Direct C++ clip-string mutation is not a
live rebinding API. Imported root motion, deforming mesh collision, static LOD
and non-joint selected channels reject.

Editor model-only reload stages a complete parse/initial preview before swap.
It keeps authored JSON, saved bytes, selection, camera, dirty state and both
history stacks on resource-only success, and keeps the entire last-good preview
on failure. Attempt watches acknowledge failures once and retain failed new
dependencies for repair. Dirty authored JSON remains the source if its external
scene file is broken. Existing named Host reload is exercised without changing
Host; unnamed Play hot reload is outside this packet.

## Reviewed regression matrix, passed in Debug and Release

Original OLD payloads and both OLD case bodies remain unchanged. Additional
tests use owned temporary roots/files with checked normal-completion cleanup;
aborted cases retain evidence. The editor copy of the original payloads must
match the frozen proof. New variant recipes/byte expectations are external
review metadata; no new runnable fixture was generated during source work.
Frozen `fixed-fixture-source-proof.json` SHA256:
`0ef3f6f96319995d847d4858f7de4e44164f373a42a1fcd6e204571259342350`.
It confirms the whole original fixture/helper/two-case block is byte-equivalent
after LF normalization, SHA256
`7df02a0c1cbc5f9109d1459b0587f44712aa24b78988b713cf00df397e9ef1fe`.
The 620-byte two-skin binary expectation is
`ba174766ef3a10e27c5029670f5b49ec7565088203a286d06f2f9cf85edcff42`;
the 492-byte initial-Raise-Y2 expectation is
`f8e34472517ff521b5fcdbbbc5c57102a2d00fe5d8bb546dda8d89eb81978108`.

| Reviewed case | Verified contract |
| --- | --- |
| Original conversion / scene cases | Literal coordinates, reversed slots, distinct inverses, eight parts/two speeds, independent buffers, cached bind immutability and document/cache-release lifetime. |
| ImportedAnimationInitialPosePreservesBehaviourLifecycle | Correct zero-time render geometry; unchanged clocks; no play-on-start systems or OnStart before caller Tick. |
| ImportedAnimationFlattenedControllerRoundTrips | No duplicate expansion; retained controller and original affine frames; independent Bend/Raise after save/reopen. |
| ImportedAnimationBindingValidationIsTransactional | Reject invalid/fractional indices, roles, references, overrides, matrix size/projective/singular/out-of-Float32 values, root motion, LOD/collision and orphan; retain output sentinel. |
| ImportedAnimationNamedAndUnnamedClipSelection | Named reorder works; removal/ambiguity rejects; unnamed auto-first survives flattened round trip; non-joint channels reject. |
| ImportedAnimationTwoSkinsStayIndependent | Second reversed-slot skin has independent joints/IBMs; untargeted second rig remains bound at independently expected X+2. |
| ImportedAnimationNestedModelsUseNearestController | Same-source nested Bend controller is independent; nested static opt-out remains static. |
| EditorImportedAnimationHistorySaveReopenAndPlay | Inspector clip/speed, Undo/Redo, SaveAs/reopen and three Play/Stop cycles retain authored state and motion. |
| EditorImportedAnimationModelOnlyReloadPreservesDirtyState | Bin-only replacement changes initial pose, retains dirty JSON, saved bytes, selection/camera and both histories. |
| EditorImportedAnimationFailedDependencyRecoversOnce | Clean/dirty/untitled scenes retain complete last-good state on bin/syntax failure, acknowledge once and recover on repair. |
| EditorImportedAnimationNewFailedDiskDependencyRecovers | Newly missing model is watched and recovers without another JSON edit. |
| EditorImportedAnimationDirtyExternalJsonDoesNotBlockResourceReload | Current dirty authored JSON reloads a model despite broken external JSON. |
| EditorImportedAnimationFlatControllerInspector | Managed primitive Animator hidden; edited child-frame offset survives SaveAs/reopen. |
| EditorImportedAnimationNamedHostReloadIsAtomic | Existing named Host reload retains last-good world on bin failure and recovers on repair. |

Independent full-source/variant review preceded separately granted fixed
Debug/Release Framework/Editor/Assets/Animation/Sample/Layout and safety/M1–3
checks, followed by actual native original-fixture checks. All passed in the
final attempts recorded below. Final documentation review remains pending.
No importer, Animation library, Host, Tool, CMake, D3D, renderer or Game source
was changed.

## Additional normal witness and prepared correction

Archived source packet aad7b479 / manifest bd88880c received coordinator and
independent source review, receipt SHA256
`1af95f98b6b7abc9f22af57ed784d9fc9beb743467d9dd39d04510884e69c2eb`.
The coordinator then found that the imported helper's ordinary vector normal
transform disagrees with the renderer's inverse-transpose for nonuniform mesh
frames. The actual bounded witness below subsequently reproduced this issue.

The newly prepared `ImportedAnimationNonuniformMeshNormalMatchesWorld` uses
the original fixture, adding only BodyOne.scale [2,3,4] and replacing both
primitives' normals with raw glTF (-2,3,6)/7, engine (2,3,6)/7. The existing
position expectations remain. Its first intended OLD rejection is the safe
world-normal equality against (-3,2,6)/7. The later local equality is against
normalize(4,9,24). Original OLD2 fixtures and case bodies are unchanged.

Prepared `ImportedAnimationHelperNormalBlendHasDocumentFallback` independently
expects world normalize(2,3,6) for identity and singular document-space fallback,
and normalize(10,21,48) for .25/.75 influences with P0=diag(1,2,1),
P1=diag(2,1,1). With B=diag(2,3,4), corresponding local directions are
normalize(4,9,24) and normalize(20,63,192). Position outputs are checked too.
This is the declared per-influence inverse-transpose normal blend.

Production was byte-identical to the archived reviewed packet during that
witness, including its defective imported helper. The literal source/proof and
bounded runner were independently reviewed before the exclusive grant.
Prepared normal proof SHA256
`508627e5153622f6313e83f33dd5171e4153b407397754e20f9ae665ac65fcb2`:
3473-byte model `945f0bcb3984ea0b2d077c93c7eed04541d8fc12096ca326dada814668dcba3d`,
492-byte buffer `19d24a231ec2cc1386a8e440c1ebbb9232918b183c398b74332567b92e2433c0`,
unchanged 301-byte scene. First intended numeric assertion is line1750,
`Near(worldNormal, expectedWorldNormal, 2e-4f)`. The source-only proof confirms all previously reviewed
Framework cases/helpers are unchanged apart from an explicit include.

Actual partial-implementation OLD-normal evidence is under
`work/imported-animation-binding-evidence/normal-old-third`. This is separate
from the original 8056 missing-binding OLD evidence above. Reviewed runner
SHA256 `d49dbe9f6d532758d58ab5cd15f1fb0920a6ab9813e2c2ddcd1877b1d782e2fc`:

| Actual child | PID | Exit | Seconds |
| --- | ---: | ---: | ---: |
| Configure | 41740 | 0 | 3.813 |
| Debug Framework build | 47412 | 0 | 36.640 |
| ImportedAnimationNonuniformMeshNormalMatchesWorld | 33356 | 3 | 0.063 |

The exact intended line1750 numeric assertion failed after safe binding,
finite-normal and unchanged-position checks. Actual world normal was
(-0.470588118, 0.705882370, 0.529411912), local normal
(0.485071301, 0.485071093, 0.727606952); required world was (-3,2,6)/7.
The child ran 07:33:00.023238–07:33:00.088255 UTC on 2026-10-18. Every helper
child was reaped without timeout. Fresh memory gates were 12.5033 and 12.5402
GiB. All six current/original `.initial` fixture files matched the literal
proof and were retained with exclusive evidence copies. The preserved binary
is 6,059,008 bytes, SHA256
`28e2ce8a4f1d562c52390fb483b012be19d50c8dcdeebadbb3f467deaf9a929c`.
283 build-input before/after JSON SHA256
`f9620df114e23bb3ac4f1da55a53500086e513d3175a86fa0dddd5867bac31bf`
and eleven-scope `719171e075fbf446a5cabeefaa428cb372a588642bfe49791b3e9bb5546ffe5f`
were unchanged.
Final cache hash `454bb3dace19328b1793caacc901307e09ccb83ac2cc44d3cf568aff7d6d842f`
matched configured cache. Fresh 07:33:32.1710841 UTC read-only cleanup observed
all 22 receipt PIDs plus the final query console absent, and zero build,
compiler, CTest or engine processes. No UI/GPU launch occurred.

Earlier `normal-old-first` stopped on inventory-output parsing before configure;
`normal-old-second` configured successfully then stopped on a runner cache-type
check before Framework build. Both failed harness attempts and the original
cache are preserved; neither ran an engine regression test.

The coordinator accepted the actual normal regression and authorized only the
existing Framework imported helper correction plus Task/QA. It now preserves
document-space P matrices for inverse-transpose normal blending, applies B^T
before the renderer's (W B)^-T, and defines document-identity normal fallback
for singular/nonfinite inverses. Double intermediates and scaled normalization
keep usable directions finite/nonzero; failed blends fall back to bind normal,
then a document up direction through the same mesh basis. Position skinning,
the Animation library and the legacy animation path are unchanged. Every OLD,
normal and other prepared case/fixture remains byte-identical to its reviewed
source. The corrected helper was uncompiled at that source freeze; its later
Debug compilation and unnamed-selector failure are recorded below.
Fixed D/R and native gates were pending at that historical source freeze.
Normal-corrected review metadata `normal-corrected-fixture-source-proof.json`
SHA256 `3063b53534ad4926358030eb862a9d2c1277387a754ee1e067e2f0d4fb49be9e`
binds both unchanged test sources, all existing literal payloads/expectations,
the actual partial OLD-normal receipt and the corrected helper source. The
matrix contains 17 imported cases and 18 private fixture roots per configuration:
nine Framework roots (the direct helper adds none) plus nine Editor roots.

## First corrected matrix: unnamed automatic selector

Reviewed runner318d5dfa configured (PID58196 exit 0, 4.031s) and built all seven
Debug targets (PID17572 exit 0, 49.515s). CTest PID57100 exited8 after5.594s:
Layout, Animation and Assets passed; Framework child rejected at unchanged
line1630 in ImportedAnimationNamedAndUnnamedClipSelection, requiring flattened
InstanceA.clip to remain empty. Existing legacy Animator, original OLD2, initial
pose/lifetime, flattened-controller and binding-validation cases passed first.
Editor/Sample, later Framework normal cases, Release and safety/M1–3 were not
reached. Evidence `work/imported-animation-binding-evidence/fixed-first` is
retained; this is not a full matrix pass or fixed normal numeric acceptance.

Actual retained first animation has no source name. Gltf.cpp796 supplies
animation0 for display, which the initial binding implementation mistakenly
pinned. Coordinator authorized preserving the requested selector directly:
empty always remains automatic-first, while a validated explicit unique name
remains pinned. No importer or additional policy change; every test/fixture
byte remains unchanged. This Scene.cpp correction subsequently passed the
fixed-second and native gates recorded below.

All seven Debug binaries were exclusively copied and hash-verified before any
rebuild under fixed-first/preserved-Debug-seven. Preservation manifest SHA256
`77061839e5ebbc206e5aaa0a9155ececdb0a7256f990572eff2f9ae5d359df32`;
Framework executable `3514b6bc3b5b08d263e7d87d419216427050b857d758057123db6224bd39168a`,
editor `dc2c1dc67615013d733715497c99eb5c6f197b0e22344300d4809416ec510655`.
The failed six-file fixture/current-original inventory is preserved separately.
Full283 before/after hash59d8185b and eleven-scope3d2e21e5 were identical;
configured/final cache hash
`02f26aee2ffcc86313f26c9d34fc54866e92496a63e30029004ab6f97c877dbb` matched.
All helper children were reaped without timeout. Fresh read-only cleanup at
07:46:22.6328794 UTC observed zero owned or build/compiler/CTest/engine processes.
No native/GPU launch occurred.

The historical next gate was a separately reviewed incremental Debug/Release runner with new
fixed-second evidence, exact owned-cache/options/source/base validation, pinned
first-failure receipts and all seven original/preserved binary checks. No
configure was planned. It retained six anchored suites, 17 imported cases,
18 private fixture roots per configuration and safety/M1–3. All native gates
were still pending at that point.
Selector-corrected proof `selector-corrected-fixture-source-proof.json` SHA256
`f76bfe5d98c86430288df0d5c19ae39e44a2825bea0f62b3a1295dfdedcdf4ce`
binds unchanged test/fixture sources, the Scene-only delta and prior failed
matrix/preserved binaries. That proof is source metadata; actual results follow.

## Final affected CPU acceptance

Reviewed incremental runner SHA256
`44b7995fb12600475c9ef7c0513ee5b36e512884b25881da1ae3427816a59930`
used new `work/imported-animation-binding-evidence/fixed-second`, the exact
owned fixed-first cache, no configure, fresh >=8 GiB before each configuration,
max two jobs and no MSBuild node reuse. Actual results:

| Child group | Exit | Seconds |
| --- | ---: | ---: |
| Debug seven-target build | 0 | 7.750 |
| Debug six anchored suites | 0 | 10.844 |
| Release seven-target build | 0 | 59.984 |
| Release six anchored suites | 0 | 1.828 |
| Safety checks | 0 | 5.328 |
| M1 / M2 / M3 checks | 0 / 0 / 0 | See individual receipts |

Framework, Editor, Assets, Animation, Sample and Layout all passed. Each
configuration ran 137 engine cases plus Layout, including all 17 unique
imported-animation cases; all 18 private fixture roots were cleaned per build.
Framework had 31 cases and Editor 37. Every one of the 40 supervised children
exited 0, was reaped and had no timeout. Fresh memory was 12.5206 GiB before
Debug and 12.3487 before Release. Final process observation at
07:57:43.0948734 UTC found all 40 receipt PIDs plus final query console absent
and zero relevant engine/compiler/CTest processes.

The corrected normal witness produced world direction
(-0.428571373,0.285714298,0.857142866), matching (-3,2,6)/7; direct identity,
two-influence blend and singular-fallback controls passed. The unnamed-auto
round trip, all rejection cases and editor reload/history/lifetime cases passed.
Original OLD2/normal/test/fixture bytes were unchanged throughout the retry.

All 283 Engine/Game/Tests/Tools/cmake/root-CMake build inputs matched before,
after and current physical files. Binding JSON SHA256:
`fcee08ad5a586dc9c37ce44bc9e4c1905b6b189cd8e147117787c62406e46ac9`.
The eleven-file compiled freeze manifest is SHA256
`2b40fe673a65035c65435c0cfcbf21b90537b34aa27191de0a77d60d2a61dcb9`;
docs are separately bound. Fourteen final binaries were rehashed, the exact
cache remained `02f26aee2ffcc86313f26c9d34fc54866e92496a63e30029004ab6f97c877dbb`,
and all seven preserved prior Debug binaries/failure evidence stayed exact.
Root independently verified the actual child receipts, case/root arithmetic,
final binaries and source. Its acceptance receipt SHA256 is
`bda6844c845b47a796e96e9cc8a4c960763f38ddb839306d6a76c1ed99097bcf`.

## Actual first native failure and camera-only correction

The first Debug editor PID 47344 ran 08:05:30.693464–08:08:42.640064 UTC,
exit 0/reaped/no timeout, 191.938s. Root used real Assets import, set the authored
model at origin, added Animator, set speed 0.5 and framed the selection. Only
bounds/grid were visible in authoring, and Play showed sky without filled RGB
geometry. The sequence stopped, saved its owned diagnostic document and closed
normally. No motion/UndoRedo/SaveAs/reopen/Release acceptance came from this run.
Evidence `native-debug-first/root-native-blank-view-observation.json` SHA256:
`929edc4626b30738f0a31a202132cb260b686446a71e32d5d5946c3007bf7c8b`;
owned diagnostic scene SHA256:
`5e6bb45f7247ce9ff7c9dd3e4881395d34b4c3534ff39bc312c60e98cdbf5b07`.
Both remain retained and unchanged.

Source diagnosis corrected the initial plan's front-face assumption: FlipX plus
importer index reversal converts raw 0,2,1 to engine 0,1,2. Both zero-pose skinned
triangles have geometric +Z normals and unlit, single-sided materials. The old
runtime camera [1,4,-7] looking +Z sees their backs. Source-derived 320×180 signed
fixed16 areas are −388962; proposed camera [1,4,7]/yaw 180 gives +388962, with both
skinned triangles inside both frusta. The rigid control is horizontally outside
that runtime camera. The independent proof was reviewed against importer,
renderer, projection and rasterizer source; proof SHA256
`f716faf90504503736ec9be1e48f4f4e39b6645baeb0d905e525fb59f8c5775b`.
No engine/render test execution was used to calculate that proof.

The new exclusively staged fixture root is
`C:/AI/astral-demo/Imported Animation Front Camera 20261018/Content`.
All three original payloads and project bytes stayed unchanged; only the
camera-only startup metadata changed Z−7→+7 and added Euler [0,180,0]. No model
or Animator was pre-authored in that startup scene. No geometry, material,
winding, joint or clip edits were made. The original reference scene remains
separate and exact. New plan SHA256:
`71fe88eb2e1692ed42f8c6233ee9c947d907f7d8fe200d9eb57059fe6a71c850`.

The private editor camera is independent of the authored runtime camera and
Frame Selection does not orbit it. View has no Front/Back/align command. The
reviewed supported preview check temporarily sets the authored model yaw 180,
frames selection to see filled faces, then focuses Outliner and Undo restores
yaw 0 before Save/Play. This changes only an ordinary authored transform during
the check; no temporary rotation was saved, no asset changed and no unsupported
mouse chord or test hook was used.

## Actual corrected native Debug and Release

Both editors used the CPU renderer, original unlit verification assets and the
owned 900-second supervisor from cwd `C:/AI/astral-demo`. Exact binary bindings:

| Editor | Bytes | SHA256 |
| --- | ---: | --- |
| Debug | 7332864 | 69312b5317c12f4002f3d58db5424f0a926ef519ec69b0f0e1b959976e0b2bbc |
| Release | 1886720 | c4e1e0b627f6dd41a989f037feebf644054d6d6b12ff9c59a70179c2d42e7d2c |

| Actual child | PID | UTC start–finish, 2026-10-18 | Exit | Seconds |
| --- | ---: | --- | ---: | ---: |
| Debug front native | 43032 | 08:26:52.817857–08:34:35.869654 | 0 | 463.047 |
| Release front native | 22560 | 08:35:34.796946–08:37:18.337736 | 0 | 103.531 |

Both children were reaped, with no timeout. Root's actual Debug observations:
Assets scroll/double-click created the original model; native Inspector edits
set origin and added Animator empty clip/speed 1→0.5. Temporary yaw 180 plus Frame
Selection showed filled blue/orange and rigid blue geometry; Outliner Undo
restored yaw 0. Play showed distinct Raise frames with vertical tip motion; Stop
restored authored state. Clip Bend and speed 0.25 each passed scene Undo/Redo.
Save produced a clean document. Save As opened a real picker; Escape retained
state, the second Save As wrote a new owned file, and Open Scene reopened it.
Two more Play/Stop cycles showed distinct horizontal Bend deformation, including
another real reopen. Final state was clean, model origin/rotation 0, Bend/0.25.

Cached file-dialog UIA indices failed twice; current-screenshot coordinate
fallback verified the filename/caret and succeeded. A root-entered POSIX-slash
relative filename was rejected by the Windows picker; dismissing it and using
native backslashes opened successfully. These retained tool/test-entry issues
are not attributed to the engine.

Release loaded the saved scene, verified model origin/rotation 0/scale 1 and
Bend/0.25, repeated front-preview rotation/Frame/Undo, and visibly played Bend
across two distinct frames. Stop restored authored state and clean title, then
normal Alt+F4 closed. Final saved bytes were unchanged. Root observed these
native images directly; screenshots remain in root's tool transcript. No saved
PNG files or pixel-processing measurements are claimed.

Saved startup and `scenes/animation-demonstration.scene.json` are byte-identical,
SHA256 `cbfcb70c86c63da71057d594bfec955592636e5e0ff1b0d214fd12d08137c611`.
Exact parsed JSON retains original model reference, origin, default identity
rotation, Animator Bend/speed 0.25 and camera [1,4,7]/yaw 180/FOV 50/near 0.1/far 100/
priority 1. No temporary preview rotation remains. All six owned Content files
are inventoried; original glTF/bin/reference scene/project bytes stayed exact
in both first and corrected roots. Fresh read-only verification at
08:38:04.6460199 UTC found 22560/43032/47344 absent and zero relevant engine,
test, compiler, CTest or CMake processes. Root's fresh window lists contained no owned editor windows.

External post-Debug check receipt SHA256:
`401bdb812880b6d4e3a2be2d06e6fb1e94b65c04355feb8b30ebeb47400588b4`.
Post-Release check receipt SHA256:
`a37eef33d08e1dc3aeda851c20a9cb12982b117dadfce4e4020b099a1dec7f0d`.
Combined CPU/native acceptance `native-front-combined-acceptance.json` SHA256:
`601cb6da634f4be379326e60669d3213004daf78eeac8c40938e0fee82392190`.
Final verification matched all 283 inputs, all 11 compiled-scope hashes and both
editor binaries before this Task/QA-only reconciliation. Production/test bytes
remain exact to that compiled/native-tested freeze; only these two docs change.

## Coverage and limitations

This is CPU-renderer imported-animation authoring/playback support with original
unlit verification geometry. It does not establish GPU animation, shaded visual
normal correctness, broad DCC parity or an already rebuilt combined delivery.
Independent CPU numeric tests cover the normal transformation. The sealed 8056
CPU demonstration package is untouched.

Empty clip remains automatic-first even for unnamed clips; changing clip order
can change its meaning. Explicit selection requires a unique nonempty name.
Controller association is captured when the scene is instantiated: runtime
reparenting is not a live controller-rebinding API. Direct C++ clip-string
mutation is not a live binding API either. Runtime controller authoring uses
the scene/editor rebuild path. Imported root motion, deforming mesh collision,
static skinned LOD and non-joint selected channels reject. No unnamed-Play hot
reload claim is made; resource-only editor reload and existing named Host
atomic reload have their own passing CPU cases.

## PR107 portable CI finding and source-only correction

PR107 head ad30857e6bc63a2c9472a88c262dd2590279b69a has tree
7c235a27e5e0a87ba5bb5fac1d4e6cdf6c646ff9. All ten hosted checks completed:
package-manifest, MSVC Debug/Release head and merge, and D3D11 head and merge
passed. GCC Debug, GCC Release, Clang Release, GCC ASan+UBSan and GCC TSan
failed compilation at EngineFrameworkTests.cpp:1768, before their engine or
sanitizer suites ran. GCC reports missing blendSamples and blendParameter;
Clang reports the missing blendSamples initializer, with warnings-as-errors.
No portable runtime or sanitizer result is claimed from these failed jobs.

The actual source was `machine->AddState({"normal-control", &clip});` in the
direct normal-blend helper regression. AnimState declares name, clip,
blendSamples, blendParameter and speed(default1). The authorized correction
uses `Animation::AnimState normalState{};`, assigns the same name and clip,
then passes that state. Empty vector/string and speed1 defaults are preserved.
No assertion, numeric expectation, fixture recipe or production path changes.
Reversing only this exact initializer substitution recovers the prior complete
test bytes. At that source freeze the repair had not been compiled or run.
The separately reviewed/granted local MSVC result follows; portable hosted
validation remains pending before merge acceptance.

Actual hosted Windows and D3D11 logs identify head checkout ad30857e, parent
8056ff6a6d04589bffbac84120365c271f2e9e85, and synthetic merge checkout
bde672d8cdc3af8461c452602544a0d6e8e13a9e with ordered parents
bbbed2a2b201fef675c9aa7caa5009beeb501542 then ad30857e. GitHub commit objects
for those exact logged checkouts give head tree7c235a27 and merge tree
4e00908ec1005e27cb12bd1293e452a8f5eda8dd. Tree SHAs are API evidence, not
values printed by the hosted workflow. Current main/base was bbbed2a; branch
API reported unprotected, protection GET404 "Branch not protected", and the
repository ruleset list was empty. Root's merge gate remains closed despite
the absence of enforced status contexts.

Read-only evidence `work/imported-animation-binding-evidence/ci-first` retains
all metadata, actual logs and bounded command receipts. Verification SHA256:
`a2dde171e26744203a73aa09e00c18540020adac41501353f9cc26b66438670c`.
Two gh run-log reads failed with an invalid cached ZIP; their receipts remain.
Direct per-job log API retrieval, explicitly allowing raw escape bytes only
into evidence pipes after the CLI refused them, succeeded without deleting
caches. These collection issues are separate from the actual compile failure.
No merge, source production edit or local engine execution occurred during
the read-only verification or this narrowly authorized source repair.

## Actual initializer-repair Framework Debug/Release pass

Root reported outer session 84818 closed 0. Reviewed runner SHA256
`627223197f61536a503cebe24b4f102e251730fb89f7f83ec39945e328d53dad`
used the existing exact owned cache without configure. The six exact commands
are in independent plan SHA256
`4afc141ca4ede6876a76e65d51ce9f92c909ca771a06d2d12099fc329bed02f6`:
only EngineFrameworkTests Debug/Release builds with `--parallel 2 -- /nr:false`,
anchored `^EngineFrameworkTests$` JSON inventories, then verbose one-suite CTest
with `-j 1 --stop-on-failure --output-on-failure --no-tests=error`.

| Actual child | PID | Exit | Seconds |
| --- | ---: | ---: | ---: |
| Debug Framework build | 40900 | 0 | 2.921 |
| Debug Framework suite | 30724 | 0 | 1.016 |
| Release Framework build | 54600 | 0 | 3.532 |
| Release Framework suite | 44648 | 0 | 0.203 |

Both CTest inventories contained exactly the expected single unfiltered EXE.
Each suite passed 31 cases, with all ten imported RUN/OK pairs exactly once and
nine distinct owned fixture roots absent afterward. The original normal witness
remained (-0.428571373,0.285714298,0.857142866), matching (-3,2,6)/7. Assertions,
numeric expectations, fixture recipes and all production bytes stayed exact.
All 57 owned children—six build/inventory/test commands, 32 Git queries,
14 process queries, three memory queries and two diff checks—exited0, reaped
without timeout. Fresh memory was 12.4693 GiB before archive, 12.4566 before Debug,
and 12.4756 before Release. No local native/GPU check was repeated.

| Rebuilt Framework EXE | Bytes | SHA256 |
| --- | ---: | --- |
| Debug | 6066688 | 5251f198bfbc4eefa28c5963fa0c5045fdfea059a57bc94efd0ca4700b33647a |
| Release | 1669120 | 07bdfa74c2ada02b7869e30d4996daa74112e3ea24849da124bde3acaae16896 |

Both old Framework EXEs were copied exclusively before compilation and rehashed
afterward. All twelve other accepted EXEs, including both native editors, stayed
exact. Cache 02f26/owner 696089 remained exact. All 621 tracked source files,
283 build inputs and eleven scoped hashes matched before/config/after/final
maps; current build-input binding SHA256 is
`03d382f075a5583c8ca70efe2bf25be67952262d33df27a8ad1614fd312f2c65`.
All 711 pinned prior evidence files and eleven native Content files stayed exact.
Only these Task/QA documents change after the actual compiled repair freeze.

The final process inventory at 09:38:55.5592794 UTC had blockingCount 0 and one
row: query 46288's direct conhost 13648, currentQueryConsole=true/nonblocking.
No other row or blocker was present. That bounded query and all 57 recorded
children exited0 and were reaped; the runner excludes itself while querying.
The inventory is reported at its actual scope, without a later claim that the
exempt console was observed absent. Root reported the outer session closed 0.

Independent evidence-only acceptance SHA256:
`595c494351fe7536ad225dcdf7d6a85231f0fbcb78506ab6f1401821bc5d0e1e`,
file `ci-repair-framework-independent-actual-acceptance.json`. The review matched
all individual receipts/logs,57-child arithmetic, source/cache/binary/archive
maps, numeric cases and final inventory. This is local MSVC Framework acceptance
of the initializer-only repair. Fresh GCC/Clang warnings-as-errors and all five
previously failed portable/sanitizer jobs for the corrected head remain pending,
along with relevant current head/merge CI. No merge acceptance follows yet.
