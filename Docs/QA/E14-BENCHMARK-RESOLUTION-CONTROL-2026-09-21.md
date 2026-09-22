# E14 benchmark resolution-control verification, 2026-09-21

Status: bounded implementation verified in hosted CI. Native registered-Windows GUI benchmarking and independent review remain pending. This is evidence for E14 benchmark reproducibility only, not UE5/Unity parity or engine acceptance.

## Identity and scope

- Repository: `LucasKazaki/AnimeRPG`.
- Draft PR: #9, branch `repair/2026-09-20-r0-runner-safety`.
- Pre-packet head: `a97fbd74965e9904727ef0df3f803c32f05a00ea`.
- Task-admission commit: `8071b23d13b48cd2c7ee51f82c81c13935743fef`.
- Exact hosted implementation candidate: `5f60ea83490ead4c2544b026da5b8fb152995df7`.
- Compare result from the pre-packet head: seven commits, seven admitted paths, 401 changed lines, with no path outside the bounded task plus six implementation/test paths.
- R0 was not invoked. Issue #7 remains open. No merge, release, deployment, dependency/plugin install, renderer/API change, Company Runtime change, local-PC process control, or paused game-content work occurred.

Implementation commit chain:

1. `8071b23d13b48cd2c7ee51f82c81c13935743fef` - admit the bounded resolution-control packet.
2. `ce77839de587a41051ce00c078d919049fb41765` - add requested-client-area contract to `BenchmarkRunControl`.
3. `c68daa4f6f4a057a5171bbca32692df35b6aa641` - bind requested benchmark client dimensions to run control.
4. `ecc7539320c6349cd494d53491d0bab1e03c23b6` - cover the new contract in production-linked C++ tests.
5. `4b042383571e7446bb3ddf641ffed24b1085fe88` - require explicit resolution evidence in the production verifier.
6. `60b6378e3210acaca39062a5256cf184a5d57327` - add Python verifier regression cases.
7. `5f60ea83490ead4c2544b026da5b8fb152995df7` - request and verify the exact Win32 benchmark client resolution.

Exact changed paths from the baseline-to-candidate GitHub compare:

- `Engine/Core/BenchmarkRunControl.cpp`
- `Engine/Core/BenchmarkRunControl.h`
- `Engine/Platform/Win32Application.cpp`
- `Scripts/test_benchmark_run_control.py`
- `Scripts/verify_benchmark_run_control.py`
- `Tasks/E14-BENCHMARK-RESOLUTION-CONTROL-2026-09-21.md`
- `Tests/BenchmarkRunControlTests.cpp`

Published candidate blobs:

- `Engine/Core/BenchmarkRunControl.h`: `0051626db80f62b51067722d56a55cf75834c3e8`
- `Engine/Core/BenchmarkRunControl.cpp`: `ea07862f7de3d636886de6d16782fa1ecf7a5176`
- `Engine/Platform/Win32Application.cpp`: `83d622748a54ac62a29715b32cf2c49e66825f97`
- `Tests/BenchmarkRunControlTests.cpp`: `790bb7d9d2eee1584141a2fc186b0340b1b5c801`
- `Scripts/verify_benchmark_run_control.py`: `71737a69f8ed262b1f74eaa0bee23d8b1ee98e7b`
- `Scripts/test_benchmark_run_control.py`: `f62499800de336bb618bbff7f13e2df61238158b`

## Reproducible gap repaired

The prior client-area packet could prove that the actual Win32 drawable client area stayed stable and matched the benchmark manifest, but Astral still created a fixed `1280x720` outer window. A requested matched protocol such as `1920x1080 windowed` therefore could not be expressed through the benchmark control plane. The operator could only discover a mismatch after launch.

The production path now adds benchmark-only controls:

- `ASTRAL_BENCHMARK_CLIENT_WIDTH_PX`
- `ASTRAL_BENCHMARK_CLIENT_HEIGHT_PX`

When `ASTRAL_BENCHMARK_MODE=1`, both are mandatory strict unsigned decimal integers in `[1, 16384]`. Before `CreateWindowExW`, Astral calls `AdjustWindowRectEx` for the existing `WS_OVERLAPPEDWINDOW` style to convert the requested drawable client rectangle into outer window dimensions. It then creates the window and immediately checks `GetClientRect`. A failure, zero/invalid client size, or any actual/requested mismatch fails closed before benchmark frames begin.

`BenchmarkRunControl::ConfigureFromEnvironment` independently parses the same requested dimensions and requires the actual initial client dimensions supplied by the production Win32 path to match them. Existing per-render-frame client-area observation remains in place, so later resize or client-area drift still invalidates the run.

The completion receipt is now schema version 2 and records `client_area_control: "environment_requested_and_verified"` for the production environment-controlled path. The production Python verifier rejects old schema-1 receipts, configured-only receipts, width/height/window-mode mismatches, hash mismatches, changed acceptance claims, and malformed frame/rate/client fields.

Ordinary non-benchmark launch behavior remains unchanged: Astral still uses its prior default outer `1280x720` window unless benchmark mode requests an explicit client area.

## Primary-source research

Sources were rechecked on 2026-09-21:

- Microsoft `AdjustWindowRectEx`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-adjustwindowrectex
- Microsoft DPI-aware alternative `AdjustWindowRectExForDpi`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-adjustwindowrectexfordpi
- Epic Games Unreal Engine 5.8 command-line arguments: https://dev.epicgames.com/documentation/unreal-engine/command-line-arguments-in-unreal-engine
- Unity 6.0 `Screen.SetResolution`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/Screen.SetResolution.html

Microsoft documents `AdjustWindowRectEx` as calculating a window rectangle from a desired client rectangle for a specified window style, suitable for `CreateWindowEx`. Microsoft also explicitly warns that this API is not DPI-aware and points per-monitor-DPI-aware callers to `AdjustWindowRectExForDpi`. For this bounded packet, Astral therefore does not trust the computed outer size as proof. The subsequent real `GetClientRect` equality check and the existing per-frame viewport checks remain the authoritative evidence.

Epic documents `-windowed -ResX=<width> -ResY=<height>` as launch controls that make a UE executable run at a particular resolution. Unity 6.0 exposes `Screen.SetResolution(width, height, ...)`, including windowed mode. These are reference workflow semantics only. No Epic or Unity proprietary source was copied, and no third-party dependency was imported.

The Microsoft API is provided by `User32.dll` / `User32.lib`, already part of Astral's Win32 application dependency surface because the application already uses Win32 USER32 windowing APIs. This packet adds no new dependency family.

## Sandbox status

A disposable partial Linux fixture was used before final persistence to exercise the proposed `BenchmarkRunControl` environment parsing, exact requested-vs-actual dimension check, schema-v2 receipt behavior, and existing output/frame safety under C++17 Debug, optimized Release, and Clang ASan+UBSan with leak detection. Those early local draft blobs were not byte-identical to the final published candidate, so this QA record does not present that fixture as exact-source candidate evidence.

The exact final candidate is instead covered by the complete-checkout hosted workflows below, including GNU Debug/Release, Clang sanitizers, MSVC Debug/Release, the real modified `AstralGame` link, and production Python manifest/verifier integration.

## Hosted exact-source verification

All four workflows associated with exact source head `5f60ea83490ead4c2544b026da5b8fb152995df7` completed successfully. GitHub Actions checked out PR merge ref `f798180f21ffb1adc25d544e4187306cf5ff2043`, which merges that exact head into the fixed PR base `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`.

### Profiling capture portability

- Run `35619333953`, job `106398149092`, Ubuntu 24.04.5, runner image `ubuntu-24.04` version `20260907.300.1`: **PASS**.
- Production Python profiling/provenance suites passed. The updated `Scripts/test_benchmark_run_control.py` ran **11 tests**, including old-schema rejection, `client_area_control` claim-boundary rejection, manifest width/height/window-mode mismatch rejection, hash binding, exact frame/rate/client bounds, and output no-overwrite behavior.
- GNU 13.3.0 Debug production-linked FrameTiming subproject: **5/5 tests PASS**, including `BenchmarkRunControlTests: 9 groups passed`.
- GNU 13.3.0 optimized Release: **5/5 tests PASS**, including the same 9 run-control groups.
- Clang 18.1.3 ASan+UBSan with `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`: **5/5 tests PASS**, including the same 9 run-control groups.
- Existing whole-frame timing, phase timing, process-memory, and fixed-simulation tests remained green.

### Windows build and deterministic tests

- Run `35619333807`, job `106398277172`, Windows Server 2022 `10.0.20348`, image version `20260913.307.1`: **PASS**.
- Windows SDK: `10.0.26100.0`.
- MSVC compiler: `19.44.35228.0`; Visual C++ tools: `14.44.35207`.
- The real modified `Win32Application.cpp` and `BenchmarkRunControl.cpp` compiled and linked into both Debug and Release `AstralGame.exe`.
- Debug deterministic non-GUI CTest: **13/13 PASS**.
- Release deterministic non-GUI CTest: **13/13 PASS**.
- M1, M2, and M3 static verifiers passed, and the final tracked-tree check passed.
- The hosted Release executable was `122,880` bytes, SHA-256 `63af3cfc78d2d2315fe23bdaf32039f8775e3a9f790f826f4e3320a911cdc4f3`.
- PE/runtime evidence explicitly retains `package_launch_verified: false`, `clean_machine_compatibility_verified: false`, and `independent_acceptance: false`.
- R0 safety **tests** ran and passed 13/13 as CI contracts. The historical R0 runner itself was not invoked by this pass, and issue #7 remains open.

### Release/package/provenance

- Run `35619333911`, job `106398316882`: **PASS**.
- Manifest, runtime-receipt, restart-stress, continuous-soak contract, soak-analysis contract, benchmark-manifest, and PE reproducibility contract steps passed.
- Two same-candidate Release builds were classified as byte-identical.
- The exact hosted package and benchmark-manifest contract fixture passed, followed by the clean-tree check.

These are packaging and contract checks. They do not constitute the required real 86,400-second native soak or a clean-machine package launch.

### Benchmark environment

- Run `35619333788`: **PASS**.
- Portable-contracts job `106398147417`: PASS.
- Windows CIM-capture job `106398147902`: PASS, including a real environment receipt for the ephemeral hosted Windows runner.

The hosted CIM receipt describes the GitHub runner, not Lucas's PC.

## Capability-to-evidence map impact

This packet advances only the benchmark/profiling evidence chain: a reference-scene protocol can now request one specific windowed render-client resolution and prove that the production Win32 renderer actually received that size initially and on every completed benchmark frame.

It does not change the status of the broader catalogue. Still unresolved or only partially evidenced are runtime/jobs/memory-system breadth; scene ownership/serialization; asset pipeline breadth; GPU rendering/materials; lighting/shadows/reflections; large-world streaming/detail; animation; physics/collision; AI/navigation; audio; UI/editor tooling; genuine 2D; networking; GPU timestamps; active-render-adapter proof; VRAM; allocator/thread/task attribution; terrain/foliage; particles/VFX; cinematics; scripting/reflection; input/replay; localization/accessibility; additional platforms; approved performance/RAM/VRAM budgets; matched UE5/Unity reference scenes/workflows; clean-machine launch; recovery stress; the real 86,400-second native soak; and independent acceptance.

Green hosted builds, this document, or the ability to request a client resolution do not establish engine parity.

## Registered-Windows native handoff

The next native benchmark attempt must use the frozen procedural 3D workload and retain the exact source SHA, package/release-manifest hashes, machine/toolchain/driver identity, command lines, stdout/stderr, exit codes, UTC timestamps, immutable raw evidence hashes, and captured images where applicable.

Set the benchmark-control environment before launch:

```text
ASTRAL_BENCHMARK_MODE=1
ASTRAL_SIMULATION_FIXED_HZ=60
ASTRAL_BENCHMARK_CLIENT_WIDTH_PX=<exact run_protocol.width, for example 1920>
ASTRAL_BENCHMARK_CLIENT_HEIGHT_PX=<exact run_protocol.height, for example 1080>
ASTRAL_BENCHMARK_WARMUP_FRAMES=120
ASTRAL_BENCHMARK_MEASURED_FRAMES=3600
ASTRAL_BENCHMARK_CONTROL_JSON=<fresh absolute path for the run-control receipt>
```

Retain the previously admitted E14 whole-frame timing, main-thread phase timing, and process-memory capture controls and boundaries unchanged. The benchmark manifest must use `window_mode=windowed`, and its `run_protocol.width` / `height` must exactly equal the requested dimensions above. Do not edit a receipt, CSV, or manifest after collection to make values agree.

Before interpreting measurements, require all of the following:

1. exact package/release-manifest verification;
2. Windows environment receipt binding;
3. schema-v2 run-control verification with `client_area_control=environment_requested_and_verified`;
4. whole-frame, phase, and process-memory analyzers passing against their immutable source hashes;
5. cross-stream coherence verification passing for the same descriptor/candidate/package/frame interval; and
6. a separate matched capture-off control for instrumentation-overhead estimation.

If `AdjustWindowRectEx` plus window creation does not produce the exact requested client area on that Windows/DPI configuration, Astral must fail the benchmark before frame collection. Do not weaken that gate. A later dedicated DPI-awareness packet can address that case using measured native evidence.

## Limits and single next action

No native interactive GUI benchmark, actual GPU timing, VRAM measurement, accepted frame-time/RAM budget, matched UE5/Unity workload, clean-machine launch, recovery-stress run, real 24-hour soak, or independent review occurred in this pass. Code review by the implementation author is not independent review. PR #9 remains draft, and issue #7 must remain open.

Single next useful action: run the registered-Windows frozen procedural 3D benchmark at one manifest-declared client resolution using the environment-controlled path above, preserve the full receipt chain, and require all provenance/coherence checks plus a matched capture-off run before interpreting performance.
