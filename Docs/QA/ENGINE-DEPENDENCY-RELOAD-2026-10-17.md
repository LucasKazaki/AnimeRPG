# Dependent reload and diagnostics QA — October 17, 2026

Owner: `/root/candidate_review`; branch `engine/dependency-reload-20261017`,
worktree `work/astral-reload`, base `92e4880489d2478f2fa45fbb690fabd8caca63dd`.
Scope and authority: `Tasks/ENGINE-DEPENDENCY-RELOAD-2026-10-17.md` and Lucas's
October 17 engine-only assignment, C/D. This record distinguishes source
original failures, executed focused checks and remaining integration gates.

## Original-source observations

- The original AssetManager published failure diagnostics while retaining ready
  data and successful Version; GameHost checked only the scene-JSON Version.
  Syntax failures therefore never reached host LastError/console through polling.
- File prefab/model/texture/audio content is copied into SceneDocument; changing
  an asset cache slot does not rebuild the live document/world. The host's watch
  originally included neither those slots nor glTF external buffers/images.
- PollChanges ignored missing stamps; deletion and recovery lacked a complete
  dependency-state contract.
- Original world/resource owners remain alive while a candidate parses, which
  supports transactional activation without rolling back the entire asset cache.
- The coordinator reports exact-base Debug full build plus deterministic 31/31
  passing in 55.99 seconds. This owner has not run or independently repeated it.
  Combined Release baseline is pending at packet preparation.

## Original-failure regressions

`HotReloadUpdatesReferencedPrefab` uses only a disposable scene and file prefab.
It requires the cache to advance the prefab to version 2, the scene JSON version
to stay unchanged, and the live entity/name/transform to update exactly once.

`HotReloadReportsMalformedSceneJson` uses a disposable scene. It requires the
cached last-good JSON/version and WORLD to survive incomplete syntax, one host
error/console line with no repeated-poll spam, and recovery clearing the error.

Both ran with unchanged exact-base production on DESKTOP-AKPLL81:

| Command | UTC October 17 | Exit/result |
| --- | --- | --- |
| VS CMake configure owned source/external `work/build-reload` | 18:15:08–18:15:11 | 0 |
| Debug build, target EngineSampleTests, parallel 2 | 18:15:19–18:15:48 | 0 |
| EngineSampleTests HotReloadUpdatesReferencedPrefab | 18:16:12 | 3; live PrefabAfter absent after cache version 2 |
| EngineSampleTests HotReloadReportsMalformedSceneJson | 18:16:12 | 3; LastError still empty after malformed JSON |

The wrapper verified `git diff --name-only 92e4880 -- Engine` was empty before
both tests. Failure locations at reproduction were EngineSampleTests.cpp:477
and :499. Stdout/stderr and JSON receipts are retained outside source under
`work/reload-evidence/old-*`. No surviving owned EngineSampleTests process was
observed; the compiler/test slot was released immediately. These are expected
old-defect failures, not passing acceptance gates.

## Implemented and focused-verified behavior

The repair adds completed attempts, dependency observations,
structured image error reporting, staged async metadata, missing-file recovery
and reached-slot scene collectors/strict reload validation. Default custom-loader
signatures and initial glTF warning compatibility remain. Dependencies are
observed before their contained read, so a repair during an import is not stamped
as though it had already been attempted. Failed parses leave the caller's
SceneDocument untouched while returning reached watches separately.
This is not an atomic filesystem snapshot: concurrent mutations can require a
future poll/retry, and edits preserving existence, size, target and timestamp are
outside this bounded watcher contract.

The host now watches successful document dependencies plus the latest failed
candidate's reached slots. It rejects current required-asset errors, reports once
per changed relevant attempt, activates only a fully parsed candidate, and clears
LastError after successful recovery. Failed opens of a different scene do not
schedule an automatic switch. Unrelated cached failures do not block a candidate.
Successful JSON syntax loads can advance the JSON asset Version even if scene
schema validation fails; the accepted document/WORLD and scene-load counters
remain unchanged. Import/decode failures preserve their asset Version as well.

Standalone glTF/initial managed loads preserve documented image warnings.
Managed reimports with image read/decode failures retain their prior data/version;
strict scene candidates also reject structured image errors in newly referenced
models. Unsupported points/lines warnings stay nonfatal and are not claimed as
supported rendering features. Existing custom three-argument loaders retain their
policy; automatic external-file discovery belongs to the built-in glTF loader.

| Disposable fixture | Actual consumer and regression |
| --- | --- |
| File and nested prefab | Live names/transforms update; schema errors/deletion preserve WORLD; repair clears error; no poll spam |
| Direct PNG texture | MeshRenderer material receives decoded replacement; old material retains owned old texture; deletion recovers |
| GLB model | Live imported mesh extent changes; malformed source retains world/model Version; source-only repair recovers |
| External glTF buffer | Live mesh geometry changes; buffer deletion retains world/model Version and dedupes; buffer-only repair recovers |
| External glTF image | Live material texture changes; corrupt replacement retains WORLD/version; newly missing image in failed NEW model recovers by creating only that image, even after Collect |
| WAV | Live AudioSource clip and software-mixer voice ownership update; invalid WAV preserves good world/clip; WAV-only repair recovers |
| Scene JSON | Incomplete syntax and valid JSON/invalid schema preserve WORLD; deleted scene recovers; LastError/console report once and clear on recovery |
| New denied dependency links | Actual Windows junctions initially point outside content; direct-child and valid ../shared URI variants both preserve WORLD/error, then recover by retargeting only the same owned link inside |
| Asset/Scene boundary | Async attempts publish on owner thread; custom-loader signature/policy; approved missing paths/outside refusal; partial failed collectors retain cache identity |

## Ownership and executed checks

Non-host Assets/Scene and reserved test files were implemented only after the
coordinator accepted both old exit-3 receipts. Before B host ownership cleared,
focused Debug/Release Assets+Framework builds passed and CTest passed 2/2 each
(1.42/0.34 seconds; 20 asset and 14 framework cases per configuration).

After B's final independent review, exact commits
`74eed63f3a684e207653499963317b93fce6a4f3` and
`797782e4bb96485ab3ae0b35e4da7bbf54ad0a7c` were cherry-picked as
`4f3f954` and `75d0a6dac6bc8d617b2de0c7427bed30ac20d74d`. The committed tree
at 75d0a6d matches reviewed B head 797782e (git diff exit 0). Reserved non-host
patch SHA256 before/after B adoption was identical:
`1821247DAA14E2E2D087F69B2A8969F7FFD172085AB10BCEDE66C42DEB5B7504`.
Host wiring began only after explicit ownership transfer, with no build running.
B's input-routing block was compared unchanged after normalized line endings.

The first wired production/test source was frozen for both configuration gates.
Source identity: 75d0a6d plus `work/reload-evidence/wired-source.patch`, SHA256
`1BAC353530863E375B2A9849F7EBCFE6964A258E17612AFA940D3DD06E61FCC4`.
That preliminary candidate was superseded by the reviewed fixes below.

| Command (full paths in task packet) | Exit/result |
| --- | --- |
| Build Debug EngineAssetsTests EngineFrameworkTests EngineSampleTests EngineUITests, parallel 2 | 0; 18:31:48–18:31:59 UTC |
| CTest Debug four named suites, verbose, stop on failure | 0; 4/4 suites, 3.39 seconds |
| Build Release same four targets, parallel 2 | 0; 18:32:26–18:32:36 UTC |
| CTest Release same four suites | 0; 4/4 suites, 0.64 seconds |
| git diff --check | 0 |

Each preliminary configuration ran 73 cases: Assets 20, Framework 14, UI 19,
Sample 20. Both original failure regressions passed. Source stayed unchanged
throughout each build/run. Existing test
registration/assertions were retained; Release's /UNDEBUG-over-/DNDEBUG warning
is the existing assertion-preservation mechanism, not disabled test checks.
Machine/toolchain: DESKTOP-AKPLL81, Windows 11, Ryzen 9 7950X3D, VS2022 Community,
MSVC 19.44, SDK 10.0.26100, x64; external multi-config build root work/build-reload.
Complete stdout/stderr, exits, timestamps, source/patch identities and B adoption
receipt remain outside source in work/reload-evidence.

## Independent-review recovery fixes and final freeze

Source review found that a NEW failed candidate's outward dependency link was
rejected before a dependency observation was retained. The good manifest could
not help because it had never referenced that link. After a fresh focused build,
`HostReloadRecoversWhenNewDeniedDependencyLinkIsRetargetedInside` exited 3 at
18:37:19 UTC, EngineAssetsTests.cpp:1274, after the owned junction was retargeted
inside: LastError remained set. Earlier assertions verified preserved WORLD,
approved-root rejection and no poll spam. Receipt: denied-old-*; production was
still the preliminary 73-case tested freeze.

The first bounded child-URI fix passed Debug/Release 4/4 (3.26/0.62 seconds;
74 cases each) and safety 3/3 (4.906 seconds), with no skipped junction fixture.
Its source patch is denied-fixed-source.patch, SHA256
`70A8B64CE6DA8F600D596739A5FEA44D4C9E6E82806628BA2E7D6A31C87B9D91`.
Review then identified that a child-only guard missed E's legitimate parent URI
contract. Against that unchanged first-fix production,
`HostReloadRecoversWhenNewDeniedParentDependencyLinkIsRetargetedInside` exited 3
at 18:41:25 UTC, EngineAssetsTests.cpp:1278, for models/new.gltf referencing
`../shared/link/mesh.bin`. Only the owned shared/link junction was retargeted;
the scene, source model and bytes were unchanged. Receipts: denied-parent-old-*.

Final behavior independently approves the base, admits a normalized logical
candidate only by whole-root component containment, and retains the ORIGINAL
logical path so link/.. filesystem semantics are preserved. Absolute/scheme/NUL
URIs and normalized lexical escapes are not admitted. A denied observation keeps
no outside target path, size or timestamp; canonical resolution can consult
filesystem metadata while checking containment. Actual reads still require the
existing canonical approved-root validation. An intermediate successful Debug
compile and its patch are retained under parent-equivalent-prefix-*; source
inspection tightened that fallback to pure lexical component-prefix equality
before final tests, avoiding an outside alias being treated as a lexical child.

Final production/tests: 75d0a6d plus work/reload-evidence/final-source.patch,
SHA256 `70750DB24125023EA0044B64E213A32715DCDFB2518148007DA3A1903D4B6A2E`.
Both final builds/tests and safety used this unchanged freeze; subsequent edits
are task/QA documentation only.

| Final command | Exit/result |
| --- | --- |
| Debug build same four targets, parallel 2 | 0; 18:43:26–18:43:30 UTC |
| Debug CTest same four suites, verbose/stop-on-failure | 0; 4/4, 3.26 seconds |
| Release build same four targets, parallel 2 | 0; 18:43:51–18:44:05 UTC |
| Release CTest same four suites | 0; 4/4, 0.60 seconds |
| Bundled Python Scripts/test_test_safety.py, process PATH includes VS CMake/bin | 0; 3/3, 4.885 seconds |

Final configurations each ran 75 cases: Assets 22, Framework 14, UI 19, Sample
20. Both real-host denied-link junction regressions executed and passed without
skip; existing containment/lexical escape negatives and every prior check passed.
All expected old-failure receipts remain retained rather than overwritten as
successes. No unexpected repaired-matrix failure occurred. No surviving owned
process under the build root was observed; the compiler/test slot was released
immediately after final safety so the next owner could run. Logs/receipts:
work/reload-evidence/{final-*,denied-*,parent-equivalent-prefix-*}, alongside the
earlier original/non-host/wired/B adoption evidence.

## Pending gates

Substantive independent review of the actual final diff/commit and coordinator
full integration/native gates remain pending. This packet is focused-locally
verified, not main adoption or complete development-workflow acceptance. No
native-window/GPU execution, installation, driver change, historical R0 run,
performance/soak claim, push or merge was performed by this packet owner.

Concurrent filesystem mutations are not atomic snapshots, and equal target/
existence/size/timestamp edits can be missed. Custom loader external dependencies
are not inferred. Expanded prefab maxEntities/maxDepth accounting remains the
coordinator's separately reserved follow-up; this packet preserves existing
limits and does not claim that independent finding is fixed.
