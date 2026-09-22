# E14 profiling capture-state CI repair and verification, 2026-09-21

Status: bounded E14 verification checkpoint on owned draft PR #9. This record covers the regression repair from broken candidate `e61a64af1487538a2a6fab306a13c25e9b97dfc8` to verified implementation candidate `6d469df8553961297c4606989e9a9610722cfd0b`. It does not claim native execution on Lucas's PCs, performance acceptance, profiler-overhead acceptance, Unreal/Unity parity, clean-machine acceptance, 24-hour soak completion, or independent review. Issue #7 remains open and the historical R0 runner was not invoked.

## Bounded packet and failure

The active task is `Tasks/E14-PROFILING-CAPTURE-STATE-2026-09-21.md`. Its admitted production implementation adds a startup profiling-capture-state receipt that records the three known Astral profiling subsystems after configuration and before the frame loop, while keeping instrumentation-overhead and parity claims false.

The branch head `e61a64af1487538a2a6fab306a13c25e9b97dfc8` failed hosted `Profiling capture portability` run `35646310015`, job `106487515319`, at `Verify profiling analyses and production provenance binding`. The later build and sanitizer steps were skipped. Inspection found two coupled test/CI defects inside the task's allowed paths:

1. `Scripts/test_benchmark_capture_pair.py` still built legacy pair fixtures without the newly required `profiling_capture_state_json` evidence role or `capture-state.json`, even though `verify_benchmark_capture_pair.py` now requires valid profiled and control startup receipts.
2. `.github/workflows/frame-timing-validation.yml` did not execute `Scripts/test_profiling_capture_state.py` and did not include the new receipt implementation/verifier/test paths in its profiling workflow path filters.

The acceptance test was not weakened. The fixture was upgraded to satisfy the new production contract, and new negative coverage requires a startup receipt and rejects a profiled run whose receipt says the captures were not requested.

## Repair commit and exact files

Verified implementation candidate: `6d469df8553961297c4606989e9a9610722cfd0b`, commit message `test(profiling): repair startup capture-state CI coverage`.

Changed paths:

- `.github/workflows/frame-timing-validation.yml`, Git blob `65326d5226060a94c6de6b1376a1a48be440f795`.
- `Scripts/test_benchmark_capture_pair.py`, Git blob `18a0bdd109063ce91dca660e98ec12fa2dfcece5`.

The workflow now py-compiles the capture-state and capture-pair scripts, runs `Scripts/test_profiling_capture_state.py` before the pair suite, and triggers when the new receipt implementation, verifier, or native receipt test changes. The pair fixture now writes valid profiled/control startup receipts and tests the new report fields and failure cases.

Coordinator scratch validation before publication was syntax-only: Python `ast.parse` accepted the exact replacement test content, and a YAML parser accepted the exact replacement workflow content. The locally computed Git blob IDs matched the post-publication GitHub blob IDs above. These checks are not substitutes for repository integration or native runtime evidence.

## Hosted verification for exact candidate

All workflow runs below are bound to candidate `6d469df8553961297c4606989e9a9610722cfd0b`.

- Profiling capture portability: run `35647218339`, job `106490503612`, **PASS**. The production provenance step passed, then the Debug and optimized Release profiling contracts passed, followed by the Clang AddressSanitizer and UndefinedBehaviorSanitizer contracts with leak checking.
- Windows build and deterministic tests: run `35647218347`, job `106490503634`, **PASS**. R0 safety contracts passed without invoking R0, Visual Studio 2022 x64 configured successfully, real Debug and Release builds completed, deterministic Debug and Release tests passed, static milestone verifiers passed, and the tracked tree remained clean.
- Release manifest integrity: run `35647218451`, job `106490503478`, **PASS**. Manifest/runtime/restart/soak-contract/benchmark-manifest checks passed; two independent Release builds were classified as byte-identical; exact hosted package staging and benchmark-manifest binding passed.
- Benchmark environment evidence: run `35647218348`, portable job `106490503173` **PASS**, Windows CIM job `106490503468` **PASS**. The CIM result describes only the ephemeral GitHub runner and is not evidence about Lucas's hardware.

No hosted result above is native interactive/GPU evidence, a clean-machine package launch, a measured frame-time or RAM/VRAM budget, a stress/recovery result on the registered executor, or independent review.

## Current primary-source research, accessed 2026-09-21

Behavior references only. No proprietary source code was copied and no dependency was imported.

1. Epic Games, Unreal Engine 5.8, `Trace in Unreal Engine 5`: https://dev.epicgames.com/documentation/unreal-engine/trace-in-unreal-engine-5
   - Trace channels control which event types are emitted and the trace data rate. A required disabled channel means the event is not emitted.
   - Applicability: enabled profiling state is part of the measured workload and must be recorded rather than reconstructed later.
2. Epic Games, Unreal Engine 5.8, `Developer Guide to Tracing in Unreal Engine`: https://dev.epicgames.com/documentation/unreal-engine/developer-guide-to-tracing-in-unreal-engine
   - Trace channels are explicitly opted into and constrain CPU/memory work by limiting emitted events.
   - Applicability: a profiled/control comparison needs machine-verifiable capture-state provenance.
3. Unity 6.0 (6000.0), `UnityEngine.Profiling.Profiler`: https://docs.unity3d.com/6000.0/ScriptReference/Profiling.Profiler.html
   - Unity documents that profiler use affects application performance and that disabling Development Build is faster while removing most profiler API functionality.
   - Applicability: Astral must not interpret a profiled/control timing difference unless it proves which captures were enabled.
4. Unity 6.0 (6000.0), `Profiler.SetCategoryEnabled`: https://docs.unity3d.com/6000.0/ScriptReference/Profiling.Profiler.SetCategoryEnabled.html
   - Disabling categories stops stats/samples and can reduce profiler overhead.
   - Applicability: capture configuration is itself a benchmark variable.
5. Microsoft, `Environment Variables`: https://learn.microsoft.com/en-us/windows/win32/procthread/environment-variables
   - A child process inherits its parent's environment by default unless a different environment block is supplied at process creation.
   - Applicability: omission of capture files from a manifest cannot prove that a control launch did not inherit capture controls.
6. Microsoft, `Changing Environment Variables`: https://learn.microsoft.com/en-us/windows/win32/procthread/changing-environment-variables
   - Child processes receive a copy of the inherited or explicitly supplied environment block.
   - Applicability: the running process should record its interpreted controls before measurements begin.

Licensing constraint: these public documents are used only as behavioral references for Astral's original implementation and test design. No Unreal Engine, Unity, or Microsoft implementation source is copied.

## Capability-to-evidence checkpoint

This pass advances only profiling provenance. It does not narrow the comparison catalogue.

| Capability area | Current evidence after this pass | Remaining comparison work |
| --- | --- | --- |
| Runtime/jobs/memory | Hosted capture-state, timing, process-memory, run-control, environment and package contracts are green | Native measured budgets, allocator/task attribution, long-run stability |
| Scene ownership/serialization | Not advanced here | Versioned ownership/serialization workflows and failure recovery |
| Asset pipelines | Existing separate asset-validation work only | Broader import/reimport/cook/cache coverage and native acceptance |
| GPU rendering/materials | Not advanced here | GPU timestamps, active adapter proof, VRAM, materials, matched workloads |
| Lighting/shadows/reflections | Not advanced here | Versioned feature scenes, correctness and performance evidence |
| Large-world streaming/detail | Not advanced here | Terrain, streaming, LOD/HLOD/detail and memory/performance evidence |
| Animation | Not advanced here | State/graph/blend/retarget/tooling comparison |
| Physics/collision | Not advanced here | Fixed-step/substep behavior, collision correctness, stress/performance |
| AI/navigation | Not advanced here | Navigation generation/query/runtime/tooling evidence |
| Audio | Not advanced here | Runtime mixing/spatialization/tooling/performance evidence |
| UI/editor tools | Not advanced here | Authoring/debugging/import workflows and measurable productivity features |
| Genuine 2D | Not advanced here | Native 2D renderer/assets/physics/animation/editor workflows and matched scene |
| Networking | Not advanced here | Replication/transport/prediction/debugging catalogue and tests |
| Profiling | Startup capture-state and pair provenance are now hosted-CI verified | Native profiled/control pair, instrumentation-overhead calculation, GPU profiling |
| Packaging/platforms | Hosted Windows package/provenance/repro contracts green | Clean-machine launch and additional approved platforms |
| Terrain/VFX/cinematics | Retained as open catalogue rows | Research, measurable requirements, implementation and acceptance |
| Scripting/reflection | Retained as open catalogue row | Research, API/tooling requirements and acceptance |
| Input/replay | Benchmark live-input suppression is covered | General input mapping/rebinding/replay determinism and tooling |
| Accessibility/localization | Retained as open catalogue rows | Requirements, implementation and workflow evidence |

## Claim boundaries and next action

This checkpoint proves that the current hosted profiling verification chain actually exercises the new startup capture-state contract and that the exact candidate is green across profiling, Windows build, release/package and environment workflows. It does not prove instrumentation overhead, frame-time parity, RAM/VRAM parity, GPU behavior, clean-machine compatibility, stress/recovery, 86,400-second soak completion, or independent acceptance.

Issue #7 is still open. R0 was not invoked.

Single next useful action: the registered Windows executor should run one frozen procedural 3D profiled/control pair from one exact package and machine. Both launches must produce fresh startup capture-state receipts. The profiled launch must enable whole-frame, phase and process-memory capture; the control launch must omit all three controls. Retain exact launch commands/environment controls, source/package/executable hashes, machine/toolchain/driver identity, stdout/stderr, exit codes, UTC timestamps, run-control/environment receipts, CSV/receipt hashes and captured images where applicable. Run the existing per-run verifiers, stream-coherence verifier and capture-pair verifier before any later packet calculates instrumentation overhead.
