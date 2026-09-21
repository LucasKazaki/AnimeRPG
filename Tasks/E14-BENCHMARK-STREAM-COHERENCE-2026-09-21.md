# E14 bounded benchmark stream coherence packet, 2026-09-21

Status: admitted bounded E14 verification packet on draft PR #9. This packet proves that Astral's whole-frame timing, main-thread phase timing, and process-memory streams belong to one verified benchmark/package identity and one coherent measured frame interval. It does not run Astral on Lucas's PCs, invoke R0, establish a performance/RAM/VRAM budget, prove leak freedom, measure GPU work, merge/release/deploy, add a dependency, change the graphics architecture, or restart paused content work.

## Identity and dependency

- Repository: `LucasKazaki/AnimeRPG`.
- Owned branch / existing draft PR: `repair/2026-09-20-r0-runner-safety`, PR #9.
- Pre-packet head: `6c6cbc95bb88810b13f5ab127d3b618363e05810`.
- Dependencies: the existing E14 whole-frame, main-thread phase, and process-memory analyzers on this branch.
- PR #9 remains stacked on PR #6 / `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`.
- Issue #7 remains open. R0 must not be invoked by this packet.
- Capability impact: E14 evidence coherence only. No E00-E17 capability becomes comparable or independently accepted.

## Reproducible gap

Each existing analyzer proves one stream is bound to a valid benchmark manifest and package, but no single verifier requires all three roles in one descriptor and checks their measured frame ranges together. A future native bundle could therefore contain individually valid files with mismatched warmup/ranges, missing scheduled memory samples, changed hashes, or fixture memory evidence presented as native. Acceptance here is structural/provenance coherence only, not timing accuracy or performance acceptance.

## Allowed paths

Only these paths may change:

1. `Scripts/verify_benchmark_stream_coherence.py`
2. `Scripts/test_benchmark_stream_coherence.py`
3. `.github/workflows/frame-timing-validation.yml`
4. `Tasks/E14-BENCHMARK-STREAM-COHERENCE-2026-09-21.md`
5. `Docs/QA/E14-BENCHMARK-STREAM-COHERENCE-2026-09-21.md`

No Engine/Game/CMake source, renderer/API, dependency, package authority, Company Runtime state, scheduler, permission, release, deployment, or content path is authorized.

## Primary-source research

Rechecked on 2026-09-21:

- Epic Games, Unreal Engine 5.8, Timing Insights: https://dev.epicgames.com/documentation/unreal-engine/timing-insights-in-unreal-engine . It presents CPU/GPU activity as related tracks within one trace session and selected time/frame ranges. Applicability: related Astral streams need one retained benchmark/session identity and explicit frame interval.
- Epic Games, Unreal Engine 5.8, Trace: https://dev.epicgames.com/documentation/unreal-engine/trace-in-unreal-engine-5 . Frame, CPU, GPU, memory-related, bookmark, and screenshot channels share trace context. Applicability: Astral adopts only the evidence principle, not UE internals.
- Unity 6.0 / 6000.0, `FrameTimingManager`: https://docs.unity3d.com/cn/6000.0/ScriptReference/FrameTimingManager.html . Captures/accesses multiple frames. Applicability: Astral timing streams must identify the same frame set before combined interpretation.
- Unity 6.0 / 6000.0, `CaptureFrameTimings`: https://docs.unity3d.com/jp/current/ScriptReference/FrameTimingManager.CaptureFrameTimings.html . Only complete valid finished frames are retained, so actual counts can vary. Applicability: verify retained ranges instead of assuming requested counts.
- Unity 6.0 / 6000.0, `Profiler.EmitSessionMetaData`: https://docs.unity3d.com/cn/6000.0/ScriptReference/Profiling.Profiler.EmitSessionMetaData.html . Associates metadata with a profiler session. Applicability: Astral's immutable benchmark descriptor is session-level provenance.
- Unity 6.0 Profiler API: https://docs.unity3d.com/cn/6000.0/ScriptReference/Profiling.Profiler.html . Profiling can affect application performance. Applicability: instrumentation-overhead acceptance stays false.
- Unity 6.0 Performance Testing API 3.2.0: https://docs.unity3d.com/cn/6000.0/Manual/com.unity.test-framework.performance.html . It collects configuration metadata. Applicability: Astral retains environment/protocol metadata but still requires machine receipts.

No proprietary engine source is copied and no third-party dependency is added.

## Implementation contract

`Scripts/verify_benchmark_stream_coherence.py` must fail closed and:

- re-run `benchmark_manifest.verify_benchmark_manifest`, including package/release/evidence hashes and descriptor hash;
- require exact caller-supplied candidate revision and `AstralGame.exe` SHA-256;
- require exactly one `cpu_frame_timing_csv`, `cpu_phase_timing_csv`, and `process_memory_csv` entry;
- independently resolve/re-hash those paths and reject traversal, drive/backslash paths, symlinked roots/components, root escape, changed sizes, or changed hashes;
- parse all three raw files with the production analyzers;
- require equal warmup values and exact warmup start for all streams;
- require whole-frame and phase streams to have identical contiguous start/end/count plus matching max-sample and saturation state;
- treat process memory as scheduled downsampling, requiring every declared-stride sample within the timing interval and the exact final scheduled frame;
- require the production Windows `GetProcessMemoryInfo` source when benchmark provenance is `local_native`, rejecting fixture-source laundering;
- emit a fresh JSON receipt binding descriptor/candidate/package, benchmark metadata, exact three stream path/size/hash records, and alignment results;
- refuse overwrite; and
- keep performance, RAM, VRAM, leak-free, GPU, parity, overhead, clean-machine, and independent-acceptance claims hard-false.

## Verification contract

Regression coverage must include coherent streams, valid memory downsampling, warmup mismatch, phase-range mismatch, missing scheduled memory samples, timing capture-limit mismatch, local-native fixture-source rejection, exclusive output, real release/benchmark-manifest and all-three-production-parser integration, raw-stream tamper rejection, candidate/hash mismatch, manifest-bound interval mismatch, and native provenance laundering rejection.

The existing profiling portability workflow must run this suite alongside the three analyzer suites before Debug, optimized Release, and Clang ASan+UBSan/leak capture contracts. Existing Windows/package workflows must stay green for the final candidate. Because this packet changes no C++ source, those lanes are regression evidence only.

## Native handoff

For the registered native benchmark, use one benchmark manifest containing all three roles, then run:

```powershell
python Scripts/verify_benchmark_stream_coherence.py `
  --benchmark-manifest <benchmark.json> `
  --package-root <exact-package-root> `
  --release-manifest <release-manifest.json> `
  --evidence-root <immutable-evidence-root> `
  --expected-commit <40-hex-admitted-revision> `
  --expected-executable-sha256 <64-hex-AstralGame.exe-sha256> `
  --output <fresh-stream-coherence.json>
```

Retain source/package hashes, raw CSVs, manifest/analysis/coherence hashes, machine/OS/toolchain/CPU/RAM/GPU/driver identity, resolution/window/VSync, exact command/environment, UTC timestamps, stdout/stderr, exit codes, runtime logs, and required images. A matched capture-off control remains required before instrumentation overhead can be evaluated.

## Stop, rollback, next action

Stop on the first deterministic parser/integration/CI failure until understood within these paths. Never weaken provenance, alignment, hash, or acceptance guards merely to turn CI green. Do not force-push or discard concurrent work. Keep PR #9 draft, issue #7 open, and R0 uninvoked.

After this packet, the single useful next action is the already specified frozen procedural 3D benchmark on the registered Windows executor: 120 warmup frames, 3,600 whole-frame and phase samples, process-memory sampling on its declared stride, and a matched capture-off control. Bind all three raw streams to one benchmark manifest and require this verifier to pass before interpreting individual timing or memory analyses. This coordinator must not operate that local executor.
