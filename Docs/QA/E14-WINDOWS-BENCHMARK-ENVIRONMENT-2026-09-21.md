# E14 Windows benchmark environment evidence QA, 2026-09-21

## Checkpoint

- Repository: `LucasKazaki/AnimeRPG`.
- Existing draft PR: #9, branch `repair/2026-09-20-r0-runner-safety`.
- Pre-packet head: `1852532efc3a6983b93d0d2cda05c053f7bb301c`.
- Implementation candidate: `500794e22284be2616f8bc64e4ae6e2329522b24` (`E14: bind benchmark environment to Windows CIM evidence`).
- PR state after implementation: open, draft, mergeable, unmerged.
- Issue #7 remains open. R0 was not invoked.
- No Engine/Game/CMake source, graphics API, dependency, Company Runtime state, package authority, release/deployment, or content changed.

## Why this packet exists

The production benchmark manifest deliberately treated `os`, `cpu`, `logical_cpus`, `ram_bytes`, `gpu`, and `gpu_driver` as operator-supplied labels. Package hashes and timing/memory streams could therefore be internally coherent while those machine labels were mistyped or copied from a different machine. This packet adds a bounded Windows CIM receipt and exact manifest binding so a future native benchmark cannot silently substitute different environment labels.

This is environment provenance only. It does not measure performance, prove the active render adapter, measure VRAM, establish RAM/VRAM/frame-time budgets, prove clean-machine compatibility, or establish Unreal/Unity parity or independent acceptance.

## Primary-source basis

Checked 2026-09-21:

- Microsoft PowerShell 7.5 `Get-CimInstance`: https://learn.microsoft.com/en-us/powershell/module/cimcmdlets/get-ciminstance?view=powershell-7.5 . The cmdlet is Windows-only and, without `ComputerName` or `CimSession`, queries local WMI via a local COM session.
- Microsoft `Win32_ComputerSystem`: https://learn.microsoft.com/en-us/windows/win32/cimwin32prov/win32-computersystem . Used for logical-processor count and total physical memory. Microsoft notes `TotalPhysicalMemory` can differ from exact installed-memory capacity in some firmware-reserved cases, so the receipt labels it as the system-reported value rather than a memory-budget result.
- Microsoft `Win32_Processor`: https://learn.microsoft.com/en-us/windows/win32/cimwin32prov/win32-processor . One instance exists per processor and exposes core/logical-processor data.
- Microsoft `Win32_OperatingSystem`: https://learn.microsoft.com/en-us/windows/win32/cimwin32prov/win32-operatingsystem . Used for caption/version/build/architecture.
- Microsoft `Win32_VideoController`: https://learn.microsoft.com/en-us/windows/win32/cimwin32prov/win32-videocontroller . Used for enumerated adapter names and driver versions; Microsoft warns non-WDDM hardware can report inaccurate properties.
- Unreal Engine 5.8 Insights Session Browser: https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-insights-session-browser-for-unreal-engine . Trace sessions retain application/configuration/target/branch context, supporting the requirement that performance evidence retain execution context.
- Unity 6.0 `SystemInfo`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/SystemInfo.html and https://docs.unity3d.com/6000.0/Documentation/ScriptReference/SystemInfo-graphicsDeviceVersion.html . Unity exposes processor/memory/graphics-device and graphics API/driver context; Astral's receipt is a narrower provenance layer, not profiler parity.

No proprietary engine source was copied and no dependency was added.

## Changed paths and exact published blobs

The implementation commit changes exactly five paths relative to `1852532...`:

| Path | Git blob | SHA-256 of exact sandbox source |
| --- | --- | --- |
| `Scripts/benchmark_environment.py` | `d9e4f6f6c2de16db26f430e1e227ec274442af04` | `87a4bee41d8bd366b137766a94720370dacd3589262e15c7ea61325e76250a19` |
| `Scripts/test_benchmark_environment.py` | `9249c1a8409cd1cc0a4f92fcfa6c616aef325c1a` | `1f99e74df444a2f123d3ecb3d471fe57c57141fd8c9594e8ee38843715943b12` |
| `.github/workflows/benchmark-environment-validation.yml` | `d3d9678d2e8215d54ce922463d5bc9861d25d38d` | `8fd016978af64b0ee62875b8dd29eaa165fa2d189a212afa76527f50e9b3f1d0` |
| `.github/workflows/frame-timing-validation.yml` | `f31b7c91ddeb04e187d58d2b8a1514d5ca4d2520` | `046af7f866f16be7e621df5f8408892b80b045313ebd101f1f0dec62ae31d535` |
| `Tasks/E14-WINDOWS-BENCHMARK-ENVIRONMENT-2026-09-21.md` | `6d60c60e943653daa96a280223f5d8a9f044fbeb` | `c5161ed0fbfc27a1decc986cbbb0563bda9272e8c179799a32a7f8b1bedb9e88` |

GitHub compare reported one fast-forward commit, ahead by one, with only those five files changed. No concurrent branch commits were overwritten and the branch update was non-force.

## Implementation evidence

`Scripts/benchmark_environment.py` now:

- captures only local Windows CIM data with a bounded PowerShell subprocess and no remote computer/session argument;
- deliberately omits hostname, serial/UUID, user identity and network identifiers;
- retains Windows version/build/architecture, system manufacturer/model, logical CPUs and reported physical memory, per-processor core/thread identity, and all enumerated video controllers plus driver/PNP/reported-adapter-RAM/video-processor data;
- deterministically derives the exact six environment fields consumed by `benchmark_manifest.py`;
- requires raw CIM properties and derived benchmark labels to agree exactly, so either side cannot be edited independently;
- rejects boolean-to-integer laundering, missing CPU/GPU inventories, noncanonical CPU/GPU ordering, malformed receipts, symlink/path escapes, duplicate environment-evidence roles, changed receipt hashes, machine-label changes and candidate/package mismatches;
- refuses to overwrite capture or verification output;
- re-runs the production benchmark/package verifier before accepting a binding; and
- requires `Windows_CIM_GetCimInstance` plus `capture_platform=Windows` for a `local_native` benchmark while keeping every acceptance field false.

The JSON receipt is not cryptographic hardware attestation. The `source` field could be forged by an operator who is already able to edit evidence. Retained commands/logs, package hashes and independent review therefore remain required. Enumerating video controllers also does not prove which adapter actually rendered the workload, and reported `AdapterRAM` is retained only as raw context, not VRAM usage or a budget result.

## Verification results

### Disposable sandbox

Exact published script/test content was checked in the disposable Linux fixture:

```text
python -m py_compile benchmark_environment.py test_benchmark_environment.py
python test_benchmark_environment.py
```

Result: PASS. Test discovery: 17. The 14 dependency-free cases passed; three production-manifest integration tests were intentionally skipped because the disposable fixture did not contain repository `benchmark_manifest.py` / `release_manifest.py`. Published Git blob IDs exactly matched `git hash-object` for the sandbox-tested files before commit.

### GitHub Actions for implementation candidate

GitHub pull-request workflows check out synthetic merge revision `97ab526268fa6a17e2275d5ed8929090081675a0`, which GitHub formed by merging implementation head `500794e22284be2616f8bc64e4ae6e2329522b24` into base `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`. Therefore these are hosted candidate-integrated checks, not a claim that Actions checked out the branch-head SHA directly.

- **Benchmark environment evidence**, run `35594647699`: PASS.
  - Ubuntu portable job `106316579169`: PASS. All 17 tests ran in the full repository, including the three production release/benchmark-manifest integration cases.
  - Windows CIM job `106316579325`: PASS. The same 17 tests passed on Windows Server 2022, then the real local `Get-CimInstance` capture/validate path succeeded. The hosted receipt derived Windows Server 2022 build 20348, 4 logical CPUs, approximately 17.17 GB system-reported RAM, Intel Xeon 6973P-C, and Microsoft Hyper-V Video driver 10.0.20348.1. This describes the ephemeral hosted runner only, not Lucas's hardware.
- **Profiling capture portability**, run `35594647707`, job `106316579484`: PASS. Existing E14 analyzers plus the new environment suite passed, followed by Debug/optimized Release profiling contracts and Clang ASan+UBSan/leak checks.
- **Release manifest integrity**, run `35594647732`, job `106316579683`: PASS. Existing manifest/runtime/restart/soak-contract/benchmark/reproducibility/package/clean-tree checks remained green, including byte-identical repeated Release builds.
- **Windows build and deterministic tests**, run `35594647679`, job `106316579417`: PASS. R0 safety, PE/prerequisite/runtime/bootstrap contracts, VS2022 Debug and Release builds/tests, milestone verification and clean-tree checks all completed successfully.

No native GUI test, local GPU behavior, clean-machine package launch, measured performance, stress/recovery, 86,400-second soak, or independent review was executed by this coordinator.

## Capability-to-evidence checkpoint

- Runtime/jobs/memory: process-level memory capture and descriptive analysis exist; allocator/tag/callstack attribution and accepted native budgets remain open.
- Scene ownership/serialization: still below comparison target; unchanged by this packet.
- Asset pipelines: E0 candidate remains separate/unmerged; unchanged by this packet.
- GPU rendering/materials; lighting/shadows/reflections; large-world streaming/detail; animation; physics/collision; AI/navigation; audio; UI/editor; genuine 2D; networking: no new parity evidence in this packet.
- Profiling: whole-frame CPU intervals, flat Win32 main-thread phases, process memory, stream coherence, deterministic benchmark manifests, and now machine-captured Windows environment binding have hosted contract evidence. GPU timestamps, active-adapter proof, worker/render-thread attribution, VRAM and allocator attribution remain open.
- Packaging/platforms: deterministic hosted Release/package evidence exists; clean-machine and broader platform acceptance remain open.
- Remaining catalogue: terrain, particles/VFX, cinematics, scripting/reflection, input/replay, accessibility/localization and additional platforms remain explicit unresolved capability rows. None is removed to make a parity claim easier.

## Native handoff and single next action

The registered Windows executor should capture the environment receipt before constructing the frozen native benchmark manifest, include it as role `windows_benchmark_environment_json`, copy the six derived environment fields verbatim, and run `benchmark_environment.py verify` after manifest creation. Retain receipt/hash, package/revision hashes, exact commands/stdout/stderr/exit codes/UTC timestamps and machine/toolchain/GPU/driver evidence.

Then run the already-defined frozen procedural 3D benchmark: 120 warmup frames, 3,600 whole-frame and main-thread-phase samples, process-memory samples at the declared stride, plus a matched capture-off control. Require environment binding and cross-stream coherence to pass before interpreting measurements.

If the registered native gate is still unavailable, a later coordinator pass may advance another independent E14 evidence-quality packet. It must not fabricate native results or repeatedly poll unchanged local state.
