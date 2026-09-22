# Astral Engine: UE5 / Unity comparison and implementation plan

Assessed September 20, 2026. This is an initial source-backed capability survey,
not an exhaustive audit or a parity claim. Track work in ENGINE-CAPABILITIES.json.

## Scope and evidence

Keep custom C++17 Astral Engine. UE5 and Unity are references, not dependencies;
AnimeRPG-UE5 stays separate. No paused content, art, narrative or combat work resumes.
Remote base: e12e6c559bf776ffc9c715c809a517f8e02ce5d5, on unmerged PR #6.
Inspected Engine tree: 804d643d3b253bd020b7bf6aa3d86c1113ec5d1b.
The baseline has Win32/GDI wireframes and small tested gameplay domains, not a
production engine. Observations do not describe unknown local branches or processes.

All 19 references below were read on September 20. Epic pages identify UE 5.8;
Unity references pin 6.0 and Netcode 2.7. This does not claim Unity 6.0 is the latest
release. Freeze exact reference installations before benchmarking and research
specific API/version details when admitting each implementation packet.

## Capability and acceptance map

Partial means limited related code exists, not comparable functionality. Not found
means absent from the inspected tree, not proof that no local prototype exists.
Each acceptance contract below is proposed work, not a claim that tests have run.

| ID | Capability | Astral baseline | Acceptance contract | Sources |
|---|---|---|---|---|
| E00 | Evidence/recovery safety | CTest/CI partial; PR #6 native review pending, R0 issue #7 open | Fresh baselines, safe outputs/deadlines, owned worktrees/desktops, independent review | UE-QA |
| E01 | Runtime, jobs, memory | Clock/logger, no general job or memory-budget subsystem found | Bounded work admission, dependencies/results, shutdown/cancellation contracts, contention and memory tests; integrate a real engine caller | UE-TASKS, UNITY-JOBS |
| E02 | Scene ownership/serialization | Basic transforms/cameras | Stable handles, cycle-safe hierarchy, versioned round trips, corruption and recovery tests | Detailed research pending |
| E03 | Asset pipeline/resources | Text wireframe loader only | Safe parsing, stable identity/dependencies, import cache, async load/cancel/unload, resource budgets and provenance | UE-ASSETS, UNITY-ASSETS |
| E04 | GPU rendering/materials | GDI wireframes | Approved backend, buffers/textures, passes, depth/culling, materials, shader errors, resize/device loss and image tests | UNITY-RENDER |
| E05 | Lighting/shadows/reflections | Not found | Direct light/shadows, then indirect lighting/reflections with matched quality, temporal stability and cost evidence | UE-LUMEN |
| E06 | Large worlds/detail | Three fixed landmark proxies | Cell streaming, prefetch/teleport, LOD/HLOD, memory bounds, then measured geometry virtualization | UE-WORLD, UE-NANITE |
| E07 | Animation | Not found | Skeletal import/skinning, clips/blends, root motion/events, constraints and deterministic pose tests | UNITY-ANIMATION |
| E08 | Physics/collision | Bounds are not physics | Broad/narrow phase, queries, rigid bodies, collision layers, stable timesteps and 2D/3D stress | UNITY-PHYSICS |
| E09 | AI/navigation | Bounded encounter/command state only | Behavior/state execution, pathfinding, cancellation/budgets and deterministic agent fixtures; never use an LLM as test oracle | UE-AI; navigation details pending |
| E10 | Audio engine | Not found | Decode/stream/mix, spatialization, latency, underrun/device recovery and budgets using procedural signals | UNITY-AUDIO |
| E11 | UI/editor tooling | Debug HUD only | Scene/asset inspectors, selection/gizmos, undo/redo, save/reopen, UI and editor/runtime boundaries | UNITY-UI |
| E12 | Genuine 2D | Orthographic math only | Sprites/batching/layers, tilemaps, 2D cameras/physics, text and image/runtime tests | UNITY-2D |
| E13 | Multiplayer runtime | Not found | Offline schema tests first; later authorized transport, authority/replication, loss/reorder, prediction decisions and disconnect recovery | UNITY-NET |
| E14 | Profiling/budgets | Timing/logging, no comparable trace suite | CPU/GPU timelines, p50/p95/p99 times, RAM/VRAM, allocations/stalls and reproducible benchmark manifests | UE-PROFILE, UNITY-PROFILE |
| E15 | Packaging/platforms | Win32 and an unsafe historical recovery runner | Clean-machine launch, runtime dependencies, asset cooking/provenance, crash recovery and platform-specific evidence | UE-PACKAGE |
| E16 | Remaining full-catalogue audit | Research incomplete | Source and split terrain/foliage, VFX/particles, cinematics, input/replay, scripting/reflection, plugins, localization/accessibility, console/mobile/XR/web requirements | Research pending |
| E17 | Comparative acceptance | No matched UE/Unity run here | Freeze versions/scenes/hardware/quality, meet approved limits, native stress/recovery, 24-hour soak and independent acceptance | UE-QA, UE-PROFILE, UNITY-PROFILE |

## Work order

Preserve PR #6's test fixes and reconcile native receipts. Do not invoke R0 until
issue #7 is repaired. The current E0 repair addresses an existing parser defect;
it is not permission to start unrelated features while recovery gates are open.

The old loader accepted vertex data before its header. The repair adds header
order, complete numeric tokens, finite coordinates, edge validation and file/record
budgets. It preserves the original API and empty-on-failure contract. It compiles
real engine source in a portable subproject. It does NOT complete E03, create an
asset database, add GPU rendering, or establish production performance.
See ../QA/E0-ASSET-VALIDATION-2026-09-20.md and the corresponding task.

After acceptance, split E01 into small runtime/lifetime, job-queue, dependency and
shutdown/failure tasks. E02/E03 establish data contracts before a GPU backend.
Changing the GDI baseline or importing libraries requires an architecture decision.
The first graphics spike should prove a depth-tested textured mesh, resource
lifetime, resize and readback comparisons before attempting advanced lighting.
E14 instrumentation starts alongside runtime work, not after rendering is finished.
Animation, physics, audio, UI and 2D follow dependency-ready contracts. Network
schema tests can be offline; this plan does not authorize live servers or sockets.

## Comparability is a measured claim

Distinguish game-relevant comparison for named anime-RPG scenes/workflows from
broad UE5/Unity comparability. The latter includes the still-incomplete E16 audit.
Do not remove unmet features, count broad checklist rows as percentages, or call
a simpler scene at a higher FPS equivalent image quality.

Freeze reference versions, compiler/build mode, hardware/driver, resolution,
scene/asset hashes, camera path, quality settings, warmup and sampling duration.
Measure functional/image/pose/physics correctness with frame-time percentiles,
stalls, RAM/VRAM peaks, allocation growth, load/unload latency and recovery.
Initial proposals only: 1080p/1440p profiles and p95 frame time <=16.67 ms on agreed
scenes. Scene complexity, image-error thresholds, comparative cost tolerances and
RAM/VRAM caps remain unset. No sandbox timing proves performance on Lucas's GPUs.
Real native 3D/2D checks, stress/recovery, the 24-hour soak and independent review
remain required. These thresholds are not approved or measured results.

## Research/implementation cycle, not an activated scheduler

Read current controls, source SHA, pending PRs and local receipts. Select one
dependency-ready gap, consult primary sources, define a bounded task, reproduce
or specify a failing test, implement, run available gates, and write a local test
handoff. Preserve failures and block dependent work instead of weakening tests.
Do not create duplicate workers or repeatedly rerun an unchanged failing command.

Lucas requested 15-minute passes. This chat scheduler's minimum is hourly, so no
recurring task was created. The existing Company Runtime remains the sole local
scheduler/executor. This document does not install a scheduler, modify a runtime
database, start local models, or promise unattended future work. An hourly research
coordinator would require a supported cadence to be approved and actually scheduled.

Local models must execute commands, not certify code by reading it. Retain source
SHA, machine/toolchain identity, exact commands, stdout/stderr, exits, UTC times,
artifact hashes and captured frames when applicable. Distinguish source checks,
portable execution, hosted Windows checks and actual local GPU/interactive evidence.
No merge, release, content restart or parity claim follows from an author's report.

## Primary-source catalogue

- UE-TASKS: UE Tasks System, dependent task graphs/results/events/tracing. https://dev.epicgames.com/documentation/en-us/unreal-engine/tasks-systems-in-unreal-engine
- UNITY-JOBS: Unity 6.0 multithreaded job system. https://docs.unity3d.com/6000.0/Documentation/Manual/job-system.html
- UE-ASSETS: Asset identity, loading/unloading, cooking and memory/disk auditing. https://dev.epicgames.com/documentation/en-us/unreal-engine/asset-management-in-unreal-engine
- UNITY-ASSETS: Source-to-imported asset conversion and relationship tracking. https://docs.unity3d.com/6000.0/Documentation/Manual/AssetDatabase.html
- UNITY-RENDER: URP/HDRP/custom SRP and rendering paths. https://docs.unity3d.com/6000.0/Documentation/Manual/render-pipelines.html
- UE-LUMEN: Indirect illumination/reflections and quality/cost tradeoffs. https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-global-illumination-and-reflections-in-unreal-engine
- UE-WORLD: Distance-based cell streaming, data layers and HLOD integration. https://dev.epicgames.com/documentation/en-us/unreal-engine/world-partition-in-unreal-engine
- UE-NANITE: Virtualized geometry and detail management. https://dev.epicgames.com/documentation/unreal-engine/nanite-virtualized-geometry-in-unreal-engine
- UNITY-ANIMATION: Animation import, state machines, editing and blending. https://docs.unity3d.com/6000.0/Documentation/Manual/AnimationSection.html
- UNITY-PHYSICS: 3D/2D simulation and data-oriented physics options. https://docs.unity3d.com/6000.0/Documentation/Manual/PhysicsSection.html
- UE-AI: Behavior-tree documentation; navigation details require another audit. https://dev.epicgames.com/documentation/en-us/unreal-engine/behavior-trees-in-unreal-engine
- UNITY-AUDIO: Audio engine/authoring overview. https://docs.unity3d.com/6000.0/Documentation/Manual/Audio.html
- UNITY-UI: UI Toolkit, uGUI and IMGUI. https://docs.unity3d.com/6000.0/Documentation/Manual/UIToolkits.html
- UNITY-2D: Sprites, tilemaps and 2D physics. https://docs.unity3d.com/6000.0/Documentation/Manual/Unity2D.html
- UNITY-NET: Netcode for GameObjects 2.7, networked objects/world data. This overview does not prove a specific prediction implementation. https://docs.unity3d.com/Packages/com.unity.netcode.gameobjects@2.7/manual/index.html
- UE-PROFILE: Unreal Insights trace capture and CPU/GPU performance analysis. https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-insights-in-unreal-engine
- UNITY-PROFILE: Unity profiling overview. https://docs.unity3d.com/6000.0/Documentation/Manual/Profiler.html
- UE-QA: Unit, feature, content-stress and screenshot-comparison testing. https://dev.epicgames.com/documentation/en-us/unreal-engine/automation-test-framework-in-unreal-engine
- UE-PACKAGE: Build, cook, stage and package distinctions. https://dev.epicgames.com/documentation/en-us/unreal-engine/packaging-your-project
