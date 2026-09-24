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

Primary/near-primary comparator context:

- Genshin Impact Version 5.4 map update details, as retained by the Genshin version changelog and HoYoverse update mirrors: map selections can select a destination, quest hints surface actionable quest state, reward hints are consolidated, and tracked custom markers stay visible through map zoom changes. Version 5.4 released 2025-02-12. Source: https://genshin-impact.fandom.com/wiki/Version/5.4 ; update-detail mirror: https://traveler.gg/moonlight-amidst-dreams-version-5-4-update-details/
- Genshin Impact Version 5.6 map update details: custom-pin management gained category-scoped quantity selection, and map sidebars were reduced to currently relevant information. Version 5.6 released 2025-05-07. Source: https://genshin-impact.fandom.com/wiki/Version/5.6 ; update-detail mirror: https://traveler.gg/paralogism-version-5-6-update-details/
- The project adapts the interaction lessons only. It does not copy Genshin map content, names, UI art, code, monetization, or world structure.

Community improvement source:

- Original Genshin player discussion, 2023-09-20, requested more pin icons/colors and the ability to turn pins on/off; a highly upvoted reply also asked for symbol-scoped deletion. Source: https://www.reddit.com/r/Genshin_Impact/comments/16nle96/
- A January 15, 2026 retrospective community post still lists `Pin filter` among older requested QoL while noting that many old requests had gradually been added. Source: https://www.reddit.com/r/Genshin_Impact/comments/1qd7l2l/
- Later official-version evidence shows category-scoped batch pin selection/deletion arrived by Version 5.6, but the inspected current sources do not establish an equivalent in-game per-category visibility filter. Therefore QOL-030 is recorded as a historical/specific player preference whose full current resolution is unestablished, not as proof that current Genshin lacks the feature.

## Five comparator features plus one community increment

### GAME-142: operation-aware next-step hint

**Reference lesson:** map quest hints should surface the next relevant action without forcing progression.
**Gap:** only the Mall survey can currently produce a meaningful next target; Rift Investigation and Shadow Crypt Lead expose counts but no next-step semantics.
**Adaptation:** `CurrentHint()` reports one bounded next action for the tracked operation: next unvisited Mall landmark, Rift Residue, Cooling Anomaly, Crypt Sigil, Shadow Crypt lore, or operation complete.
**Acceptance:** hints advance deterministically as authoritative evidence arrives, never mutate state, and invalid operations still fail closed.

### GAME-143: deterministic next-incomplete operation tracking

**Reference lesson:** selecting a relevant map entry should focus the player on actionable content rather than make them manually rediscover it.
**Gap:** after finishing one operation the player must explicitly know which remaining operation is incomplete.
**Adaptation:** `TrackNextIncompleteOperation()` advances cyclically from the current operation to the next incomplete one and refuses mutation once all operations are complete.
**Acceptance:** completed operations are skipped, wraparound is deterministic, and all-complete state is idempotent.

### GAME-144: bounded multi-stop field route

**Reference lesson:** tracked custom markers and map target management support intentional exploration routes.
**Gap:** Astral supports only one temporary pin even though the National Mall field loop has three bounded sites.
**Adaptation:** add a fixed-capacity three-stop route with explicit insertion order, duplicate/visited/invalid rejection, individual removal, batch clear, and automatic pruning when a site is discovered. Legacy single-pin precedence remains unchanged.
**Acceptance:** no allocation/unbounded growth, route order is stable, discovering a stop removes it exactly once, and single-pin behavior from pass 14 remains green.

### GAME-145: unread field-journal notices

**Reference lesson:** consolidated map/reward hints make new information visible without requiring the player to inspect every category repeatedly.
**Gap:** journal entries are only locked/unlocked, so the game cannot distinguish newly discovered notes from already-read notes.
**Adaptation:** newly unlocked entries begin unread; read-one and read-all acknowledgement are explicit and idempotent; duplicate evidence never re-notifies an already-read entry.
**Acceptance:** unread counts are exact, invalid/locked/read entries cannot be acknowledged again, and repeated evidence does not resurrect notices.

### GAME-146: consolidated field briefing

**Reference lesson:** relevant map sidebars should expose the currently useful subset instead of forcing menu hopping.
**Gap:** callers must query operation progress, hint/target, route count, journal state, and completed-operation count separately.
**Adaptation:** `Briefing()` returns a read-only snapshot of those authoritative values with no duplicated progression authority.
**Acceptance:** snapshot matches the underlying guide state before/after route/evidence changes and is non-mutating.

### QOL-030: player route visibility filter

**Community lesson:** players have repeatedly asked for pin filtering / pin visibility control to reduce map clutter.
**Gap:** a multi-stop route would otherwise display/select every route category at once.
**Adaptation:** each route stop has one of three original Astral categories (`Objective`, `Resource`, `Note`); an `All`/category visibility filter changes which route pin becomes the current visible target without deleting hidden stops. Invalid filters fail closed.
**Acceptance:** filtering changes target visibility only, preserves insertion/state, hidden stops remain removable/discoverable, and restoring `All` reveals them again.

## Verification plan

1. Add pass-29 tests to the already registered `ThoughtCommandsTests` aggregate without changing CMake.
2. Preserve and rerun pass-14 `ExplorationFieldGuide` regression behavior: operation validation, chronology, monotonic narrative evidence, single-pin precedence/reconciliation, and survey completion.
3. Add pass-29 boundary tests for invalid enums, route duplicates/capacity, visit pruning, category filtering, read acknowledgement idempotency, operation wraparound/all-complete behavior, and non-mutating briefing values.
4. Run the existing hosted Windows Debug/Release lane and Release-manifest lane on the exact final candidate.
5. Obtain a fresh Codex independent review of the exact final candidate. Repair every material finding, resolve threads, and rerun affected exact-head gates before merge.
6. Immediately before merge, re-read `main`, PR head, changed paths, checks, full review state, and merge with expected head only.

### Exact reproducible hosted commands

The existing `.github/workflows/windows-ci.yml` runs these core commands on `windows-2022` with an external `$env:BUILD_ROOT`:

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

The same workflow also runs the existing R0-parser safety, PE dependency, Windows prerequisite/runtime/compatibility/bootstrap, and Release test-safety scripts. Every required non-skipped step must conclude `success` (exit code 0).

The existing `.github/workflows/release-manifest-validation.yml` runs its existing contract/package commands, including:

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

No native interactive runtime command is applicable to acceptance because this packet does not add Win32/controller/menu wiring. Hosted CTest excludes `RuntimeSmoke`; native playable verification therefore remains separate and must not be claimed from this packet.
