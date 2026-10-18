# Astral Engine progress assessment

Snapshot: September 24, 2026. Inspected main:
`2958741188279a0b438dd489ad00cac546012c25`.
Read the [free-toolchain plan](FREE-TOOLCHAIN-2026-09-24.md),
[calculation data](engine-progress-2026-09-24.json), and
[audit task](../../Tasks/FREE-TOOLCHAIN-AUDIT-2026-09-24.md).

## Bottom line

**Approximately 10% of the current broad engine capability roadmap**, using the
author-estimated weighted maturity rubric below. A reasonable judgment range is
**5-15%**. This is a planning index, not a measured percentage of remaining labor,
a statistical confidence interval, a promised completion date, or literal parity
with every feature in Unreal/Unity. The exact arithmetic is 9.75; reporting more
precision than about 10% would exaggerate the quality of the estimates.

The inspected capability register records **0 of 18 areas independently accepted**,
with no native evidence entries. That does not mean zero useful code. It means
none of the broad areas has met its full acceptance definition in this register.
The register itself is dated September 22 and declares the inventory incomplete;
this audit cross-checks selected current source and live PR/workflow state rather
than pretending it is a fresh native-machine report. [R1]

The custom-engine direction remains intact. However, current evidence does not
justify calling the project near completion or predictably on schedule for
top-tier parity. The most important missing implementation is the integrated
render/asset/animation/editor pipeline, alongside unresolved native verification.
There is no accepted time estimate or measured delivery velocity to forecast a
finish date. The free toolchain offers implementation components, not automatic
completion.

## What is actually present

Current main has a successful Windows hosted build/deterministic-test workflow,
run 36015583984, for this exact main SHA. Debug and Release steps passed. This is
real positive evidence for the integrated prototype. Hosted tests deliberately
exclude interactive RuntimeSmoke execution; they do not certify native GUI/GPU,
visual quality, performance budgets, clean-machine packaging or soak. [R2]

Source inspection finds a Win32/GDI wireframe renderer with projected line/box
primitives and HUD drawing, rather than a modern GPU renderer. The StaticMesh
interface holds vertices and edges in a bounded custom format, not the complete
triangle/UV/tangent/material/skin asset pipeline. These are useful foundations
but cannot render the desired finished game yet. [R3][R4]

The editor has an outliner/inspector and procedural preview. Its own status text
says transform tools, undo/redo, save/reopen, Play and real import are pending.
The Select/Move/Rotate/Scale/Play controls are marked pending. This is an editor
shell, not a production editor that happens to lack polish. [R5]

Game-side combat, progression, encounter and narrative logic has advanced beyond
the original prototype. That does not automatically advance engine rendering,
physics or authoring. PR #68 is a separate localized-damage foundation; PR #71 is
voice/dialogue direction with source tools. Neither establishes production
body-part collision, character voice playback or facial animation. [R6][R7]

## Current blockers and stale evidence

**Recovery PR #52 is not green on its current head.** The actual head observed is
`1b7a80715b33080372405ff357b1dafb3979d178`, while its body still describes older
head `237241a15af20a64ae3d865544e96c1a9c3a0682` and a successful old run. Current
workflow 36026761079/job 107725163484 failed `Verify R0 runner safety contracts`.
The log identifies `test_windows_ci_verifies_pr_head_and_merge_ref`: the test
expects a head-and-merge-ref matrix, but the workflow exercises the exact head
only. It ran 31 tests, with one failure and two platform skips. Subsequent build
and verification steps were skipped. This is a workflow/test contract mismatch,
not evidence of a compiler failure or an engine crash. [R8]

The owner should align the workflow and intended verification contract, run both
appropriate refs, obtain fresh independent exact-head review and update the PR
body. Do not weaken the test merely to obtain green status. This audit does not
edit that worker's runner, invoke historical R0, or treat main as broken because
an unmerged candidate failed.

**Editor PR #13 remains native-pending.** Its current reported head
`daf872debbba5fb775c57ab9e2e16c82bc32c13f` records hosted/source and independent
review evidence, but the interactive Windows acceptance matrix remains undone
in the visible record. Run it through the existing authorized local executor;
repeated source edits do not substitute for that evidence. [R9]

No new local machine, native performance run or 24-hour soak was observed by this
audit. Missing evidence means unverified here, not proof a local experiment never
occurred. Do not estimate completion from milestone labels, PR/commit counts,
number of tests or an asset archive's number of files. The older project status
already warns that implementation milestone M10 does not equal ten out of fifteen
original product milestones. [R10]

## Percentage method

Denominator: the existing E00-E17 capability catalogue, weighted to emphasize the
renderer, resource pipeline, animation, physics and usable editing rather than
administrative activity. Weights are declared author judgment and sum to 100.
Maturity values are judgments about usable implementation within each whole
area, not fractions of checkboxes that already have strict acceptance.

Rubric: 0 = not found/no usable implementation credit; 1-20 = isolated prototype;
21-40 = partial integrated foundation; 41-70 = broader integrated subsystem;
71-90 = substantial native/performance evidence; 100 = complete defined scope
with independent acceptance. These ranges are heuristic, not a new claim that
any current native gate has passed. Candidate work is credited only where present
in usable main; failed or unintegrated PRs receive no extra maturity credit.

Weighted score = sum(weight_percent * estimated_maturity_percent) / 100.

| ID | Capability | Weight | Estimated maturity | Evidence rationale |
|---|---|---:|---:|---|
| E00 | Evidence and recovery safety | 5% | 40% | Integrated CI/assertion guards and bounded safety work; current PR #52 fails its workflow contract. |
| E01 | Runtime, jobs and memory | 8% | 30% | Win32 loop, timing and existing runtime core; production job/memory infrastructure incomplete. |
| E02 | Scene ownership and serialization | 7% | 15% | Transforms and bounded world state; general scene save/load/editor ownership incomplete. |
| E03 | Asset pipeline and resources | 8% | 15% | Bounded custom wireframe loader and source fixtures, not a production glTF/material cooker. |
| E04 | GPU rendering and materials | 15% | 0% | Inspected production renderer is GDI wireframe, not GPU mesh/material rendering. |
| E05 | Lighting, shadows and reflections | 8% | 0% | No accepted production lighting pipeline found. |
| E06 | Large worlds and virtualized detail | 6% | 0% | No accepted streaming/virtualized-detail implementation found. |
| E07 | Animation | 7% | 0% | No accepted skeletal/facial animation runtime found. |
| E08 | Physics and collision | 6% | 0% | Combat range logic is not a general 2D/3D physics/query system. |
| E09 | AI and navigation | 4% | 10% | Bounded encounter/attack logic; no general navigation/AI tooling acceptance. |
| E10 | Audio engine | 4% | 0% | PR #71 has instructions and a generated source cue, not playback. |
| E11 | UI and editor tooling | 7% | 10% | Outliner/inspector shell; transform tools, undo/redo, save, Play and imports pending. |
| E12 | Genuine 2D support | 3% | 10% | Some screen-space/core primitives; not an accepted 2D authoring/game template. |
| E13 | Networking and multiplayer runtime | 2% | 0% | Future engine capability, not a required first RPG slice. |
| E14 | Profiling and budgets | 4% | 30% | Capture/instrumentation groundwork; no accepted comparative GPU budgets. |
| E15 | Packaging and platform acceptance | 2% | 25% | PE/runtime prerequisite planning; clean-machine/package acceptance missing. |
| E16 | Complete feature-catalogue audit | 1% | 0% | Catalogue declares inventory incomplete; documents alone receive no implementation credit. |
| E17 | Comparative acceptance and soak | 3% | 0% | No accepted matched benchmark or required 24-hour soak in inspected register. |

The 0-18 acceptance count and the roughly 10% maturity estimate answer different
questions. Some useful implementation can exist inside a capability that is
nowhere near full acceptance. There is no basis to convert this index into
"90% of the development time remains." Expanding or reweighting the catalogue
requires versioning the rubric, not silently inflating the number.

## Route to a credible top-tier engine

These are recommended stages, not newly granted dependency/merge permissions.
They preserve the long-term broad engine goal while giving each iteration a
visible, testable deliverable.

**1. Close the verification bottleneck.** Repair the actual PR #52 contract failure,
complete fresh review and the owned native editor matrix. Keep safety and native
gates; stop repeatedly rewriting green source while missing runtime evidence is
the actual dependency. Resolve owner/executor handoffs with exact SHAs.

**2. Prove an asset-to-GPU vertical slice.** Agree scene/resource ownership and the
renderer backend. Load one original static glTF mesh with normals, UVs and a real
material; render triangles, depth, camera and one light. Verify scale, color space,
resource teardown, malformed input and deterministic asset identity. This is the
highest-value visible implementation milestone, not merely generating more files.

**3. Make the editor genuinely authorable.** Select and transform that object,
undo/redo, save the scene, reopen it and enter/exit Play. Inspect live properties,
not hard-coded strings. Import changes predictably. Add a usable 2D template as
well as the 3D scene, with matching input/asset controls and beginner instructions.

**4. Integrate one complete game slice.** A rigged protagonist, collision,
locomotion/combat, one enemy with actual part hits, pathfinding, audio, a voiced
English/Japanese dialogue with expressions, HUD, save/checkpoint and retry.
Preserve one health owner and one narrative transaction owner. Test keyboard and
controller. Keep one playable character and equipment/cosmetic-only gacha scope.

**5. Scale fidelity and content safely.** Add material authoring, shadows,
reflections, post-processing, particles, terrain/foliage/water and streaming.
Use measured LOD and crowd/asset budgets before advanced virtualized geometry or
real-time global illumination. Libraries do not confer equivalent Nanite/Lumen
behavior: Epic describes those as dedicated systems with their own rendering and
content constraints. Unity's manual likewise spans far more than a renderer. [R11]

**6. Accept a reproducible release-quality engine.** Define versioned feature
contracts and matched comparison scenes. Record hardware, settings, actual p95/
p99 frame times, CPU/GPU split, RAM/VRAM, load/unload behavior, recovery, package
launch on a clean machine and the required soak. The old 16.67 ms p95 budget is
only a proposal, not a measured achievement. Publish accepted capabilities and
remaining gaps. Do not claim all-engine parity from one attractive screenshot.

Development reporting should track integrated demonstrations, native gates,
regressions and critical-path blockers. It should not optimize for five more
isolated features per cycle while core rendering/authoring dependencies remain
missing. Parallel research and independent game data work remain useful, but
must be reported separately from usable engine capabilities.

## Evidence references

[R1] Current-main capability register (dated assessment September 22):
https://github.com/LucasKazaki/AnimeRPG/blob/2958741188279a0b438dd489ad00cac546012c25/Docs/Research/ENGINE-CAPABILITIES.json

[R2] Current-main hosted workflow:
https://github.com/LucasKazaki/AnimeRPG/actions/runs/36015583984

[R3] Inspected renderer, first 220 lines:
https://github.com/LucasKazaki/AnimeRPG/blob/2958741188279a0b438dd489ad00cac546012c25/Engine/Renderer/Renderer.cpp

[R4] Wireframe asset interface:
https://github.com/LucasKazaki/AnimeRPG/blob/2958741188279a0b438dd489ad00cac546012c25/Engine/Assets/StaticMesh.h

[R5] Inspected editor, first 220 lines:
https://github.com/LucasKazaki/AnimeRPG/blob/2958741188279a0b438dd489ad00cac546012c25/Tools/AstralEditorMain.cpp

[R6] Localized-damage source foundation (prior conversation publication):
https://github.com/LucasKazaki/AnimeRPG/pull/68

[R7] Current voice/dialogue source proposal:
https://github.com/LucasKazaki/AnimeRPG/pull/71

[R8] Recovery candidate and failed exact-head workflow/job:
https://github.com/LucasKazaki/AnimeRPG/pull/52
https://github.com/LucasKazaki/AnimeRPG/actions/runs/36026761079/job/107725163484

[R9] Editor native verification candidate:
https://github.com/LucasKazaki/AnimeRPG/pull/13

[R10] Dated repository status and numbering caveat:
https://github.com/LucasKazaki/AnimeRPG/blob/2958741188279a0b438dd489ad00cac546012c25/Docs/Project-Status.md

[R11] Primary comparator documentation, not licensed code being imported:
https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-virtualized-geometry-in-unreal-engine
https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-global-illumination-and-reflections-in-unreal-engine
https://docs.unity3d.com/6000.0/Documentation/Manual/UnityManual.html
