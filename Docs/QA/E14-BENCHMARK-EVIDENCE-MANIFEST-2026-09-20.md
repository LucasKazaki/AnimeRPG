# E14 benchmark evidence manifest QA receipt, 2026-09-20

Status: bounded E14 provenance implementation verified in hosted Windows CI. This receipt does **not** establish an Astral performance budget, UE5/Unity parity, clean-machine compatibility, native GPU measurements, the required 24-hour soak, or independent acceptance.

## Packet identity

- Repository: `LucasKazaki/AnimeRPG`.
- Existing draft PR: #9, branch `repair/2026-09-20-r0-runner-safety`.
- Packet baseline: `f15ede2fadb56012e5ccf22dd1f6a776ec2a4fe9`.
- Implementation candidate tested by the PR workflows: `536c213fd1af85ebf7acc6afb818805222d26497`.
- GitHub pull-request merge revision actually checked out and compiled by the hosted workflows: `9f2f31c134d6d527f045bcbd8afeaff4a2369216`.
- Bounded task contract: `Tasks/E14-BENCHMARK-EVIDENCE-MANIFEST-2026-09-20.md`.

The implementation candidate changed only the packet's authorized files before this QA receipt was added:

- `Scripts/benchmark_manifest.py`
- `Scripts/test_benchmark_manifest.py`
- `.github/workflows/release-manifest-validation.yml`
- `Tasks/E14-BENCHMARK-EVIDENCE-MANIFEST-2026-09-20.md`

No `Engine/`, `Game/`, `Tests/`, CMake, dependency, graphics API, Company Runtime, or paused content path changed in this packet.

## What was implemented

`Scripts/benchmark_manifest.py` adds a machine-readable provenance boundary for future E14 performance evidence. It verifies the existing schema-v2 release manifest and exact `AstralGame.exe` bytes, then binds them to:

- a 40-hex candidate revision and Release/RelWithDebInfo build label;
- a named 2D or 3D workload and fixture description;
- resolution, window mode, VSync state, warmup duration, and sample duration;
- OS, CPU, logical CPU count, RAM, GPU, and GPU-driver labels;
- frozen Unreal Engine and Unity reference versions;
- retained evidence files, sizes, roles, and SHA-256 hashes;
- a canonical descriptor SHA-256 covering candidate, protocol, environment, reference versions, provenance class, and evidence metadata.

The tool rejects traversal, absolute or drive-qualified evidence paths, backslashes, Windows case-collisions, evidence-root/intermediate/final symlinks, package/evidence mutation, malformed protocol/environment values, malformed hashes/counts, oversize inputs, and attempts to promote a verified manifest into a performance/parity/clean-machine/independent acceptance claim. A self-review found and closed a Python truthiness/type-laundering case: integer `0` cannot stand in for an explicit false acceptance field, booleans cannot stand in for integer counts, and evidence byte counts must be real non-negative integers.

This is provenance infrastructure only. It does not collect frame timings, CPU/GPU timelines, RAM/VRAM budgets, or matched UE/Unity reference measurements.

## Primary-source research

Sources were rechecked on 2026-09-20 America/New_York / 2026-09-21 UTC:

1. Epic Games, Unreal Engine 5.8, **Timing Insights**: https://dev.epicgames.com/documentation/unreal-engine/timing-insights-in-unreal-engine
   - UE exposes frame-by-frame performance data and separate CPU/GPU timing tracks. Applicability: Astral's future measurements need an exact run/candidate provenance envelope before timing values are accepted for comparison.
2. Epic Games, Unreal Engine 5.8, **Using the Timing Panel in Unreal Insights**: https://dev.epicgames.com/documentation/unreal-engine/using-the-timing-panel-in-unreal-insights-for-unreal-engine
   - UE analyzes selected time ranges on CPU/GPU timelines. Applicability: Astral's protocol records explicit warmup and sampled intervals, but this packet does not implement a UE-style event timeline.
3. Unity Technologies, Unity 6.0 / 6000.0, **FrameTimingManager**: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/FrameTimingManager.html
   - Unity captures/accesses per-frame timing data. Applicability: Astral still needs a native timing collector; this packet only guarantees exact provenance for the evidence once collected.
4. Unity Technologies, Unity 6.0 / 6000.0, **Performance testing API**, package 3.2.0: https://docs.unity3d.com/6000.0/Documentation/Manual/com.unity.test-framework.performance.html
   - Unity's performance package collects configuration metadata in addition to performance results. Applicability: Astral's new benchmark descriptor serves the configuration/provenance side only.

No Epic or Unity source code was copied. No external SDK, dependency, plugin, or redistributed binary was imported. The implementation uses Python standard-library facilities and the repository's existing `release_manifest.py` interface.

## Portable sandbox check

The connected repository was not mounted into the disposable Linux sandbox, so the portable check used a clearly labeled compatibility fixture implementing the already-published `release_manifest.py` surface. This is limited contract evidence, not production-repository or Windows evidence.

Commands:

```text
python -m py_compile Scripts/benchmark_manifest.py Scripts/test_benchmark_manifest.py
python Scripts/test_benchmark_manifest.py
```

Result after hardening: **12/12 tests passed**. The fixture exercised round-trip binding, package mutation, evidence mutation, commit mismatch, malformed protocols/environment, traversal/case collisions, final and intermediate symlink rejection, claim/descriptor tampering, release-manifest binding tampering, boolean/integer type laundering, and the CLI round trip.

The local fixture files and the GitHub-published files were not byte-identical representations after connector serialization/formatting, so this receipt does **not** claim a local-file SHA match to the published blobs. Hosted Windows tested the actual repository sources and is the authoritative implementation check for this packet.

## Hosted package/provenance lane

GitHub Actions workflow: `Release manifest integrity`

- Run: `35551113747`
- Job: `106185934632`
- Conclusion: **success**
- Runner: GitHub runner `2.337.0`, Windows Server 2022 Datacenter `10.0.20348`, `windows-2022` image `20260913.307.1`.
- Checkout: synthetic PR merge revision `9f2f31c134d6d527f045bcbd8afeaff4a2369216`, whose PR head was `536c213fd1af85ebf7acc6afb818805222d26497`.
- Compiler: MSVC `19.44.35228.0`, Visual C++ toolset `14.44.35207`, Windows SDK `10.0.26100.0`.

Regression results on the published repository sources:

- release manifest: **7/7 passed**;
- package runtime-smoke receipt contracts: **11/11 passed**;
- package restart-stress receipt contracts: **11/11 passed**;
- package continuous-soak receipt contracts: **11/11 passed**, including the Windows process-sampler test;
- package soak-analysis contracts: **11/11 passed**;
- new E14 benchmark-manifest contracts: **12/12 passed**;
- real Release `AstralGame.exe` build: **passed**;
- exact staged package release-manifest verification: **passed**;
- hosted benchmark contract fixture creation and reverification: **passed**;
- tracked-tree cleanliness check: **passed**.

Exact staged-package evidence for that job:

- `AstralGame.exe`: 69,632 bytes;
- `AstralGame.exe` SHA-256: `48e47c21fa7825523f44dc93b5ca084d9cc8889f8be3d2619f90ce72edcdb60b`;
- schema-v2 release entries SHA-256: `6ce1eaed15c69cd8615b776aa5663687ee0dad9d6c7749fb20974a5fb3f312d5`;
- release-manifest file SHA-256 bound by the benchmark descriptor: `d06c0c23664bd12507ef54d953484f84c3017854774d0e7ba794b4973cd76a7c`.

The hosted benchmark contract fixture was deliberately labeled `hosted_contract_fixture`, not native performance evidence. It recorded a 1920x1080 windowed, VSync-off, one-second schema fixture with zero warmup, Unreal `5.8`, Unity `6000.0`, Windows Server 2022, Intel Xeon Platinum 8370C, 4 logical CPUs, 17,174,360,064 bytes of RAM, and the hosted `Microsoft Hyper-V Video` adapter. No rendered benchmark or GPU measurement occurred.

Fixture evidence hashes:

- `contract-frame-timings.json`: 50 bytes, SHA-256 `63ca5782f07fb9431e0a3d62d65d4927423b307bef24a0024498aaedde5ea08e`;
- `contract-machine.txt`: 266 bytes, SHA-256 `6336af1d76a540ddeda379b2b256d0652cd8e881fd54934425713256a36f4c67`;
- resulting benchmark descriptor SHA-256: `76132c2d23382ee521024d7b00fdfb77bdbfdb988f36c68ba1fd34dfb5f7d83f`.

The verifier retained all claim guards as false: `performance_budget_verified`, `comparative_parity_verified`, `clean_machine_compatibility_verified`, and `independent_acceptance`.

## Separate full Windows regression lane

GitHub Actions workflow: `Windows build and deterministic tests`

- Run: `35551113776`
- Job: `106185931141`
- Conclusion: **success**
- Same PR head / synthetic merge checkout as above.

Verified results:

- R0 runner safety contracts: **13/13 passed**. These tests do not invoke R0 itself.
- PE dependency inspector: **5/5 passed**.
- Windows prerequisite planner: **8/8 passed**.
- Windows runtime environment probe: **8/8 passed**.
- Windows runtime compatibility audit: **7/7 passed**.
- Visual C++ Redistributable bootstrap tests: **8 passed, 1 expected skip** for the off-Windows refusal case.
- Release assertion/CTest safety: **3/3 passed**.
- Debug deterministic native tests excluding GUI RuntimeSmoke: **8/8 passed**.
- Release deterministic native tests excluding GUI RuntimeSmoke: **8/8 passed**.
- M1/M2/M3 static milestone verifiers: **passed**.
- tracked-tree cleanliness check: **passed**.

The hosted VC runtime inventory remained newer than the 14.44 build-tool floor, but installer provenance, package launch, and clean-machine compatibility remain unverified.

## New reproducibility finding

The two independent hosted jobs compiled the **same synthetic merge revision** with the same reported MSVC compiler/toolset family, but they produced different 69,632-byte `AstralGame.exe` SHA-256 values:

- package/provenance lane: `48e47c21fa7825523f44dc93b5ca084d9cc8889f8be3d2619f90ce72edcdb60b`;
- full Windows lane: `884a7a78d7a364f06a59966fcea9899029c06dfbaec08088f035cccb1c73d3dc`.

This does **not** invalidate the new benchmark manifest because the manifest deliberately binds each result to the exact executable bytes actually measured. It does mean Astral has **not** established reproducible byte-identical Release builds across independent hosted jobs. The cause was not diagnosed in this packet because the executable artifacts were not retained for binary comparison and the bounded packet did not authorize build-system changes. Possible sources such as PE/COFF timestamps, linker-produced debug/reproducibility records, environment-dependent metadata, or other link inputs must be measured rather than guessed.

Until investigated, source revision identity must not be treated as a substitute for exact package SHA-256 identity in performance or acceptance evidence.

## Acceptance state and limitations

Passed in this packet:

- benchmark provenance schema/validation behavior;
- exact release-package binding;
- evidence-file hash binding;
- hosted contract-fixture binding to an actual compiled package;
- existing hosted Debug/Release deterministic regression gates.

Still unresolved and **not claimed**:

- native package RuntimeSmoke on an exclusively owned interactive Windows desktop;
- a supported clean-Windows finished-package launch;
- real per-frame CPU/GPU timings and p50/p95/p99 results;
- RAM and VRAM budgets on approved hardware/settings;
- matched versioned 2D and 3D Astral/UE5/Unity workloads;
- allocator-level profiling where growth requires explanation;
- the required continuous 24-hour soak;
- build reproducibility across independent builds;
- independent review/acceptance;
- completion of the remaining engine feature catalogue.

R0 was **not invoked**. Nothing was merged, released, deployed, installed, or published.

## Single next useful action

Run a new bounded **Release build reproducibility diagnostic** before treating source revision as a portable binary identity: compile the same admitted revision twice on one controlled Windows runner with the same compiler/toolset/options, retain both executables and link metadata, compare hashes and PE/COFF/debug-directory fields, and identify the exact changing bytes/inputs. If a deterministic-build configuration change is required, it must be separately bounded and must not weaken the exact-package SHA-256 binding added here.

After that diagnostic, the registered local Windows executor can use this benchmark-manifest tool to bind actual native frame-time/RAM/VRAM/soak evidence to the exact package, workload, machine, driver, and reference-engine protocol. Those local measurements remain separate acceptance work.
