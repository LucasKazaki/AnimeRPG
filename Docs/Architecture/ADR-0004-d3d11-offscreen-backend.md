# ADR 0004: first connected D3D11 offscreen pass

Date: 2026-10-17. Status: bounded offscreen implementation, hardware evidence and
independent review accepted (packet QA). October18 optional native consumers now
have bounded CPU/hardware evidence and actual application observations with
explicit remaining gates below. This is not full GPU adoption.

`GameWorld::BuildRenderScene` already emits indexed meshes/materials/transforms.
The CPU `SceneRenderer` remains the reference. The installed VS2022/Windows SDK
10.0.26100.0 supplies d3d11/dxgi/d3dcompiler headers and import libraries; Windows
system D3D11 and D3DCompiler47 DLLs were observed without creating a device.

Choose D3D11 feature level 11.0 and SM5 for the first real opaque Unlit pass.
It requires no new dependency and avoids D3D12 command queues, descriptor heaps,
barriers and fences before Astral has a connected hardware pass. bgfx would add a
dependency/build and packaging surface. The historical audit's D3D12 lead is
superseded for this bounded spike by current Windows tooling/evidence. Reassess
only on measured requirements; this does not select a cross-platform strategy.

Keep Windows code in a separate optional static library; root options default
OFF, portable source list unchanged. The original offscreen packet had no
player/editor switch or CPU replacement; the optional native extension below
adds explicit selection while preserving CPU default and legacy behavior.
An isolated Windows-2022 workflow explicitly builds only the D3D11 test target
in Debug/Release and runs only CPU shader diagnostics, with event-aware exact
head/merge/commit provenance. It must never create a graphics device or run the
hardware suite. This closes optional-source compile coverage without claiming
hosted hardware acceptance; the existing Windows workflow remains unchanged.
The explicitly named `RenderOpaqueUnlit` consumes `RenderScene` draws and returns
linear HDR and object IDs. Environment, post and other material/primitive passes
are explicitly outside this API. Tests include an actual GameWorld scene,
overlap/depth/clipping/transforms/textures and CPU HDR comparison, not an isolated
triangle or CPU framebuffer blit. The task fixes budgets/tolerances before runs.

Use `row_major` and `mul(matrix, columnVector)` to preserve Astral's row-major
storage, LH 0..1 clip depth and CW fronts. Upload existing linear floating texture
levels without decoding sRGB again. Shader source is embedded, compiled through a
shared diagnostic-producing helper; no CWD shader-file dependency or implicit
include search. GPU inputs are owned for each call, with only frame-local dedup;
no raw-pointer cache survives hot reload. This favors correctness over residency
performance in the spike. Immediate context and renderer lifetime have one owner
thread. Checked budgets bound resources; ComPtr owns all COM interfaces.

Readback uses color/ID staging textures, RowPitch-aware Map and bounded no-wait
polling. GPU timestamps are valid only with a non-disjoint clock; CPU upload,
completion/readback and end-to-end time remain separate. Device errors report
HRESULT and removal reason; explicit shutdown/reinitialize is supported. Hardware
tests fail when no hardware device is available. WARP never satisfies this gate.
Optional missing debug-layer fallback is diagnosed and never installs software.
Do not physically remove/reset the GPU to manufacture failure evidence.

Rollback disables optional CMake options/consumer attachment; ordinary CPU and
legacy paths remain available. Later work must add Toon/Lit/Water and other
passes, broader hardware/package coverage and observed device-failure recovery.
The October18 native evidence closes a bounded consumer increment, with its
input/diagnostics/timeout limits retained rather than claiming full adoption.

Primary documentation read:

- [Device creation and explicit-adapter driver rules](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-d3d11createdevice)
- [D3DCompile and diagnostic blob](https://learn.microsoft.com/en-us/windows/win32/api/d3dcompiler/nf-d3dcompiler-d3dcompile)
- [HLSL matrix packing](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-per-component-math)
- [Rasterizer front-face/depth clipping](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_rasterizer_desc)
- [Mapped row pitch](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_mapped_subresource)
- [No-wait Map](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_map_flag)
- [Timestamp/disjoint validity](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_query_data_timestamp_disjoint)
- [Device/context thread ownership](https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-render-multi-thread-intro)

## October 18: optional native consumers and bounded evidence

ENGINE-NATIVE-GPU-CONSUMERS-2026-10-18 keeps CPU default/legacy behavior and adds a
shared Windows adapter for the Runtime-only native player and editor Play's
actual GameHost. Portable Host frame extraction/UI-only methods carry no native
header. The same validated indexed geometry pass renders GPU HDR/IDs/depth;
normal presentation retains these targets and performs no scene readback.
Only the transparent CPU UI bitmap is uploaded, explicitly disclosed as CPU UI.

One GPU-only child HWND owns one two-buffer flip chain; the CPU/GDI parent is a
different HWND. A GPU display pass implements the existing neutral tone map and
sRGB once, then composites display-encoded straight-alpha UI. This does not add
lighting/sky/fog/shadow/general post or other shading models. Invalid/unsupported
selection gives a concrete reason; a later failure detaches and visibly selects
CPU once until another user choice, without material mutation or automatic retry.

Bounds cover swap buffers/UI/in-flight resources. Local failed submissions retain
their uploads/staging through a bounded event fence even before queue publication.
Resize retires work/unbinds views, verifies actual backbuffer dimensions and skips
zero/minimized sizes; no hidden half-resolution enters this path. Query identities
separate current CPU submission from actual completed GPU upload/scene/display
times. Display overlay text explicitly labels the previous submitted frame.

Before acceptance require authored original-model geometry, actual HWND captures,
unchanged HDR/ID comparison, independent neutral-display/UI byte comparison,
twenty cycles with real equivalent debug-layer live-object reports, actual
player/editor controls and cleanup, and fixed 1280x720/30+120 measurement. Both
GPU-work and presentation-inclusive full-frame p95 targets are 16.67 ms; a failed
target remains explicit. The offscreen tolerance/coverage/timing gates stay
unchanged. DPI/client/backbuffer/capture sizes and render adapter versus actual
presentation monitor/output/refresh are separate metadata, with no system change.
The actual Debug/Release hardware suite passed15/15 in each configuration on an
RTX5080, including unchanged HDR/ID and independent display/UI comparisons,
20 real resize/zero/restore/reload/detach cycles and matching actual debug-driver
live-object reports. At declared1280x720,30warmup+120measured, fixed scene dt0,
GPU/full-loop p95 was Debug0.135712/11.804800ms and Release4.334048/13.015900ms;
both meet unchanged16.67ms targets. This is a fixed-fixture result, not a production
FPS/4K claim. Raw clocks, separate output/DPI metadata, captures and source/binary
bindings are retained in ENGINE-NATIVE-GPU-CONSUMERS-2026-10-18 QA.

Root observed actual indexed authored geometry, normal CPU/GPU selection and
supported controls in both player/editor configurations. Native editor Play
keeps existing disk hot reload disabled: open/restart use cached assets; changed
documents enter through Stop, clean edit-mode poll, then new CPU Play. This is
initial unsupported selection/restoration evidence, not active-session disk-
reload fallback. Player live fallback/restoration was observed separately.

Keep the acceptance limits: Debug player and Release editor each had a retained
600s timeout, followed by separately bounded fresh-process normal closes. A
followup does not convert a timed-out attempt to a normal close. Stationary
scaled-UI first-click acceptance remains open because the existing append-only
console offers no visible hit/caret oracle. Debug postresize explicit diagnostics
remain unobserved. Release explicit renderer.info was a fresh1104x655 window;
earlier resized972x655 was automatic metadata only. Long console suffixes can
clip; unobserved DPI/output fields must not be inferred from titles/screenshots.
The entire native matrix, Toon/Lit/Water, GPUUI, clean-machine package, long soak
and real device-removal recovery are not established by this packet.

- [Flip-model HWND separation](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model)
- [Native chain creation and lifetime](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgifactory2-createswapchainforhwnd)
- [Resize unbinding rules](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers)
- [Present status and synchronization](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present)
- [Actual debug-layer live reports](https://learn.microsoft.com/en-us/windows/win32/api/d3d11sdklayers/nf-d3d11sdklayers-id3d11debug-reportlivedeviceobjects)
