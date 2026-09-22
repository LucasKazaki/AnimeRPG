# E14 bounded benchmark resolution-control packet, 2026-09-21

Status: admitted independent E14 benchmark-reproducibility repair on draft PR #9. This packet makes benchmark client resolution an explicit launch control instead of merely observing whatever client area the default outer-window size happens to produce. It does not invoke R0, run Astral on Lucas's PCs, establish a performance budget or parity, merge/release/deploy, add dependencies, change the renderer/API, or restart paused content work.

## Identity and dependency

- Repository: `LucasKazaki/AnimeRPG`.
- Existing owned branch / draft PR: `repair/2026-09-20-r0-runner-safety`, PR #9.
- Pre-packet head: `a97fbd74965e9904727ef0df3f803c32f05a00ea`.
- Dependency: verified client-area binding candidate `c1710164e23826d7e0a84becf30795b728cf0b96` and its evidence-only follow-up at the pre-packet head.
- Issue #7 remains open. The historical R0 runner is not authorized for this packet.
- Capability impact: E14 benchmark control/provenance only. No engine feature becomes comparable or independently accepted.

## Reproducible gap

The run-control receipt now proves the rendered Win32 client area stayed stable and matches the benchmark manifest, but Astral still creates a fixed `1280x720` *outer* window and only observes the resulting client area afterward. That makes an intended matched protocol such as `1920x1080` impossible to request through the benchmark control plane. It also leaves window-frame metrics and DPI behavior as hidden inputs. A native operator could therefore only discover a protocol mismatch after launch rather than asking Astral to create the intended client area and then proving that Windows actually supplied it.

The bounded repair adds explicit benchmark-only requested client width/height environment controls, sizes the initial `WS_OVERLAPPEDWINDOW` from the requested client rectangle, and still fails closed unless the actual `GetClientRect` result equals the request and remains stable for every completed frame. Normal non-benchmark window behavior remains unchanged.

## Primary-source research

Rechecked 2026-09-21:

- Microsoft `AdjustWindowRectEx`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-adjustwindowrectex . It computes the required window rectangle from a desired client rectangle for a specified window style and extended style. Applicability: Astral must translate an admitted render-client size into an outer `WS_OVERLAPPEDWINDOW` size before `CreateWindowExW`. The API is not per-monitor-DPI aware, so the packet keeps `GetClientRect` equality as the authoritative fail-closed runtime check instead of assuming sizing succeeded.
- Microsoft window/client-area model: https://learn.microsoft.com/en-us/windows/win32/winmsg/window-features . Applicability: the benchmark dimension is the drawable client area, not the nonclient-inclusive outer window rectangle.
- Unreal Engine 5.8 command-line arguments: https://dev.epicgames.com/documentation/unreal-engine/command-line-arguments-in-unreal-engine . Epic documents `-windowed`, `-ResX`, and `-ResY` as explicit launch controls. Applicability: matched reference runs need a requested runtime resolution plus observed evidence.
- Unreal Engine 5.8 dedicated-server testing example: https://dev.epicgames.com/documentation/unreal-engine/setting-up-dedicated-servers-in-unreal-engine . The documented client launch uses `-WINDOWED -ResX=800 -ResY=450`, confirming explicit per-run sizing is a normal test control.
- Unity 6.0 `Screen.SetResolution`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/Screen.SetResolution.html . Unity exposes explicit requested width/height and window/fullscreen mode. Applicability: Astral needs an equivalent bounded benchmark launch control before matched measurements.

No proprietary engine source is copied and no third-party dependency is added.

## Allowed paths

Implementation may change only:

1. `Engine/Core/BenchmarkRunControl.h`
2. `Engine/Core/BenchmarkRunControl.cpp`
3. `Engine/Platform/Win32Application.cpp`
4. `Tests/BenchmarkRunControlTests.cpp`
5. `Scripts/verify_benchmark_run_control.py`
6. `Scripts/test_benchmark_run_control.py`
7. `Tasks/E14-BENCHMARK-RESOLUTION-CONTROL-2026-09-21.md`

After verification, `Docs/QA/E14-BENCHMARK-RESOLUTION-CONTROL-2026-09-21.md` may be added as an evidence-only follow-up. No renderer implementation, scene/game-content, package authority, Company Runtime, scheduler, permission, deployment, or dependency path is authorized.

## Runtime contract

When `ASTRAL_BENCHMARK_MODE=1`:

- `ASTRAL_BENCHMARK_CLIENT_WIDTH_PX` and `ASTRAL_BENCHMARK_CLIENT_HEIGHT_PX` are mandatory unsigned decimal integers in `[1, 16384]`.
- Before `CreateWindowExW`, Astral converts that requested client rectangle to an outer `WS_OVERLAPPEDWINDOW` rectangle with `AdjustWindowRectEx` and rejects sizing failure or invalid outer dimensions.
- `BenchmarkRunControl::ConfigureFromEnvironment` independently re-parses the request and requires the actual initial `GetClientRect` dimensions to equal it exactly. A mismatch fails before benchmark frames begin.
- Existing per-frame client-area observation remains mandatory, so a later resize or client-area drift still invalidates the run.
- Run-control receipt schema v2 records `client_area_control: "environment_requested_and_verified"`; the production verifier rejects old/uncontrolled receipts and still requires width, height, and window mode to match the SHA-256-bound benchmark protocol.
- Benchmark-only sizing must not affect ordinary launches when benchmark mode is absent.

## Verification contract

Portable/sandbox:

- compile the exact production `BenchmarkRunControl.cpp` and production-linked test under C++17 Debug/non-optimized and optimized Release;
- run Clang ASan+UBSan with leak checking;
- cover missing/orphan/malformed width/height controls, exact bounds, actual-vs-requested mismatch, happy path, receipt schema/source field, and unchanged no-overwrite/frame/client-stability contracts;
- byte-compile the Python verifier/test changes and, in a complete checkout, run the full production manifest integration suite.

Hosted exact-source gates:

- profiling portability workflow must pass the Python verifier plus run-control tests in Debug, optimized Release, and ASan+UBSan;
- Windows Server 2022 must compile the actual modified `AstralGame` in Debug and Release and run all deterministic non-GUI tests;
- release/package/provenance/reproducibility and benchmark-environment workflows must remain green.

No timeout, assertion, provenance, or acceptance gate may be weakened to make this packet pass.

## Native handoff

After hosted verification, the registered Windows executor should launch the frozen procedural 3D benchmark with `ASTRAL_BENCHMARK_CLIENT_WIDTH_PX` / `ASTRAL_BENCHMARK_CLIENT_HEIGHT_PX` exactly matching the benchmark manifest. Retain the 60 Hz fixed simulation, 120 warmup frames, 3,600 measured frames, timing/phase/memory streams, environment/package/run-control evidence, cross-stream coherence check, and matched capture-off control. Keep exact source/package hashes, machine/toolchain/driver identity, command lines, stdout/stderr, exit codes, UTC timestamps, and captured images where applicable.

## Stop, rollback, and next action

Stop at the first deterministic compile/test/CI regression and repair only inside the allowed paths. Do not bypass failures or invoke R0. Rollback is a revert of this bounded packet only, with no force push or destructive cleanup.

If this packet verifies, the next useful action is the registered-Windows frozen procedural 3D benchmark using one explicitly requested and observed client resolution. GPU timestamps/active-adapter proof, VRAM, approved performance/RAM budgets, matched UE5/Unity 3D and genuine-2D workloads, clean-machine launch, recovery stress, the real 86,400-second soak, and independent acceptance remain separate unresolved gates.
