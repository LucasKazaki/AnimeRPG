# E14 benchmark run-control CI repair evidence, 2026-09-21

Status: hosted verification complete for the bounded CI regression-repair packet. This evidence repairs verification of the E14 benchmark run-control candidate. It does not establish native benchmark performance, GPU behavior, clean-machine compatibility, the 24-hour soak, UE5/Unity parity, or independent acceptance.

## Identity and scope

- Repository: `LucasKazaki/AnimeRPG`.
- Draft PR: #9, branch `repair/2026-09-20-r0-runner-safety`.
- Parent benchmark run-control candidate: `e52289f8f18a90c6b21ca4c7d174b829850d18c1`.
- Windows test-harness stabilization: `3dd02cb181da05d409dca922bd58df2d0fe696ec`.
- Windows open-handle repair: `fc0ff7d3cd9c84a088aa620ebdc8091880efd3f5`.
- Bounded repair task admission: `8d51cb059cb3e6afc00516a4e78e54791eccea5f`.
- Final static-verifier repair candidate: `f7f7679a5bf017fb683dd8c6f74e5a854b15779a`.
- `Tests/BenchmarkRunControlTests.cpp` blob after the open-handle repair: `2d66601393726b706f9147663187fcf6a9ee762f`.
- `Scripts/verify_milestone3.py` blob at the final candidate: `9bac93bc2bfdfc6e64d5ea55c6831894f2228479`.
- Issue #7 remains open. The historical R0 runner was not invoked.
- No Engine/Game/CMake/workflow/runtime/package/dependency/renderer/content source was changed by this repair packet.

## Reproduced failures and repair sequence

### 1. Windows deterministic CTest timeout

At parent candidate `e52289f8f18a90c6b21ca4c7d174b829850d18c1`, hosted Windows run `35606960562`, job `106356293648`, built Debug successfully but the deterministic Debug CTest step consumed the full 60-second non-runtime timeout and failed before Release.

Commit `3dd02cb181da05d409dca922bd58df2d0fe696ec` made the test harness explicitly synchronize the narrow and wide Windows CRT environment views, checked environment mutation results, and changed CHECK failure termination from `abort()` to a bounded failure exit. Windows run `35608156814`, job `106360284341`, still timed out in the same deterministic Debug CTest step. This falsified the hypothesis that CRT environment-view synchronization alone caused the hang.

Inspection then found that `TestExactFrameCompletionAndReceipt` kept an `std::ifstream` on the published receipt alive while calling `std::filesystem::remove_all` on its parent temporary directory. Commit `fc0ff7d3cd9c84a088aa620ebdc8091880efd3f5` scopes and closes that stream before cleanup and checks that the receipt was opened successfully. Windows run `35608525667`, job `106361530889`, then completed deterministic Debug and Release CTests successfully in roughly one second each without changing the repository's 60-second timeout. This is the direct hosted reproduction/repair evidence for the Windows test-harness blocker.

### 2. Stale M3 static marker after benchmark-safe input refactor

The same `fc0ff7d3cd9c84a088aa620ebdc8091880efd3f5` Windows run advanced past Debug/Release CTests and then failed `Run static milestone verifiers`. `Scripts/verify_milestone3.py` still required the historical literal `GetAsyncKeyState('W')`, but the benchmark run-control implementation intentionally routes live input through a `keyDown` wrapper so benchmark mode can suppress physical input without changing normal interactive behavior.

Commit `f7f7679a5bf017fb683dd8c6f74e5a854b15779a` strengthens the verifier instead of weakening it. The Win32 application gate now requires all of the following executable-source markers:

- `const auto keyDown =`
- `benchmarkRunControl.SuppressLiveInput()`
- `GetAsyncKeyState(virtualKey)`
- `keyDown('W')`
- existing clock/camera/window-close markers

This verifies both sides of the intended contract: normal movement still reaches `GetAsyncKeyState` through the current input abstraction, and benchmark mode has an explicit suppression gate. No comment/dead-string compatibility shim was added to production source.

## Primary-source research

Rechecked 2026-09-21:

1. Microsoft `_putenv_s`, `_wputenv_s`: https://learn.microsoft.com/en-us/previous-versions/eyw7eyfw(v=vs.140) . Microsoft documents that `getenv`/`_putenv_s` use `_environ`, while `_wgetenv`/`_wputenv_s` use `_wenviron`, and notes that these environment families are not thread-safe. Applicability: the single-threaded test setup explicitly mutates both CRT views because production benchmark configuration intentionally reads the wide Windows environment for path fidelity.
2. Microsoft `SetEnvironmentVariable`: https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setenvironmentvariable . It modifies the current process environment and does not modify other processes or system environment. Applicability: benchmark tests must remain process-local and must not mutate machine/user configuration.
3. Unreal Engine 5.8 Command-Line Arguments: https://dev.epicgames.com/documentation/unreal-engine/command-line-arguments-in-unreal-engine . Epic documents launch parameters, resolution/framerate controls, and project-defined key/value arguments used for testing/optimization. Applicability: explicit benchmark controls are a legitimate reproducibility surface, but this packet does not claim Unreal-equivalent tooling.
4. Unity 6.0 `Time`: https://docs.unity3d.com/6000.0/ScriptReference/Time.html . Unity separates capture/fixed-time controls from ordinary per-frame time. Applicability: Astral's fixed simulation control remains separate from wall-clock profiling evidence.

No proprietary engine source was copied and no dependency was added.

## Exact hosted verification for final candidate

Final candidate: `f7f7679a5bf017fb683dd8c6f74e5a854b15779a`.

### Windows build and deterministic tests

- Workflow run: `35609100704`
- Job: `106363456329` (`windows-2022`)
- Result: PASS.
- R0 safety contracts: PASS.
- PE dependency/prerequisite/runtime/bootstrap contracts: PASS.
- Release assertion/CTest safety contracts: PASS.
- Visual Studio 2022 x64 configure: PASS.
- Debug build: PASS.
- Deterministic Debug CTests: PASS, 2026-09-21T14:00:35Z to 14:00:37Z.
- Release build: PASS.
- Runtime dependency/prerequisite/version/bootstrap verification: PASS.
- Deterministic Release CTests: PASS, 2026-09-21T14:00:59Z to 14:01:00Z.
- Static milestone verifiers: PASS.
- Clean tracked-tree gate: PASS.

### Profiling portability

- Workflow run: `35609100746`
- Job: `106363456300` (`ubuntu-24.04`)
- Result: PASS.
- Profiling analysis and production provenance binding: PASS.
- Debug and optimized Release profiling capture contracts: PASS.
- Clang AddressSanitizer + UndefinedBehaviorSanitizer profiling contracts: PASS.

### Release/package/provenance/reproducibility

- Workflow run: `35609100694`
- Job: `106363455796` (`windows-2022`)
- Result: PASS.
- Manifest, runtime receipt, restart-stress, continuous-soak contract, soak-analysis contract, benchmark-manifest and PE reproducibility diagnostic contract tests: PASS.
- Same Release candidate built twice: PASS; workflow classified the pair as byte-identical.
- Hosted package staging, exact-byte manifest verification and hosted benchmark-manifest fixture binding: PASS.
- Clean tracked-tree gate: PASS.

The soak steps above validate receipt/analyzer contracts only. They are not an executed 86,400-second runtime soak.

### Benchmark environment evidence

- Workflow run: `35609100720`
- Windows CIM job: `106363456670`, PASS.
- Portable contract job: `106363457031`, PASS.
- The Windows CIM capture describes the ephemeral hosted runner only. It is not evidence about Lucas's Windows machine or the active render adapter used by a native benchmark.

## Capability-to-evidence status

This repair does not promote an engine capability to comparable/accepted status. It preserves the current E14 `partial` state and improves the trustworthiness of the benchmark-control verification chain:

- fixed simulation rate: hosted source/test evidence exists;
- explicit benchmark input suppression/exact frame limit: hosted source/test evidence exists;
- CPU whole-frame/main-thread-phase/process-memory capture and provenance analysis: hosted contract evidence exists from prior E14 packets;
- benchmark environment receipt and cross-stream coherence: hosted contract evidence exists from prior E14 packets;
- native measured frame-time/RAM budgets: not established;
- GPU timing/active adapter/VRAM: not established;
- allocator/thread/task attribution comparable to UE5/Unity: not established;
- matched UE5/Unity 3D and genuine-2D scenes: not executed;
- clean-machine package launch: not established;
- failure-recovery stress on the registered machine: not established;
- required 24-hour runtime soak: not executed;
- independent review/acceptance: absent. PR #9 currently has no reviews.

The broader E00-E17 catalogue remains incomplete and unaccepted. Terrain/foliage, particles/VFX, cinematics, scripting/reflection, input/replay beyond this benchmark control, localization/accessibility, and additional platform requirements remain in the research backlog rather than being silently removed from parity.

## Native handoff and single next useful action

The coordinator-side benchmark-control regression is now green. The next useful action belongs to the registered Windows executor, not this GitHub coordinator:

1. Use the exact admitted package/candidate and produce the existing Windows benchmark environment receipt.
2. Configure the frozen procedural 3D workload with `ASTRAL_SIMULATION_FIXED_HZ=60`, `ASTRAL_BENCHMARK_MODE=1`, 120 warmup frames and 3,600 measured frames, with a fresh benchmark-control receipt path.
3. Enable the existing whole-frame, main-thread phase and process-memory streams with matching boundaries/declared stride.
4. Retain exact source SHA, package/executable hashes, machine/CPU/RAM/GPU/driver identity, resolution/window/VSync state, command lines, stdout/stderr, exit codes and UTC timestamps.
5. Bind all evidence into the benchmark manifest, then require both benchmark-run-control verification and cross-stream coherence verification to pass before interpreting measurements.
6. Run a matched capture-off control before any instrumentation-overhead claim.

Do not interpret hosted CI as that native run. GPU timestamps, active-adapter proof, VRAM, approved performance/RAM limits, matched UE5/Unity 3D and genuine-2D workloads, clean-machine launch, recovery stress, the real 86,400-second soak and independent acceptance remain separate gates.
