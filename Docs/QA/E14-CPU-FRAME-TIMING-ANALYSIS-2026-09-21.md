# E14 CPU frame-timing analysis verification, 2026-09-21

Status: bounded implementation checkpoint on draft PR #9. This record does not
claim native GUI benchmarking, an approved performance budget, GPU timing, matched
Unreal Engine/Unity parity, clean-machine acceptance, soak acceptance, or independent
review.

## Candidate and bounded diff

- Repository: `LucasKazaki/AnimeRPG`
- PR: #9, draft, unmerged
- Branch: `repair/2026-09-20-r0-runner-safety`
- Pre-packet head: `77721fd0947072052d890f27c7a96a00fb7c7c23`
- Implementation commit: `2a44ba88cb8541717327e175021a1ba2bc5c2e9b`
- First QA checkpoint commit: `d3dc2c828e0ed20581f6c30c7ad6d19acc58903f`
- Dependency/base revision retained by the stacked PR: `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`

Implementation commit changed only the packet-authorized paths:

- `Scripts/analyze_frame_timing_capture.py`
- `Scripts/test_frame_timing_analysis.py`
- `.github/workflows/frame-timing-validation.yml`
- `Tasks/E14-CPU-FRAME-TIMING-ANALYSIS-2026-09-21.md`

This QA file is the fifth authorized path. No Engine/Game/CMake source, dependency,
graphics API, content, Company Runtime state, release, deployment, or repository
permission was changed. R0 was not invoked.

Published Git blob identities verified after the write:

- analyzer: `0e78ba263cecdb9a58970484310efa4a3a14cc6e`
- analyzer test: `27edced4e1aef321cf0f5666f834ca55885a7401`
- workflow: `f7238b5ed52633a49f263c7edf2d62477e21335f`
- task contract: `955ba9fad1da716087ab888d27e708b700745ee0`

The analyzer blob and workflow/task blobs were re-read from GitHub after publication.
The analyzer blob exactly matches the source exercised in the disposable sandbox.
The GitHub test blob is the repository source of truth and was subsequently executed
by hosted CI through the PR synthetic merge described below.

## What was implemented

`Scripts/analyze_frame_timing_capture.py` is a fail-closed descriptive analyzer for
the schema emitted by the existing production `FrameTimingCapture`. A bound analysis:

1. invokes the production `benchmark_manifest.verify_benchmark_manifest(...)` path;
2. requires the caller to state the expected 40-hex candidate revision and the exact
   64-hex `AstralGame.exe` SHA-256;
3. requires exactly one benchmark evidence record with role
   `cpu_frame_timing_csv`;
4. re-resolves that evidence beneath an explicit non-symlink root and rechecks its
   recorded size and SHA-256;
5. accepts only the exact production timing schema, fixed CPU wall-clock metric and
   fixed `gpu_timing=unavailable` / `acceptance_claim=none` claim boundary;
6. rejects missing, duplicate or unknown metadata, unsafe paths, non-canonical frame
   values, non-finite/non-positive intervals, invalid warmup/max/saturation values,
   non-contiguous measured frame indices, empty captures and row-count violations;
7. calculates only sample count, first/last frame, interval sum, min, mean,
   p50/p95/p99 and max. Percentiles use deterministic linear interpolation at
   `position=(n-1)*p`;
8. carries the verified benchmark descriptor hash, exact package identity,
   workload/run protocol/provenance labels and raw CSV hash into the result; and
9. keeps performance-budget, comparative-parity, instrumentation-overhead,
   GPU-timing and independent-acceptance flags hard false. Optional JSON output is
   create-only and refuses to overwrite an existing receipt.

This is evidence plumbing, not a profiler-parity claim. The source sample is still
the wall-clock interval between consecutive `Clock::Tick()` calls and includes waits
and overhead.

## Primary-source comparison research

Accessed 2026-09-21:

- Epic Games, Unreal Engine 5.8, Timing Insights:
  https://dev.epicgames.com/documentation/unreal-engine/timing-insights-in-unreal-engine
  Epic documents per-frame performance data, CPU/GPU tracks, per-thread timelines,
  spike inspection and aggregate analysis over selected ranges. Applicability:
  Astral needs validated raw multi-frame evidence first, but this packet does not
  match Unreal's thread/GPU trace attribution. Documentation only; no proprietary
  Unreal source or assets were copied.
- Epic Games, Unreal Engine 5.8, Timing Panel:
  https://dev.epicgames.com/documentation/unreal-engine/using-the-timing-panel-in-unreal-insights-for-unreal-engine
  Epic documents separate CPU/GPU thread tracks and selected time ranges.
  Applicability: Astral's current interval stream is deliberately narrower.
- Unity Technologies, Unity 6.0, `FrameTimingManager` and `FrameTiming`:
  https://docs.unity3d.com/6000.0/Documentation/ScriptReference/FrameTimingManager.html
  https://docs.unity3d.com/6000.0/Documentation/ScriptReference/FrameTiming.html
  Unity documents multiple captured frame timings plus total CPU frame time,
  main/render-thread timing, Present wait information and GPU frame time.
  Applicability: Astral must not infer any of those finer measurements from its
  wall-clock interval stream.
- Unity Technologies, Unity 6.0 performance testing package 3.2.0:
  https://docs.unity3d.com/6000.0/Documentation/Manual/com.unity.test-framework.performance.html
  Applicability: benchmark results need configuration/provenance context instead of
  free-floating timing numbers.

No third-party dependency was added and no proprietary implementation was copied.

## Executed verification

### Disposable sandbox

The sandbox did not contain a full repository checkout and direct Internet access
from the compiler container was unavailable. It therefore exercised the exact
published analyzer source plus the packet's parser/analysis test fixture, but could
not import the repository production manifest modules there.

Commands:

```text
python3 -m py_compile analyze_frame_timing_capture.py test_frame_timing_analysis.py
python3 test_frame_timing_analysis.py
```

Result: exit 0. Twelve tests were discovered; nine parser/analysis tests passed and
three production-binding integration tests were explicitly skipped because
`benchmark_manifest.py` was absent from the partial sandbox. The executed coverage
includes known percentile values, malformed/duplicate/unknown metadata, claim
laundering, invalid numeric values, frame-index continuity, sample/saturation caps,
incomplete-but-descriptive captures, create-only output, and 250 deterministic
single-byte mutations that must either be rejected or revalidate all invariants.

The current GitHub production modules were then inspected directly. They expose the
public aliases used by the packet:

```text
build_benchmark_manifest = build_manifest
verify_benchmark_manifest = verify_manifest
```

and `release_manifest.py` exposes `write_release_manifest(...)`, so the integration
test targets the actual repository call surface rather than a mock.

### Hosted frame-timing portability

GitHub Actions run `35564646701`, job `106223963453`, completed successfully for
source head `2a44ba88cb8541717327e175021a1ba2bc5c2e9b`. As expected for a pull-request
workflow, `actions/checkout` executed synthetic merge revision
`e39b21782ba0a4d1ae8e876181d39f9b0181cafb`, which GitHub recorded as merging that
implementation head into base `e12e6c559bf776ffc9c715c809a517f8e02ce5d5`.

Runner evidence: Ubuntu 24.04.5 LTS, `ubuntu-24.04` image version
`20260907.300.1`, Azure `centralus`.

The exact hosted command:

```text
python Scripts/test_frame_timing_analysis.py
```

ran all 12 tests, including the three production-manifest integration cases, and
reported `Ran 12 tests ... OK`. The production binding, exact expected-candidate
identity and tampered-evidence rejection cases all passed.

The same job then configured the production-linked frame-timing fixture and passed:

- GNU 13.3.0 Debug: `FRAME TIMING CAPTURE TESTS: PASS (11 groups)`;
- GNU 13.3.0 optimized Release: `FRAME TIMING CAPTURE TESTS: PASS (11 groups)`;
- Clang 18.1.3 Debug with AddressSanitizer + UndefinedBehaviorSanitizer and
  `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1`: `PASS (11 groups)`.

All three CTest executions reported 100% passed and zero failed.

### Hosted Windows and package regressions

GitHub Actions run `35564646707`, job `106223963508`, also completed successfully
for source head `2a44ba88cb8541717327e175021a1ba2bc5c2e9b` and the same synthetic PR merge
revision. Runner: Windows Server 2022 build 20348, `windows-2022` image version
`20260913.307.1`, Azure `eastus`. Toolchain: MSVC `19.44.35228.0`, Visual C++ tools
`14.44.35207`, Windows SDK `10.0.26100.0`.

Observed regression results included:

- R0 runner safety: 13/13 pass. These are parser/safety contracts only; R0 itself was
  not invoked.
- PE dependency inspection: 5/5 pass.
- Windows prerequisite plan: 8/8 pass.
- Windows runtime-environment probe: 8/8 pass.
- Windows runtime-compatibility audit: 7/7 pass.
- VC Redistributable bootstrap: 8 pass, 1 expected platform-specific skip.
- assertion/CTest safety: 3/3 pass.
- native non-GUI Debug CTest: 9/9 pass, including `FrameTimingCaptureTests`.
- native non-GUI Release CTest: 9/9 pass, including `FrameTimingCaptureTests`.
- static milestone verifiers and clean tracked-tree check: pass.

The resulting hosted Release `AstralGame.exe` was 91,136 bytes, SHA-256
`89dd35915c4ea4c30e9050e8d70052521421e9f9708aa425ca1084b6b3d54de5`. The
hosted prerequisite evidence explicitly retained `package_launch_verified=false`,
`clean_machine_compatibility_verified=false`, and `independent_acceptance=false`.

GitHub Actions release-manifest run `35564646662`, job `106223963409`, also completed
successfully for the same implementation source head. Its manifest, package-smoke,
restart-stress, continuous-soak contract, soak-analysis, benchmark-manifest, PE
reproducibility, repeated Release build, exact package-verification and clean-tree
steps all concluded successfully. Runtime-smoke/soak contract tests in hosted CI are
not a substitute for the deferred native GUI launch, continuous 86,400-second local
soak, or independent acceptance.

## Claim boundaries and remaining gates

Passed in this packet:

- exact analyzer syntax and parser/analysis contracts in the disposable sandbox;
- 250 seeded malformed-input mutations in the disposable sandbox;
- published analyzer blob identity against GitHub;
- all 12 analyzer tests against the complete repository and production manifests in
  hosted Ubuntu CI;
- production-linked C++ Debug, optimized Release and Clang ASan+UBSan/leak capture
  contract suites in hosted Ubuntu CI;
- full hosted Windows build/deterministic regression lane for the exact
  implementation source head;
- hosted release-manifest/package contract regression lane for the exact
  implementation source head.

Deferred/not established:

- native Windows GUI timing capture and matched capture-off control;
- instrumentation overhead, GPU timestamps and CPU thread/render attribution;
- RAM/VRAM budgets and approved performance thresholds;
- matched Astral/UE5/Unity 3D and genuine-2D workloads;
- clean-machine package launch, load/unload and recovery stress;
- the continuous 86,400-second soak;
- independent QA/review.

No green workflow, document count or descriptive percentile may be used as proof of
those unresolved requirements.

## Registered local executor handoff

After the exact package passes the existing prerequisite and package-bound M10 smoke
gates on the registered Windows executor, run the already-defined frozen procedural
3D capture with warmup 120 and max 3,600 samples. Retain the raw CSV unchanged. Add
it to the verified benchmark manifest as role `cpu_frame_timing_csv`, then run:

```text
python Scripts/analyze_frame_timing_capture.py \
  <benchmark-manifest.json> <package-root> <release-manifest.json> <evidence-root> \
  --expected-commit <admitted-40-hex-revision> \
  --expected-executable-sha256 <independently-recorded-AstralGame.exe-SHA256> \
  --json <new-analysis-receipt.json>
```

Retain the command, stdout/stderr, exit code, UTC timestamps, raw/analysis hashes,
package/release/benchmark manifests, exact source revision, machine/OS/CPU/RAM/GPU/
driver identity, resolution/window/VSync settings and captured logs. Run a matched
capture-off control separately before claiming instrumentation overhead.

## Single next useful action

The complete-repository analyzer gate is now green. The next useful native packet is
the existing 3,600-sample package-bound procedural-scene capture plus a matched
capture-off control on the registered Windows executor. Bind the raw CSV to the
benchmark manifest and run this analyzer. Do not select or claim a parity threshold
until matched UE5/Unity reference workloads are frozen and measured.
