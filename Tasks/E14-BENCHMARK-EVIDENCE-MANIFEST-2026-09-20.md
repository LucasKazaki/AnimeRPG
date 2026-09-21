# E14 benchmark evidence manifest packet, 2026-09-20

Status: bounded implementation packet for draft PR #9. This packet advances one E14 profiling/budget prerequisite while native package launch and the 24-hour soak remain local acceptance work.

## Dependency and baseline

- Repository: `LucasKazaki/AnimeRPG`.
- Candidate branch at admission: `repair/2026-09-20-r0-runner-safety`.
- Exact baseline revision: `f15ede2fadb56012e5ccf22dd1f6a776ec2a4fe9`.
- Dependency: the schema-v2 package manifest and exact-package verification already present on PR #9.
- Capability-map source: `Docs/Research/ENGINE-CAPABILITIES-2026-09-20.md` on draft PR #8. Its E14 contract requires reproducible benchmark manifests in addition to CPU/GPU timing and RAM/VRAM evidence. This packet implements only the manifest/provenance subrequirement; it does not modify PR #8 or claim E14 completion.

## Reproducible gap

Astral can now bind package bytes, runtime-smoke receipts, restart stress and soak telemetry, but there is no strict record that binds a performance run to all of the following at once: exact package revision and executable hash, workload identity/dimension, display protocol, warmup/sample duration, hardware/driver labels, frozen UE/Unity reference versions and retained evidence-file hashes. Without that binding, later p50/p95/p99, RAM or VRAM measurements can be accidentally compared across different packages, fixtures or machine configurations.

## Research basis

Primary sources accessed 2026-09-20:

- Unreal Engine 5.8 Timing Insights: https://dev.epicgames.com/documentation/unreal-engine/timing-insights-in-unreal-engine
  - Applicability: Unreal records frame-by-frame performance and separate CPU/GPU timing tracks. Astral needs equally explicit run provenance before comparative timing results are treated as evidence.
- Unreal Engine 5.8 Timing Panel: https://dev.epicgames.com/documentation/unreal-engine/using-the-timing-panel-in-unreal-insights-for-unreal-engine
  - Applicability: benchmark evidence must identify the exact time/workload interval being analyzed; this packet records warmup and sample windows but does not implement a UE-style timeline.
- Unity 6.0 `FrameTimingManager`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/FrameTimingManager.html
  - Applicability: Unity exposes captured frame timing data; this packet only binds future Astral timing evidence to an exact candidate and protocol.
- Unity 6.0 performance testing API, package 3.2.0: https://docs.unity3d.com/6000.0/Documentation/Manual/com.unity.test-framework.performance.html
  - Applicability: Unity's performance testing package collects configuration metadata. Astral needs a machine-readable equivalent for candidate/workload/environment provenance.

No Epic or Unity source code is copied. No dependency, SDK or plugin is added. The implementation uses Python standard-library facilities and the repository's existing `release_manifest.py` API.

## Allowed paths

Only these paths may change in this packet:

- `Scripts/benchmark_manifest.py`
- `Scripts/test_benchmark_manifest.py`
- `.github/workflows/release-manifest-validation.yml`
- `Tasks/E14-BENCHMARK-EVIDENCE-MANIFEST-2026-09-20.md`
- `Docs/QA/E14-BENCHMARK-EVIDENCE-MANIFEST-2026-09-20.md`

No `Engine/`, `Game/`, `Tests/`, CMake, dependency, graphics API, local-runtime or content changes are authorized.

## Required behavior

The new manifest tool must:

1. Verify the existing release manifest and exact `AstralGame.exe` bytes before creating benchmark evidence.
2. Require a 40-hex candidate revision and Release/RelWithDebInfo build label.
3. Bind a named 2D or 3D workload, resolution, window mode, VSync state, warmup duration and sample duration.
4. Bind OS, CPU, logical CPU count, RAM, GPU and GPU-driver labels plus frozen Unreal/Unity reference versions.
5. Hash every retained evidence file under an explicit evidence root; reject traversal, absolute/drive-qualified paths, symlinks and Windows case-collisions.
6. Bound spec/manifest/evidence sizes and file count.
7. Reject changed package bytes, changed evidence bytes, malformed/invalid protocols and any attempt to promote manifest verification into performance/parity/clean-machine/independent acceptance.
8. Emit a canonical descriptor SHA-256 over the bound candidate, protocol, environment and evidence metadata.

## Verification plan

Portable sandbox, using a clearly labeled compatibility fixture for the already-published `release_manifest.py` API because the connected repository file is not mounted into the sandbox:

```text
python -m py_compile Scripts/benchmark_manifest.py Scripts/test_benchmark_manifest.py
python Scripts/test_benchmark_manifest.py
```

Hosted Windows must then run the same tests against the repository's real `release_manifest.py`. The release-manifest workflow must also build the real Release `AstralGame.exe`, create/verify its release manifest, create a hosted-contract benchmark fixture bound to those exact bytes, and verify it again. Hosted fields must be labeled `hosted_contract_fixture`; they are not native performance data.

## External outputs

Build/package/benchmark fixtures must stay under the GitHub runner's temporary directory. No generated package, benchmark JSON or test output is committed as a production result.

## Rollback and stop conditions

- Stop at the first deterministic source/test/integration failure and retain the failing command/result.
- Do not weaken schema validation or acceptance guards to turn CI green.
- If PR #9 head moves before a write, re-read the changed files and reconcile before continuing.
- Rollback is removal/reversion of only the five allowed paths above.
- Do not invoke R0, merge, release, deploy, install prerequisites, or claim local Windows/GPU execution from this packet.

## Local executor handoff after hosted verification

For a real benchmark, the registered Windows executor should use the exact admitted package, fill the protocol/environment fields from retained machine receipts, hash the original runtime/soak/frame-time evidence files, and preserve the generated benchmark manifest with the command, timestamps and toolchain/driver identity. The 24-hour soak, real CPU/GPU frame timings, VRAM measurements, 2D/3D matched workloads and independent review remain separate gates.
