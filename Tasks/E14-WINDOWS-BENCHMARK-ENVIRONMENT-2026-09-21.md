# E14 bounded Windows benchmark environment evidence packet, 2026-09-21

Status: admitted independent E14 verification packet on draft PR #9 while the registered native 3D benchmark remains an external/local gate. This packet makes benchmark OS/CPU/RAM/GPU/driver labels machine-captured and manifest-bound instead of purely operator-entered text. It does not run Astral on Lucas's PCs, invoke R0, measure frame time/RAM/VRAM usage, prove the active render adapter, establish budgets/parity, merge/release/deploy, install dependencies, change the graphics architecture, or restart paused content work.

## Identity and dependency

- Repository: `LucasKazaki/AnimeRPG`.
- Owned branch / existing draft PR: `repair/2026-09-20-r0-runner-safety`, PR #9.
- Pre-packet head: `1852532efc3a6983b93d0d2cda05c053f7bb301c`.
- Dependency: E14 benchmark manifest/provenance tooling already present on PR #9.
- PR #9 remains stacked on PR #6 / `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`.
- Issue #7 remains open. The historical R0 runner is not authorized for this packet.
- Capability impact: E14 environment provenance only. No capability becomes comparable or independently accepted.

## Reproducible gap

`Scripts/benchmark_manifest.py` deliberately labels its environment fields as operator-supplied and states that independent machine receipts are still required. A future native benchmark can therefore have valid package and stream hashes while its OS, CPU, logical-CPU count, RAM capacity, GPU name, or driver label is mistyped or copied from another machine. The current coherence verifier cannot detect that mismatch.

## Allowed paths

Only these paths may change:

1. `Scripts/benchmark_environment.py`
2. `Scripts/test_benchmark_environment.py`
3. `.github/workflows/benchmark-environment-validation.yml`
4. `Tasks/E14-WINDOWS-BENCHMARK-ENVIRONMENT-2026-09-21.md`
5. `.github/workflows/frame-timing-validation.yml`
6. `Docs/QA/E14-WINDOWS-BENCHMARK-ENVIRONMENT-2026-09-21.md`

No Engine/Game/CMake source, renderer/API, dependency, package authority, Company Runtime state, scheduler, permission, release, deployment, or content path is authorized.

## Primary-source research

Rechecked on 2026-09-21:

- Microsoft PowerShell 7.5 `Get-CimInstance`: https://learn.microsoft.com/en-us/powershell/module/cimcmdlets/get-ciminstance?view=powershell-7.5 . The cmdlet retrieves local CIM/WMI snapshots on Windows when no remote computer/session is supplied. Applicability: use a bounded local query only, with no networking.
- Microsoft `Win32_ComputerSystem`: https://learn.microsoft.com/en-us/windows/win32/cimwin32prov/win32-computersystem . `NumberOfLogicalProcessors` reports logical processors available to the computer; `TotalPhysicalMemory` supplies installed physical memory. Applicability: bind benchmark logical-CPU and RAM-capacity labels to the machine receipt.
- Microsoft `Win32_Processor`: https://learn.microsoft.com/en-us/windows/win32/cimwin32prov/win32-processor . One instance exists per processor and exposes processor names/core/logical-processor counts. Applicability: retain the enumerated CPU set and derive a deterministic CPU label.
- Microsoft `Win32_OperatingSystem`: https://learn.microsoft.com/en-us/windows/win32/cimwin32prov/win32-operatingsystem . It represents the installed Windows OS and exposes caption/version/build/architecture fields. Applicability: bind the benchmark OS label to a concrete Windows build.
- Microsoft `Win32_VideoController`: https://learn.microsoft.com/en-us/windows/win32/cimwin32prov/win32-videocontroller . It exposes video-controller name and driver version but warns that non-WDDM hardware may report inaccurate properties. Applicability: retain all enumerated adapters and driver versions, but do not claim active-adapter selection or VRAM acceptance.
- Unreal Engine 5.8 Insights Session Browser: https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-insights-session-browser-for-unreal-engine . Sessions expose configuration/target/branch metadata for interpreting traces. Applicability: benchmark interpretation depends on retained execution context, not raw timing values alone.
- Unity 6.0 `SystemInfo`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/SystemInfo.html . Unity exposes processor, memory, graphics device, graphics API/driver, and related hardware/runtime information. Applicability: Astral needs similarly explicit environment provenance before comparative interpretation.

No proprietary engine source is copied. No third-party dependency is added. The implementation uses Python standard library plus Windows PowerShell/CIM already present on supported Windows systems.

## Implementation contract

`Scripts/benchmark_environment.py` must fail closed and:

- provide a Windows-only `capture` mode that queries only the local machine with `Get-CimInstance` and a bounded subprocess timeout;
- retain Windows caption/version/build/architecture, computer manufacturer/model, logical-processor count, total physical memory, per-processor name/core/thread counts, and each enumerated video-controller name/driver/PNP ID/reported adapter RAM/video processor;
- deliberately omit host name, serial number, UUID, user identity, network identifiers and credentials;
- derive the exact six fields used by `benchmark_manifest.py`: `os`, `cpu`, `logical_cpus`, `ram_bytes`, `gpu`, `gpu_driver`;
- sort multi-CPU/GPU records deterministically and keep all enumerated GPUs rather than silently selecting one;
- label the source `Windows_CIM_GetCimInstance` and make WDDM/active-adapter/VRAM limitations explicit;
- refuse output overwrite and reject malformed types, including boolean-to-integer laundering;
- provide `validate` for an immutable receipt;
- provide `verify` that re-runs production benchmark-manifest/package verification, requires exactly one `windows_benchmark_environment_json` evidence role, re-hashes the receipt from the evidence root, requires exact environment and machine-label equality, and requires the Windows CIM source for `local_native` provenance; and
- keep environment-binding acceptance, performance budget, comparative parity, clean-machine compatibility and independent acceptance hard-false. A JSON receipt is evidence, not cryptographic hardware attestation.

## Verification contract

- Test discovery contains 17 cases: 14 dependency-free normalization/binding tests plus three production-manifest integration tests that run in a full repository checkout.
- Cases cover deterministic normalization, multi-GPU ordering, missing GPU, integer type laundering, raw-to-derived environment tamper rejection, hard-false claim boundaries, hosted fixture binding, native fixture-source rejection, exact environment/machine/hash matching, duplicate-role rejection, exclusive output, real production release/benchmark-manifest integration, raw receipt tampering, and exact candidate identity.
- Hosted `windows-2022` must execute the real local CIM capture path and validate the resulting receipt. This is hosted machine-evidence contract testing only, not evidence about Lucas's PCs.
- The profiling portability workflow must run the environment suite alongside existing E14 analyzers, while the dedicated environment workflow must exercise real hosted Windows CIM capture. Existing PR #9 Windows and package workflows must remain green for the implementation candidate. This packet changes no C++ source, so those lanes are regression evidence only.

## Native handoff

Before constructing the frozen native benchmark manifest on the registered Windows executor:

```powershell
python Scripts/benchmark_environment.py capture `
  --machine-label <registered-machine-label> `
  --output <immutable-evidence-root>/windows-benchmark-environment.json
```

Add that fresh file to the benchmark specification as role `windows_benchmark_environment_json` and use the six values from its `benchmark_environment` object verbatim. After building the benchmark manifest, run:

```powershell
python Scripts/benchmark_environment.py verify `
  --benchmark-manifest <benchmark.json> `
  --package-root <exact-package-root> `
  --release-manifest <release-manifest.json> `
  --evidence-root <immutable-evidence-root> `
  --environment-receipt <immutable-evidence-root>/windows-benchmark-environment.json `
  --expected-commit <40-hex-admitted-revision> `
  --expected-executable-sha256 <64-hex-AstralGame.exe-sha256> `
  --output <fresh-environment-binding.json>
```

Retain the environment receipt/hash, command/stdout/stderr/exit/UTC timestamps, source/package hashes, and the normal benchmark machine/toolchain/GPU/driver evidence. The registered local executor, not this coordinator, owns execution on Lucas's PC.

## Stop, rollback, next action

Stop at the first deterministic parser/CIM/CI failure. Do not weaken exact environment equality or native-source requirements to make a run pass. Rollback is deletion/revert of the six allowed packet paths only; do not force-push or discard concurrent work.

After this packet, the single useful next action remains the frozen registered-Windows 3D benchmark, but its manifest must now include and verify the machine-captured environment receipt before stream coherence or performance observations are interpreted. If that native gate remains unavailable, the next coordinator pass should continue with another independent E14 evidence-quality packet rather than fabricate native progress.
