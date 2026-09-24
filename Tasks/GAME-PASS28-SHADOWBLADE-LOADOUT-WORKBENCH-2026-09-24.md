# GAME pass 28: Shadowblade loadout workbench

Date: 2026-09-24  
Owner: `animerpg-game-hourly`  
Repository: `LucasKazaki/AnimeRPG`  
Target: `main`  
Baseline: `6d22da88402db71843ecd5a35766c0c77e62dca6`  
Branch: `game/2026-09-24-shadowblade-loadout-workbench-pass28`  
PR: `#48`

## Scope and ownership

This packet deepens the already-live single-protagonist Shadowblade loadout. It does not change the Astral Engine renderer/platform/editor/import/animation/audio/physics layers, shared build or CI ownership, networking, R0, releases, deployment, or other workers' PRs. `ShadowbladeActions` remains the live game-domain owner. The workbench itself is stateless for player metadata; preset labels and last-successful-preset state are stored with the authoritative `ShadowbladeLoadout` so ordinary loadout assignment cannot split metadata from the build it describes.

Allowed production paths:
- `Engine/Scene/ShadowbladeLoadoutWorkbench.h`
- `Engine/Scene/ShadowbladeLoadout.h`, only to pair pass-28 preset label/last-applied metadata with the existing presets and build state
- `Engine/Scene/ShadowbladeActions.h`, only to own/expose the workbench and bind stateful operations to the live owner
- `Engine/Scene/ShadowbladeActions.cpp`, only for the existing transient-reset helper admitted during review

Verification/records:
- `Tests/ShadowbladeLoadoutWorkbenchPass28Tests.inc`
- `Tests/ThoughtCommandsTests.cpp`, include/call registration only
- this task packet
- `Docs/Agents/animerpg-hourly/PASS28-BACKLOG.json`
- `Docs/Agents/animerpg-hourly/STATE.json`
- `Docs/Agents/animerpg-hourly/RUN-2026-09-24-PASS28.md`

The `ShadowbladeLoadout.h` admission was added after independent review demonstrated that keeping labels and recall metadata in a separate workbench object was incompatible with the pre-existing public mutable `ShadowbladeActions::Loadout()` reference. Moving only that metadata into the loadout makes copy assignment carry or clear it atomically with the saved presets. The earlier narrow `ShadowbladeActions.cpp` reset repair remains in the diff; with metadata now loadout-owned, its workbench copy is semantically redundant but harmless and does not expand ownership.

## Research provenance

### Official reference source

Genshin Impact Official, **Version 5.7 Update Details**, published 2025-06-17 and accessed 2026-09-23:  
https://www.hoyolab.com/article/39384127

The developer-authored update documents several equipment/configuration UX mechanics used only as design references here: Artifact Fast Equip recommendation changes; Recommended Configurations with **Check Alternatives**; two Custom Configuration presets; a sorting scope switch between **Current Slot** and **All Slots**; Crafting **Filter by Character**; Mystic Offering remembering the last selected Artifact Strongbox; and retained/clearable previous challenge configurations. Astral adapts those interaction lessons to its original Cooling/Rift/Civic modules and persistent Shadowblade protagonist. No Genshin assets, item names, formulas, characters, code, or monetization are copied.

### Community source

Original player discussion: **agent presets delete how?**, r/ZenlessZoneZero, published 2026-07-11 and accessed 2026-09-23:  
https://www.reddit.com/r/ZenlessZoneZero/comments/1utacze/agent_presets_delete_how/

The post asks about removing preset configurations. Replies also specifically complain that a preset cannot be returned to an unnamed/default-looking state and that player-entered names cannot contain spaces. Other replies describe manual workarounds. This is a small anecdotal discussion, not consensus. A search of current official Zenless Zone Zero Version 3.2 material did not establish whether the naming complaint has since been changed, so current resolution is **not established**. This pass adapts only the narrow naming usability request, not a claim about ZZZ's present defect.

## Five reference increments plus one community increment

### GAME-137: non-mutating equipment comparison

**Reference mechanic:** Genshin 5.7 Recommended Configurations can check alternatives before changing equipment.  
**Repository gap:** existing `ShadowbladeLoadout` can equip gear and compute a profile, but callers must mutate or make their own copy to understand the exact effect.  
**Adaptation:** workbench previews for owned module or weapon candidates produce exact before/after Attack, Guard, Resource Recovery, Mobility, active-family and readiness deltas using a copied authoritative loadout. Rejected candidates report the real loadout action result and zero mutation.  
**Acceptance:** compatible preview gives exact deltas; unowned/locked/invalid candidates fail without changing equipped gear, tuning resources, presets, or profile.

### GAME-138: focus-aware module recommendation

**Reference mechanic:** Genshin 5.7 explicitly optimizes configuration recommendations and exposes suggested alternatives.  
**Repository gap:** the existing `RecommendedUpgrade()` only answers what equipped item to tune next, not which owned compatible module best serves a desired build goal.  
**Adaptation:** deterministic Balanced, Assault, Guard, Recovery, and Mobility recommendation focuses rank owned slot-compatible modules with original Astral weights and stable enum-order tie breaking. It recommends only and never auto-equips.  
**Acceptance:** different focuses can choose different owned modules; malformed focus/slot fails closed; repeated calls are deterministic and non-mutating.

### GAME-139: resonance-family recommendation filter

**Reference mechanic:** Genshin 5.7 adds filtering to equipment/material configuration interfaces.  
**Repository gap:** the Astral recommendation has no way to constrain suggestions to a planned Cooling, Rift, or Civic identity.  
**Adaptation:** Any/Cooling/Rift/Civic filters restrict candidate modules while preserving slot compatibility and ownership.  
**Acceptance:** a requested owned family returns only that family; a family with no compatible owned candidate returns unavailable rather than fabricating gear; malformed filter fails closed.

### GAME-140: current-slot versus whole-build recommendation scope

**Reference mechanic:** Genshin 5.7 adds a recommendation sorting scope of **Current Slot** or **All Slots**.  
**Repository gap:** module strength and full-build resonance synergy are currently conflated when a player evaluates a replacement.  
**Adaptation:** CurrentSlot scores the candidate's direct bounded contribution while excluding a family bonus newly activated by that candidate; WholeBuild scores the complete resulting authoritative profile, including existing Astral family synergy.  
**Acceptance:** a constructed build demonstrates a direct-stat winner that differs from the whole-build synergy winner, and neither query mutates the live loadout.

### GAME-141: remember and reapply the last successful preset

**Reference mechanic:** Genshin 5.7 says Mystic Offering remembers the last selected Strongbox and other challenge interfaces retain prior configuration for quicker repeat use.  
**Repository gap:** `ShadowbladeLoadout` has three exact presets, but callers must remember the last successfully applied slot themselves.  
**Adaptation:** the live `ShadowbladeActions` workbench records only a successful preset apply into metadata paired with the authoritative loadout and can reapply that same saved slot after later manual equipment changes. Failed/empty preset attempts do not replace the remembered selection.  
**Acceptance:** saving alone creates no remembered selection; successful apply records it; failed apply preserves it; transient combat/training resets preserve it with the loadout; replacing the entire loadout replaces/clears recall metadata with that build; one-action reapply restores the preset through the existing validated `ApplyPreset()` path.

### QOL-029: human-readable preset labels with spaces

**Community request:** the 2026-07-11 ZZZ discussion includes frustration that custom preset names cannot contain spaces and cannot be returned to an unnamed-looking state.  
**Repository gap:** Astral's three loadout slots have no player-facing label metadata at all.  
**Adaptation:** each saved slot can receive an optional 24-byte label paired directly with the authoritative loadout. Outer ASCII spaces are trimmed, internal spaces are preserved, control characters/empty/overlong labels are rejected atomically, and the label can be cleared without altering the underlying saved preset or current equipment. Stateful edits are routed through the paired `ShadowbladeActions` owner.  
**Acceptance:** `"  Mall Patrol  "` becomes `"Mall Patrol"`; an internal space remains; rejected edits preserve the previous label; empty slots cannot be labeled; unrelated owners cannot mutate each other's metadata; assigning an empty/different loadout cannot leave stale labels behind; transient combat/training resets preserve labels with the loadout; clearing a label leaves saved/equipped gameplay state unchanged.

## Verification plan

1. Registered `ThoughtCommandsTests` must compile/run on the exact candidate in both Debug and Release through existing hosted Windows CI.
2. Pass-28 regressions cover preview non-mutation and exact deltas, focus changes, family filtering, malformed filters, scope divergence, owner-bound preset metadata, whole-loadout replacement pairing, transient-reset persistence, successful/failed preset recall, reapply, label spaces, label rejection atomicity, and clear-label non-mutation.
3. Existing pass 13-27 registered tests stay in the same aggregate and must remain green.
4. Release-manifest integrity must pass on the exact same head.
5. A fresh independent authorized reviewer must review the exact final head. Material findings must be repaired and all affected checks rerun before merge.
6. Re-read `main`, PR head, full diff, checks and review state immediately before an expected-head merge.

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

For the superseded head `06b5f57e880737ab286e421244d75234139968bc`, Windows run `35934957614` and Release-manifest run `35934957584` both completed successfully, so every required non-skipped command/step above returned exit code `0`; the release lane classified its two fresh Release executables as byte-identical. Because this task-packet repair changes the PR head, those results are historical evidence only and fresh exact-head runs are still required.

No native interactive runtime command is applicable to pass-28 acceptance because this packet adds no Win32/controller/menu wiring. Hosted CTest intentionally excludes `RuntimeSmoke`. Therefore native playable verification remains `0`, and no runtime exit code is claimed.

## Evidence boundary

These APIs are integrated into the live `ShadowbladeActions` game-domain owner, but there is still no Win32/controller/menu loadout workbench UI. Hosted deterministic tests are not a native interactive playtest. No GPU/rendering, animation, art/audio, cross-process workbench-state persistence, or performance claim is made by this packet.
