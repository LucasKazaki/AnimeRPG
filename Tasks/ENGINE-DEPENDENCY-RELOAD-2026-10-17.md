# Transactional dependent scene reload and diagnostics — October 17, 2026

Owner: `/root/candidate_review`, implementation role for this packet only.
Worktree: `C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/astral-reload`.
Branch: `engine/dependency-reload-20261017`. Deliberate base:
`92e4880489d2478f2fa45fbb690fabd8caca63dd` (PR80, reviewed containment,
merged numeric repair and reviewed finite input). Lucas's October 17 pasted
engine-only assignment, items C/D, authorizes this bounded repair and supersedes
older local-only/per-change approval wording. Technical gates remain required.

## Problem and starting gates

The host watches only the successful version of scene JSON. A referenced file
prefab can reload in the cache without replacing its live instance. A malformed
scene edit preserves the cached JSON and world but does not increment that
version, so the host records no LastError/console diagnostic. Existing polling
also skips missing files, and external glTF buffer/image reads are not watched.

Coordinator reported the exact assembled base full Debug build and 31/31
deterministic suites passing (55.99 seconds). Release baseline is still pending.
Contract/task and baseline-compatible regressions were prepared first. The
coordinator accepted both original exit-3 regressions on unchanged production
and cleared non-host production implementation after the focused slot released.
No build or native execution runs concurrently with another owner's checks.

## Allowed paths and ownership

- `Engine/Assets/{AssetManager,Gltf}.{h,cpp}`
- `Engine/Framework/{Scene,GameHost}.{h,cpp}`
- `Tests/Engine{Assets,Framework,Sample}Tests.cpp`
- This packet and `Docs/QA/ENGINE-DEPENDENCY-RELOAD-2026-10-17.md`

The UI packet B owner exclusively owned `GameHost.{h,cpp}` until its reviewed
freeze and coordinator-cleared integration. Ownership transferred after exact
B commits 74eed63/797782e were independently reviewed; they were adopted as
4f3f954/75d0a6d only after the non-host build slot released. B uses EngineUITests; the three test files
above and Assets/Gltf/Scene are reserved to this owner. Preserve all other
worktrees, the CI branch and dirty local editor. No UI/InputSystem/Math/native,
game-content, CMake, policy or dependency changes. No GPU implementation.

## Implemented contract

1. Retain successful `Version`; add a completed load `Attempt` that advances on
   failure too. Last-good cached data/version survive a failed reimport.
2. Poll source/dependency observations, including existence transitions. Keep
   successful and failed-attempt file dependencies so repairing only a newly
   missing referenced file retries the candidate. No whole-content-root scan.
3. Built-in glTF import reports contained dependency paths and image failures
   into a per-call result, even on failure. Worker loads stage their metadata;
   only owner-thread publication changes slots. Existing three-argument custom
   loaders remain supported without fabricated external-dependency discovery.
   An optional importing-thread observer records pre-read file observations,
   avoiding a missed repair while a failed async import is finishing.
4. Scene parsing gathers only reached typed asset slots, including cache hits,
   direct texture/audio/model loads and nested/file prefab material parses.
   Preserve partial attempted dependencies when parsing fails. Required assets
   with a current failed attempt must not silently supply stale good data to a
   new scene candidate.
5. The host watches the union of the last-good scene and latest failed candidate
   dependencies, retries on relevant attempt changes, and activates only a fully
   valid candidate. Failure leaves the live WORLD intact, emits one diagnostic
   per relevant attempt, and recovery clears the active reload error. Unrelated
   cached errors do not block the scene.
6. Standalone glTF import and first managed load retain their documented nonfatal
   image/unsupported-mode warnings. A managed reimport with an image read/decode
   failure must preserve its previous model/version. Unsupported points/lines
   warnings remain nonfatal; this is not new support for those features.
   Strict scene reload also rejects structured image errors in newly referenced
   models; retaining first-load warnings cannot admit a degraded new candidate.

Containment remains decoded/canonical and approved-root constrained. A missing
dependency can be watched only after proving its existing ancestor/resolved
in-root path is approved. Watching it must never authorize an outside-root read.
New denied links retain only bounded logical paths: approve the base, require
normalized whole-root component containment (including legitimate ../shared),
then retain the original path for link/.. semantics. Outside targets are not
watched and provide no size/time observations; actual reads remain canonical-
approved. Canonical containment resolution can consult filesystem metadata.
Timestamps/existence/size are incremental observations, not a content-hash watcher
or an adversarial atomic-filesystem guarantee.

## Acceptance and bounded stop

- Retain independent old-source failures for prefab-only edits and malformed
  scene JSON diagnostics through GameHost before production changes.
- Original disposable engine fixtures verify model, direct texture, external
  buffer/image, file/nested prefab and WAV updates through live scene consumers.
- Cover syntax and schema errors, deletion/missing then repair, a newly missing
  failed-candidate dependency repaired alone, no diagnostic poll spam, unchanged
  successful versions on failure, recovery clearing, and unrelated cached errors.
- Execute actual owned link/junction cases for NEW denied dependencies through
  GameHost, including ../shared references; repair only the link and recover
  without admitting lexical escapes or reading outside bytes/size/timestamps.
- Verify async publication and custom-loader compatibility in asset tests;
  preserve every existing assertion and the test-safety registration mechanism.
- Focused Debug/Release suites, substantive independent diff/commit review, then
  coordinator full integration/native gates. Stop at a deterministic failure;
  retain its receipt and fix it before accepting the packet. No push or merge by
  this owner, and no workflow/GPU/editor/package/soak completion claim.

## Exact command plan

Only after a coordinator grants the serialized compile/test slot:

```powershell
$source = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/astral-reload'
$build = 'C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/build-reload'
$cmake = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
$ctest = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/ctest.exe'
& $cmake -S $source -B $build -G 'Visual Studio 17 2022' -A x64
& $cmake --build $build --config Debug --target EngineAssetsTests EngineFrameworkTests EngineSampleTests EngineUITests --parallel 2
& "$build/Debug/EngineSampleTests.exe" HotReloadUpdatesReferencedPrefab
& "$build/Debug/EngineSampleTests.exe" HotReloadReportsMalformedSceneJson
& "$build/Debug/EngineAssetsTests.exe" HostReloadRecoversWhenNewDeniedDependencyLinkIsRetargetedInside
& "$build/Debug/EngineAssetsTests.exe" HostReloadRecoversWhenNewDeniedParentDependencyLinkIsRetargetedInside
& $ctest --test-dir $build -C Debug --output-on-failure -V -R '^Engine(Assets|Framework|Sample|UI)Tests$' --no-tests=error --stop-on-failure
& $cmake --build $build --config Release --target EngineAssetsTests EngineFrameworkTests EngineSampleTests EngineUITests --parallel 2
& $ctest --test-dir $build -C Release --output-on-failure -V -R '^Engine(Assets|Framework|Sample|UI)Tests$' --no-tests=error --stop-on-failure
$python = 'C:/Users/lucas/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$env:PATH = (Split-Path $cmake) + ';' + $env:PATH
& $python "$source/Scripts/test_test_safety.py"
git diff --check
git status --short
```

Configure/build/test stdout, stderr, exits, source identity and machine/toolchain
receipts stay outside source in `work/reload-evidence`. Baseline targeted runs
are expected to fail; they are evidence, never presented as a passing gate.

Primary references read October 17:
[C++ filesystem time operations](https://eel.is/c++draft/fs.op.last.write.time),
[Microsoft canonical/weakly_canonical contracts](https://learn.microsoft.com/en-us/cpp/standard-library/filesystem-functions?view=msvc-170),
[Khronos glTF URI specification](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#uris).

Risks: changed files during reads; coarse or deliberately preserved timestamps;
cycles/nested prefab limits; failed candidate dependencies surviving collection;
distinguishing documented nonfatal skips from corrupt replacement images; and
preserving B's input ownership when the host edit reservation clears.
