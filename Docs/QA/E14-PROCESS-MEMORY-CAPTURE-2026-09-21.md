# E14 process-memory capture QA checkpoint, 2026-09-21

## Checkpoint identity

- Repository: `LucasKazaki/AnimeRPG`.
- Existing owned draft PR: #9, branch `repair/2026-09-20-r0-runner-safety`.
- Pre-packet branch head: `c3429ac0e576f2301564f9318caad22f7d27008c`.
- Implementation candidate verified by hosted CI: `abc91216322e8ce1fe4e1eea0ee3896551e1e996`.
- GitHub pull-request synthetic merge tested by the hosted workflows: `149906c485c7c463364557569987c7c9509699f2`, merging the implementation candidate into the unchanged PR base `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`.
- This QA document is the documentation-only follow-up after the verified implementation candidate. Its own commit is not substituted for the implementation SHA in the test receipts below.
- Issue #7 remains open. The historical R0 runner was not invoked.
- PR #9 remains a draft. Nothing was merged, released, deployed, installed, or published to end users.

## Selected gap and result

The bounded gap was E14 process RAM evidence aligned with Astral's existing benchmark frame identity. Before this packet, Astral had opt-in whole-frame CPU timing, Win32 main-thread phase timing, package/benchmark provenance, and an external long-run soak sampler, but no production-loop, frame-indexed process-memory evidence stream for the same benchmark interval.

This packet adds `ProcessMemoryCapture` and binds it to the existing `Clock::Tick()` frame index. It is disabled unless explicitly requested. On Windows, sampled frames call `GetProcessMemoryInfo(GetCurrentProcess(), PROCESS_MEMORY_COUNTERS_EX)` and retain:

- `frame_index`;
- current working-set bytes;
- peak working-set bytes;
- `PrivateUsage` / private commit-charge bytes;
- page-fault count.

The CSV explicitly records that its scope is current-process OS counters only, allocator attribution is unavailable, VRAM is unavailable, leak detection is not established, no performance budget has been accepted, and no engine-acceptance claim follows from the capture. Any production-sampler failure or invalid measured sample blocks final publication.

Evidence publication follows the existing E14 fail-closed style: absolute fresh output path, existing parent required, bounded warmup/stride/sample cap, fresh `.partial` output, no-overwrite hard-link publication, and saturation recorded explicitly rather than hidden. Warmup and non-stride frames are skipped before sample validation so excluded fixture rows cannot poison or upgrade measured evidence.

## Primary-source research

Sources were rechecked on 2026-09-21 before implementation.

1. Microsoft Learn, `PROCESS_MEMORY_COUNTERS_EX`: https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-process_memory_counters_ex
   - `WorkingSetSize` is the current working set; `PeakWorkingSetSize` is the lifetime peak; `PrivateUsage` is process commit charge/private committed memory.
   - Applicability: enough for OS-level resident/private-commit evidence, not allocator ownership, leak attribution, or VRAM.
2. Microsoft Learn, `GetProcessMemoryInfo`: https://learn.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-getprocessmemoryinfo
   - Retrieves process memory counters and returns zero on failure. With current PSAPI headers, the modern symbol resolves through Kernel32 for current Windows SDK targets.
   - Applicability: sampler failure must be treated as evidence failure rather than converted into zeros or ignored.
3. Epic Games, Unreal Engine 5.8, Memory Insights: https://dev.epicgames.com/documentation/en-us/unreal-engine/memory-insights-in-unreal-engine
   - UE can inspect allocation/free events, callstacks, LLM tags, live allocations, growth/decline, and leak-oriented queries.
   - Applicability: Astral's process totals remain materially below this attribution depth.
4. Epic Games, Unreal Engine 5.8, Low-Level Memory Tracker: https://dev.epicgames.com/documentation/en-us/unreal-engine/using-the-low-level-memory-tracker-in-unreal-engine
   - UE can tag engine/OS allocation classes and emit memory CSV data.
   - Applicability: tagged allocator accounting remains a future Astral requirement.
5. Unity 6.0 / 6000.0, Memory Profiler package 1.1.9: https://docs.unity3d.com/6000.0/Documentation/Manual/com.unity.memoryprofiler.html
   - Unity provides allocation-focused memory profiling for Unity 6000.0.
   - Applicability: Astral's process counter stream is only an initial measurable RAM layer, not Unity Memory Profiler parity.

No proprietary engine source was copied. No third-party package was imported. The implementation uses documented Windows APIs and the Windows SDK already required by the project. Self-review removed an unnecessary explicit `psapi` link after confirming the modern call resolves through the normal Windows system API surface; the final PE dependency report contains `KERNEL32.dll` and no new PSAPI runtime dependency.

## Changed paths

Relative to `c3429ac0e576f2301564f9318caad22f7d27008c`, this packet is intentionally limited to:

1. `.github/workflows/frame-timing-validation.yml`
2. `CMakeLists.txt`
3. `Engine/Core/Clock.cpp`
4. `Engine/Core/Clock.h`
5. `Engine/Core/ProcessMemoryCapture.cpp`
6. `Engine/Core/ProcessMemoryCapture.h`
7. `Tasks/E14-PROCESS-MEMORY-CAPTURE-2026-09-21.md`
8. `Tests/FrameTiming/CMakeLists.txt`
9. `Tests/ProcessMemoryCaptureTests.cpp`
10. `Docs/QA/E14-PROCESS-MEMORY-CAPTURE-2026-09-21.md`

The task's first draft named `Win32Application.cpp`, but inspection showed `Clock` already owns the monotonic frame identity used by whole-frame timing. The packet was narrowed before application integration so `Win32Application.cpp` remained unchanged.

Published implementation blobs at the verified candidate:

- `Engine/Core/ProcessMemoryCapture.h`: Git blob `900e45b7e85fde77655e4b0db5d9218f8e69bc6b`.
- `Engine/Core/ProcessMemoryCapture.cpp`: Git blob `b08f3d77c95846ababb80eca7125efdbd49c5e3e`.
- `Tests/ProcessMemoryCaptureTests.cpp`: Git blob `d73bd171ae415ed8e8234d89adce39d8759e31bd`.

## Regression contract

Portable tests cover invalid/relative paths and limits, existing final/partial output refusal, warmup and sampling stride, exact scope/claim metadata, invalid working-set/peak relation, bounded saturation, no-sample refusal, no-overwrite output races, sampler-source misuse/failure, and environment-disabled state. On Windows, the same test binary additionally enables the production environment path and executes the real `GetProcessMemoryInfo` sampler against the test process before publishing a fresh receipt.

The existing frame-timing portability subproject now builds three production-linked contracts together: whole-frame timing, phase timing, and process-memory capture. The new production source is also compiled into `AstralGame` and into the full Windows Debug/Release test graph.

## Sandbox/compiler evidence

A disposable partial fixture under `/tmp/astral-mem` was used before hosted CI. It contained the new memory-capture source/test plus minimal CMake glue, not the full repository.

Attempted and passed:

- GNU C++ Debug configure/build/CTest: PASS, 1/1 test, 9 portable memory-capture groups.
- GNU C++ optimized Release configure/build/CTest: PASS, 1/1 test, 9 portable groups.
- Clang Debug with ASan+UBSan and leak checking (`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1`, `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`): PASS, 1/1 test, 9 portable groups.

Source-hash limitation: the local fixture was semantically equivalent but was created before final GitHub formatting/self-review. Only the header byte-matched the final Git blob. The local `.cpp` and test bytes did not exactly match the final published blobs, so this partial fixture is not used as proof for the final GitHub bytes. Exact final-source verification comes from the hosted runs below.

## Hosted verification of the exact implementation candidate

All three current workflows checked out synthetic PR merge `149906c485c7c463364557569987c7c9509699f2`, whose head side is the exact implementation candidate `abc91216322e8ce1fe4e1eea0ee3896551e1e996` and whose base side is the unchanged PR base `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`.

### Profiling portability, run `35578216908`, job `106264796839`

Result: PASS on Ubuntu 24.04.5 / runner image `20260907.300.1`.

- Existing frame analyzer tests: 12/12 PASS.
- Existing phase analyzer tests: 13/13 PASS.
- GNU 13.3.0 Debug capture contracts: 3/3 CTest PASS.
  - whole-frame capture: 11 groups PASS;
  - phase capture: 9 groups PASS;
  - process-memory capture: 9 portable groups PASS.
- GNU 13.3.0 optimized Release: the same 3/3 CTest PASS with the same group counts.
- Clang 18.1.3 Debug ASan+UBSan plus leak checking: the same 3/3 CTest PASS with the same group counts.

This validates portable parsing/state/storage/publication behavior. Linux does not execute the Windows process sampler.

### Windows Debug/Release, run `35578216852`, job `106264817215`

Result: PASS on Windows Server 2022 `10.0.20348`, runner image `20260913.307.1`.

Toolchain:

- MSVC compiler `19.44.35228.0`;
- Visual C++ Build Tools `14.44.35207`;
- Windows SDK `10.0.26100.0`.

Safety and prerequisite contracts remained green:

- R0 runner safety: 13/13 PASS. These are parser/contract tests only; R0 itself was not invoked.
- PE dependency tests: 5/5 PASS.
- Windows prerequisite-plan tests: 8/8 PASS.
- Runtime-environment tests: 8/8 PASS.
- Runtime-compatibility tests: 7/7 PASS.
- Redistributable bootstrap tests: 8 PASS, 1 intentional platform-specific skip.
- Release assertion/CTest safety tests: 3/3 PASS.

Actual C++ builds/tests:

- real MSVC Debug build: PASS;
- Debug non-GUI CTest: 11/11 PASS, including `ProcessMemoryCaptureTests` using its Windows-only live `GetProcessMemoryInfo` production path;
- real MSVC Release build: PASS;
- Release non-GUI CTest: 11/11 PASS, again including `ProcessMemoryCaptureTests`;
- Milestone 1/2/3 static verifiers: PASS;
- tracked-tree cleanliness: PASS.

The Release PE was 110,080 bytes, SHA-256 `d052219b4eb5eea192e82289ffbe0b1bd3db370f4ec94f69148ffa82e35f6363`. The dependency report shows `KERNEL32.dll`, `USER32.dll`, `GDI32.dll`, the expected VC/UCRT imports, and no debug CRT. Clean-machine compatibility remains explicitly false.

### Release/package/provenance, run `35578216848`, job `106264817083`

Result: PASS on Windows Server 2022 runner image `20260913.307.1`, with the same compiler/toolset/SDK family.

- release manifest contracts: 7/7 PASS;
- package runtime-receipt contracts: 11/11 PASS;
- package restart-stress contracts: 11/11 PASS;
- continuous-soak contracts: 11/11 PASS;
- soak-analysis contracts: 11/11 PASS;
- benchmark-manifest contracts: 12/12 PASS;
- PE reproducibility diagnostic contracts: 7/7 PASS;
- fresh Release build: PASS;
- exact staged package verification: PASS;
- benchmark-provenance fixture: PASS;
- clean tracked tree: PASS.

Two fresh Release builds were byte-identical, both 110,080 bytes with SHA-256 `d052219b4eb5eea192e82289ffbe0b1bd3db370f4ec94f69148ffa82e35f6363`. Both contain `IMAGE_DEBUG_TYPE_REPRO`; the generated `AstralGame.vcxproj` was also identical between the two builds, SHA-256 `0980954d3b457439f2fdddf5e01b84607149c57675b3645fad8e0e00b8649ea6`.

The hosted benchmark descriptor SHA-256 was `3dd48ef23af56eb14062ef9448d199fba89b3b2ad4ca65f42eb286f82db39fdf`. It remains a hosted schema/provenance fixture, not native performance evidence. Performance-budget, comparative-parity, package-launch, clean-machine, 24-hour-soak, and independent-acceptance claims remain false.

## Capability-to-evidence map after this packet

No row is promoted to comparable/accepted by this packet.

| Capability area | Current evidence after this packet | Remaining material gap |
| --- | --- | --- |
| runtime / jobs / memory | R0 safety contracts plus frame-indexed OS process-memory capture | native R0 acceptance; allocator/tag attribution; job system/memory architecture parity |
| scene ownership / serialization | existing project inventory only | stable IDs, ownership/lifetimes, versioned serialization, round-trip/fault evidence |
| asset pipelines | bounded mesh validation work exists on separate draft PR #8 | broad importer/reimport/cache/dependency/cook evidence and native acceptance |
| GPU rendering / materials | existing renderer baseline | programmable material system, GPU timing, modern render-path capability/evidence |
| lighting / shadows / reflections | inventory remains incomplete | versioned implementations and matched visual/performance reference scenes |
| large-world streaming / detail | inventory remains incomplete | terrain/streaming/LOD/HLOD/origin/large-scene evidence |
| animation | inventory remains incomplete | skeletal animation, blend/state tooling, compression/runtime evidence |
| physics / collision | inventory remains incomplete | production collision/physics capability, determinism/stress evidence |
| AI / navigation | inventory remains incomplete | navmesh/pathfinding/behavior/runtime tooling evidence |
| audio | inventory remains incomplete | spatial/mixing/streaming/tooling evidence |
| UI / editor tools | inventory remains incomplete | retained editor workflow, inspection/authorship/debugging tooling |
| genuine 2D | required and explicitly retained | sprites/tilemaps/2D camera/render/collision/tooling and matched reference workflow |
| networking | inventory remains incomplete | transport/state sync/prediction/replay/security/testing |
| profiling | whole-frame timing, main-thread phases, provenance/analyzers, new process working-set/private-commit stream | native measured runs, GPU timing, thread/task attribution, allocator attribution, RAM/VRAM budgets, instrumentation-overhead measurement |
| packaging / platforms | deterministic Release bytes, manifest/prerequisite/package contract tooling | native package launch, clean machine, supported platform matrix, 24-hour soak, installer/signing/release acceptance |
| remaining feature catalogue | terrain, particles/VFX, cinematics, scripting/reflection, input/replay, accessibility/localization, extra platforms remain explicit | research, implementation, tests, matched workflows and acceptance for each |
| integrated parity | not established | matched, versioned Astral/UE5/Unity reference workloads with correctness, performance, memory, reliability, tooling and independent acceptance |

## Native executor handoff

This coordinator did not execute Lucas's PCs or the Company Runtime. The registered Windows executor can use the new capture only after the existing prerequisite and exact-package smoke gates pass and it owns an exclusive interactive desktop.

For the frozen procedural 3D benchmark, use fresh absolute evidence paths and the same frame identity for all three streams:

```powershell
$env:ASTRAL_FRAME_TIMING_CSV = "<fresh-absolute-evidence-root>\frame.csv"
$env:ASTRAL_FRAME_TIMING_WARMUP_FRAMES = "120"
$env:ASTRAL_FRAME_TIMING_MAX_SAMPLES = "3600"

$env:ASTRAL_FRAME_PHASE_TIMING_CSV = "<fresh-absolute-evidence-root>\phase.csv"
$env:ASTRAL_FRAME_PHASE_TIMING_WARMUP_FRAMES = "120"
$env:ASTRAL_FRAME_PHASE_TIMING_MAX_SAMPLES = "3600"

$env:ASTRAL_PROCESS_MEMORY_CSV = "<fresh-absolute-evidence-root>\process-memory.csv"
$env:ASTRAL_PROCESS_MEMORY_WARMUP_FRAMES = "120"
$env:ASTRAL_PROCESS_MEMORY_SAMPLE_EVERY_FRAMES = "1"
$env:ASTRAL_PROCESS_MEMORY_MAX_SAMPLES = "3600"
```

Launch the exact admitted package normally and retain the existing required receipt set: source revision, package manifest and executable hash, machine/OS/toolchain/CPU/RAM/GPU/driver identity, resolution/window/VSync settings, exact command/environment, UTC timestamps, stdout/stderr, runtime logs, raw CSV hashes, exit codes, and required captured images. Run a matched capture-off control separately so instrumentation overhead is not silently ignored. Prose-only model approval is not evidence.

The current process-memory stream is not sufficient to claim leak freedom or a RAM budget. A long-run trend still needs the existing soak path, and suspicious growth needs allocator-level attribution rather than inference from working-set/private-commit totals alone. VRAM remains completely unresolved by this packet.

## Claim boundaries and unresolved gates

Passed in this packet:

- exact published production source compiles on Windows Debug and Release;
- portable contract suite passes Debug, optimized Release, ASan, UBSan, and leak checking;
- real Windows process-memory sampling path is exercised in hosted non-GUI tests;
- existing safety/prerequisite/provenance/reproducibility regression lanes remain green;
- package bytes remain deterministic for the observed repeated hosted pair.

Not performed or not accepted:

- no interactive/native benchmark run on Lucas's registered Windows executor;
- no GPU or VRAM measurement;
- no allocator ownership/callstack/tag attribution;
- no approved RAM budget or performance budget;
- no instrumentation-overhead result;
- no matched UE5/Unity 3D or genuine-2D reference run;
- no clean-machine package launch;
- no real 86,400-second soak;
- no independent review/acceptance;
- no merge, release, deployment, dependency import, graphics-API change, or game-content restart.

## Single next coordinator action

Implement one bounded, provenance-bound analyzer for the new process-memory CSV before treating native memory runs as comparable evidence. It should fail closed on schema/claim tampering, bind the exact package/benchmark manifest and raw CSV hash, report descriptive working-set/private-commit/page-fault statistics plus first/last-window and time/frame trend evidence, and keep RAM-budget, leak-free, VRAM, comparative-parity, and independent-acceptance claims false. Native benchmark execution can proceed in parallel only through the already registered local executor under the existing controls.
