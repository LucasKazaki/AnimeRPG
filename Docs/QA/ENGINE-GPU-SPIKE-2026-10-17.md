# D3D11 offscreen spike QA

Owner `/root/numeric_implementation`, reviewer/coordinator `/root`.
Base `5245a3dd079083a534b4afd16924a6a5496d1a0d`, dedicated
`engine/d3d11-offscreen-20261017` in `work/astral-d3d11`.

## Current evidence boundary

Hosted C++ and actual production HLSL compilation have passed on the exact head
and synthetic merge recorded below. After an actual CPU-only oracle failure and
independently reviewed finite guard, local Debug/Release oracle cases and the
registered D3D11/Graphics matrix pass. Hardware was the actual NVIDIA GeForce RTX
5080, feature level 11_0, debug layer active. The coordinator inspected all eight
paired/difference/object-ID captures. Fixed-workload timing/memory budgets pass.
This is offscreen opaque Unlit validation; native presentation, a driver
live-object baseline and the full GPU milestone remain open. The first hosted
compile and oracle failures remain retained. Local source/receipts await final
commit binding and coordinator review; no newer hosted head is implied by the
earlier 91f6966 shader receipts. Other applications/processes are untouched.

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

Seven original behavioral cases were authored: shader diagnostics/recovery; actual world
HDR/ID/texture parity; depth/clipping/winding; pointer-identity replacement/mips;
invalid input/budgets/thread/zero viewport/output preservation; twenty
resize/reload/reinitialize cycles; fixed 1280x720 hardware timings and memory.
They were unexecuted at the initial preparation checkpoint. Actual execution,
including the three later oracle cases, is recorded at the end of this receipt.

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
satisfy local hardware/performance/image acceptance. The coordinator subsequently
reviewed the complete
eleven-path candidate, recomputed matching source/whole hashes, published it as
draft PR94, and retained the first failed hosted attempt below.

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

## First hosted compile attempt: failed, retained

Draft [PR94](https://github.com/LucasKazaki/AnimeRPG/pull/94) published WIP commit
`e6885ae0658ea638c024dc860bb8743442076a98` against frozen base
`2c69888774c074a69ec12c5f659e3632280db420`. Its source patch was the reviewed
`2399E8E25C1365C083D2AA903DB49A5BE0B0ED4116EA22190EC8565203D202C2`; complete
eleven-path patch was
`814CC840DCD0FDD3DB767332F2E1838D24F942A98558BDB13B3E7B7183F339BA`.

[Hosted run 39063303365](https://github.com/LucasKazaki/AnimeRPG/actions/runs/39063303365)
executed both PR legs on October 17, 2026. Exact checkout/ordered-parent checks
passed, followed by successful configure. Both compiled AstralEngineRuntime and
AstralD3D11 Debug static libraries, then failed compiling EngineD3D11Tests.

| Job | Exact checkout | Debug compile failure UTC |
| --- | --- | --- |
| [Head 117485605758](https://github.com/LucasKazaki/AnimeRPG/actions/runs/39063303365/job/117485605758) | `e6885ae0658ea638c024dc860bb8743442076a98` | 19:38:54 |
| [Merge 117485606050](https://github.com/LucasKazaki/AnimeRPG/actions/runs/39063303365/job/117485606050) | `3914f228d5d5408aa708c58710450f79b62d4d72` | 19:39:07 |

The merge's ordered parents were
`2c69888774c074a69ec12c5f659e3632280db420` and
`e6885ae0658ea638c024dc860bb8743442076a98`. The first diagnostic in both jobs was
EngineD3D11Tests.cpp:231 C2513, Material missing a variable before `=`, followed
by cascading syntax errors. Windows SDK minwindef.h defines legacy `near`/`far`
macros as empty; those names erased two fixture-local identifiers. The bounded
correction renames them to `nearMaterial`/`farMaterial` and updates their uses.
No production, CMake, workflow, budget or tolerance changed.

Both CPU shader filter steps and Release build steps were skipped. No HLSL
compilation, device creation, rendering or hardware measurement ran in this
attempt. It does not establish even complete hosted compile acceptance.
Actual run metadata and complete head/merge logs are retained externally in
`work/d3d11-evidence/hosted-39063303365.json`, `hosted-first-head.log`,
`hosted-first-merge.log` and `hosted-39063303365-watch.log`. No identical rerun was
requested. The reviewed correction was committed as
`5c94667c0456cc5e7ad79f5866527ff630f3458a`; its exact two-file delta SHA256 was
`0BBCD2C3D0577D9C05239939DD336A8B56D5521255169C77A41830F9F3A2021A`.
The coordinator bound/published it for the new immutable run below. Human
desktop/GPU hold and local RAM/build/test hold remain in force.

## Second hosted attempt: C++ compile and existing diagnostics pass

[Run 39063541722](https://github.com/LucasKazaki/AnimeRPG/actions/runs/39063541722)
completed successfully on October 17, 2026, before the next test edit.

| Job | Exact checkout | Start / finish UTC | Actual result |
| --- | --- | --- | --- |
| [Head 117486326133](https://github.com/LucasKazaki/AnimeRPG/actions/runs/39063541722/job/117486326133) | `5c94667c0456cc5e7ad79f5866527ff630f3458a` | 19:41:29 / 19:43:47 | Debug/Release target builds; one CPU diagnostic case passes each |
| [Merge 117486326033](https://github.com/LucasKazaki/AnimeRPG/actions/runs/39063541722/job/117486326033) | `db2aa0beedb094f38e3f4334b3cd11bfc1302a18` | 19:41:29 / 19:43:29 | Debug/Release target builds; one CPU diagnostic case passes each |

Both exact-checkout provenance steps passed. The merge's ordered parents were
`2c69888774c074a69ec12c5f659e3632280db420` and
`5c94667c0456cc5e7ad79f5866527ff630f3458a`. Clean tracked-tree steps passed.
Complete logs/metadata: `work/d3d11-evidence/hosted-second-head.log`,
`hosted-second-merge.log`, `hosted-39063541722-final.json` and its watch log.
These runs compiled all Windows backend/test C++ in both configurations and
executed the deliberately invalid plus trivial valid shader compiler fixtures.
They did not call Initialize or compile the actual built-in VS/PS, which was
previously compiled only inside device initialization. Thus they establish C++
and diagnostic-helper coverage, not production HLSL or hardware acceptance.

## Production HLSL coverage extension: prepared, then hosted pass

After that immutable run completed, the coordinator requested the same unique
CPU-only filter additionally compile the exact production
`Detail::kOpaqueUnlitShader` through the existing production CompileShader helper:
VSMain/vs_5_0 and PSMain/ps_5_0, each with debug and optimized flags. The test
includes the existing BuiltinShaders header, asserts successful/nonempty bytecode
and prints full diagnostics on failure. It keeps the existing invalid/valid
diagnostic assertions, duplicates no HLSL, and creates no device. No production,
CMake, workflow, hardware assertion, budget or tolerance changes.

At the initial preparation checkpoint this extension had not run. The
coordinator reviewed and committed it as
`91f6966d4880a279b1a96bb9417439bc826465c9`, then confirmed all eleven hosted PR
checks passing. The relevant dedicated CPU-only shader workflow was
[run 39063908626](https://github.com/LucasKazaki/AnimeRPG/actions/runs/39063908626).

| Job | Actual checkout | Production shader cases Debug / Release UTC |
| --- | --- | --- |
| [Head 117487416987](https://github.com/LucasKazaki/AnimeRPG/actions/runs/39063908626/job/117487416987) | `91f6966d4880a279b1a96bb9417439bc826465c9` | 19:47:50 / 19:48:55 |
| [Merge 117487417242](https://github.com/LucasKazaki/AnimeRPG/actions/runs/39063908626/job/117487417242) | `84027f627a6c120fdd847f96b12b03f7f5e236d4` | 19:47:56 / 19:49:10 |

The merge's actual ordered parents were
`2c69888774c074a69ec12c5f659e3632280db420` and
`91f6966d4880a279b1a96bb9417439bc826465c9`. Both jobs built the complete optional
C++ target Debug/Release and ran only the one selected CPU shader test in each
configuration. It retained invalid/valid shader diagnostics and compiled actual
VSMain/PSMain using both debug and optimized flags: VS/PS bytes 17324/19828 and
1380/1800 respectively. Full logs were downloaded and read at
`work/d3d11-evidence/hosted-production-head.log` and
`hosted-production-merge.log`. No renderer or device was initialized. All local
hardware, visual, performance, device-failure and live-object validation remains
pending; hosted compilation cannot satisfy those gates.

## Comparison-oracle old witness prepared; not yet executed

The coordinator independently found that the untextured comparison can accept a
NaN in the CPU HDR reference: `std::max` can retain its prior finite maximum and
the mean is checked only for textured comparisons. A CPU-only regression now
builds matching 16x16 foreground HDR/ID buffers and places exactly one quiet NaN
in the interior CPU red channel, then invokes the actual unchanged `Compare`
function with `textured=false`. A private self-child argument prevents this
expected abort from stopping the complete suite. The parent launches only its
own executable, with a hidden console, waits at most five seconds, closes its
handles, and requires the child's assertion-abort exit code 3. The old oracle
returns 0 instead, so the parent check must fail before the fix.

This preparation changes only EngineD3D11Tests.cpp and this receipt. It does not
change the oracle, production backend, shaders, workflow, CMake, performance
budgets or comparison tolerances. No configure, build or test ran during this
preparation. The user has now released desktop/GPU access; local execution still
requires the coordinator's exclusive slot and fresh memory headroom. The live
demonstration deadline is October 18 at noon America/New_York (16:00 UTC).
The old child/parent failure must be retained before adding finite guards.

### Actual CPU-only old-oracle failure

The coordinator reviewed the prepared witness and granted an exclusive CPU-only
slot. On October 18 at 03:53:38 UTC, VS2022 x64 configure with backend and hardware
test-target options ON completed with exit 0 (3.85 seconds). Only the Debug
EngineD3D11Tests target was built, with two jobs: exit 0 in 27.696 seconds. At
03:54:26 UTC, only `ComparisonOracleRejectsNonfiniteCpuReference` ran. The hidden
child returned 0 after accepting the CPU NaN, and the parent failed its genuine
`RunCompareNonfiniteProbe() == 3` assertion with exit 3 in 0.162 seconds. This is
the expected pre-fix failure; the shell receipt checker returned 0 after verifying
the actual child/parent outputs and exits.

The test source SHA256 before and after was identical:
`5D3C9AE60006013E7962AA8C5FC93BF6605A82458280D3E8A6E95F45072D1D8F`.
Receipts are `work/d3d11-evidence/oracle-old-configure.log`,
`oracle-old-build-debug.log`, `oracle-old-test-debug.log` and
`oracle-old-receipt.json`. No shader compilation, renderer, graphics device,
hardware test, CTest or desktop action ran. The build session completed, both
process handles closed, and no executable under the owned build prefix remained.
The exclusive slot was explicitly released. The oracle is still unchanged;
finite guards require coordinator acceptance of this receipt first.

### Finite guard after accepted old receipt

The coordinator accepted the actual old receipt and authorized this bounded
correction. `Compare` first validates matching nonempty HDR/ID shapes, then
checks every RGB channel of both CPU and GPU HDR buffers for finiteness in a
separate pass. This includes boundary and background pixels and precedes all
boundary exclusions, subtraction and maximum/mean arithmetic. All existing
coverage bands, coverage fraction and RGB error tolerances remain unchanged.

The same private CPU-only self-child now supports twelve invalid variants:
CPU/GPU side, NaN/+infinity/-infinity and untextured/textured comparison mode.
The three invalid kinds exercise red, green and blue channels respectively;
every invalid variant must abort with code 3. The original CPU NaN witness
remains registered separately. Two matching finite foreground controls must
return 0, one in each comparison mode. All use actual `Compare`, matching 16x16
arrays and the same hidden, bounded child/handle cleanup path. The earlier
private old-witness argument remains supported. The existing production shader
checks are unchanged. There are now ten registered cases, of which three are
comparison-oracle cases. The coordinator independently reviewed this exact source
at SHA256 `B93563058A618D99622295B4701C1E4DCD2B4C7CA43AE932C1FF42A6678A81C8`
and cleared the fixed CPU-only run recorded below.

Only the test and this QA file changed. Backend production, shaders, workflow,
CMake and declared budgets are untouched. No hardware result is inferred from an
oracle correction or CPU shader coverage.

### Actual fixed CPU-only matrix

The coordinator granted an exclusive CPU-only slot after the previous owner
explicitly released it. The prepared runner independently parsed with zero
PowerShell AST errors and checked HEAD
`91f6966d4880a279b1a96bb9417439bc826465c9`, the reviewed test source hash, existing
backend/test-target options ON, and at least 8 GiB available before each two-job
target build. It did not invoke CTest or an unfiltered test process.

On October 18 from 04:14:17 to 04:15:01 UTC:

| Configuration | EngineD3D11Tests build | ComparisonOracle filter |
| --- | --- | --- |
| Debug | exit 0, 1.910 seconds | exit 0, 0.407 seconds, three cases pass |
| Release | exit 0, 40.323 seconds | exit 0, 0.409 seconds, three cases pass |

Each configuration recorded thirteen invalid-child aborts with code 3 (the
original witness plus twelve variants) and two finite-child exits 0. Available
host memory before the Release build was 14,159,577,088 bytes. Test source SHA256
before/after matched the independently reviewed B9356305 hash above. No owned
build-prefix process remained; the build session completed exit 0 and the CPU
slot was explicitly released.

Exact structured receipt and full logs:
`work/d3d11-evidence/oracle-fixed-20261018-041417-receipt.json`,
`oracle-fixed-20261018-041417-Debug-build.log`, `-Debug-test.log`,
`-Release-build.log` and `-Release-test.log`; prepared runner
`work/d3d11-evidence/run-oracle-fixed.ps1`. No renderer, graphics device, shader
test, audio device, native UI or hardware/performance case ran in this slot.
Hardware acceptance still requires its own explicit coordinator grant.

## Actual exclusive RTX 5080 hardware matrix

After inspecting the fixed CPU-only receipts, the coordinator separately granted
the hardware/CPU slot. The external runner checked reviewed HEAD 91f6966 and
test SHA256 B9356305, admitted at least 8 GiB before two-job affected builds and
hardware execution, and ran only the registered anchored D3D11/Graphics pair
with verbose output, stop-on-failure and one test at a time. Captures were scoped
to `work/d3d11-evidence/Debug` and `Release`; prior evidence was not overwritten.
No desktop window, player/editor, WARP, driver installation or system setting
was used. From October 18 04:16:23 to 04:17:24 UTC:

| Configuration | Affected build | Registered CTest result |
| --- | --- | --- |
| Debug | exit 0, 8.318 seconds | 2/2, 34.44 seconds: ten D3D11 cases and sixteen Graphics cases |
| Release | exit 0, 10.134 seconds | 2/2, 7.70 seconds: ten D3D11 cases and sixteen Graphics cases |

The ten D3D11 cases comprise three CPU oracle cases, one shader compiler case
and six actual hardware cases. Actual adapter identity in both configurations:
NVIDIA GeForce RTX 5080, vendor `10de`, device `2c02`, LUID `73259`, feature level
`b000` (11_0), debug layer `1`. No missing-debug-layer fallback occurred. All
device/compiler initialization diagnostics reported success. Expected invalid
inputs/budget/thread rejections preserve the prior frame and recover; twenty
resize/reload cycles and explicit shutdown/reinitialize pass.

The real GameWorld fixture has two draws, 28 vertices, fourteen triangles and
one texture at 1280x720. The World untextured interior max absolute RGB error was
0.000000238. Repeat texture max/mean were 0.001921505/0.000048647; Clamp plus
emissive/vertex-toggle max/mean were 0.000313818/0.000026770. Coverage differed at
ten of 921,600 pixels, all inside the existing one-pixel band. Separate
depth/winding/clipping, same-pointer data replacement and actual mip checks pass.
No coverage/error tolerance was changed after measurement.

Thirty hardware warmup frames preceded 120 measured frames in each configuration.
GPU timestamps were valid. The complete logs retain min/p50/p95/p99/max for every
phase. Recorded p95 values in milliseconds:

| Phase | Debug | Release |
| --- | ---: | ---: |
| Actual GPU opaque Unlit pass | 0.030880 | 0.021536 |
| CPU preparation/upload | 12.053200 | 2.633600 |
| CPU submission | 0.377200 | 0.307100 |
| CPU completion/readback | 66.590700 | 14.802000 |
| Synchronous total | 78.568700 | 17.327200 |

GPU-pass p95 <=16.67 ms and synchronous p95 <=100 ms both pass. This tiny fixture
does not establish general scene performance. The separately recorded CPU full
Render reference (two warmups, ten samples) includes CPU post and is not a
pass-equivalent speed comparison.

Fixed-workload logical resource bytes were 40,553,400 (eleven logical resources).
Process private bytes were 226,021,376 Debug and 225,644,544 Release, below the
declared 768 MiB limit. DXGI reported local usage 56,193,024 and budget
15,947,792,384 bytes in both final workload receipts. Lowest sampled available
host memory was 13,813,366,784 Debug and 13,750,874,112 Release, above the 6 GiB
runtime guard. At the small-size resource check, logical count/bytes returned to
eleven/636,600 after twenty cycles. This logical accounting and bounded process
memory do not establish the actual driver live-object baseline or prove no leak.

Complete command/results receipt, build/verbose CTest logs and unchanged checks
are under `work/d3d11-evidence/hardware-20261018-041623`: `receipt.json`,
`Debug-build.log`, `Debug-ctest.log`, `Release-build.log`, `Release-ctest.log`,
`safety-milestones.json`, the four corresponding check logs and
`final-cleanup.json`. The test source hash before/after remained exactly
`B93563058A618D99622295B4701C1E4DCD2B4C7CA43AE932C1FF42A6678A81C8`.
No first hardware failure occurred or was discarded.

The coordinator viewed all four actual PNGs in each Debug/Release capture folder:
`d3d11-cpu.png`, `d3d11-gpu.png`, `d3d11-difference-x16.png` and
`d3d11-object-ids.png`. Root's inspection found aligned scene framing, foreground
occlusion, textures and colors; differences were sparse/minor and IDs distinct
and coherent, with no apparent missing geometry or visual artifacts. This
inspection is only for the bounded offscreen fixture, not player/editor output.

Unchanged test safety passed 3/3 in 5.575 seconds; M1, M2 and M3 static verifiers
all exited 0. Final cleanup at 04:18:09 UTC found zero owned build/compiler/test
processes, the same source hash and 13,802,909,696 available bytes. Both runner
sessions completed; the exclusive slot was explicitly released.

Final test/QA delta remains only the two reviewed paths. Backend production,
shaders, CMake, workflows, budgets and tolerances are unchanged. Actual driver
live-object/leak baseline, device-failure simulation/recovery, player/editor
integration, swapchain/minimize/restore, other material/environment/post/HUD
passes, constrained hardware, packaging and a long soak remain open. Await final
coordinator receipt review and exact committed-source binding before publication.
