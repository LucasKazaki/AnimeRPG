# Connected D3D11 offscreen spike

Owner: `/root/numeric_implementation`; independent reviewer/integration: `/root`.
Authority: Lucas's October 17 engine-only request. Started October 17, 15:13
America/New_York, before the October 18 23:59 deadline. Base:
`5245a3dd079083a534b4afd16924a6a5496d1a0d`; dedicated branch
`engine/d3d11-offscreen-20261017`, worktree `work/astral-d3d11`.

## Problem and bounded result

Astral has a real backend-neutral `RenderScene`, indexed `MeshData`, linear
textures/materials and a CPU reference renderer, but no hardware backend.
Implement one Windows D3D11 opaque Unlit pass consuming this boundary, including
an actual `GameWorld::BuildRenderScene` fixture. Return owned linear HDR and
object-ID readback. This increment is offscreen only: lighting models other than
Unlit, environment, post, transparency, HUD, player/editor presentation,
swapchains, native minimize/restore and physical device-loss recovery remain open.

Read ADR-0004 before production. No new dependency, installation or driver/system
change. Existing portable/runtime/world/host/importer code stays untouched.

## Allowed files

Existing: root `CMakeLists.txt` only. New:

- `Engine/Graphics/D3D11/D3D11Renderer.h` and `.cpp`
- `Engine/Graphics/D3D11/ShaderCompiler.h` and `.cpp`
- `Engine/Graphics/D3D11/BuiltinShaders.h`
- `Tests/EngineD3D11Tests.cpp`
- `Docs/Architecture/ADR-0004-d3d11-offscreen-backend.md`
- this task and `Docs/QA/ENGINE-GPU-SPIKE-2026-10-17.md`
- `.github/workflows/windows-d3d11-compile.yml` (coordinator-approved expansion,
  eleven paths total; the existing workflow remains unchanged)

## Contracts and budgets declared before execution

Use feature level 11.0/SM5, hardware adapter only, LH/column-vector transforms
with row-major storage, clockwise front faces, depth clear 1 and LESS/write.
Support opaque Unlit baseColor, tint, vertex RGB toggle, linear mipmapped texture
Repeat/Clamp, UV scale, emissive and double-sided culling. Require opacity 1;
reject unsupported shading/blend/camera fade and nonempty billboards/lines.
The explicitly named pass does not process environment/lighting/post. No skin
deformation is performed; already skinned mesh vertices are ordinary input.

Borrow input only during Render; own GPU uploads and readback, no cross-frame
raw-pointer cache. Validate finite/structural data and checked byte admissions
before allocation. Invalid input, allocation/compiler/readback failure preserves
the caller's frame. Zero viewport skips without allocation; invalid negative or
over-budget sizes fail. Resource recreation and Shutdown are explicit/idempotent.
One calling thread owns device context and renderer lifetime. Report actual
HRESULT/operation/compiler diagnostics and removal reason. Never silently WARP.

Default admission limits: 256 draws, 32 MiB mesh data, 64 MiB texture levels,
1920*1080 pixels, 256 MiB accounted owned GPU resources; validate by subtraction
before copying/narrowing/allocation. Five seconds maximum completion wait;
CTest 180 seconds. Accounting is logical allocation size, not physical VRAM.

Fixed measured fixture: 1280x720, at most 64 draws, 100k vertices, 200k triangles,
32 MiB textures, no automatic quality/resolution changes. Warm up 30 GPU frames,
measure 120; CPU reference warmup 2 + 10 frames. Record p50/p95/p99/min/max, GPU
timestamp/disjoint validity, upload/submission/readback/end-to-end wall times,
process working/private bytes and DXGI local-memory usage/budget when available.
Initial gates: GPU-pass p95 <=16.67 ms, synchronous end-to-end p95 <=100 ms;
process private <=768 MiB; owned GPU resources <=256 MiB. Leave 6 GiB available
host memory; stop on pressure. Check resource counts after 20 resize/reload cycles.
These are targets, not achieved performance claims.
`resourceCount` is the logical per-frame buffer/texture admission count. Constant
counts and a process-private cap after twenty cycles do not establish an actual
driver live-object baseline or prove no leak; RAII is a source-level property.

CPU HDR/ID oracle: same neutral environment, opaque Unlit pass. Outside a one-pixel
object boundary band IDs/coverage must agree; coverage differences must stay in
the band and <=0.5% pixels. Untextured interior RGB max absolute error <=2e-4;
magnified texture max <=1/128, mean <=1/512. Constant-color minification verifies
actual mip selection; arbitrary derivative LOD bit identity is not promised.
Do not loosen these limits after measuring without coordinator review. Capture
paired/difference images for actual inspection; diagnostic CPU tone mapping does
not count as GPU post-processing.

## Execution gates

Implementation and source review are authorized now; no build, device creation,
runtime or desktop execution until the coordinator grants the exclusive slot.
External build/evidence roots: `work/build-d3d11` and `work/d3d11-evidence`.
Plan commands (absolute work/build paths resolved in retained receipts):

```text
cmake -S <work/astral-d3d11> -B <work/build-d3d11> -G "Visual Studio 17 2022" -A x64 -DASTRAL_BUILD_D3D11=ON -DASTRAL_ENABLE_HARDWARE_GPU_TESTS=ON
cmake --build <work/build-d3d11> --config Debug --target EngineD3D11Tests EngineGraphicsTests --parallel 2
ctest --test-dir <work/build-d3d11> -C Debug --output-on-failure -R "^(EngineD3D11Tests|EngineGraphicsTests)$" --no-tests=error
```

Repeat Release, unchanged safety 3/3 and milestone verifiers, inspect real images,
record device identity and resource cleanup, bind frozen source SHA/patch/logs to
substantive independent review. Coordinator owns full builds/native integration.
No existing failure is claimed: this is a new capability, so assertions verify
real GPU behavior and failure recovery rather than marker strings.

## Hosted compile coverage (separate from hardware acceptance)

Normal options OFF leave this optional Windows backend outside ordinary CI.
Add a dedicated Windows-2022 workflow using the reviewed PR92 exact-checkout and
ordered-parent provenance prefix, source commit
`ebec4e6234cc2d7ce6c6926a8aef385acd352278` in `work/astral-current-ci`.
PR events build separate head and merge legs with fail-fast disabled; non-PR
events build their commit exactly once. Each leg configures both backend/test
options ON, builds only EngineD3D11Tests Debug and Release with two jobs, then
runs only `ShaderCompilerReturnsRealDiagnosticsAndRecovers`. This filter uses
CPU shader compilation and never initializes a device. No CTest, hardware case,
WARP, native UI, R0 execution or graphics performance run belongs in this workflow.
Hosted results prove compilation and shader diagnostics only. All local hardware
acceptance stays pending until the user releases the desktop/GPU and headroom
permits an exclusive coordinator slot. No commit/push before coordinator review.
