# E14 benchmark client-area binding verification, 2026-09-21

Status: bounded implementation verified in sandbox and hosted CI; native registered-Windows benchmark and independent review remain pending. This is evidence for E14 benchmark reproducibility only, not UE5/Unity parity or engine acceptance.

## Identity and scope

- Repository: `LucasKazaki/AnimeRPG`.
- Draft PR: #9, branch `repair/2026-09-20-r0-runner-safety`.
- Pre-packet head: `6b01a7d69c66e4026b7838c7f7e3fd00862dc8d2`.
- Task-admission commit: `4270b517ad06575978c30284bb0d2004531b6091`.
- Exact hosted implementation candidate: `c1710164e23826d7e0a84becf30795b728cf0b96`.
- Diff from the pre-packet head: seven commits, seven admitted files, 326 changed lines. No paths outside `Tasks/E14-BENCHMARK-CLIENT-AREA-BINDING-2026-09-21.md` and its six allowed implementation/test paths changed.
- R0 was not invoked. Issue #7 remains open. No merge, release, deployment, dependency/plugin install, renderer/API change, Company Runtime change, local-PC process control, or paused game-content work occurred.

Published implementation blobs at the verified candidate:

- `Engine/Core/BenchmarkRunControl.h`: `88eebe7a3547352558d3ebc9d937926f8cd3198f`
- `Engine/Core/BenchmarkRunControl.cpp`: `f19966296731f2ac242c518c5f05013730950b84`
- `Engine/Platform/Win32Application.cpp`: `9b98b7cf6d153720ce470d1e0310733cb35412c9`
- `Tests/BenchmarkRunControlTests.cpp`: `942f97c94e9c2c4d09ee1ac85259b7fc562e9eb0`
- `Scripts/verify_benchmark_run_control.py`: `1e9ddd9345f30b222b302ca689612e66392bf0f1`
- `Scripts/test_benchmark_run_control.py`: `cd74538f2208a8cb6d5d057ef0f3d8a83ceba1b0`
- `Tasks/E14-BENCHMARK-CLIENT-AREA-BINDING-2026-09-21.md`: `3299978c94512b62340a6ce5b7099f27dc5a4097`

## Gap repaired

Before this packet, the benchmark manifest could declare a width, height, and window mode without proving the actual drawable Win32 client area. Astral creates an overlapped window with outer dimensions passed to `CreateWindowExW`, but the renderer consumes a `GetClientRect` viewport. Those are not equivalent measurements, and the client area can change during a run.

The production benchmark path now:

1. captures the initial positive Win32 client width/height before enabling benchmark run control;
2. stores those dimensions in `BenchmarkRunControl`;
3. uses the exact `RECT viewport` already consumed by the real GDI renderer as the per-rendered-frame observation;
4. requires exactly one matching client-area observation before every `CompleteFrame()` call;
5. latches duplicate, missing, invalid, or changed dimensions as a failed benchmark and refuses the completion receipt;
6. writes `client_width_px`, `client_height_px`, `client_area_observations`, `client_area_stable: true`, and `window_mode: "windowed"` only after exact frame-count completion; and
7. makes `verify_benchmark_run_control.py` reject a receipt whose width, height, or window mode differs from the SHA-256-bound benchmark manifest protocol.

Normal non-benchmark input and rendering behavior remain unchanged. This packet measures and binds the real client area; it does not add a resolution-setting feature or declare any resolution an approved parity target.

## Primary-source research

Sources were rechecked on 2026-09-21:

- Microsoft, Win32 Window Features: https://learn.microsoft.com/en-us/windows/win32/winmsg/window-features
- Microsoft, `GetClientRect`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getclientrect
- Epic Games, Unreal Engine 5.8 Command-Line Arguments: https://dev.epicgames.com/documentation/unreal-engine/command-line-arguments-in-unreal-engine
- Unity 6.0, `Screen` current window dimensions: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/Screen.html
- Unity 6.0, Windows Player settings: https://docs.unity3d.com/6000.0/Documentation/Manual/playersettings-windows.html

Applicability: Microsoft defines the client rectangle as the drawable client area with lower-right coordinates equal to its width/height. Unreal exposes explicit window/resolution launch controls (`-windowed`, `-ResX`, `-ResY`). Unity exposes current Player window dimensions and explicit windowed width/height/resize settings. These references support treating actual render dimensions as runtime evidence rather than trusting prose metadata. No proprietary engine source was copied and no new dependency was imported.

## Sandbox verification

A disposable partial fixture under `/mnt/data/astral_client_area_fixture` compiled the exact published `BenchmarkRunControl.h/.cpp` blobs. `git hash-object` matched the published GitHub blobs exactly:

- local `BenchmarkRunControl.cpp` -> `f19966296731f2ac242c518c5f05013730950b84`
- local `BenchmarkRunControl.h` -> `88eebe7a3547352558d3ebc9d937926f8cd3198f`

Commands and results:

```text
g++ -std=c++17 -Wall -Wextra -Werror -I. Engine/Core/BenchmarkRunControl.cpp Tests/ClientAreaFixtureTests.cpp -o build-debug/client_area_test
./build-debug/client_area_test
PASS: client-area fixture: PASS

g++ -std=c++17 -O2 -DNDEBUG -Wall -Wextra -Werror -I. Engine/Core/BenchmarkRunControl.cpp Tests/ClientAreaFixtureTests.cpp -o build-release/client_area_test
./build-release/client_area_test
PASS: client-area fixture: PASS

clang++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -I. Engine/Core/BenchmarkRunControl.cpp Tests/ClientAreaFixtureTests.cpp -o build-san/client_area_test
ASAN_OPTIONS=detect_leaks=1 ./build-san/client_area_test
PASS: client-area fixture: PASS
```

The fixture exercised stable 1280x720 observations, receipt publication, dimension change, missing observation, duplicate observation, zero dimensions, and dimensions beyond the 16,384-pixel hard bound. It is explicitly a partial source fixture, not a Windows GUI/native benchmark and not a substitute for the production test suite. A direct repository clone was attempted in the sandbox only to enable broader local checks, but that environment could not resolve `github.com`; hosted CI below therefore provides the complete-checkout exact-source evidence.

## Hosted exact-source verification

All workflows attached to exact candidate `c1710164e23826d7e0a84becf30795b728cf0b96` completed successfully.

### Profiling portability

- Run `35612878052`, job `106376145521`, Ubuntu 24.04: **PASS**.
- The workflow ran the Python profiling/provenance suites including `Scripts/test_benchmark_run_control.py`.
- It then configured and ran the production-linked FrameTiming subproject in Debug and optimized Release.
- It repeated the production-linked contracts under Clang AddressSanitizer + UndefinedBehaviorSanitizer with leak checking and halt-on-error behavior.
- The current benchmark run-control Python regression source contains ten unittest methods, including manifest width/height/window-mode mismatch rejection and client-area claim-boundary cases.
- The portable C++ target uses the real `Engine/Core/BenchmarkRunControl.cpp` with `Tests/BenchmarkRunControlTests.cpp`; that production test now reports eight groups and covers exact stable observations, missing/duplicate observations, dimension changes, invalid dimensions, output safety, environment parsing, and exact completion.

### Windows build and deterministic tests

- Run `35612877591`, job `106376280829`, Windows Server 2022: **PASS**.
- R0 safety, PE dependency, prerequisite plan, runtime-environment, runtime-compatibility, Redistributable bootstrap, and Release assertion/CTest safety contract steps all passed before the build.
- Visual Studio 2022 x64 configure passed.
- The real modified `AstralGame` Debug build passed, followed by deterministic Debug tests.
- The real modified `AstralGame` Release build passed, followed by runtime-dependency/prerequisite checks and deterministic Release tests.
- Static milestone verifiers and the final clean tracked-tree check passed.

This is hosted Windows compile/test evidence. It is not a claim that the GUI benchmark ran interactively on Lucas's machine.

### Release/package/provenance

- Run `35612877875`, job `106376196902`, Windows Server 2022: **PASS**.
- Manifest, package runtime, restart-stress, continuous-soak contract, soak-analysis, benchmark-manifest, and PE reproducibility diagnostic contract steps passed.
- Two same-candidate Release builds were classified byte-identical.
- A fresh Release executable was staged, manifested, and verified; the hosted benchmark-manifest contract fixture passed; final tracked tree was clean.

These are contract/packaging checks only. The required real 86,400-second native soak was not performed by this hosted workflow.

### Benchmark environment

- Run `35612877532`: **PASS**.
- Portable contract job `106376144625`: PASS.
- Windows CIM capture job `106376145068`: PASS, including a real hosted Windows CIM receipt for the ephemeral GitHub runner.

That CIM receipt describes the hosted runner, not Lucas's PC.

## Capability-to-evidence map impact

The authoritative initial comparison map remains on unmerged PR #8 and still marks broad comparability unproven. This packet advances only one measurable E14 requirement: protocol-declared width/height/window mode can now be checked against the actual Win32 client area used by the renderer for every completed benchmark frame.

Still unresolved: runtime/jobs/memory-system breadth; scene ownership/serialization; full asset pipeline; GPU rendering/materials; lighting/shadows/reflections; large-world streaming/detail; animation; physics/collision; AI/navigation; audio; UI/editor tooling; genuine 2D; networking; GPU timestamps; active-render-adapter proof; VRAM; allocator/thread/task attribution; terrain/foliage; particles/VFX; cinematics; scripting/reflection; input/replay; localization/accessibility; additional platforms; approved performance/RAM/VRAM budgets; matched UE5/Unity workloads; clean-machine launch; recovery stress; 86,400-second native soak; and independent acceptance.

## Native registered-Windows handoff

The next accepted benchmark attempt must retain exact source/package/machine/toolchain/driver identity, UTC timestamps, commands, stdout/stderr, exit codes, file hashes, and captures required by the existing E14 handoffs. Use the frozen procedural 3D workload with:

- fixed simulation: 60 Hz;
- warmup: 120 rendered frames;
- measured frames: 3,600;
- live input suppressed by benchmark mode;
- whole-frame timing, main-thread phase timing, and process-memory captures enabled with their admitted boundaries/stride;
- environment receipt and package/release manifest bound into the benchmark manifest;
- run-control receipt included as `benchmark_run_control_json`;
- manifest `run_protocol.width`, `height`, and `window_mode` exactly equal to the completed run-control receipt;
- `verify_benchmark_run_control.py` and `verify_benchmark_stream_coherence.py` both passing before interpreting measurements; and
- a separate matched capture-off control before estimating instrumentation overhead.

A client-area change must invalidate the benchmark. Do not edit the receipt or manifest afterward to make them agree. Because Astral still creates a resizable overlapped window with an outer 1280x720 creation size, the actual client dimensions should be treated as measured facts. This packet does not guarantee a 1280x720 client area or provide an approved 1920x1080 control path.

## Limits and single next action

No native GPU/GUI benchmark, performance budget, RAM/VRAM budget, matched Unreal/Unity comparison, clean-machine launch, real 24-hour soak, or independent review occurred in this pass. Code review by the implementation author is not independent review. PR #9 remains draft and issue #7 must remain open.

Single next useful action: run the registered-Windows frozen procedural 3D benchmark using the exact package/control/evidence chain above. If that run exposes a client-resolution mismatch against the desired comparison protocol, admit a separate bounded benchmark-resolution-control packet rather than rewriting the recorded evidence.
