# Native opaque-Unlit GPU consumers

Owner `/root/demo_gpu`; coordinator and independent reviewer `/root`.
Prepared October 18, 2026 for Lucas's noon Eastern demonstration deadline.
Root accepted the actual CPU package/player launch and explicitly transferred
these fifteen paths for implementation in this isolated worktree. Every execution
phase requires reviewed source/bounded runners and a separate exclusive root slot.
The complete CPU and fixture hardware matrices passed. The actual F10 regression
was corrected and rebuilt; root observed supported player/editor workflows in
Debug/Release. Two native sessions timed out and have separate focused normal-close
followups. Pointer hit testing and specific diagnostics/control gaps remain open;
this Task does not claim the entire native matrix or full GPU milestone accepted.

## Immutable source and purpose

Implementation base: `8056ff6a6d04589bffbac84120365c271f2e9e85`, tree
`7758351dfb56415ea945693db01847a74738226d`. It includes the accepted native-player
merge, creator, editor/ingress packets and offscreen D3D11 oracle fix. Rebind and
review this Task if root supplies a later combined base. The isolated
worktree is `work/astral-native-gpu-consumers`; branch
`engine/native-gpu-consumers-20261018`. Do not use another owner's checkout.

Deliver one optional, real native D3D11 opaque-Unlit rendering path shared by the
Runtime-only Windows player and the editor's actual Play GameHost. Actual authored
RenderScene indexed geometry must be submitted to hardware; uploading the CPU
scene framebuffer does not satisfy this Task. CPU remains the default, and the
legacy game/player and editor edit preview retain their behavior. The CPU-drawn
HUD/console overlay is disclosed; this is not a GPU UI or full renderer milestone.

There is no old functional bug to reproduce: at this base neither native consumer
has a swap chain or calls D3D11Renderer. Source evidence is the current
GameHost::Render -> CPU SceneRenderer and each Tool's StretchDIBits presentation.
The accepted offscreen pass is real hardware geometry, but its per-call staging
readback cannot establish this native capability.

## Exact allowed paths: fifteen

1. `Engine/Framework/GameHost.h`
2. `Engine/Framework/GameHost.cpp`
3. `Engine/Graphics/D3D11/D3D11Renderer.h`
4. `Engine/Graphics/D3D11/D3D11Renderer.cpp`
5. `Engine/Graphics/D3D11/BuiltinShaders.h`
6. New `Engine/Platform/Win32/D3D11Viewport.h`
7. New `Engine/Platform/Win32/D3D11Viewport.cpp`
8. `Tools/AstralPlayerWin32.cpp`
9. `Tools/AstralEditorMain.cpp`
10. `CMakeLists.txt`
11. `Tests/EngineD3D11Tests.cpp`
12. `Tests/EngineSampleTests.cpp`
13. New `Tasks/ENGINE-NATIVE-GPU-CONSUMERS-2026-10-18.md`
14. New `Docs/QA/ENGINE-NATIVE-GPU-CONSUMERS-2026-10-18.md`
15. `Docs/Architecture/ADR-0004-d3d11-offscreen-backend.md`

No `SceneEditor.*` or `EngineEditorTests.cpp` edit is allowed; those belong to the
independent editor-reopen packet. No importer, Material/RenderScene structure,
Game content, global Console, runtime helper, portable CMake/source list, workflow,
dependency, driver or system-setting edit. Root transferred Host, both Tools
and CMake ownership explicitly. Additional paths require a reviewed scope change.

## Supported contract and visible selection policy

Use the existing explicit opaque-Unlit contract: real meshes, opaque blend,
material/draw opacity exactly 1, no cameraFade, valid finite transforms/colors/
texture levels, existing vertex colors, UV scaling, wrap/mips, depth and culling.
Reject unsupported draws with draw index, material name when available and a
specific property/model/value. Billboards/debug lines remain unsupported.

The mode uses a neutral display transform with authored exposure and sRGB output,
plus the CPU HUD/console. It does not implement Toon/Lit/Water, lighting, sky,
fog, shadows, bloom, outlines, grading, FXAA, vignette or dither. Its menu/status
and normal console describe that scope; never imply CPU final-image parity.
Do not edit or coerce authored materials/settings. Existing Toon Basic3D and the
unchanged original marker project must remain valid CPU projects and fail an
unsupported GPU selection honestly.

Use familiar Win32 radio/check menu choices `CPU` and `D3D11 Opaque Unlit`.
The new engine player gets a normal renderer menu only when compiled with the
optional backend; the legacy player gains no renderer switch. Editor gets the
choice in Play, enabled only with an actual Play host. Each new Play session
starts on CPU; edit mode never creates or renders through this GPU consumer.
Keep keyboard access, current focus and title/status text. No custom animated
control, cosmetic redesign or modal renderer-warning loop.

The following settled policy is mandatory:

- Initial unsupported GPU choice leaves CPU selected, keeps the current world
  and gives a concrete visible reason and one normal console line.
- After a supported GPU activation, an unsupported reload/input or actual device
  failure detaches GPU, returns to CPU exactly once and visibly records CPU plus
  the reason, also in the normal console. CPU remains selected until a new
  explicit user choice. No automatic retry, hidden WARP/software path, silent
  coercion, lowered resolution or repeated failure message.
- A zero/minimized client is a suspended GPU frame, not unsupported content and
  not a reason to retry or fall back. Occlusion is a separate skipped status.
- Existing CPU F6/auto-half behavior remains; active GPU uses scale 1. Disable or
  explain Half Resolution while GPU is selected, and use the same effective scale
  1 for UI pointer coordinates. Do not silently alter the saved CPU scale.
- A client exceeding the unchanged backend pixel/resource budget returns to CPU
  visibly. Do not clamp a 4K client to 1080p or increase old limits to hide it.

Editor Play retains its existing hotReload=false configuration and starts from
document JSON. Console open/restart use cached assets; they do not force disk
refresh. The observed editor workflow is Stop, clean edit-mode disk poll with
actual Reloaded status, then new CPU Play and explicit renderer selection.
That verifies changed-document rejection/restoration, not uninterrupted active-
GPU disk-reload fallback. Player hot reload and its actual fallback are separate.

## Concrete implementation sequence

1. In Host only, factor actual viewport/World::BuildRenderScene preparation and
   UI-only drawing from Render. CPU Render still does the same target resize,
   scene renderer/no-camera text, HUD and stats in the same order. The portable
   Runtime receives no Windows/D3D headers or platform dependency. A prepared
   scene borrows world resources only until the next mutation/Update; a consumer
   must finish copying/uploading before returning. UI-only drawing never ticks,
   reloads, builds another world or invokes SceneRenderer.
2. Add truthful portable external-frame statistics and a cumulative CPU scene
   raster-call counter. Increment it at the actual CPU SceneRenderer invocation.
   GPU mode must not show stale CPU raster timings as GPU statistics. Record
   CPU scene extraction, CPU UI drawing and upload separately. Tests compare
   counter deltas so an earlier CPU-default frame cannot be mistaken for GPU
   scene work. Every successfully presented supported GPU frame has delta zero.
3. Refactor the existing validated Prepare/Upload/indexed-draw internals so both
   offscreen RenderOpaqueUnlit and native presentation call the same actual
   geometry pass. Preserve the existing shader, finite guard, offscreen output
   transaction, validation, exact oracle tolerances and all original budgets.
   Normal presentation owns HDR/ID/depth targets on GPU and does no scene staging
   readback. Explicit capture alone allocates staging/readback.
4. Extend the existing renderer PIMPL with native attach/resize/present/detach and
   capture/lifetime diagnostics; keep Windows/D3D types out of its public header.
   Its initialized hardware device owns swap-chain COM state. Use one dedicated
   child HWND per active shared D3D11Viewport and one chain per child. A two-buffer
   windowed flip chain has no exclusive fullscreen or tearing/system changes;
   disable implicit Alt+Enter using its factory. No GDI output goes to that HWND.
5. The shared Win32 D3D11Viewport owns the child HWND and renderer on the native
   owner thread. The CPU parent/viewport retains its GDI route for CPU. Child
   placement follows the actual full client size. It takes no keyboard focus,
   avoids input duplication and lets the existing parent process pointer input
   with full-resolution coordinates. HTTRANSPARENT is a same-thread strategy
   requiring actual pointer/focus/console checks, not an assumed input fix.
6. Add GPU display shaders: sample HDR, clamp negatives, apply the existing
   ToneMapNeutral formula with finite authored exposure and sRGB transfer, then
   composite only the transparent straight-alpha CPU UI image in display space.
   UI bytes are already display encoded; no second sRGB decode/encode or complete
   CPU scene image upload. Use an UNORM backbuffer and encode exactly once.
   Known background clear and mode scope are explicit. Compile actual production
   scene and display VS/PS with both existing shader flag variants in the CPU
   shader test; retain its existing hosted-safe filter and zero-device behavior.
7. Native player uses its actual GameHost; editor Play uses Scene().Game().
   Route presentation and paint through the shared adapter only when GPU is
   active. Before editor Stop replaces/destroys that host, detach/hide GPU and
   release borrowed frame state; then use existing edit preview. Project/scene
   load errors remain transactional and keep current world behavior.
8. CMake adds the shared Windows viewport library only under ASTRAL_BUILD_D3D11,
   consumed by the new Runtime-only native player and editor. Keep optional
   options OFF by default and preserve the existing runtime/audio/legacy target
   boundaries. Neither the portable source list nor ordinary CPU consumers gain
   Windows graphics/shader links.

## Resource, resize and shutdown rules

All renderer/adapter calls, HWND operations and destruction stay on their owning
thread. Frame uploads own COM resources through GPU completion; no MeshData,
Material or Texture2D raw pointer cache survives a call/reload. Bound in-flight
frames/queries and retire them through bounded completion checks. Frame-local
pointer dedup is acceptable; no unbounded queue or wait per presented frame.

Unbind/release every old backbuffer view before ResizeBuffers. Recreate dependent
HDR/ID/depth/UI state transactionally, report exact HRESULT/removal reason and
actual HWND/client/backbuffer dimensions. Minimized/zero sizes submit no GPU
draw/Present and allocate no 1x1 substitute. Restore uses actual client size and
keeps existing simulation semantics and responsive message handling. Record
occlusion independently. No physical GPU removal/reset or tool installation.

Detach orders completion/bindings, size/frame resources and swap chain before
child-window destruction and before the host/world disappears. Existing audio
adapter ordering remains intact. Count two actual swap buffers, HDR/ID/depth,
UI texture, constants, uploads and in-flight resources in checked admission;
capture staging counts only while it exists. Do not reuse the offscreen 44-byte
pixel accounting for the new presentation path or omit chain/UI resources.

## Separate original fixture, never the current demo project

When authorized, create only a new external fixture root under
`work/native-gpu-consumer-evidence/fixture`; preserve all NeutralBox/current demo
content. Copy the original FacetedMarker source from
`work/project-creation-evidence/import-original-20261018`. Its binary SHA256 is
`7b522670ae131cc50db2978f6604468066d3c3c660caddb84f4685a2493e2e24`.
Keep that binary, accessors, indices and geometry byte-identical. Explicitly
author KHR_materials_unlit on both materials in the new glTF before load and
record the exact glTF JSON diff and hashes. This is authored fixture content,
not a renderer override. The importer already honors the extension.

Use an actual astral-project, camera/AudioListener, model entry at [2,0,1], ground
plane and dynamic box. Author the ground/box as opaque Unlit with cameraFade off;
disable authorable fog/shadows/bloom/outlines and use exposure 1. Add one original
8x8 checker texture on the ground; declare its exact bytes, wrap/mip policy and
hash before any run. No Showcase/Playground/National Mall behavior or downloaded
asset. The mode's neutral display intentionally differs from the CPU full post
defaults that the existing scene schema does not expose; invent no JSON fields.

Freeze loaded workload counters before warmup: **4 draws, 76 submitted vertices,
22 triangles, 1 texture**. Each marker primitive uses the full 24-position
accessor, so the existing loader produces 48 submitted marker vertices; cube 24,
plane 4. A mismatch stops for source/fixture review, not an edited expected value
after measurement. Record actual object IDs, mesh/material and texture hashes.
The fixed performance scene remains static at the authored initial transform and
fixed simulation dt=0 during timing; a separate actual native gate demonstrates
dynamic-box motion/restart with normal dt. Record this distinction in QA.

## Tests and truthful images

Before hardware, source-review all fifteen paths and bind their physical hashes.
Add meaningful Sample tests for actual Host preparation/UI-only/counter seams,
authored camera/material/transform content, reload and unchanged CPU rendering,
including no-camera and console transparency. No expected image hash may simply
be regenerated after a failure. No production test flag or new exported test
entry point in either Tool.

CPU-only D/R shader tests compile actual scene/display source with both flag
variants and real diagnostics without Initialize/device calls. Keep all three
ComparisonOracle cases and their actual self-child witnesses unchanged. Add
CPU-only strict-support validation tests if validation is exposed without a
device; never label a device-creating filter CPU-only.

The hardware suite creates a real bounded owned HWND and uses the shared
presenter with the actual fixture GameHost. Verify exact draw/vertex/triangle/
texture counts and CPU-scene-raster delta zero, including paint and console
frames. Compare geometry HDR/IDs through the existing finite-guarded oracle with
its unchanged bands/tolerances. Capture actual HDR/IDs and the actual swap-chain
backbuffer at a recorded presented frame, not a substitute offscreen texture.
Build a CPU pass/display reference explicitly matching the declared neutral
scope; comparison copies may isolate unsupported CPU environment/post in tests
only. Do not mutate the production scene to fit the reference. Separately apply
the independent existing CPU ToneMapNeutral/LinearToSrgb functions and captured
CPU UI pixels to the captured GPU HDR. Compare this expected display image to
the actual swap-chain buffer: RGB maximum <=2 byte values and mean absolute RGB
error <=0.5 byte over the entire frame, with alpha 255 and no geometry-boundary
exclusion. This isolates display/composition from the already checked geometry
oracle; preserve paired CPU-geometry display and difference images as well.
Root must inspect both these captures and the actual native HWND on screen.

Exercise at least 20 attach/present/resize/zero/restore/reload/detach cycles with
distinct authored transforms or texture contents, return to identical initialized
quiescent dimensions and account for all resources. Collect actual debug-layer
live-device-object reports at equivalent baseline/after points, with intentional
device/context/debug objects identified. Raw reports must be retained and reviewed;
equal logical counts/private memory do not prove driver lifetime. If actual
debug output needs a private owned test-child capture, prepare its bounded
exact-PID/argv/cleanup design for review before running it. No installed debugger,
unrelated process attachment or guessed empty report. If unavailable, report this
lifetime gate open rather than claim it passed.

## Predeclared timing and memory gates

In both configurations record the real adapter/vendor/device/LUID, feature level,
debug-layer result, full actual client/backbuffer **1280x720**, Present sync
interval 1, exact workload/quality/counters, **30 warmup + 120 measured frames**.
After capture, retire capture resources before warming up. No per-frame readback,
auto-half, pixel/quality reduction, sync-interval change or altered sample count.

GPU-work p95 must be **<=16.67 ms**. Presentation-inclusive full application
frame p95 must also be **<=16.67 ms**, the initial 60 Hz target. Record both
failures honestly; 33.34 ms is not this Task's threshold. Full-frame wall time
covers the complete fixed-fixture loop including message/input processing,
GameHost Update, scene extraction, CPU UI, upload/submission, Present wait and
any scheduling wait; do not report only the renderer call. All 120 GPU timestamps
must be valid and non-disjoint. Record p50/p95/p99/max separately for scene,
display/total GPU, CPU extraction/UI/upload/submission, Present and full frame.
Report slow samples and missing clocks; do not discard them or retry into a pass.
The unchanged old offscreen gate stays GPU p95<=16.67 ms and synchronous total
p95<=100 ms, with its existing 30/120 and separate CPU reference sample counts.

Fresh available RAM >=8 GiB before each admitted configuration; >=6 GiB while
running. Private process bytes <=768 MiB; checked logical GPU bytes <=256 MiB,
including swap/UI/in-flight resources. Record actual DXGI local usage/budget,
min free RAM, logical object/byte counts and raw process samples. A budget or
counter mismatch stops the run and remains evidence.

## Exact proposed execution and ownership sequence

These commands are a declaration, not execution permission. Root first accepts
the combined CPU/package candidate and transfers paths; then activate the Task
on the reviewed base in its isolated worktree. Root grants CPU, hardware and
native slots separately and exclusively. No compiler/test/UI/GPU overlaps another
owner. Preserve exact argv, cwd, source before/after, logs, binary hashes and
cleanup with the reviewed run_owned.py evidence helper or an equally reviewed
bounded owner. Configure/build <=1200 s, focused CPU CTest <=600 s, individual
CPU shader/oracle <=60 s, hardware CTest <=180 s, native launches <=600 s,
safety <=120 s and milestones <=60 s. Kill/reap only a still-live owned tree.

Use external attributable build `C:/AI/astral-build/native-gpu-consumers-first`; require exact
CMAKE_HOME_DIRECTORY or refuse a nonempty unowned directory. Set and finally
restore MSBUILDDISABLENODEREUSE=1 and CMAKE_BUILD_PARALLEL_LEVEL=2. Resolve the
already installed VS2022 CMake binary; do not install or configure system tools.
For each command below pass exact separate argv through that helper:

```text
cmake -S <owned-source> -B <owned-build> -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON -DASTRAL_BUILD_D3D11=ON -DASTRAL_ENABLE_HARDWARE_GPU_TESTS=ON
cmake --build <owned-build> --config Debug --target AstralGame AstralPlayerWin32 AstralEnginePlayerWin32 AstralEditor EngineD3D11Tests EngineFrameworkTests EngineSystemsTests EngineSampleTests EngineProjectTests EngineEditorTests EngineGraphicsTests --parallel 2
<owned-build>/Debug/EngineD3D11Tests.exe ComparisonOracle
<owned-build>/Debug/EngineD3D11Tests.exe ShaderCompilerReturnsRealDiagnosticsAndRecovers
<owned-build>/Debug/EngineD3D11Tests.exe NativeValidationRejectsUnsupportedBeforeDeviceCreation
ctest --test-dir <owned-build> -C Debug -R "^(EngineFrameworkTests|EngineSystemsTests|EngineSampleTests|EngineProjectTests|EngineEditorTests|EngineGraphicsTests)$" -V --output-on-failure --stop-on-failure --no-tests=error -j 1
```

Repeat the exact build/CPU filters/focused CTest for Release after fresh memory
admission. Confirm CPU command filters enumerate expected cases and create no
device/HWND. Inspect actual new-player and editor generated/link/PE inputs with
and without optional graphics, preserving the Runtime-only player's lack of Game
registration. An OFF-cache compile of the affected native targets proves default
CPU portability; its exact runner also needs root review before execution.

After CPU pass and root's separate hardware grant, run anchored registered
`ctest --test-dir <owned-build> -C <Debug|Release> -R "^EngineD3D11Tests$" -V --output-on-failure --stop-on-failure --no-tests=error -j 1`
with the reviewed scoped capture directory and fresh admission. The suite must
include the unchanged old offscreen cases and the real native presenter gates.
Stop on the first deterministic failure and release ownership after cleanup.
Retain all receipts, including failure/budget evidence; no repeat without a
material correction and fresh review/grant. Under a granted CPU slot, run the
unchanged safety/milestone commands with the installed bundled Python and owned
source cwd, preserving the accepted native-player runner's bounded helper:

```text
<bundled-python> <owned-source>/scripts/test_test_safety.py
<bundled-python> <owned-source>/scripts/verify_milestone1.py
<bundled-python> <owned-source>/scripts/verify_milestone2.py
<bundled-python> <owned-source>/scripts/verify_milestone3.py
```

Finally root owns actual player/editor UI: explicitly select CPU/GPU; inspect
actual original Unlit geometry and mode/status; exercise box motion, console
typing/pointer/focus, F5 restart or Play/Stop, authored transform changes, resize,
minimize/restore, CPU return and close. Exercise unchanged Toon fixture initial
rejection and a supported-to-unsupported reload for one-time visible CPU return,
with document/asset hashes unchanged. Verify real HWND captures and zero CPU
scene-raster delta during active supported frames. Repeat D/R, record source and
binary identities, actual normal exit code/reap and fresh zero owned-window/
process inventories. Never infer native controls from a headless fixture.

## Review and limits

Freeze fifteen-path diff and receipts for substantive independent root review
before builds, after fixes and before exact scoped commit. Preserve the first
failure; do not bypass full-frame, image, lifetime, memory or source gates.
Update ADR-0004 to distinguish the accepted offscreen evidence from this optional
native increment and its measured remaining gaps. Rollback disables the optional
consumer code/CMake attachment; CPU remains usable throughout.

A working supported native path still does not complete Toon/Lit/Water,
environment/general post, GPU HUD/skinning, cross-hardware, clean-machine package,
device-removal runtime proof or a long soak. Source-reviewed failure handling is
not observed device-removal recovery. A failed 60 Hz or lifetime gate remains
explicit even if geometry/images are correct. Actual CPU/hardware and partial or
focused production-native results are recorded in the companion QA. A prepared
Tools change or a focused followup does not erase an earlier timeout or close
unobserved native gates. Final acceptance scope requires independent root review.

## Activation identity and presentation metadata

Original reviewed external draft SHA256 7BCBD51676CA008EF237E563E41B88EA14B8806DD451E5FDE09664A1D8BB931A remains preserved with its 8867bb39 binding. All eleven existing path blobs were verified identical on transferred base 8056ff6a before the new worktree was created. Rebound evidence: work/native-gpu-consumer-evidence/task-rebound-identity.json. Initial activation covered source/fixture preparation; later CPU, hardware and root-native phases each received separate exclusive grants.

Record actual process/thread DPI-awareness, HWND-reported client dimensions, actual backbuffer dimensions and captured image dimensions separately. Root observed DPI-scaled screenshots larger than a 1280x720 logical client; never call a title/logical size physical display pixels. Record renderer adapter/LUID separately from actual window/presentation monitor/output and current refresh, including cross-adapter costs or failed queries. Present(1) synchronizes to that output; 16.67 ms is the frame-time target, not an assumption of a 60 Hz monitor. No DPI, monitor, driver or system-preference changes are authorized.

The shared adapter must validate the actual prepared scene and viewport before
allocating/drawing the full-size CPU overlay. Keep the public Present validation
as well; both costs belong to full-frame timing. Renderer selection refreshes
actual cursor/client coordinates and clears pending mouse deltas, and button
events use their own coordinates with the current CPU/GPU scale. The first UI
click after a keyboard menu choice must not depend on a new mouse-move event.
The corresponding native acceptance gate remains open: the fixture's append-only
console has no caret placement or visible hit/hover oracle, so typing after a
click cannot prove this requirement. No caret feature or false-positive test is
added to close that gap. Player CPU default is scale1; explicit CPU F6 selects2.

External `work/native-gpu-consumer-evidence/run-matrix.ps1` has separate Cpu and
Hardware phases; neither may run without its own exact root grant. Every child
uses the preserved reviewed `run_owned.py`, exact argv/cwd, bounded timeout,
owned-tree cleanup and reap receipts. Source hashes are checked before, after
and in finally. Cache HOME and ON/OFF identity are checked, fresh memory is
admitted per configuration, jobs stay two and modified environment is restored.
Configure/build bounds are 1200 seconds, CPU filters 60, six focused CTest suites
600, safety 120, milestones 60, hardware wrapper 240 with existing registered
CTest 180. Hardware also requires the completed accepted CPU receipt and exact
test-binary hashes; no automatic build or UI launch occurs in that phase.

The runner also binds every physical checkout file and directory (excluding only
the linked-worktree .git administrative file), tracked index inventory and exact
fifteen changed paths; unexpected tracked/untracked/ignored files fail admission.
Reject reparse/symlink inputs before traversing source or fixture trees, and bind
the exact fixture file and directory inventory. Cache checks include HOME,
BUILD_TESTING, ASTRAL_BUILD_D3D11 and ASTRAL_ENABLE_HARDWARE_GPU_TESTS before both
phases. Use short external caches C:/AI/astral-build/native-gpu-consumers-first
and C:/AI/astral-build/native-gpu-cpu-default-first to avoid the earlier observed
MSBuild path-length failure.

Native validation takes the actual caller clear color as well as exposure.
Reject non-finite clear values and clear-times-exposure overflow before any
submission; do not rely on normal readback. CPU regression/control cases and an
actual native public-boundary check must cover this and preserve output/stats on
failure. Submitted-frame counters and completed GPU clocks remain distinct.

## Observed native menu regression and bounded correction

Root's actual Debug player first launch on the accepted CPU/hardware packet
showed CPU-default geometry and working Alt+R Renderer access, but F10 then R
did not activate the menu. The application closed normally; the gate stopped
before GPU selection and all later native checks. Preserve the exact observation
and all four prior Tool binaries under external evidence before correction.

Only when ASTRAL_NATIVE_D3D11 is defined, both Tools now pass ordinary F10 down/up
and every system-key down/up to their existing DefWindowProc path before recording
game/UI input or running game shortcuts. Player forwards directly; editor's
PlayInput declines them so ViewportProc forwards normally. OFF/legacy routes,
renderer/Host/tests, menus and other key handling remain unchanged. This preserves
native menu/Alt/F4 responsibility without inventing a menu implementation.

Independent source review caught a release-bookkeeping gap in the first prepared
early route: an ordinary-held key can arrive as SYSKEYUP after Alt is pressed.
Both routes therefore clear only that released key before forwarding SYSKEYUP
or F10 KEYUP. System/F10 down still records no game/UI press or shortcut. This
preserves the existing per-key release behavior without relying on WM_KILLFOCUS
or menu activation to clear state; stuck input was not observed in the native run.

The corrected input routes passed independent review and NativeMenuCpu's41
bounded commands: ON/OFF D/R native targets, Sample35+Editor30 each, all16 actual
PE/link checks, safety3/M1–M3 and before/after/final source/fixture/HW-test-binary
bindings. Production/test bytes are frozen at the accepted3D13 manifest/A096
patch; final reconciliation changes only this Task, QA and ADR. No unchanged
hardware timing rerun is justified by the Tools/doc-only corrections.

## Actual outcome and retained limits

Fixed hardware Debug/Release15/15 passed unchanged image, memory,20-cycle real
live-report and16.67ms GPU/full-frame gates. Root inspected native captures and
observed actual optional GPU geometry in both production consumers. The accepted
CPU demo/package remains separate and unchanged. Exact receipts, failure history,
native observation provenance and binary identities are in the companion QA.

Debug player and Release editor each retain a600s supervisor timeout. Their later
focused fresh-process normal closes establish only the observed followup scope.
Release player completed live unsupported reload/CPU return/restoration/explicit
GPU recovery within one normally closed process. Editor uses Stop/poll/new Play.
Release editor's explicit renderer.info followup was at fresh ordinary1104x655;
its earlier resized972x655 observation was automatic activation metadata only.
Debug explicit postresize diagnostics and final GPU selection after budget restore
were unobserved. Pointer first-click testing remains open in both consumers.
Do not label this entire matrix fully accepted or silently infer those controls.
