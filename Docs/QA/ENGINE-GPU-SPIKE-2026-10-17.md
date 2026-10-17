# D3D11 offscreen spike QA

Owner `/root/numeric_implementation`, reviewer/coordinator `/root`.
Base `5245a3dd079083a534b4afd16924a6a5496d1a0d`, dedicated
`engine/d3d11-offscreen-20261017` in `work/astral-d3d11`.

## Current evidence boundary

Source preparation only. No configure, compilation, GPU device creation,
rendering, native UI, performance result or image inspection has run for this
packet. Production source pre-review is clean; workflow review, final binding
and all execution receipts are pending.
The coordinator currently holds hardware execution pending desktop/GPU ownership
and RAM headroom. Other applications/processes are untouched.

The task declares fixed budgets/tolerances before measurement. No old failure is
claimed for this new feature. The planned hardware suite uses the actual
`GameWorld::BuildRenderScene` path and links `AstralEngineRuntime`/`AstralD3D11`,
without Showcase or game registration. CPU reference is `SceneRenderer` HDR/IDs.
Compiler error tests call the production compiler helper. Hardware tests never
substitute WARP or skip an unavailable adapter as a pass.

## Source contracts to review

- Separate opt-in Windows library, normal options OFF; no portable source-list or
  existing editor/player consumer changes. All tests use `astral_add_test`.
- Real vertex/index buffers, DrawIndexed, VS/PS, depth target and linear texture
  upload; two render targets return owned HDR and object IDs.
- Checked resource admissions precede per-frame upload; frame-local dedup owns
  COM resources, with no borrowed pointer retained across calls. Data mutation
  during a call violates the documented single-owner contract.
- Transactional output, RowPitch-aware no-wait staging, completion deadline,
  typed operation/HRESULT/removal diagnostics and explicit reinitialization.
- GPU timestamp/disjoint timing is distinct from CPU upload/submission/readback.
  Report DXGI local usage as driver-managed memory, not exact resource accounting.

Seven behavioral cases are authored: shader diagnostics/recovery; actual world
HDR/ID/texture parity; depth/clipping/winding; pointer-identity replacement/mips;
invalid input/budgets/thread/zero viewport/output preservation; twenty
resize/reload/reinitialize cycles; fixed 1280x720 hardware timings and memory.
These are unexecuted assertions, not passing receipts.

`FrameStats::resourceCount` is a logical per-frame buffer/texture admission count,
not a query of actual driver live objects. The twenty-cycle test's constant
logical counts and process-private cap cannot prove a return to a driver
live-object baseline or establish no leak. Source review finds coherent ComPtr
RAII, but actual driver live-object/leak validation remains open. DXGI local
usage is driver-managed accounting and likewise is not a no-leak proof.

Root independently reviewed all current production/compiler/shader/CMake and
seven fixtures at source patch SHA256
`2399E8E25C1365C083D2AA903DB49A5BE0B0ED4116EA22190EC8565203D202C2`:
source pre-review clean; all compilation/hardware gates still pending. The
coordinator requested the scope expansion to eleven files before candidate
publication: new `.github/workflows/windows-d3d11-compile.yml`, leaving the
existing workflow untouched. It copies the exact reviewed provenance prefix
from PR92 source commit `ebec4e6234cc2d7ce6c6926a8aef385acd352278`, with distinct
PR head/merge legs or one non-PR commit leg and exact SHA/ordered-parent checks.
Each Windows-2022 leg configures the backend/tests ON, builds only
EngineD3D11Tests Debug/Release with two jobs, and executes only the CPU
`ShaderCompilerReturnsRealDiagnosticsAndRecovers` filter. It never runs CTest,
Initialize, rendering, WARP or native UI. Hosted compile/shader success cannot
satisfy local hardware/performance/image acceptance. These hosted runs are also
pending until the coordinator reviews and authorizes candidate publication.

The selected adapter identity must be recorded during actual hardware execution;
the first non-software DXGI adapter is not assumed to be the RTX5080 when more
than one hardware adapter exists. No actual adapter was selected in preparation.

## Required receipts before acceptance

Bind source patch/hash and commit to exact VS2022 x64 Debug/Release configure,
affected build and registered CTest commands; record actual hardware adapter,
feature level/debug-layer state, shader diagnostics, image differences and
declared timing/memory limits. Retain failures, inspect paired/difference/ID PNGs,
run unchanged safety/milestone checks, verify owned process cleanup and obtain
substantive independent review. Coordinator owns full native matrix/integration.

Full GPU milestone remains incomplete even if this increment passes: Unlit only,
offscreen only, no player/editor/swapchain/native resize/minimize/restore,
lighting/environment/post/transparency/HUD or physical device-loss recovery.
Per-call uploads are not a residency system or a general performance claim;
CPU reference includes CPU post and is explicitly not pass-equivalent timing.
No 24-hour soak, constrained-hardware, packaging or whole-engine acceptance is
inferred from this packet.
