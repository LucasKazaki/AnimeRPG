# E14 bounded benchmark presentation-policy packet, 2026-09-21

Status: admitted E14 verification/implementation packet on draft PR #9. This packet closes the remaining benchmark VSync/presentation-policy provenance hole before native measurements are interpreted. It does not invoke R0, run Astral on Lucas's PCs, merge/release/deploy, add dependencies, change the graphics API, change Company Runtime state, restart paused content work, or establish performance/parity acceptance.

## Identity and dependency

- Repository: `LucasKazaki/AnimeRPG`.
- Existing owned branch / draft PR: `repair/2026-09-20-r0-runner-safety`, PR #9.
- Pre-packet head: `0ed1e055fe4374747ee86e4710bded06182a83b3`.
- Dependency: E14 run-control, fixed-timestep, stable client-area/resolution, duration-coherence, package/environment, and cross-stream verification already present on this branch.
- Issue #7 remains open. The historical R0 runner is not authorized for this packet.
- Capability impact: E14 profiling/benchmark provenance only. E04 remains the current GDI renderer and no renderer/backend migration is authorized.

## Reproducible gap

`benchmark_manifest.py` already binds a boolean `run_protocol.vsync`, but the production run-control receipt does not state what presentation mechanism Astral actually uses. `Win32Application.cpp` renders through a window `HDC` obtained with `GetDC`, releases that DC after GDI drawing, and uses an unconditional `Sleep(1)` in the frame loop. There is no DXGI/Vulkan/OpenGL swap-chain present interval or equivalent VSync control in the inspected production path. The existing verifier therefore cannot distinguish a truthful `vsync=false` comparison descriptor from a descriptor that incorrectly claims `vsync=true`.

Acceptance for this packet is fail-closed presentation-policy coherence without pretending that GDI exposes swap-chain VSync:

1. The benchmark run-control receipt identifies the production presentation path as `win32_gdi_window_dc`.
2. It identifies VSync control as `unavailable_in_gdi_path` rather than claiming physical display sync is disabled.
3. It records the existing pacing policy as `sleep_1ms_not_refresh_locked`.
4. The verifier requires the SHA-256-bound benchmark descriptor to declare `run_protocol.vsync=false`; a descriptor claiming VSync must fail.
5. The verifier must not promote this source/runtime-policy proof into compositor timing, physical scanout, tearing, GPU-present timing, performance-budget, or parity evidence.

## Primary-source research

Rechecked 2026-09-21:

- Microsoft `GetDC`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getdc . `GetDC` returns a device context for a window client area for subsequent GDI drawing, and common DCs are released with `ReleaseDC`. Applicability: this is Astral's current production presentation surface; it is not a swap-chain present API.
- Microsoft `GdiFlush`: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-gdiflush . It flushes the calling thread's current GDI batch. Applicability: GDI batching/flush semantics do not provide a configurable vertical-refresh interval.
- Microsoft `GdiGetBatchLimit`: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-gdigetbatchlimit . The limit controls how many eligible GDI calls may accumulate before a batch flush. Applicability: again, batching is distinct from VSync/present scheduling.
- Unreal Engine 5.8 console-variable reference: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-console-variables-reference . `r.VSync` explicitly distinguishes disabled (`0`) and enabled (`1`) renderer VSync states. Applicability: a matched comparison must retain the reference engine's VSync policy as an explicit control rather than an unverified label.
- Unity 6.0 `QualitySettings.vSyncCount`: https://docs.unity3d.com/6000.0/ScriptReference/QualitySettings-vSyncCount.html . Values greater than zero synchronize rendering to display refresh intervals; zero disables that synchronization and allows frame-rate pacing controls. Applicability: a comparison manifest must describe an actually controllable presentation policy.

These documents are behavior references only. No proprietary source is copied and no external dependency is imported.

## Allowed paths

Implementation may change only:

1. `Engine/Core/BenchmarkRunControl.cpp`
2. `Tests/BenchmarkRunControlTests.cpp`
3. `Scripts/verify_benchmark_run_control.py`
4. `Scripts/test_benchmark_run_control.py`
5. `Tasks/E14-BENCHMARK-PRESENTATION-POLICY-2026-09-21.md`

After verification, `Docs/QA/E14-BENCHMARK-PRESENTATION-POLICY-2026-09-21.md` may be added as an evidence-only follow-up. No header/API surface, Game, renderer, platform-loop, CMake, workflow, dependency, package-authority, scheduler, or content path is authorized.

## Verification contract

- Increment the run-control receipt schema when presentation fields are added; stale schema-2 receipts must fail closed.
- Native `BenchmarkRunControlTests` must verify the exact presentation fields and retain all existing output/no-overwrite/frame-count/client-area safety checks.
- Python verification must reject missing/changed presentation fields and reject a freshly rehashed benchmark descriptor with `run_protocol.vsync=true`.
- A valid descriptor with `vsync=false` must remain accepted and report presentation-policy coherence without claiming display/compositor synchronization was measured.
- Byte-compile the Python verifier/tests and run the complete run-control Python suite in the available fixture.
- Compile and execute the exact changed `BenchmarkRunControl.cpp` and native test source in external Debug, optimized Release, and Clang ASan+UBSan partial fixtures where the sandbox permits.
- Hosted profiling, Windows Debug/Release, release/package/provenance/reproducibility, and benchmark-environment gates must remain green for the implementation candidate before this packet is treated as hosted-verified.
- Do not weaken timeouts, hashing, schemas, claim boundaries, or existing acceptance tests to make the packet pass.

## Stop, rollback, and native handoff

Stop at the first deterministic regression outside the five allowed implementation paths. Rollback is a revert of this packet only, with no force push or destructive cleanup.

If this packet verifies, the registered Windows executor may run the frozen procedural 3D benchmark only with `run_protocol.vsync=false`, because the current Astral GDI path has no admitted swap-chain VSync control. Retain 60 Hz fixed simulation, 120 warmup frames, 3,600 measured frames, one explicit client resolution, timing/phase/memory captures, package/environment/run-control receipts, cross-stream verification, and a separate matched capture-off control. A future graphics-backend decision may introduce real presentation controls, but that is outside this packet. GPU timing, active-adapter proof, VRAM, approved budgets, matched UE5/Unity 3D and genuine-2D workloads, compositor/scanout timing, clean-machine launch, recovery stress, the real 86,400-second soak, and independent acceptance remain unresolved.
