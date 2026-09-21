# E14 benchmark presentation-policy evidence, 2026-09-21

Status: bounded implementation packet complete and hosted-verified on draft PR #9. This record is evidence for the exact implementation candidate below, not independent acceptance, native performance evidence, a merge/release authorization, or a claim that Astral is comparable to Unreal Engine 5 or Unity.

## Candidate and bounded diff

- Repository: `LucasKazaki/AnimeRPG`.
- Branch / owned draft PR: `repair/2026-09-20-r0-runner-safety`, PR #9.
- Pre-packet head: `0ed1e055fe4374747ee86e4710bded06182a83b3`.
- Implementation candidate: `55618f32db925e975c4e7aa58a9f9e34804c2e77`.
- Packet commit chain:
  - `29fc4d5b241d9c474bc40bf7a4a1be973dba4e0b` task/authority record.
  - `38fc217bcf3a9c859f47c264c52e2f65d81c0233` run-control receipt schema/presentation fields.
  - `92edfe2f29d680564cfb0badeb0042db5ebe6952` native run-control regression checks.
  - `c2e5f5881107ec9380b433c6399eb22e3839cce6` provenance verifier changes.
  - `55618f32db925e975c4e7aa58a9f9e34804c2e77` verifier regression changes.
- Exact changed production/test/task paths from the pre-packet head to the implementation candidate:
  - `Engine/Core/BenchmarkRunControl.cpp` (+4/-1), candidate blob `d01c92ab3c975c1a0759c65970c5bef60abd30d5`.
  - `Tests/BenchmarkRunControlTests.cpp` (+11/-2), candidate blob `ea6045f46eb9c52cf74abceb53ab37ea0b4b76ef`.
  - `Scripts/verify_benchmark_run_control.py` (+24/-5), candidate blob `60044c95d3e9f9539e4956384b58660bd2b19b05`.
  - `Scripts/test_benchmark_run_control.py` (+25/-5), candidate blob `25c5b7deddfde5e71478295f2bd33f796abd5f75`.
  - `Tasks/E14-BENCHMARK-PRESENTATION-POLICY-2026-09-21.md` (+65).
- `git compare` through the connected GitHub API reported the candidate five commits ahead of `0ed1e055...`, zero behind, and only those five paths changed. No Game path, renderer source, Win32 loop source, CMake, workflow, dependency, graphics API, Company Runtime, content, deployment, or release-authority path changed in this packet.

## Reproducible gap and implementation

Before this packet, `benchmark_manifest.py` SHA-256-bound `run_protocol.vsync`, but the production run-control receipt did not state the presentation backend or whether Astral had a runtime VSync control. That permitted an internally rehashed benchmark descriptor to claim `vsync=true` even though the inspected production path does not expose a swap-chain present interval.

The exact candidate's production loop still obtains the window client `HDC` with `GetDC(window_)`, calls the existing GDI renderer with that DC and the measured client rectangle, calls `ReleaseDC`, and then calls `Sleep(1)`. The packet did not modify that loop. It instead makes the benchmark receipt honest about what that path can establish.

`BenchmarkRunControl` receipt schema is now version 3 and adds exact fields:

- `presentation_backend = win32_gdi_window_dc`
- `vsync_control = unavailable_in_gdi_path`
- `frame_pacing = sleep_1ms_not_refresh_locked`

`verify_benchmark_run_control.py` requires those values, requires the bound benchmark descriptor's `run_protocol.vsync` to be exactly `false`, and reports `benchmark_presentation_policy_coherent=true`. A descriptor freshly rebuilt and rehashed with `vsync=true` now fails rather than being accepted. The verifier also carries explicit limitations: this proves the admitted source/runtime policy and descriptor coherence, not that physical display synchronization is disabled. It does not measure DWM/compositor scheduling, scanout, tearing, display refresh, GPU presentation, or the actual duration of `Sleep(1)`.

The Python run-control suite now defines 15 cases, including a positive coherent policy case, rehashed `vsync=true` rejection, stale schema-2 rejection, changed/missing presentation-field rejection, duration/frame-count coherence, immutable evidence, claim-boundary, client-area, role, size, and no-overwrite cases. The native C++ run-control suite retains its nine test groups and now verifies schema 3 plus the three presentation fields in configured-contract and environment-driven receipts.

## Primary-source research, rechecked 2026-09-21

Behavior references only. No proprietary source was copied and no external dependency was imported.

1. Microsoft `GetDC`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getdc
   - Documents that `GetDC` retrieves a device context for a window client area for subsequent GDI drawing, and that a common DC is released with `ReleaseDC`.
   - Applicability: identifies Astral's inspected production drawing surface as Win32 GDI, not a DXGI/Vulkan/OpenGL swap-chain present call.
2. Microsoft `GdiFlush`: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-gdiflush
   - Documents flushing the calling thread's GDI batch.
   - Applicability: GDI batching/flush semantics are not a configurable vertical-refresh interval and do not justify a `vsync=true` benchmark claim.
3. Microsoft `GdiGetBatchLimit`: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-gdigetbatchlimit
   - Documents the eligible-call batch limit.
   - Applicability: reinforces the distinction between GDI batching and presentation synchronization.
4. Epic Unreal Engine 5.8 console-variable reference: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-console-variables-reference
   - Documents `r.VSync` with `0` disabled and `1` enabled.
   - Applicability: a matched reference-engine benchmark must make its presentation policy explicit and controlled.
5. Unity 6.0 `QualitySettings.vSyncCount`: https://docs.unity3d.com/6000.0/ScriptReference/QualitySettings-vSyncCount.html
   - Documents synchronization to vertical refresh for values above zero and no VSync synchronization at zero, with frame pacing then controlled separately.
   - Applicability: a matched comparison needs an actually controlled presentation policy rather than a descriptive label.

Licensing constraint: these public documentation pages are reference material only. This packet copies no Unreal/Unity/Microsoft implementation code and adds no third-party engine or library.

## Verification evidence for exact candidate `55618f32...`

### Hosted profiling and sanitizer lane

- Workflow: `Profiling capture portability`.
- Run: `35631749721`.
- Job: `106439349758`, Ubuntu 24.04.
- Result: **passed** for exact head `55618f32db925e975c4e7aa58a9f9e34804c2e77`.
- Passed steps:
  - complete profiling/provenance Python analysis step, including `Scripts/test_benchmark_run_control.py`;
  - production-linked external CMake Debug and optimized Release profiling-capture contracts;
  - Clang ASan+UBSan contracts with `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.
- The workflow file confirms that the same step also runs timing, phase, process-memory, stream-coherence, and environment suites before the C++ contracts.

### Hosted Windows build and deterministic tests

- Workflow: `Windows build and deterministic tests`.
- Run: `35631749750`.
- Job: `106439752691`, Windows Server 2022.
- Result: **passed** for exact head `55618f32db925e975c4e7aa58a9f9e34804c2e77`.
- Passed steps include R0 safety-contract parsing/tests without invoking the historical runner, Release assertion/CTest safety, VS2022 x64 configuration, actual Debug build and deterministic tests, actual Release build and deterministic tests, PE dependency/prerequisite/runtime/bootstrap checks, static milestone verifiers, and clean tracked-tree verification.

### Release/package/provenance lane

- Workflow: `Release manifest integrity`.
- Run: `35631749699`.
- Job: `106439387292`, Windows Server 2022.
- Result: **passed** for exact head `55618f32db925e975c4e7aa58a9f9e34804c2e77`.
- Manifest, runtime-receipt, restart-stress receipt, continuous-soak contract, soak-analysis contract, benchmark-manifest, and PE-repro diagnostic checks passed.
- Two independent hosted builds of the same Release candidate were classified as **byte-identical** by the existing reproducibility gate.
- The subsequently configured Release executable, exact staged package verification, hosted benchmark-manifest fixture binding, and clean-tree check also passed.
- These are contract/reproducibility results, not a real 86,400-second native soak or clean-user-machine launch result.

### Benchmark environment lane

- Workflow: `Benchmark environment evidence` run `35631749870`.
- Portable job `106439350468`: **passed**.
- Windows CIM job `106439350797`: **passed**, including capture of a real environment receipt on the ephemeral hosted Windows runner.
- That CIM receipt describes GitHub's hosted machine only. It is not evidence about Lucas's PC or the eventual reference benchmark machine.

### Sandbox limitation

A direct container-side source clone was attempted through `git ls-remote` and could not resolve `github.com` in the sandbox network, so no claim is made that this candidate was independently compiled in the local container. Rather than reconstruct a potentially divergent manual tree, this record relies on the exact-SHA hosted full checkouts above for compiler/sanitizer evidence. This is a tooling limitation, not a code failure.

## Capability-to-evidence map impact

This packet changes evidence quality for **profiling/benchmark protocol only**. It does not reduce the remaining engine catalogue to an easier subset.

- Runtime/jobs/memory: existing fixed simulation, process-memory capture/analysis and safety evidence retained; allocator ownership and mature jobs/memory systems remain unresolved.
- Scene ownership/serialization: unresolved parity work retained.
- Asset pipeline: prior bounded mesh/import work remains separate; production-scale import/cook/DDC-style workflows remain unresolved.
- GPU rendering/materials: current inspected E04 production path remains Win32 GDI. No GPU-backend, material-system, GPU timestamp, active-adapter, or VRAM parity claim is added.
- Lighting/shadows/reflections: unresolved.
- Large-world streaming/detail and terrain: unresolved.
- Animation: unresolved.
- Physics/collision: unresolved.
- AI/navigation: unresolved.
- Audio: unresolved.
- UI/editor tools: unresolved.
- Genuine 2D: remains a required matched workload/capability; not replaced by a 3D benchmark.
- Networking: unresolved.
- Profiling: presentation-policy provenance is now fail-closed for the current GDI path; CPU timing/phase/process-memory evidence remains useful, but GPU presentation/timing and approved budgets remain unresolved.
- Packaging/platforms: deterministic hosted packaging evidence retained; clean-machine launch and additional target platforms remain unresolved.
- Particles/VFX, cinematics, scripting/reflection, input/replay, accessibility/localization: all remain explicit unresolved catalogue rows.
- Comparative acceptance: blocked on matched, versioned UE5/Unity reference workloads and settings, measured correctness/performance/memory/reliability/tooling evidence, native gates, and independent review.

## Claim boundaries and native handoff

Issue #7 remains open. R0 was **not invoked** in this packet. A passing R0-safety unit-test step is not authority to use the historical runner, and this author's review is not independent review.

The single next useful action is the registered Windows executor's frozen procedural 3D benchmark on the implementation candidate (or an explicitly re-bound later candidate), using the already admitted controls:

- fixed simulation: 60 Hz;
- warmup: 120 frames, exactly 2.0 descriptor seconds;
- measured interval: 3,600 frames, exactly 60.0 descriptor seconds;
- one manifest-declared client resolution, created and verified through the existing client-area controls;
- `run_protocol.vsync=false` because the current admitted GDI path has no VSync control;
- whole-frame timing, main-thread phase timing, and process-memory capture using their existing exact evidence contracts;
- package, Windows environment, and run-control receipts;
- cross-stream coherence verification before any numbers are interpreted;
- a separate matched capture-off control to quantify instrumentation overhead.

The local executor must retain source SHA, exact package/executable hash, machine/OS/toolchain/GPU/driver identity, command lines and environment controls, stdout/stderr, exit codes, UTC timestamps, evidence hashes, and required captured images. The run-control receipt's `sleep_1ms_not_refresh_locked` value is a source/runtime-policy assertion, not a measured wait duration.

Still deferred to genuine native evidence or later authorized packets: physical display/compositor/scanout behavior, GPU timestamps, active render-adapter proof, VRAM, approved frame-time/RAM budgets, instrumentation-overhead result, matched UE5/Unity 3D and genuine-2D scenes, clean-machine package launch, recovery stress, the actual 86,400-second soak, and independent acceptance. Do not mark Astral comparable or complete while those and the remaining engine catalogue are unresolved.
