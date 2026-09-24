# GAME PASS 29: Field Guide and Map QoL

Date: 2026-09-24
Owner: separate AnimeRPG GAME worker
Base: `e38760070e0d021293e9f907834e61e6a6edca23`
Target: `main`

## Authority and boundary

This packet is GAME work only. It preserves the custom C++17 Astral Engine, the modern supernatural Washington DC / National Mall setting, the persistent protagonist, and the existing `LandmarkInteraction -> ExplorationFieldGuide` production ownership path. It does not edit renderer/platform/editor/import/animation/audio/physics infrastructure, shared CMake/CI, engine-worker PRs, or local Company Runtime state. Older engine-first blanket content pauses are superseded only for this explicitly authorized GAME worker.

Allowed paths for this packet:

- `Engine/Scene/ExplorationFieldGuide.h`
- `Tests/ExplorationFieldGuidePass29Tests.inc`
- `Tests/ThoughtCommandsTests.cpp`
- `Tasks/GAME-PASS29-FIELD-GUIDE-MAP-QOL-2026-09-24.md`
- `Docs/Agents/animerpg-hourly/PASS29-BACKLOG.json`
- `Docs/Agents/animerpg-hourly/RUN-2026-09-24-PASS29.md`
- `Docs/Agents/animerpg-hourly/STATE.json`

No shared build file or engine-owned path is admitted.

## Live gap

`ExplorationFieldGuide` is already the production game-domain owner used by `LandmarkInteraction`, Shadow Crypt gating, and Mana Reactor gating. Before this packet it supports three operations, discovery chronology, five journal entries, one manual pin, and one fixed Mall-survey recommendation. It does not expose operation-aware next-step hints for narrative investigations, a bounded multi-stop player route, deterministic next-operation tracking, unread-journal notices, a consolidated briefing, or visibility filters for player route pins.

## Research and adaptations

Access date for web research: 2026-09-24.

Primary comparator sources:

- HoYoverse's official Genshin Impact post, `"Teyvat Interactive Map" Version 2.6 Update`, published 2022-03-30, documents quick area location, adding/editing personal pins, a filter list, viewing the player's own pins, synced in-game pins, and viewing pin distributions. Source: https://www.hoyolab.com/article/4029408
- HoYoverse's official Genshin Impact Version `Luna III` update details, published 2025-12-02, documents the Adventurer Handbook tracking up to three nearby objective locations simultaneously. This is later official evidence that multi-target tracking remains part of Genshin's current-era navigation design. Source: https://www.hoyolab.com/article_pre/21389
- HoYoverse's official `Teyvat Interactive Map Usage Guide`, retained on HoYoLAB, explains syncing in-game pins with the Interactive Map and recommends pinning discoveries to improve exploration efficiency. Source: https://www.hoyolab.com/article/17673509
- Secondary Version 5.4/5.6 changelog mirrors were inspected only to revalidate later map-QoL evolution such as category-scoped custom-pin management and relevant-only map sidebars. They are corroborating context, not the primary authority for implementation.
- The project adapts interaction lessons only. It does not copy Genshin map content, names, UI art, code, monetization, or world structure.

Five distinct comparator lessons used by this packet are: actionable location/target guidance, custom route-pin creation/management, category/filter-based map decluttering, simultaneous multi-objective tracking, and consolidated relevant navigation information.

Community improvement source:

- Original Genshin player discussion, 2023-09-20, requested more pin icons/colors and the ability to turn pins on/off; a highly upvoted reply also asked for symbol-scoped deletion. Source: https://www.reddit.com/r/Genshin_Impact/comments/16nle96/
- A January 15, 2026 retrospective community post still lists `Pin filter` among older requested QoL while noting that many old requests had gradually been added. Source: https://www.reddit.com/r/Genshin_Impact/comments/1qd7l2l/
- Later official/retained version evidence shows category-scoped batch pin management exists, but the inspected current sources do not establish an equivalent in-game per-category visibility filter for the narrower historical request. Therefore QOL-030 is recorded as a specific player preference whose full current resolution is unestablished, not as proof that current Genshin lacks the feature.

## Five comparator features plus one community increment

### GAME-142: operation-aware next-step hint

**Reference lesson:** map/location guidance should surface the next relevant action without forcing progression.
**Repository gap:** only the Mall survey can currently produce a meaningful next target; Rift Investigation and Shadow Crypt Lead expose counts but no next-step semantics.
**Adaptation:** `CurrentHint()` reports one bounded next action for the tracked operation: next unvisited Mall landmark, Rift Residue, Cooling Anomaly, Crypt Sigil, Shadow Crypt lore, or operation complete.
**Acceptance:** hints advance deterministically as authoritative evidence arrives, never mutate state, and invalid operations still fail closed.

### GAME-143: deterministic next-incomplete operation tracking

**Reference lesson:** selecting/tracking relevant map objectives should focus the player on actionable content rather than require manual rediscovery.
**Repository gap:** after finishing one operation the player must explicitly know which remaining operation is incomplete.
**Adaptation:** `TrackNextIncompleteOperation()` advances cyclically from the current operation to the next incomplete one and refuses mutation once all operations are complete.
**Acceptance:** completed operations are skipped, wraparound is deterministic, and all-complete state is idempotent.

### GAME-144: bounded multi-stop field route

**Reference lesson:** Genshin's official current-era navigation can track multiple nearby objectives, while its official Interactive Map supports personal pins and synced pin distributions.
**Repository gap:** Astral supports only one temporary pin even though the National Mall field loop has three bounded sites.
**Adaptation:** add a fixed-capacity three-stop route with explicit insertion order, duplicate/visited/invalid rejection, individual removal, batch clear, and automatic pruning when a site is discovered. Legacy single-pin precedence remains unchanged.
**Acceptance:** no allocation/unbounded growth, route order is stable, discovering a stop removes it exactly once, and single-pin behavior from pass 14 remains green.

### GAME-145: unread field-journal notices

**Reference lesson:** relevant navigation/information surfaces should make newly actionable information visible without requiring the player to inspect every category repeatedly.
**Repository gap:** journal entries are only locked/unlocked, so the game cannot distinguish newly discovered notes from already-read notes.
**Adaptation:** newly unlocked entries begin unread; read-one and read-all acknowledgement are explicit and idempotent; duplicate evidence never re-notifies an already-read entry.
**Acceptance:** unread counts are exact, invalid/locked/read entries cannot be acknowledged again, and repeated evidence does not resurrect notices.

### GAME-146: consolidated field briefing

**Reference lesson:** map/navigation interfaces should consolidate the relevant tracked state rather than force menu hopping.
**Repository gap:** callers must query operation progress, hint/target, route count, journal state, and completed-operation count separately.
**Adaptation:** `Briefing()` returns a read-only snapshot of those authoritative values with no duplicated progression authority.
**Acceptance:** snapshot matches the underlying guide state before/after route/evidence changes and is non-mutating.

### QOL-030: player route visibility filter

**Community lesson:** players have specifically requested pin filtering / visibility control to reduce map clutter. HoYoverse's official Interactive Map independently demonstrates the utility of a filter list, but this increment is scoped to Astral's own in-game route state.
**Repository gap:** a multi-stop route would otherwise display/select every route category at once.
**Adaptation:** each route stop has one of three original Astral categories (`Objective`, `Resource`, `Note`); an `All`/category visibility filter changes which route pin becomes the current visible target without deleting hidden stops. Invalid filters fail closed.
**Acceptance:** filtering changes target visibility only, preserves insertion/state, hidden stops remain removable/discoverable, and restoring `All` reveals them again.

## Verification plan

1. Add pass-29 tests to the already registered `ThoughtCommandsTests` aggregate without changing CMake.
2. Preserve and rerun pass-14 `ExplorationFieldGuide` regression behavior: operation validation, chronology, monotonic narrative evidence, single-pin precedence/reconciliation, and survey completion.
3. Add pass-29 boundary tests for invalid enums, route duplicates/capacity, visit pruning, category filtering, read acknowledgement idempotency, operation wraparound/all-complete behavior, and non-mutating briefing values.
4. Run the existing hosted Windows Debug/Release lane and Release-manifest lane on the exact final candidate.
5. Obtain a fresh Codex independent review of the exact final candidate. Repair every material finding, resolve threads, and rerun affected exact-head gates before merge.
6. Re-read `main`, PR head, changed paths, checks and full review state immediately before an expected-head merge.

### Exact reproducible hosted commands

The existing `.github/workflows/windows-ci.yml` runs these literal core commands on `windows-2022` with an external `$env:BUILD_ROOT`:

```powershell
cmake -S . -B "$env:BUILD_ROOT" -G "Visual Studio 17 2022" -A x64
cmake --build "$env:BUILD_ROOT" --config Debug --parallel
ctest --test-dir "$env:BUILD_ROOT" -C Debug --output-on-failure -E "RuntimeSmoke" --no-tests=error
cmake --build "$env:BUILD_ROOT" --config Release --parallel
ctest --test-dir "$env:BUILD_ROOT" -C Release --output-on-failure -E "RuntimeSmoke" --no-tests=error
python Scripts/verify_milestone1.py
python Scripts/verify_milestone2.py
python Scripts/verify_milestone3.py
git diff --check
git status --porcelain --untracked-files=no
```

The same workflow also executes the repository's R0-parser safety, PE dependency, Windows-prerequisite/runtime/compatibility/bootstrap, and Release test-safety contract scripts before and around those build/test commands. A workflow run is accepted only when every required step concludes `success`; a nonzero command exits the step/job.

The existing `.github/workflows/release-manifest-validation.yml` additionally runs these literal contract/package commands on the same source head, with workflow-owned temporary build/package paths:

```powershell
python Scripts/test_release_manifest.py
python Scripts/test_package_runtime_smoke.py
python Scripts/test_package_restart_stress.py
python Scripts/test_package_continuous_soak.py
python Scripts/test_package_soak_telemetry_analysis.py
python Scripts/test_benchmark_manifest.py
python Scripts/test_pe_reproducibility_diagnostic.py
cmake -S . -B $build -G "Visual Studio 17 2022" -A x64
cmake --build $build --config Release --target AstralGame --parallel
python Scripts/diagnose_pe_reproducibility.py $exeA $exeB --json $report
python Scripts/release_manifest.py create $package $manifest --commit $commit
python Scripts/release_manifest.py verify $manifest $package --expected-commit $commit --expected-executable-sha256 $exeHash --json $report
git diff --check
git status --porcelain --untracked-files=no
```

A sandbox C++17 header-domain check is also permitted and must be distinguished from hosted/native evidence:

```bash
g++ -std=c++17 -Wall -Wextra -Werror -I<scratch-root> <pass29-test>.cpp -o <scratch-binary>
<scratch-binary>
```

No native interactive runtime command is applicable to pass-29 acceptance because this packet adds no Win32/controller/menu wiring. Hosted CTest intentionally excludes `RuntimeSmoke`. Therefore native playable verification remains `0`, and no runtime exit code is claimed.

## Evidence boundary

These APIs are integrated through the existing `LandmarkInteraction`-owned `ExplorationFieldGuide` production game-domain path, but there is still no Win32/controller/menu map or field-guide UI for these new interactions. Hosted deterministic tests are not a native interactive playtest. No GPU/rendering, animation, art/audio, cross-process field-guide persistence, or performance claim is made by this packet.
