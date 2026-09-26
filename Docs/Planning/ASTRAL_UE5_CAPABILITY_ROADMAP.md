# Astral Engine capability roadmap and perpetual execution contract

**Status:** active engine-only plan
**Owner:** Astral Project Director with the Builder A → Builder B relay
**Last reviewed:** 2026-09-18
**Reference survey:** `Docs/Research/UE5_CAPABILITY_REFERENCE_2026-09-18.md`
**Controlling constraints:** `AGENTS.md` and `GAME_DEVELOPMENT_CONTROL.md`

## 1. What this plan promises—and what it does not

Astral is a custom native C++17 Windows engine. The project is building a real
3D runtime with genuine, independently runnable 2D support; gameplay, game
assets, art, audio content, narrative and game playtests remain paused until
engine acceptance and an explicit operator resumption.

This roadmap makes the complete *UE5 capability surface relevant to a general
purpose real-time engine* visible and actionable. It deliberately does **not**
claim binary compatibility, API compatibility, performance equivalence, or
feature parity with Unreal Engine 5. UE5 is a research reference, not a
dependency or a source of code to copy. Each UE5-derived concept is reframed
as an original Astral requirement with its own source, test fixture, budget and
acceptance proof.

No honest plan can promise that an external event will never block work. OS
policy, toolchain defects, hardware failure, credentials, licensing, safety
reviews and explicit human release decisions are real boundaries. This plan
does promise that **ordinary internal blocks never become idle waiting**:
the runtime diagnoses them, selects a bounded repair or independent capability,
and preserves the evidence needed to retry under changed conditions.

## 2. Planning vocabulary and evidence standard

| Term | Meaning |
| --- | --- |
| **Present** | Exact source implementation, deterministic automated checks, Debug and Release native evidence, stated budget result and independent acceptance exist on the candidate. |
| **Partial** | Some source or a narrow fixture exists, but one or more integration, fallback, measurement, lifecycle, or independent-acceptance requirements are absent. |
| **Missing** | No production-consumed implementation and evidence bundle exists. |
| **Core** | Required before Astral engine acceptance. |
| **Compatible path** | The engine must expose a stable data/lifecycle seam and test fixture now; full implementation enters the queue only when a named re-entry condition is met. |
| **Deferred** | Explicitly out of the current engine acceptance critical path, with a prerequisite and re-entry condition. Never silently omitted. |
| **Not adopted** | Outside the authorized Windows/native/local-only engine scope or dependent on an external service/platform program/right not yet approved. |
| **Packet** | One clean-worktree, one-writer implementation or read-only verification task with exact parent, source ownership, external Debug/Release roots, commands, stop condition and independent acceptance. |

Every row below follows this evidence rule:

```text
reference → Astral requirement → original fixture/oracle → implementation →
Debug + Release checks → native capture/trace → budget → independent review
```

A feature can advance from Missing to Partial only after a bounded packet
changes source and produces an automated test. It advances to Present only
after Builder B or another independent verifier accepts the exact candidate.
No row advances from a plan, a compile, a mock screenshot, or a self-authored
worker report.

## 3. Product, platform and acceptance envelope

### 3.1 Fixed current envelope

- **Host:** Windows desktop, native C++17 and CMake; build artifacts outside
  the source worktree.
- **Primary graphics target:** Direct3D 12 feature discovery first. An optional
  Vulkan path is a Compatible path, not a hidden prerequisite. DX11 fallback is
  a deliberate capability decision only if the renderer matrix requires it.
- **Primary content model:** original synthetic fixtures for engine validation;
  no third-party asset/code import without separate provenance and licensing
  approval.
- **Modes:** standalone native 3D fixture and separately runnable 2D fixture.
- **Input automation:** process/window-scoped and non-activating. Global input,
  foreground stealing and ambiguous window identity are permanently rejected.
- **Security/release:** no unattended merge to main, push, release, publication,
  destructive operation, credential use, paid service, licensing decision, or
  external download/integration.

### 3.2 Baseline scenarios to build before budgets are frozen

The first benchmark packet must identify the actual Windows version, compiler,
GPU/driver, CPU, RAM, resolution, display mode and build configuration. It then
creates the following deterministic original fixtures. Budgets are not invented
until that baseline is captured.

| Scenario ID | Purpose | Minimum observables |
| --- | --- | --- |
| `A3D-01` | one camera, static mesh/material/light, resize/minimize/restore | frame CPU/GPU time, draw/dispatch count, RAM/VRAM, device-lost/recovery state |
| `A3D-02` | streamed cell traversal with entities/materials/textures | cell latency, CPU/GPU spikes, residency, visible continuity, load/unload leaks |
| `A3D-03` | skeletal animation, collision/query, audio event and input action | simulation tick drift, animation pose error, query determinism, audio latency |
| `A3D-04` | lights, shadows, transparency/particles and post processing | quality tier, shadow cache/page metrics, overdraw, GPU frame distribution |
| `A2D-01` | independently launched sprite/tile/text/UI batching fixture | draw/batch count, input/rebind behavior, pixel-oracle capture, RAM/VRAM |
| `AST-24H` | mixed 3D/2D load, resize, invalid-input and recovery loop | crash/hang count, peak/median/p95/p99 frame times, leaks, resource-lifetime errors, trace/capture sample rate |

### 3.3 Required evidence format

Each executed packet records a scenario ID, exact commit, executable hash,
command lines, configuration, machine fingerprint, input seed, captured output,
trace path, threshold values, distribution statistics and verifier verdict.
This allows later unblocking workers to reproduce a failure rather than repeat
an unchanged command.

## 4. Capability ledger

The status column intentionally begins as **Audit required** unless a future
packet cites exact local evidence. Historical game-milestone claims are not
substitutes for this engine audit.

### 4.1 Platform, build, runtime and core architecture

| ID | UE5 capability family specifically accounted for | Astral requirement / disposition | Dependencies and first bounded acceptance |
| --- | --- | --- | --- |
| ARC-001 | platform abstraction, application lifecycle, window/input routing | **Core.** Process-scoped Win32 app lifecycle, DPI/resize/minimize/restore, multi-window identity and shutdown. | Existing platform loop audit; test `A3D-01` plus invalid HWND and relaunch. |
| ARC-002 | modules, plugins, reflection/UObject-style metadata | **Core seam.** Original module registry, explicit ABI/version contracts, registration/unregistration and reflected property metadata. No Unreal-style API cloning. | ARC-001; load a test module, reject version mismatch, serialize metadata. |
| ARC-003 | memory allocators, object lifetime, garbage/reference systems | **Core.** Ownership policy, arenas/pools, allocation tags, leak reporting, stable handles and deferred destruction. | ARC-001; deterministic create/destroy storm with no leaked GPU/CPU handles. |
| ARC-004 | task graph, async loading, worker threads | **Core.** Bounded job scheduler, cancellation, fences, main-thread affinity, priority/inversion diagnostics and clean shutdown. | ARC-003; randomized dependency DAG/replay and shutdown-under-load test. |
| ARC-005 | time, tick groups, fixed/variable simulation | **Core.** Monotonic time, fixed tick option, frame pacing, tick-order contracts and pause/slowdown semantics. | ARC-004; seed-replay and long-frame catch-up oracle. |
| ARC-006 | config, console variables, project settings | **Core.** Typed, source-controlled config schema; validated command-line/ini overrides; feature-tier capture. | ARC-002; unknown/out-of-range config rejection and capture of effective config. |
| ARC-007 | logging, assertions, crash reporting and visual log | **Core.** Structured logs, assertion tiers, crash-safe breadcrumbs, minidump policy and visual/debug event stream. | ARC-001; injected failure yields a scoped, non-secret receipt. |
| ARC-008 | localization, text, culture and Unicode | **Compatible path.** UTF-8/UTF-16 boundary policy, deterministic text shaping seam and locale-aware resource IDs. | Needed before editor/UI localization; text round-trip fixture. |
| ARC-009 | object serialization, versioning and migrations | **Core.** Stable binary/text schemas, version tags, migration hooks, corrupt-input rejection and deterministic canonical form. | ARC-002/003; old/new/corrupt fixture corpus. |
| ARC-010 | hot reload/live coding | **Compatible path.** Safe reload boundary for tools/modules; no mutation of live gameplay state until lifecycle is proven. | ARC-002/004; reload a disposable module and reject unsafe reload. |

### 4.2 Worlds, scenes, entities and large-world streaming

| ID | UE5 capability family specifically accounted for | Astral requirement / disposition | Dependencies and first bounded acceptance |
| --- | --- | --- | --- |
| AWS-001 | world/level lifecycle, actor/component scene graph | **Core.** Stable entity IDs, transform hierarchy, components, lifecycle events and ownership. | ARC-003/009; create/reparent/destroy/reload oracle. |
| AWS-002 | ECS/Mass-style data-oriented processing | **Core seam.** Archetype/query storage only where profiling proves need; deterministic iteration modes and bridge to scene entities. | AWS-001/ARC-004; same-seed query/order test. |
| AWS-003 | Large World Coordinates | **Core.** Explicit coordinate precision/origin-shift policy and conversion boundaries for renderer/physics/nav. | AWS-001; far-origin camera/physics precision fixture. |
| AWS-004 | World Partition grid cells and streaming sources | **Core.** Cell manifest, spatial index, streaming source policy, async state machine, cancellation and residency telemetry. | AWS-001/004/009; `A3D-02` load/unload traversal. |
| AWS-005 | One File Per Actor / external actor data | **Compatible path.** One-entity-or-cell authoring units, atomic load/save and conflict-safe manifests. | AWS-004/009; concurrent non-overlapping edits and manifest integrity test. |
| AWS-006 | Data Layers | **Core.** Named runtime/editor layer membership, deterministic activation and persistence with no hidden content dependency. | AWS-004/005; activate/deactivate test while preserving stable IDs. |
| AWS-007 | Level Instances/prefabs | **Core.** Versioned reusable sub-world instancing, overrides and isolation rules. | AWS-001/009; nested instance transform/serialization fixture. |
| AWS-008 | HLOD, culling and world representation | **Core.** Visibility hierarchy, LOD/HLOD proxy policy, deterministic selection and measurable screen-error rule. | AWS-004/renderer; traversal count/visual continuity oracle. |
| AWS-009 | PCG frameworks and procedural spawners | **Compatible path.** Seeded generation interface, provenance and bake/rebuild lifecycle. | AWS-004/009; identical seed result and invalid-rule recovery. |
| AWS-010 | landscape, foliage, water and environment world tools | **Deferred.** Procedural test terrain/instances first; authoring tools re-enter once streaming/rendering baseline passes. | AWS-004/AWR-008; large terrain/instance stress fixture. |

### 4.3 Rendering architecture, GPU resources and shader pipeline

| ID | UE5 capability family specifically accounted for | Astral requirement / disposition | Dependencies and first bounded acceptance |
| --- | --- | --- | --- |
| ARR-001 | RHI / API backend abstraction | **Core.** Explicit D3D12 backend, adapter discovery, feature caps, device removal handling and a backend-neutral command/resource contract. | ARC-001; adapter capability receipt and device-lost safe failure. |
| ARR-002 | render graph / RDG | **Core.** Pass graph, declared read/write states, transient allocation, barrier validation, pass culling and graph capture. | ARR-001; deliberate hazard must fail validation; graph trace proves ordering. |
| ARR-003 | shader compilation, permutations and pipeline cache | **Core.** Offline/controlled shader build, reflection, variant key, error surface, cache versioning and invalidation. | ARR-001/006; compile/cache/invalidate/recover fixture. |
| ARR-004 | meshes, vertex/index buffers and draw submission | **Core.** Typed mesh resource, upload lifecycle, instancing, indirect-ready submission seam and validation. | ARR-002; render original mesh and compare stable capture hash. |
| ARR-005 | Nanite/virtualized geometry | **Compatible path.** Cluster/meshlet data, visibility/culling seam, streaming ID and fallback mesh contract. Full virtual geometry is deferred until profiling shows ordinary LOD fails budget. | ARR-004/AWS-008; cluster selection oracle and fallback visual equivalence. |
| ARR-006 | materials, material editor/graph and parameter collections | **Core.** Original material IR/bytecode or bounded material API, scalar/vector/texture params, validation, fallback material and serialized instances. | ARR-003/004/009; material corpus and invalid graph rejection. |
| ARR-007 | texture formats, mip generation, streaming and virtual textures | **Core.** Decode/import boundary, color-space metadata, mip/residency policy, sampler validation, streaming telemetry and fallback. | ARR-001/003; pressure/residency/reload fixture. |
| ARR-008 | geometry cache, morph targets, runtime mesh/deformation | **Compatible path.** Resource/lifetime API and deformation upload seam. | ARR-004/animation; deformation fixture when animation needs it. |
| ARR-009 | occlusion/frustum/distance culling | **Core.** Stable culling order, debug overlay, bounds validation, occlusion confidence policy and no invisible-object leaks. | ARR-002/004; camera sweep oracle. |
| ARR-010 | reflection captures, planar reflections and scene captures | **Compatible path.** Offscreen target/camera capture API with explicit update policy. | ARR-002; deterministic scene-capture output. |
| ARR-011 | editor render diagnostics/frame debugger | **Core.** Pass/resource inspection, draw and memory counters, capture identifiers and no secret data in traces. | ARR-002/007; one captured frame drills to each pass. |

### 4.4 Lighting, shadows, atmosphere, post and image quality

| ID | UE5 capability family specifically accounted for | Astral requirement / disposition | Dependencies and first bounded acceptance |
| --- | --- | --- | --- |
| AIL-001 | deferred/forward rendering paths | **Core decision.** Write a feature matrix and select first path; all capabilities state support/fallback, never silently assume both. | ARR-002; `A3D-01` path-specific capture. |
| AIL-002 | direct lights, light types, IES/light functions | **Core.** Directional/point/spot baseline, physically documented units, attenuation and shadow participation. | ARR-006; numerical light fixture. |
| AIL-003 | static/baked lighting and lightmass-like workflows | **Compatible path.** Bake artifact format/provenance seam; no build farm or external tool assumption. | AIL-002/AWS-005; bake/load/version test. |
| AIL-004 | Lumen-style dynamic global illumination and reflections | **Deferred high-cost capability.** Record software/hardware eligibility, scene representation, temporal history and fallback. Re-enter after base lighting/shadow/profile budgets. | ARR-005/AIL-001/002; one room GI convergence/quality benchmark. |
| AIL-005 | hardware ray tracing, software tracing and path tracing | **Compatible path / Deferred.** Capability probe, acceleration-structure seam, deterministic reference image path and no unsupported-GPU crash. | ARR-001/004; feature-unavailable fallback and simple reference scene. |
| AIL-006 | shadow maps, cascades, contact shadows | **Core.** Stable shadow depth resources, bias/filter policy, per-light settings and visual regression fixture. | AIL-002/ARR-002; moving-camera acne/peter-panning oracle. |
| AIL-007 | Virtual Shadow Maps | **Compatible path.** Page allocation/cache invalidation API, telemetry and fallback conventional shadow path. | AIL-006/ARR-005/AWS-004; cache invalidation stress test. |
| AIL-008 | ambient occlusion, screen-space effects | **Compatible path.** Defined quality tier, temporal stability test and graceful disable. | AIL-001; motion/resize capture oracle. |
| AIL-009 | sky atmosphere, volumetric clouds/fog, exponential height fog | **Compatible path.** Environment/participating-media parameter model and quality fallbacks. | AIL-002; camera-height capture set. |
| AIL-010 | water, caustics and fluid rendering | **Deferred.** Synthetic plane/reflection/refraction test only until a project need exists. | ARR-010; bounded fluid/plane fixture. |
| AIL-011 | post process volumes, tone mapping, bloom, DOF, motion blur, lens effects | **Core.** Ordered post chain, HDR/SDR color policy, disable/per-pass diagnostics and capture-based regression. | ARR-002/006; known HDR/SDR color patches. |
| AIL-012 | TAA, TSR, DLSS/FSR/XeSS-style upscaling | **Core temporal seam; vendor integrations Not adopted until separately approved.** Temporal history/jitter/motion-vector contract plus native spatial fallback. | ARR-002/004; moving scene/resolution-change ghosting oracle. |
| AIL-013 | HDR output, color management, LUTs and display calibration | **Compatible path.** Explicit color-space chain and metadata; re-enter when target display hardware is available. | AIL-011; SDR reference is required first. |
| AIL-014 | translucency, decals, volumetrics and order-independent issues | **Core baseline.** Deterministic sort/quality rule, decals and transparent resource limits. | ARR-006; overlapping-transparency capture test. |

### 4.5 Assets, import, cooking and packaging

| ID | UE5 capability family specifically accounted for | Astral requirement / disposition | Dependencies and first bounded acceptance |
| --- | --- | --- | --- |
| AAP-001 | asset registry, paths, references and redirectors | **Core.** Stable asset UUID/path map, dependency graph, rename/move redirect policy and cycle diagnostics. | ARC-009; rename/dependency/cycle corpus. |
| AAP-002 | import/reimport, factories and source provenance | **Core.** Explicit importer boundaries for selected original fixture formats, source hash, options, reimport diff and error report. | AAP-001; import/reimport/corrupt-file test. |
| AAP-003 | mesh/skeleton/animation/texture/audio format pipeline | **Core staged.** Add one format only when the consuming subsystem is ready; schema/version validation and generated test assets. | AAP-002; source-to-runtime provenance test. |
| AAP-004 | derived data cache and shader/asset build cache | **Core.** Content-addressed local cache, versioning, eviction, corruption recovery and measured cold/warm results. | ARR-003/AAP-002; cache hit/miss/corrupt entry scenario. |
| AAP-005 | virtualized bulk data, IO store, pak/chunking | **Core.** Versioned package/container, async range reads, integrity check, chunk manifest and launcher-friendly errors. | AAP-001/004/AWS-004; package/load/corrupt-chunk stress. |
| AAP-006 | cooking, staging, patching and DLC | **Core base; DLC Deferred.** Reproducible cook/stage/package and package-launch receipt first. | AAP-005; clean-machine style package-launch test. |
| AAP-007 | asset validation, audit and commandlets | **Core.** Headless validation command, dependency/license/provenance fields and machine-readable report. | AAP-001/002; malformed asset corpus. |
| AAP-008 | source-control integration and multi-user editing | **Compatible path.** Read-only status/diff abstraction and conflict-safe external-actor data first. External source-control or collaboration services remain Not adopted. | AWS-005/AAP-001; simulated conflict fixture. |

### 4.6 Animation, rigs, characters and deformation

| ID | UE5 capability family specifically accounted for | Astral requirement / disposition | Dependencies and first bounded acceptance |
| --- | --- | --- | --- |
| AAN-001 | skeletal mesh, bones, sockets and skinning | **Core.** Original skeleton/mesh data, CPU/GPU skinning path policy, bind-pose validation and debug visualization. | ARR-004/AAP-003; skinned synthetic-rig capture. |
| AAN-002 | animation sequences, compression and curves/notifies | **Core.** Versioned clips, interpolation, compression-error measurement, event/curve dispatch and deterministic sampling. | AAN-001/ARC-009; clip round-trip and event-order test. |
| AAN-003 | animation graphs, blend spaces and state machines | **Core.** Data-driven graph evaluation, blend rules, state transitions, trace output and invalid graph rejection. | AAN-002; graph corpus/replay oracle. |
| AAN-004 | IK, retargeting, Control Rig and full-body IK | **Compatible path.** Constraint/solver interface, rig debug drawing and retarget metadata. | AAN-003; two-bone and target-change fixture. |
| AAN-005 | Motion Matching / Pose Search | **Deferred.** Pose-feature database/query seam, memory budget and trajectory contract; re-enter only after clips/graphs/profiling are stable. | AAN-002/003/ARC-004; query determinism and memory benchmark. |
| AAN-006 | pose assets, facial animation, Live Link | **Compatible path / Not adopted external capture.** Generic morph/pose channel contract; device/service integration requires authorization. | AAN-001; generated pose channel test. |
| AAN-007 | physical animation and ragdolls | **Compatible path.** Animation/physics handoff, constraints and recovery to keyframed pose. | AAN-001/physics baseline; transition determinism test. |
| AAN-008 | groom/hair, cloth, ML Deformer and flesh | **Deferred.** Explicit specialized deformation register; no placeholder “character complete” claim. | AAN-001/AAN-007; re-enter with a synthetic stress fixture and budget. |

### 4.7 Collision, physics, navigation and AI systems

| ID | UE5 capability family specifically accounted for | Astral requirement / disposition | Dependencies and first bounded acceptance |
| --- | --- | --- | --- |
| APH-001 | collision channels, shapes, overlaps, sweeps and traces | **Core.** Layer/mask schema, primitive collider set, broad/narrow phase, hit ordering and debug draw. | AWS-001; deterministic query corpus. |
| APH-002 | rigid bodies, forces, constraints and solver | **Core.** Fixed-step body lifecycle, mass/inertia, joints, sleep/wake and explicit determinism limits. | APH-001/ARC-005; replay and energy/constraint test. |
| APH-003 | substepping, async physics and scene queries | **Core.** Scheduling contract and snapshot synchronization; no data race hidden behind a “fast” mode. | APH-002/ARC-004; multithread replay comparison. |
| APH-004 | destructibles, geometry collections and fields | **Deferred.** Fracture data and field-command seam; re-enter after APH-002 and streaming baseline. | APH-002/AWS-004; generated breakable fixture. |
| APH-005 | vehicle simulation | **Deferred.** Wheel/contact interface and input/physics trace requirements. | APH-002/input; test track fixture. |
| APH-006 | cloth, soft body, hair and flesh physics | **Deferred.** Specialized solver policy, budget and failure recovery; no game asset dependency. | APH-002/AAN-008; synthetic fabric/strand test. |
| AAI-001 | navmesh, navigation modifiers and dynamic rebuild | **Core.** Deterministic nav data, query API, partial rebuild and streaming-cell handoff. | AWS-004/APH-001; route corpus across load/unload. |
| AAI-002 | path following, crowd avoidance and smart links | **Compatible path.** Agent movement interface and debug trace. | AAI-001; two-agent corridor fixture. |
| AAI-003 | behavior trees, blackboards, state trees and EQS | **Core engine framework, no game behavior.** Data-driven decision graph/query abstractions, traceability and deterministic test agents. | AAI-001/ARC-009; scripted synthetic agent corpus. |
| AAI-004 | Mass/entity AI, perception and smart objects | **Deferred.** Re-enter after ECS and core navigation profile budgets demonstrate need. | AWS-002/AAI-001; population synthetic stress test. |

### 4.8 Audio, input, UI, accessibility and cameras

| ID | UE5 capability family specifically accounted for | Astral requirement / disposition | Dependencies and first bounded acceptance |
| --- | --- | --- | --- |
| AAU-001 | audio device, mixer, source/voice lifecycle and submixes | **Core.** Device enumeration, voice allocation, mixing, shutdown, mute/failure behavior and trace metrics. | ARC-001/004; generated tone mix test. |
| AAU-002 | attenuation, spatialization, occlusion, reverb and audio volumes | **Core.** Listener/source transforms, documented attenuation and fallback, deterministic occlusion query boundary. | AAU-001/APH-001; spatial sweep capture. |
| AAU-003 | sound cues, concurrency, streaming and modulation | **Core.** Data-driven event routing, concurrency budget, streamed source lifecycle and diagnostic counters. | AAU-001/AAP-003; voice-pressure/reload test. |
| AAU-004 | MetaSounds/sample-accurate DSP graphs | **Compatible path.** Original DSP graph/event interface, offline reference render and real-time safety contract. | AAU-001; generated graph determinism/audio hash. |
| AIN-001 | Enhanced Input actions, contexts, chords and rebinding | **Core.** Device-agnostic actions, mapping contexts, chord/hold/axis semantics, conflict detection, save/load rebind and window-scoped injection. | ARC-001/009; `A3D-03` and `A2D-01` input corpus. |
| AIN-002 | force feedback, haptics, raw input and HID devices | **Compatible path.** Capability detection/no-device fallback; external device testing is optional. | AIN-001; virtual/no-device test. |
| AUI-001 | Slate/UMG-like retained UI, immediate draw and layout | **Core.** Native widget/layout/render tree, focus/accessibility state, DPI scaling, input isolation and screenshot oracle. | ARR-002/AIN-001; independently running 2D/UI fixture. |
| AUI-002 | Common UI, input glyphs, menus and rich text | **Compatible path.** Style/theme/resource and glyph lookup seam, no game HUD implementation. | AUI-001/AIN-001; locale/text/glyph test. |
| AUI-003 | accessibility, screen reader, contrast and remapping | **Core.** Keyboard navigation, focus order, contrast/pref metadata, scalable text and rebind persistence. | AUI-001/AIN-001; automated focus/contrast checks. |
| ACA-001 | camera components, camera modifiers, shakes and cinematic camera | **Core engine camera.** Transform/projection, blend/modifier pipeline, deterministic capture. Cinematic effects are Compatible path. | AWS-001/ARR-004; camera sweep capture. |

### 4.9 Gameplay framework, save/replay and networking

| ID | UE5 capability family specifically accounted for | Astral requirement / disposition | Dependencies and first bounded acceptance |
| --- | --- | --- | --- |
| AGF-001 | gameplay tags, events, attributes, effects and abilities | **Core engine framework only.** Generic typed tag/event/attribute/effect data; no game abilities, combat content or balance tuning. | ARC-009/AIN-001; schema/serialization/event-order corpus. |
| AGF-002 | gameplay messages, subsystems and game feature plugins | **Compatible path.** Scoped service registry and feature activation lifecycle with safe disable. | ARC-002/004; activate/deactivate test module. |
| AGF-003 | save games and checkpoints | **Core.** Versioned save slot, atomic write/backup, corruption rejection, migration and deterministic restore. | ARC-009/AWS-001; save/reload/corruption corpus. |
| AGF-004 | replay, demos and deterministic playback | **Core.** Input/event/seed capture, replay header/version, divergence detection and visual/trace comparison. | ARC-005/009/AIN-001; same-seed replay fixture. |
| ANG-001 | actor replication, Replication Graph and Iris | **Deferred.** Network object identity, authority and snapshot boundary must be designed but not implemented before standalone determinism is proven. | AGF-004/AWS-001; loopback packet/replay prototype when re-entered. |
| ANG-002 | client prediction, rollback and networked physics | **Deferred.** Explicit latency/authority model and replay equivalence requirement. | ANG-001/APH-003; deterministic rollback fixture. |
| ANG-003 | dedicated server, sessions, online services, EOS and voice chat | **Not adopted now.** Requires external service/platform/privacy decisions. Keep a transport interface only. | Operator authorization plus security/privacy review. |

### 4.10 2D, VFX, cinematic, XR and specialized production systems

| ID | UE5 capability family specifically accounted for | Astral requirement / disposition | Dependencies and first bounded acceptance |
| --- | --- | --- | --- |
| A2D-001 | Paper2D sprites, flipbooks and sprite rendering | **Core.** Independent 2D executable, sprite resources, frame animation and camera/UI composition. | ARR-004/AAP-003; `A2D-01` visual oracle. |
| A2D-002 | tile maps, tile sets and 2D collision | **Core.** Versioned tiles/tilemap, batching, query bridge and deterministic map load. | A2D-001/APH-001; tile collision/load corpus. |
| A2D-003 | 2D particles/effects and 2D lighting | **Compatible path.** Batched particle data/lifetime and quality tiers. | A2D-001/ARR-002; fixed-seed particle capture. |
| AVF-001 | Niagara particle systems, emitters, modules and data interfaces | **Core engine VFX baseline.** Original data-driven emitter lifecycle, fixed seed mode, CPU/GPU budget metrics and debug view. | ARR-002/004/ARC-004; emission/kill deterministic fixture. |
| AVF-002 | Niagara fluids, collisions, lights, ribbons and mesh particles | **Compatible path.** Explicit modules only after AVF-001; no one-off effect code. | AVF-001/APH-001; module-specific fixture. |
| ACN-001 | Sequencer, tracks, takes and Control Rig integration | **Compatible path.** Timeline/track schema and deterministic offline playback; no content production. | AAN-003/ACA-001; timeline capture fixture. |
| ACN-002 | Movie Render Queue, high-quality capture and render passes | **Compatible path.** Offline deterministic capture, image metadata and trace link. | ARR-011/AIL-011; frame-sequence reproducibility test. |
| AXR-001 | OpenXR, VR, AR, XR input and stereo rendering | **Deferred.** Platform abstraction only; hardware integration needs a separate target matrix and safety review. | ARR-001/AIN-001; no-device feature-unavailable test. |
| APS-001 | Pixel Streaming, remote control and virtual camera | **Not adopted now.** Requires network/security/credential/remote-control policy. | Separate operator authorization and threat model. |
| AMD-001 | MetaHuman, DNA, Live Link, capture, ML Deformer and learning agents | **Not adopted / Deferred.** These entail external tooling, data rights or specialized ML/pipeline decisions. Record interfaces only where AAN-006/008 requires them. | Separate provenance/privacy/compute approval. |

### 4.11 Editor, developer tools, validation and quality systems

| ID | UE5 capability family specifically accounted for | Astral requirement / disposition | Dependencies and first bounded acceptance |
| --- | --- | --- | --- |
| AED-001 | editor shell, viewport, outliner, inspector and gizmos | **Core.** Minimal native developer editor for world/entity/property inspection and transform manipulation; runtime remains independently launchable. | AWS-001/AUI-001; edit-save-reload test. |
| AED-002 | asset browser, import UI, thumbnails and dependency views | **Core.** AAP registry-backed inspection/search/validation with no duplicated asset truth. | AAP-001/002/AUI-001; asset dependency navigation test. |
| AED-003 | Blueprint/visual scripting and graph editors | **Compatible path.** Generic node-graph data model; execution remains disabled until sandboxing/versioning and deterministic test rules exist. | ARC-002/009; serialize/validate a non-executable graph. |
| AED-004 | Python, scripting, commandlets and automation | **Core safe tooling.** Headless commands with strict arguments, machine-readable receipts and no hidden external access. Arbitrary scripting is Deferred. | AAP-007/testing; command contract test. |
| AED-005 | automation testing, functional tests, screenshot comparison and Gauntlet-like orchestration | **Core.** Unit/feature/smoke/stress/capture tiers, isolated native runner, failure artifacts and repeat-safe orchestration. | ARC-007/A3D/A2D; runner self-test. |
| AED-006 | Unreal Insights, stats, GPU profiler, memory insights and CSV profiler | **Core.** Trace channels, spans/counters, frame/memory/GPU samples, markers, export and scenario linkage. | ARC-004/007/ARR-011; trace parser test. |
| AED-007 | Visual Logger, debug draw, data validation and map check | **Core.** Inspectable live debug primitives, structured validation issues and stable suppressions. | AWS-001/APH-001/AAP-007; diagnostic capture fixture. |
| AED-008 | source control, multi-user/Concert, review and diff tools | **Compatible path.** Git-aware read/diff/status and review artifact links; live collaboration is Not adopted until external service/identity rules are approved. | AWS-005/AAP-008; conflict/read-only review test. |
| AED-009 | build graph, BuildPatch, platform packaging and crash reporter | **Core desktop.** Reproducible CMake build, test, package, launch and crash receipt; patching/platform stores Deferred. | AAP-006/ARC-007; clean package launch. |
| AED-010 | security, privacy, telemetry and compliance | **Core.** No secret capture; explicit opt-in boundary for telemetry; sanitised logs, dependency/provenance inventory and fail-closed external actions. | ARC-007/AAP-007; redaction and external-action-denial tests. |

## 5. Dependency order and implementation waves

The ledger is broad; execution is deliberately thin. A later wave cannot
replace evidence required by an earlier one. Each wave retains a useful 3D and
2D test fixture, so engine breadth never turns into a disconnected survey.

| Wave | Required ledger outcomes | Exit packet evidence | What it unblocks |
| --- | --- | --- | --- |
| 0 — truth and reproducibility | ARC-001, 003, 005–007; baseline audit of all rows | clean CMake Debug/Release builds; exact test census; baseline scenario receipt; current Present/Partial/Missing ledger | trustworthy implementation selection |
| 1 — runtime/data/world | ARC-002, 004, 009; AWS-001–004 | deterministic entity/serialization/job/cell lifecycle and load/unload evidence | renderer/asset/physics integration |
| 2 — GPU foundation | ARR-001–004, 006, 009; AIL-001/002/006; ACA-001 | graph/shader/material/mesh/light/shadow capture and resource metrics | visible native 3D foundation |
| 3 — asset and world scale | ARR-007, AAP-001–007, AWS-005–008 | import/reimport/cache/package and streamed synthetic-world stress | real asset loading and large-scene work |
| 4 — motion, interaction and sound | AAN-001–003; APH-001–003; AAI-001/003; AAU-001–003; AIN-001; AGF-001/003/004 | replayable animation/physics/nav/audio/input fixture | engine-complete interactive runtime, not game content |
| 5 — true 2D and developer velocity | A2D-001/002; AUI-001/003; AED-001/002/005–010 | independent 2D executable; editor/tool traces; package launch evidence | acceptance-scale iteration and QA |
| 6 — quality and optional capability paths | AIL-004–014, ARR-005/010, AAN-004–008, AVF, ACN and all Compatible/Deferred re-entry rows | feature-specific budget/correctness fixtures and explicit adoption decision | scalable mature-engine capabilities without speculative work |
| 7 — engine acceptance | every Core row Present; every Compatible/Deferred row has a valid disposition and re-entry condition | full Debug/Release suite; 3D+2D integration; package launch; stress matrix; frozen `AST-24H`; independent acceptance | only then, an operator may decide whether game work resumes |

## 6. Packet selection algorithm

The director must select the next task mechanically enough that “we ran out of
things to do” is impossible while a Core row is incomplete.

1. Read the current capability ledger, last accepted receipt, current source
   and failure/review history.
2. Mark a row **eligible** only when all dependencies are Present or a bounded
   synthetic seam can prove the missing dependency independently.
3. Rank eligible work: (a) a reproducible failing Core check; (b) a Core row
   that blocks the most later Core rows; (c) an acceptance/observability gap
   for an implemented row; (d) a Compatible row whose recorded re-entry
   condition has become true.
4. Select one smallest non-overlapping source change. A packet must state:
   capability IDs; starting evidence; expected semantic delta; exact
   clean-worktree parent; owned paths; Debug/Release command sequence;
   scenario/input seed; measurable pass/fail thresholds; stop condition; and
   a read-only Builder B acceptance task.
5. If the first packet blocks, do not replay its identical command or transfer
   its dirty worktree to another writer. Diagnose the root cause, preserve
   receipts, issue one bounded repair in a fresh registered worktree, or select
   the highest-ranked independent eligible row.
6. When a packet passes, write its exact evidence into a ledger update and
   queue the verifier before any new writer. The verifier either accepts the
   row, writes the smallest correction packet, or leaves a true hard gate
   visible.

## 7. Perpetual unblocking contract

### 7.1 Named roles

| Role | Durable responsibility | May autonomously repair | Must retain/park |
| --- | --- | --- | --- |
| **Astral Project Director** | owns ordering, review resolution and one runnable frontier | stale task packets, invalid task shape, missing acceptance details, ordinary dependency routing and independent-work selection | hard external/safety gates; never claims readiness from research |
| **Builder A** | sole writer for one admitted engine packet | source/test/build defects within exact task authority | dirty worktree, unowned paths and failed evidence |
| **Builder B** | independent verifier and next-packet author | acceptance gaps, reproducible test failures and narrower repair packets | source edits, merging and unsupported claims |
| **Goal Worker** | reads agenda/ledger and proposes one bounded source-grounded packet | a missing planned next step or audit stale source paths | ambiguous source evidence; escalates through native executor/inspector path |
| **Loop Operations Supervisor (Unblocking Agent)** | watches the durable scheduler, project frontier and incidents | restart-safe queue recovery, clean-worktree replacement, task/result context repair, goal-work replenishment and a verified local runtime repair | credentials, licenses, paid services, destructive actions, external publication, main merge/push/release and safety-critical bypasses |

The Loop Operations Supervisor is the explicit unblocking agent requested for
Astral. It is not a second scheduler and it does not fabricate work. It uses
the same Company Runtime, project task identities and typed receipts as the
director. The runtime must keep it desired/active, run its event-driven audit,
and record a quiet status when no material change is needed.

### 7.2 No-frontier recovery ladder

When Astral has no runnable task, the runtime and director apply this ladder in
order. Each action creates evidence and changes the hypothesis; no identical
failure command repeats unchanged.

| Condition | Autonomous action | Verification / next state |
| --- | --- | --- |
| runnable task exists | do not manufacture a duplicate; execute/verify it | frontier remains healthy |
| active task is stalled but has a reproducible internal cause | issue one minimal repair packet in a clean worktree or use project-owned repair workflow | focused test plus exact receipt |
| task packet is malformed, stale, lacks a valid path, or result context is too large | preserve original task/logs; normalize/parse/reissue a bounded inspector or clean successor | server admission/receipt proves the new task is executable |
| worktree/build directory is dirty, missing or mismatched | preserve it; allocate/recreate only the admitted clean worktree/external build root | Git/path admission and fresh configure receipt |
| a reviewer rejects evidence | retain review history; Builder B writes a correction packet; director dispatches it | independent evidence, not a self-approval |
| one capability is genuinely blocked | select the highest-ranked independent eligible Core row | new task has disjoint ownership and dependencies |
| agenda source changed or lacks a next packet | goal worker audits the current ledger and emits one bounded packet; native executor fallback produces baseline evidence if planning fails | agenda/receipt fingerprint changes, then verifier selects work |
| every Core row is Present but final evidence is incomplete | schedule the missing stress, package, capture or independent-acceptance packet | final-gate report, not a “done” claim |
| only an external/human boundary remains | create a visible parked intervention with exact authority required; continue any disjoint local engine work | quiet monitoring; no busy retry loop |

### 7.3 Anti-stall invariants

- Maintain at least one admitted ready/retry/running task while a Core or
  required-acceptance row remains incomplete, subject to the serial writer and
  native-executor limits.
- Reconcile after startup, task settlement, review/block, changed agenda source
  and the active-wake interval. These are event-driven checks, not uncontrolled
  polling.
- Treat historic blocked/review items as evidence. Do not mass-close, silently
  relabel, or replay them; create an exact recovery task that points to the
  prior ID and states the changed hypothesis.
- A no-progress event must produce one of: a runnable packet, a verified repair,
  an independent eligible capability packet, or a precise hard-boundary
  intervention. “Monitoring,” “research complete,” and “waiting” alone are not
  acceptable outcomes while local work remains.
- If the work reserve/agenda exhausts, its fallback is a native baseline build
  and test receipt followed by a result inspector that selects the next
  capability. A successful baseline proves only that baseline; it never
  implies engine or playtest readiness.
- Keep a short evidence-oriented frontier snapshot: current capability ID,
  current task/run, last accepted receipt, earliest dependent gap, next
  independent capability, and any parked hard boundary.

## 8. Review and change-control rules

1. Add or split a row when a UE5-family capability has a distinct lifecycle,
   data model, platform gate, budget or acceptance method. Do not hide it in
   “miscellaneous.”
2. To defer a row, record why it is not currently Core, its dependencies, its
   re-entry condition, and the diagnostic that will show it is now needed.
3. To mark a row Present, link exact source paths, receipt identifiers,
   scenario ID, thresholds/results, candidate commit and independent verdict.
4. A capability introduction cannot weaken the full test suite, 3D/2D
   requirements, package launch evidence, or the final 24-hour soak.
5. Any renderer/platform feature must name supported hardware/API/driver
   behavior and a safe feature-unavailable fallback.
6. Vendor SDKs, online services, marketplace plugins, downloaded assets,
   capture devices and platform store programs stay out of scope until their
   separate provenance, privacy, security and operator-approval gates pass.

## 9. Completion definition

Astral Engine is not “ready” because a feature checklist exists. It becomes
eligible for independent engine acceptance only when:

1. every **Core** ledger row has exact Present evidence on the frozen candidate;
2. all Compatible/Deferred/Not-adopted rows have a current, explicit
   disposition and no deferred item is secretly required by the test fixture;
3. Debug and Release builds pass the complete applicable test suite;
4. `A3D-01` through `A3D-04` and the independently launched `A2D-01` pass with
   defined CPU/GPU/RAM/VRAM/load budgets and retained captures/traces;
5. import/reimport, cell load/unload, resize/minimize/restore, invalid input,
   device/failure recovery, package launch and leak diagnostics pass;
6. the frozen candidate completes `AST-24H` with retained crash/hang/leak and
   frame/resource distributions; and
7. an independent verifier accepts the exact evidence bundle.

Only after that could an operator choose to resume game content and eventually
request a human playtest. This plan makes the path autonomous; it does not
pretend that a future human testing decision or external release boundary can
be automated away.
