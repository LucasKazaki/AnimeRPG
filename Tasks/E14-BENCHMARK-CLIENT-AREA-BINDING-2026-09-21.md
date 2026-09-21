# E14 bounded benchmark client-area binding packet, 2026-09-21

Status: admitted independent E14 benchmark-reproducibility repair on draft PR #9. This packet binds the actual Win32 render client area to the benchmark evidence and fails closed if that area changes during the measured run. It does not invoke R0, run Astral on Lucas's PCs, establish a performance budget or parity, merge/release/deploy, add dependencies, change the renderer/API, or restart paused content work.

## Identity and dependency

- Repository: `LucasKazaki/AnimeRPG`.
- Existing owned branch / draft PR: `repair/2026-09-20-r0-runner-safety`, PR #9.
- Pre-packet head: `6b01a7d69c66e4026b7838c7f7e3fd00862dc8d2`.
- Dependency: verified E14 benchmark run-control candidate `f7f7679a5bf017fb683dd8c6f74e5a854b15779a` and its evidence-only follow-up at the pre-packet head.
- Issue #7 remains open. The historical R0 runner is not authorized for this packet.
- Capability impact: E14 profiling/benchmark evidence only. No engine feature becomes comparable or independently accepted.

## Reproducible gap

The benchmark manifest already declares `run_protocol.width`, `run_protocol.height`, and `run_protocol.window_mode`, but the production run-control receipt does not prove that the actual Win32 client area matched those dimensions or stayed unchanged. `Win32Application::Create` passes `1280x720` as the outer `CreateWindowExW` size, while rendering later uses `GetClientRect`. On Windows, the client rectangle is the drawable client area and is distinct from the outer window rectangle. A user or OS resize can also change it during a benchmark. Therefore a manifest can currently claim one render size while the executable actually renders another size, and the run-control verifier will still accept the receipt.

This is an evidence-integrity failure for matched UE5/Unity comparisons. The repair binds the actual client width/height to the completion receipt, observes them once per completed frame, refuses completion after a mismatch or missing observation, and makes the verifier require equality with the benchmark manifest's declared width/height/window mode.

## Primary-source research

Rechecked 2026-09-21:

- Microsoft Win32 window/client-area documentation: https://learn.microsoft.com/en-us/windows/win32/winmsg/window-features . `GetClientRect` returns the client area's lower-right coordinates as its width/height, relative to `(0,0)`. Applicability: the rendered GDI viewport uses this client rectangle, so benchmark evidence must bind the client area rather than assume the `CreateWindowExW` outer dimensions.
- Epic Unreal Engine 5.8 command-line arguments: https://dev.epicgames.com/documentation/unreal-engine/command-line-arguments-in-unreal-engine . Epic documents `-windowed`, `-ResX`, and `-ResY` as explicit launch controls. Applicability: matched performance runs need explicit, evidence-backed resolution/window settings.
- Unity 6.0 `Screen`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/Screen.html . Unity exposes current window width/height in pixels and explains that initial resolution/full-screen settings are selected before rendering. Applicability: the comparison target treats render-window dimensions as concrete runtime state, not prose metadata.
- Unity 6.0 Windows Player settings: https://docs.unity3d.com/6000.0/Documentation/Manual/playersettings-windows.html . Windowed player width/height and resize policy are explicit player settings. Applicability: Astral's benchmark evidence should likewise prove the actual windowed render size used for the run.

No proprietary source is copied and no dependency is added.

## Allowed paths

Implementation may change only:

1. `Engine/Core/BenchmarkRunControl.h`
2. `Engine/Core/BenchmarkRunControl.cpp`
3. `Engine/Platform/Win32Application.cpp`
4. `Tests/BenchmarkRunControlTests.cpp`
5. `Scripts/verify_benchmark_run_control.py`
6. `Scripts/test_benchmark_run_control.py`
7. `Tasks/E14-BENCHMARK-CLIENT-AREA-BINDING-2026-09-21.md`

After verification, `Docs/QA/E14-BENCHMARK-CLIENT-AREA-BINDING-2026-09-21.md` may be added as an evidence-only follow-up. No renderer implementation, scene/game-content, package authority, Company Runtime, scheduler, permission, deployment, or third-party dependency path is authorized.

## Runtime contract

When benchmark mode is enabled:

- `Win32Application` queries the current `GetClientRect` before run-control configuration and supplies the positive client width/height to `BenchmarkRunControl`.
- Run control records that initial client area and requires a successful observation of the same width/height exactly once before every completed rendered frame.
- A missing observation, duplicate observation, zero/invalid client area, or any width/height change invalidates the run and prevents the completion receipt from being published.
- The completion receipt records `client_width_px`, `client_height_px`, `client_area_observations`, `client_area_stable: true`, and `window_mode: "windowed"` only after exact frame-count completion with matching per-frame observations.
- The production verifier requires those receipt fields to match the benchmark manifest's `run_protocol.width`, `run_protocol.height`, and `run_protocol.window_mode` exactly.
- Normal non-benchmark interaction remains unchanged.
- This packet does not claim that 1280x720 is an approved comparison resolution. The actual dimensions are evidence; future matched reference runs must use the same approved protocol.

## Verification contract

Portable/sandbox:

- compile production `BenchmarkRunControl.cpp` plus the production-linked test under C++17 with warnings as errors;
- run Debug/non-optimized and optimized Release variants;
- run Clang ASan+UBSan with leak checking;
- byte-compile and run the Python run-control verifier regression suite;
- test exact stable observations, missing/duplicate observations, dimension changes, zero dimensions, receipt fields, manifest dimension/window-mode mismatch, and unchanged claim boundaries.

Hosted exact-source gates:

- profiling portability workflow must pass the Python verifier and C++ run-control tests in Debug, optimized Release, and ASan+UBSan;
- Windows Server 2022 must compile the actual modified `AstralGame` in Debug and Release and run all deterministic non-GUI tests;
- release/package/provenance/reproducibility and benchmark-environment workflows must remain green.

No timeout, assertion, provenance, or acceptance gate may be weakened to make this packet pass.

## Native handoff

After hosted verification, the registered Windows executor should run the frozen procedural 3D benchmark only with a benchmark manifest whose `run_protocol.width`, `run_protocol.height`, and `run_protocol.window_mode` match the actual run-control receipt. A resize or client-area query failure must invalidate the run rather than be normalized away. Retain the existing fixed simulation, 120-frame warmup, 3,600 measured-frame, timing/phase/memory, environment, package, coherence, capture-off-control, and exact receipt requirements.

## Stop, rollback, and next action

Stop at the first deterministic compile/test/CI regression and repair only inside the allowed paths. Do not bypass failures or invoke R0. Rollback is a revert of this bounded packet only, with no force push or destructive cleanup.

If this packet verifies, the next useful action remains the registered-Windows frozen procedural 3D benchmark using the exact package/control/evidence chain. GPU timestamps/active-adapter proof, VRAM, approved performance/RAM budgets, matched UE5/Unity 3D and genuine-2D workloads, clean-machine launch, recovery stress, the real 86,400-second soak, and independent acceptance remain separate unresolved gates.
