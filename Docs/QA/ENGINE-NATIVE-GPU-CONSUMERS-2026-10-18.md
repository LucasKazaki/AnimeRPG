# Native opaque-Unlit GPU consumer QA

Owner `/root/demo_gpu`; independent reviewer/coordinator `/root`.
Source base `8056ff6a6d04589bffbac84120365c271f2e9e85`, tree
`7758351dfb56415ea945693db01847a74738226d`, isolated branch
`engine/native-gpu-consumers-20261018` in `work/astral-native-gpu-consumers`.

## Authorization, binding and current result

Root accepted the sealed CPU package and its actual native launch, reviewed the
complete external fifteen-path draft, then explicitly transferred those paths
for implementation. All eleven existing source blobs on 8056 were verified equal
to the original draft's 8867bb39 binding before worktree creation. The original
draft SHA256 `7BCBD51676CA008EF237E563E41B88EA14B8806DD451E5FDE09664A1D8BB931A`
and binding remain preserved externally; rebound evidence is
`work/native-gpu-consumer-evidence/task-rebound-identity.json`.

The complete CPU and hardware fixture gates passed. The actual F10 regression
was corrected, independently reviewed and rebuilt; root subsequently observed
supported native player/editor workflows in Debug/Release. Debug player and
Release editor each retain a600s timeout and a separate focused fresh-process
normal-close followup. Pointer hit testing and specific diagnostics/control gates
remain open. This records bounded evidence, not acceptance of the entire native
matrix or full GPU milestone. Final doc reconciliation changes only Task/QA/ADR;
all12 non-doc paths preserve accepted3D13/A096 physical bytes.

The first authorized CPU attempt stopped before
configure because a Git line-ending warning entered its strict path-list check;
the retained setup receipt is cpu-20261018-063505-6195441. A reviewed runner-only
correction kept normalization unchanged and passed an actual exact 15-path control.

The next authorized CPU run, cpu-20261018-063627-1682622, configured the short ON
cache and built every declared Debug C++ target successfully. ComparisonOracle
passed 3/3. The production display shader filter then failed with actual X3000
diagnostics: local identifier `linear` is reserved in HLSL. Child 48788 returned 3,
was reaped without timeout, and the matrix stopped immediately. No later filter,
suite, Release/OFF build or safety command ran. Finally bindings matched all 623
physical files/56 directories and exact fixture 5 files/3 directories; owned process
inventory was empty. Raw logs and all command receipts remain preserved.

The reviewed identifier correction renamed that local to mappedColor. The next
authorized CPU attempt, cpu-20261018-063949-3935537, built ON Debug targets and
passed ComparisonOracle 3/3 again. Actual strict display PS compilation then
reported X3129/X4000, potentially uninitialized function result (Neutral), at
line17 column46. Child 42472 returned 3, was reaped without timeout, and the matrix
stopped. All 623/56 final source bindings matched and owned processes were empty;
the exact source and diagnostic remain preserved.

The reviewed correction uses one initialized result and one final return in Neutral:
the finite low-peak result is the same offset-adjusted color; the existing
compression arithmetic runs only in the other branch. All arithmetic/constants,
strict warning/error flags, tests, tolerances, workloads and budgets remain.
The next CPU run, cpu-20261018-064336-8354813, genuinely passed production scene
and display VS/PS in both flag variants, ComparisonOracle 3/3 and NativeValidation
1/1. Graphics, Systems and Framework suites passed. The new Sample extraction/
reload regression then failed at line1280: the second extraction retained the old
draw. CTest child 42740 returned 8 and was reaped without timeout (6.797 seconds);
the matrix stopped before later suites/Release/OFF/safety. All 623/56 final source
and exact fixture bindings matched, and owned processes were empty.

Actual GameWorld::BuildRenderScene appends into caller outputs. The new public
Host seam now defaults scene/view before extraction, including the no-camera
false branch, so reused outputs cannot retain prior-world draws/pointers. The
exact failing reload assertion remains; the existing no-camera test now seeds
stale scene/view state and requires clear-on-false. GameWorld is unchanged.
This stopped attempt preceded the complete CPU/hardware results below; its raw
failure and reviewed correction remain preserved rather than overwritten.

This is an absent new capability, not an old functional failure: the accepted
base's GameHost called the CPU SceneRenderer and its two native consumers used
StretchDIBits. The earlier RTX 5080 offscreen evidence and CPU finite-oracle fix
remain recorded in ENGINE-GPU-SPIKE-2026-10-17; they are not native evidence here.
The accepted CPU demo/package and original projects remain untouched.

## Implementation and invariants

Host exposes actual-world frame extraction, transparent CPU UI-only drawing and
external submitted-frame statistics without a platform header/dependency. Its
CPU Render retains the original scene/no-camera/UI/stats route; the actual CPU
scene-raster call site increments a cumulative counter. UI-only calls clear
their transparent target each time and do not rasterize or update the world.

Native and offscreen geometry share the original indexed-draw body and validated
upload path. Old offscreen tolerance/coverage/sample/budget values remain. The
native path has GPU HDR/object-ID/depth, a dedicated GPU-only child HWND with a
two-buffer flip swap chain, GPU neutral tone mapping/sRGB and only a transparent
CPU HUD/console upload. CPU GDI output stays on the parent HWND. No complete CPU
scene image is uploaded. The geometry and display passes are distinct.

Two in-flight frames and six queries per frame are bounded. Actual completed
GPU upload/scene/display clocks carry the submitted frame ID, independently of
the current CPU submit/Present timing. GPU overlays explicitly describe the
previous submitted frame; Host statistics change to a new actual submission
after successful Present. No CPU timing is labeled a completed GPU clock.

Normal presentation has no scene readback. Captures read actual geometry targets,
the actual chain backbuffer and the exact uploaded CPU overlay for one recorded
frame. Admission includes both swap buffers, HDR/ID/depth/UI, constants, current
and pending geometry/textures, and capture staging while it exists; query count
is reported separately from logical allocation bytes and actual DXGI usage.

A failed-frame guard owns local upload/capture resources through cleanup even
before inFlight publication. It clears bindings, closes an open timestamp range,
ends an event and uses the existing five-second wait before resource release;
invalid device state is fully released. Resize drains prior work, releases all
old views before ResizeBuffers and verifies GetBuffer's actual dimensions.
Zero/minimized/occluded frames skip without a 1x1 substitution. Detach completes
work/releases the chain before destroying the GPU child and Play host.

The new native player has a normal Renderer menu; editor choice is Play-only.
CPU remains the default, and legacy player/Game acquire no GPU consumer link or
switch. Initial unsupported choice leaves CPU selected with a concrete material/
draw reason. Later unsupported reload or device failure selects CPU once,
detaches the GPU child and shows the reason in normal console/title/status;
there is no automatic retry or quality/material coercion. CPU viewport metadata
is restored on return. renderer.info exposes actual native/DPI/output metadata.
Renderer changes refresh actual cursor/client coordinates and clear pending
mouse deltas. Button messages use their own lParam coordinates at the current
scale, including the first click without a preceding WM_MOUSEMOVE.

Shared frame admission runs immediately after actual-world extraction and before
full-size CPU overlay allocation/drawing. Public Present retains its validation;
both validations remain inside full-frame time. The prepare phase includes the
early admission, and the public validation duration is reported separately.

Public native validation receives the actual caller clear color and rejects
non-finite clear values or clear-times-exposure overflow before submission.
Prepared CPU regressions include huge-finite exposure1/0 controls, exposure2
rejection, a default-clear exposure2 control and NaN/positive/negative infinity.
The native public-boundary case requires failed validation to preserve existing
stats/capture and produce no completed submission, then renders normally.

The scope remains explicitly opaque Unlit, neutral display and CPU UI. Lighting,
sky, fog, shadows and general CPU post are absent. Actual native observations
and their remaining limits are distinguished below; source intent alone does
not close a control, device-recovery or pointer gate.

## Separate fixture authoring evidence

`work/native-gpu-consumer-evidence/prepare_fixture.py` created only its new
`fixture/Content` root; receipt `fixture-authoring.receipt.json` records every
file hash and explicitly says actualRuntimeLoadPerformed=false. It copied the
original marker binary byte-identically:
`7b522670ae131cc50db2978f6604468066d3c3c660caddb84f4685a2493e2e24`.
Only the copied glTF materials' KHR_materials_unlit extensions and extensionsUsed
were authored; copied glTF SHA256 is
`d2e1e37cc5cd8510948a8c258fdc421df594372cc456401b61137f6318968ea5`.
The original 8x8 RGBA checker SHA256 is
`4a934265be835def4ee60a6dfc881c42daae7d813cbafa67e3c54915b35b201b`.
Project/scene hashes are in that receipt; no accepted CPU content was edited.

Freeze **4 submitted draws, 76 submitted vertices, 22 submitted triangles and
1 texture** before warmup. The importer copies all 24 source vertices for each
marker primitive (48 submitted), plus cube24 and plane4. CPU raster-visible
triangle counts after culling are a different metric and do not replace 22.
Actual authored loading and native hardware frames subsequently confirmed these
exact submitted counts, independently of CPU raster-visible counts.

## Checks and coverage

- Three meaningful Sample cases read actual authored files and verify extraction/
  reload, camera/material/transform data, independent CPU-render output, UI-only
  alpha clearing/console, no world advance/raster call and the no-camera branch.
- CPU-only NativeValidation creates no device; all existing ComparisonOracle
  witnesses remain unchanged. The existing hosted-safe shader filter compiles
  actual scene/display VS/PS with both flags and real diagnostics, without Init.
- Real-HWND hardware cases use the shared adapter and actual copied project;
  geometry uses the unchanged all-finite HDR/ID oracle. Display is independently
  recomputed from captured GPU HDR with CPU ToneMapNeutral/sRGB and captured UI:
  maximum RGB <=2 bytes, whole-frame mean <=0.5 byte, alpha255, no edge exclusion.
  Captures precede comparison failure and require a scoped capture directory.
- An actual debug-layer lifetime probe is a private owned self-child, not a Tool
  test flag. It warms/quiesces once, emits actual baseline/after reports around
  twenty resize/zero/minimize/restore/reload/detach cycles, then compares real
  reported object-type counts. Raw debug output is retained. Its parent captures
  only that child's debug events/read-only strings, uses a 60-second bound and
  owned termination/reap on failure, closes image/DLL file handles and leaves
  OS-owned debug-event process/thread handles to ContinueDebugEvent. It requires
  actual debug-layer output and never substitutes logical counts for a report.
- Fixed full-client/backbuffer1280x720, Present(1), fixed initial scene dt0,
  30 warmup +120 measured, no capture/readback during timing. Every measured
  frame must have its own valid completed GPU clock. Both GPU work p95 and full
  presentation-inclusive loop p95 must be <=16.67 ms. Raw timings are retained;
  neither 33.34 ms nor discarded slow samples can satisfy this gate.
- Fresh RAM >=8 GiB, runtime >=6 GiB; private <=768 MiB and logical GPU <=256 MiB.
  The actual adapter/LUID, DXGI usage/budget and resource/query counts are required.
- Before execution: independent full source/physical-hash and bounded-runner
  review; separate exclusive CPU, hardware and root-native grants. CPU D/R
  affected suites, opt-in/OFF target-link/PE checks, unchanged safety and M1–M3;
  then hardware and root's actual player/editor controls and cleanup evidence.

The external runner binds the complete physical checkout file/directory inventory
before, after and finally, excluding only its linked-worktree .git administrative
file. It also requires the exact fifteen changed paths, unchanged tracked index
inventory and no unexpected untracked scope. Exact fixture files/directories and
hashes are bound; reparse/symlink input is rejected before traversal. Cache HOME,
BUILD_TESTING and both graphics options are checked before CPU/hardware phases.
Short external ON/OFF caches are under C:/AI/astral-build/native-gpu-consumers-first
and native-gpu-cpu-default-first. The first frozen patch/manifest/runners remain
preserved externally as first-frozen-*; complete current results follow below.

## Actual CPU, hardware and first production-native results

The final pre-menu-correction CPU matrix cpu-20261018-073513-4081668 ran
07:35:13.4135519–07:36:05.3140555 UTC and passed all49 bounded actual children,
with exit0, reaped and no timeout. It built ON/OFF Debug/Release declared targets,
passed ComparisonOracle3/productionShaders1/NativeValidation1, all six focused
suites (129 cases per configuration), all16 target PE/link checks, safety3 and
M1–M3. Receipt SHA256 D503F133BF0F078E117478C1775810FA91E6A6DADC280AE569B0E10A8AF68522.
Minimum fresh admission was12.558GiB. Source623files/56dirs, exact15 changed
paths and fixture5files/3dirs matched before/after/final; owned processes were0.
Root independently accepted these actual receipts and exact binary bindings.

Earlier native lifetime attempts are retained: hardware065804 stopped after
13 passing Debug cases when its private child aborted without retained CRT
assertion text. The reviewed child-stream diagnostic exposed a fixture-client
size assertion; resize diagnostics071341 then proved requested0x0 became actual
120x0. A test-only WM_GETMINMAXINFO override remained insufficient in actual
hardware071759. The final test fixture restores DefWindowProc and applies
SWP_NOSENDCHANGING only to its explicit0x0 request; all real nonzero placements,
production behavior and strict zero/renderer/frame-ID assertions remain unchanged.
Final post-measurement fflush(stdout) retains raw performance data before strict
assertions. All stopped receipts, source freezes and raw diagnostics remain.

hardware-20261018-074241-7186404 ran07:42:41.7251887–07:43:33.295192 UTC,
receipt SHA2561B62A15121A802B406E3B167F3D8589E3DBF191FE7AF5DCE6A1A786A6929E1E6.
All15 registered cases passed in Debug37.41s and Release10.72s, all17 bounded
runner children exited0/reaped/noTimeout. Both actual private debug children
completed20 cycles, including actual0x0 client, and matched every actual live
ID3D type count between equivalent warmed/quiescent baseline and after, including
ID3DDeviceContextState. Raw driver reports/child CRT streams remain in each scoped
Debug/Release capture directory; no logical-counter substitution was used.

Actual native geometry4draws/76vertices/22triangles/1texture, CPU scene-raster
delta0. Unchanged HDR/ID oracle:23 coverage differences (.000025), RGBmaximum
.002803400/mean.000314036. Independent actual swapchain display comparison:
maximum1byte/whole-frame mean.009115byte in both configurations, within2/.5.
Each configuration retains six native PNGs (CPU geometry display, actual chain,
independent display reference, difference×16, object IDs and CPU UI only) and
the original four offscreen PNGs. Root viewed all six unmodified native Debug
captures; corresponding Release hashes exactly match, as recorded by
root-full-hardware-evidence-review.json (SHA256841630EBD893E4451C91257352A5C2A2AEA664F22C08793EBF70AC57A95A48BA).

At full reported client/actual backbuffer1280x720, Present(1), fixed authored
dt0 and30warmup+120measured unique completed GPU clocks, p95 GPU/full application
frame was Debug.135712/11.804800ms and Release4.334048/13.015900ms. Both satisfy
the unchanged16.67ms gates; raw120 GPU/frame samples and phase summaries remain
in hardware-Debug.log/hardware-Release.log. Actual renderer RTX5080/LUID73259,
debugLayer1; output/DPI/monitor metadata are logged separately. Background GPU
activity was present, so no idle-system claim is made. Source623/56 and fixture
5/3 matched before/after/final; owned processes were0. These are fixture hardware
results, not production player/editor control acceptance or a full GPU milestone.

Root then launched actual Debug player binary22FDF7D4… with explicit owned
Content and project.json from unrelated cwd. Observation receipt
native-debug-player-first/root-native-observation.json SHA256
8E11DF1830BF65B23D256F68C6B79D7B3A3CE4297BDCBF8F0B3BD6C33DA32A18
records coherent CPU-default marker/checker/cube and Alt+R Renderer menu success.
Escape then F10 left no menu/mnemonic, and R did not open Renderer. Root stopped
before GPU selection, console, pointer, resize, reload or later D/R editor/player
gates. Owned PID51908 closed normally0/reaped/noTimeout82.829s; source623 and
original/staged five-file fixtures were unchanged, final owned windows/processes0.
Unmodified screenshots/accessibility state remain in root's native transcript.

Actual source diagnosis: ordinary F10 down/up were swallowed by Player, so
DefWindowProc could not complete its menu gesture. Editor PlayInput also swallowed
system keys/F10 (source finding; no actual editor failure is claimed). The narrow
correction, only under existing ASTRAL_NATIVE_D3D11 guards, forwards ordinary
F10 down/up and all system-key down/up before game/UI recording. Player calls
DefWindowProc directly; editor PlayInput returns false to existing ViewportProc
forwarding. OFF/legacy code, other game keys, Host/renderer/tests/CMake, workload,
flags, budgets and hardware binaries are unchanged. The corrected routes passed
independent source review and the affected CPU matrix below before root's actual
native observations. Compilation is not substituted for any unobserved control.

Independent source review of that prepared correction found that early SYSKEYUP
forwarding bypassed the existing per-key release reset. An earlier ordinary-held
W/Shift can be released with Alt context without losing focus. Both Tools now
clear only the routed released key on SYSKEYUP/F10 KEYUP before native forwarding;
down messages still create no game/UI press or shortcut. Existing WM_KILLFOCUS
resets remain, but menu activation/focus loss is not assumed. This is a corrected
source finding, not an actual observed stuck-input failure. The earlier A8BA/213E
source packet and independent review remain preserved externally.

Microsoft's [WM_KEYDOWN](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-keydown)
and [WM_KEYUP](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-keyup)
contracts describe DefWindowProc's F10 state/menu responsibility;
[system key down](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-syskeydown)
and [up](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-syskeyup)
remain on their normal native route.

## Accepted menu correction build and production identities

NativeMenuCpu nativemenucpu-20261018-080954-4862794 ran08:09:54.4911915–
08:10:44.7033183 UTC. All41 actual bounded children exited0/reaped/noTimeout:
ON/OFF Debug/Release affected native targets, Sample35+Editor30 each, all16 actual
PE/link checks and unchanged safety3/M1–M3. Minimum fresh admission12.400GiB;
source623/56/exact15, fixture5/3, cache HOME/options and original hardware test
binaries matched before/after/finally; owned processes0. Receipt SHA256
2D6082CAA065775DE12C098E1C145BD68C21CBAD4BEF7107450A99177A4E85EF.
Accepted production/test source manifest3D13B07AF720659ADD8AC005E307F7C58BC8B40069BFDA5A82C8CBE37806B5EE,
full patchA0962AD06ED54DD2ECD20BD0F2E6249AC9456F22CBB3318FA9F214A7A78E5940.
These pre-reconciliation bindings and all old packets remain preserved externally.

Current ON-cache production binaries used for native observations:

| Configuration/Tool | SHA256 |
| --- | --- |
| Debug engine player | 4622BCD702B65E1B40021E0A64BF0B800B13D7BED7D012C986A33FFFF74373DC |
| Release engine player | B15F7ED1BED055F16C9D6CA64FCD428BD4BE57594748C24574BFB8BCD9ED557B |
| Debug editor | FA8F8902E924D30D98C82B9B531DE3BC81944D1BD0163E3CE79916490373CBEC |
| Release editor | C84770C60B65EF22A6F22DA87F8B1082717FE534046E24F0AF0211368DF608C4 |

The cache is C:/AI/astral-build/native-gpu-consumers-first, with exact owned HOME
and BUILD_TESTING/ASTRAL_BUILD_D3D11/ASTRAL_ENABLE_HARDWARE_GPU_TESTS=ON. Legacy
and OFF consumers preserve their CPU/Runtime/Game boundaries. EngineD3D11Tests
Debug14697BBC6A7B74E10C88525A4D2B20F13C6D488B7C4CACA2B66961E6B8601A0D
and ReleaseE50FA9B558983835DB1EC9D17F6F1FC5155D952C4C20784609019830A80C9546
remain exactly the binaries that passed the hardware matrix. No unchanged timing
rerun was performed for Tools/doc-only changes.

## Actual production-native observations and retained attempts

All case directories below are under work/native-gpu-consumer-evidence. Root
operated the actual native UI, with fresh resource admission and bounded owned
exact-argv supervision; the GPU owner separately read/hash-checked receipts and
source/cache/binaries/original/restored fixtures. Native screenshots remain
unmodified in root's tool transcript; no Sky payload was decoded or postprocessed.
Observation drafts distinguish root-reported UI facts from independent static
checks. Fresh Sky absence is a separate root observation, not inferred from exit.

| Case / actual attempt | Exit and bounded cleanup | Actual coverage and limits |
| --- | --- | --- |
| native-debug-player-menu-fixed-first | PID51184, normal0/reaped/noTimeout540.187s | Corrected Alt/F10 menus, actual GPU geometry, F3/F6, renderer.info visible RTX/debug/backbuffer1280x720, F5 restart retained GPU. Pointer attempt invalid; later gates not reached in this session. |
| native-debug-player-remaining-first / first | PID49952,600.156s timeout, child1/wrapper124/reaped by owned cleanup | Ordinary resize/minrestore, over-budget CPU return, unsupported reload CPU rejection and exact restoration observed. Final restored-content GPU recovery/normal close not reached. |
| Same Debug case / focused recovery | Fresh PID57476, normal0/reaped/noTimeout43.953s | Restored original Content, fresh CPUdefault, explicit GPU original geometry, normal close. A fresh-process recovery, not uninterrupted restoration of timed-out process. |
| native-release-player-menu-fixed-first | PID45156, normal0/reaped/noTimeout288.953s | Menus/real GPU geometry, keyboard console/history/completion, F3/F6/F5, resize/minrestore, over-budget CPU latch, unsupported live reload/CPU rejection, exact byte restoration and explicit GPU recovery in the same process. |
| native-debug-editor-menu-fixed-first | PID46488, normal0/reaped/noTimeout555.266s | CPU edit/newPlay, GPU choice, Stop/poll/newPlay Toon rejection/restoration, resize/minrestore and budget CPU return. Explicit postresize renderer.info and final GPU choice after budget restore were not observed. |
| native-release-editor-menu-fixed-first / first | PID17036,600.187s timeout, child1/wrapper124/reaped by owned cleanup | Supported editor workflow, resize/minrestore, over-budget CPU latch and explicit GPU recovery observed. Resized972x655 was automatic activation metadata. No explicit renderer.info, final Stop or normal close in this attempt. |
| Same Release editor case / focused followup | Fresh PID5488, normal0/reaped/noTimeout219.266s | Fresh ordinary GPU geometry, explicit renderer.info at1104x655, actual Stop to clean authoring controls/status, normal close. Not postresize972x655 diagnostics or retroactive completion of first attempt. |

The original pre-correction F10 failure described above is also retained.
The Debug timeout verificationC1FDB737214400C84881703C99647A7DCBDE04987E46CDB148C6C6ACB381C40B
and separate focused recoveryE85958F783D57C12063D7E704773E5EB1EB9FF57FA33103840EE51DE957C0582
are unchanged. Release editor's original exit receipt
748578572A1535232BAB6628B8A5CFD86CC9B554C55E5F29BBCBE68E5BC79F58
remains timedOut=true; focused recovery exit
C0016043F76F2E2DB01EB4EFEA60CB320A33850FBD92E53269F853B8D0395036
is separate. The latter ran09:33:24.113663–09:37:03.378123 UTC under300s supervision.
No followup rewrites a first attempt as a normal close.

Release player showed actual1280x720 explicit diagnostics; subsequent visible
activation metadata showed1148x720 after resize, without another explicit-command
claim. Its actual unsupported boxUnlit Toon hot reload selectedCPU once
with concrete reason/radio; explicit unsupported selection remainedCPU with one
additional reason. Between recorded observations no per-frame failure rows were
added. Restoring exact original bytes keptCPU until explicit GPU choice restored
original checker/cube/marker and cleared the reason in that same process.
Minimize/restore checks use settled output, excluding DWM transition captures.
Maximized3840-wide captures exceeded the declared budget and selectedCPU; they
are not GPU4K or timing evidence. Native capture width is not a backbuffer size.

Editor Play keeps existing HostSettings.hotReload=false, audio=false and starts
from document JSON. Console open/restart reuse AssetManager cached JSON; restart
of an unnamed document recreates that document, not a disk refresh. The invalid
plan assumption that repeated open would reread the editor file was corrected
without changing production. Supported authored disk changes use Stop, clean
edit-mode700ms poll with actual Reloaded status, then new CPU Play and explicit
GPU choice. Both editor configurations observed actual Toon initial rejection
and restored-document GPU selection through that workflow. No uninterrupted
editor active-GPU disk-reload fallback is claimed.

Debug editor initial activation diagnostics visibly showed RTX5080/LUID73259/
debugLayer1/backbuffer1104x655/sync1; no explicit renderer.info command was run
there. Its F5 after native menu mode and top-frame focus had no effect; those
observations are retained, while focused-viewport F5 Stop and actual toolbar
Play/Stop worked. Release focused followup used an actual viewport, empty console,
individually typed renderer.info literal and Return; visible RTX5080/LUID73259/
hardware1/debugLayer1/backbuffer1104x655/sync1/HLSL0 and DISPLAY2 desktop2560x1440
were observed. This was a fresh ordinary window, not its first attempt's resized
972x655 explicit diagnostics. Long suffixes remained clipped/unobserved. A stale
UI accessibility Play action was replaced by fresh observed screenshot selection;
the first attempt's missing-foreground input and timeout are retained, not inferred
to be production rendering failures.

Each owned staged mutation preserved exclusive original/Toon bytes and the single
semantic delta materials.boxUnlit.shading. Original scene SHA256a9395266… became
Toon1a02db8f… and restored to exacta9395266…; other four files remained unchanged.
Before/after source623/56/current4Tools/cache/originalfixture5 and restoredstaged5
were independently verified. Final focused Release postcheck
5A6B25653B39C8801983946D53AEBDD13A44F0756979FA4BC0DE2F6617883603
records scoped processes0; fresh Sky window cleanup is root's separate check.

## Open native acceptance gates

The stationary first-click CPUscale2/GPUscale1 hit gate remains OPEN. Existing
TextBox has no caret index/pointer placement, always appends and draws the caret
at the end; its console auto-focuses the only input and exposes no hover/pressed
visual. Scrollback rows are Labels, not clickable suggestions/history. Therefore
the attempted interior-caret insertion and typing after a click cannot establish
scaled pointer correctness. No caret feature, invented schema or false-positive
test was added. Player CPU default is1; explicit CPU F6 selects2.

Also retain Debug explicit postresize diagnostics/final budget-restore GPU-choice
gaps, Release postresize explicit-command gap, disabled editor active-Play disk
hot reload, and every recorded timeout. Partial observations plus focused followups
do not establish that the entire original native matrix passed. Independent root
review determines the bounded integration/acceptance scope.

## DPI, output and remaining acceptance limits

Record process/thread DPI awareness, HWND-reported client, GetBuffer dimensions
and captured image dimensions separately. A 1280x720 logical client/title is not
proof of 1280x720 physical display pixels. Record rendering adapter/LUID separately
from actual window monitor, containing output/adapter and current refresh, with
query failures explicit. Root observed AMD/RTX-connected outputs at distinct
high refresh rates; Present(1) does not imply a 60 Hz monitor or same adapter.
No DPI, monitor, driver or system preference is changed.

Toon/Lit/Water, environment/general post, GPU UI/skinning, cross-hardware,
clean-machine package and long soak remain open. Real device removal is not
manufactured; reviewed error paths do not establish observed removal recovery.
Any image, 60 Hz, lifetime, memory or source-binding failure remains a failed/open
gate even if another aspect passes. This QA records the complete fixture matrix,
retained failures/timeouts and actual partial/focused native observations. Final
Task/QA/ADR reconciliation performs no production/test/Tools/CMake edit and no
build/native/hardware/commit/push; its separate final source/tree binding is
external. Historical runners remain bound to their reviewed execution packets.

Primary Windows contracts read for the private diagnostic:
[debug-event ownership and deadlines](https://learn.microsoft.com/en-us/windows/win32/api/debugapi/nf-debugapi-waitfordebugevent),
[debug loop event continuation](https://learn.microsoft.com/en-us/windows/win32/debug/writing-the-debugger-s-main-loop).
