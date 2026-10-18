# ADR 0004: first connected D3D11 offscreen pass

Date: 2026-10-17. Status: bounded implementation approved; hardware validation and
independent review pending. Scope: ENGINE-GPU-SPIKE-2026-10-17, not full GPU adoption.

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
OFF, portable source list unchanged. No player/editor switch or CPU replacement.
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

Rollback is disabling the optional CMake options/removing the isolated target;
existing CPU/editor/player paths are unchanged. Future packets must connect real
player/editor consumers, add Toon/Lit/Water and other passes, swapchain
resize/minimize/restore and validate native device-failure recovery. Until those
pass, report this only as an offscreen opaque-Unlit backend increment.

Primary documentation read:

- [Device creation and explicit-adapter driver rules](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-d3d11createdevice)
- [D3DCompile and diagnostic blob](https://learn.microsoft.com/en-us/windows/win32/api/d3dcompiler/nf-d3dcompiler-d3dcompile)
- [HLSL matrix packing](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-per-component-math)
- [Rasterizer front-face/depth clipping](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_rasterizer_desc)
- [Mapped row pitch](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_mapped_subresource)
- [No-wait Map](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_map_flag)
- [Timestamp/disjoint validity](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_query_data_timestamp_disjoint)
- [Device/context thread ownership](https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-render-multi-thread-intro)
