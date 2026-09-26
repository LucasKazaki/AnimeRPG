# Unreal Engine 5 capability reference for Astral — 2026-09-18

## Purpose and boundary

This is a dated, source-backed reference survey for the custom C++17 Astral
Engine. It is **not** a claim that Astral is UE5-compatible, feature-complete,
or licensed to use Unreal technology. It does not authorize importing Unreal,
copying implementation details, downloading third-party code or assets, or
changing the engine's native/local-only constraints.

The useful question is narrower: *which mature-engine capabilities must Astral
account for, and which evidence would show that a selected Astral capability is
real?* The companion execution document is
`Docs/Planning/ASTRAL_UE5_CAPABILITY_ROADMAP.md`. Every row in that ledger gets
one of four planning dispositions:

- **Core** — required before Astral engine acceptance.
- **Compatible path** — a stable seam and fixture are required now; the full
  subsystem may be scheduled only when the capability has a project need.
- **Deferred** — explicitly recorded with a prerequisite, an owner, and a
  re-entry condition; it is not silently forgotten.
- **Not adopted** — outside the Windows-native/local-only project boundary or
  requires a service, platform program, rights decision, or product scope that
  the operator has not authorized.

“Outlined” means the roadmap names the capability, its disposition,
dependencies, measurable acceptance, and the next bounded packet. “Present”
means the exact Astral candidate has source, automated checks, native runtime
evidence, and independent acceptance. A documentation page, a placeholder, or
a successful compile is never enough.

## Research method

- **Research date:** 2026-09-18.
- **Reference family:** Epic Games' current Unreal Engine 5 documentation.
  The living documentation currently labels this UE5.8; page availability and
  wording can change, so a future worker must record the version/date it
  inspected rather than treating this survey as eternal.
- **Evidence rule:** links below support capability categories and acceptance
  design. They do not prove that a proprietary Epic implementation is suitable
  for Astral or that Astral should reproduce it.
- **Scope rule:** the project remains Windows, C++17, native, engine-first,
  local-model operated, and game-content paused under
  `GAME_DEVELOPMENT_CONTROL.md`.

## Primary-source findings

| UE5 surface | What the official reference establishes | Astral planning implication |
| --- | --- | --- |
| Rendering paths | The desktop feature matrix differentiates deferred and forward paths and gates Nanite, Lumen, virtual shadow maps, temporal upscaling and path tracing by API/shader-model support. | Astral must have a written backend/feature matrix, capability detection, deterministic fallbacks, and separate correctness/performance fixtures rather than one renderer “on/off” claim. |
| Virtual geometry and shadows | Nanite-style virtualized geometry and Virtual Shadow Maps couple geometry, shadow pages, caching, movable lighting and world streaming. | Treat geometry streaming, culling, shadow allocation and cache invalidation as independently testable capabilities. A simple static mesh renderer is only a seed. |
| Large worlds | World Partition uses grid cells, streaming sources, one-file-per-actor, data layers, HLOD and level instances; PCG can participate in partitioned worlds. | Astral needs an entity/cell lifecycle, stable IDs, async load/unload, visibility/HLOD, authoring data layers, and a deterministic synthetic-world stress fixture before claiming open-world readiness. |
| Animation | Motion Matching is query-driven from Pose Search databases and trajectory/bone features; UE also exposes graph, state-machine, IK, rigs, compression and physics-adjacent animation systems. | Build the conventional skeletal/graph/IK baseline first. Preserve a data-query seam so pose-search/motion matching can be added without replacing animation ownership. |
| Physics | Chaos documentation groups collision, constraints, rigid bodies, destruction, cloth, vehicles, fields, async/networked physics and visual debugging. | Implement deterministic collision/query and fixed-step rigid-body foundations before optional destruction, vehicle, cloth, hair, flesh or networking paths. Every optional path remains in the ledger. |
| Audio | MetaSounds documents sample-accurate DSP graphs, asynchronous render, reusable graphs and event-driven control; UE audio also has mixing, attenuation, spatialization, buses and streaming. | Separate runtime audio device/mixer/voice lifecycle from authored graph/DSP capabilities. Use generated signals for engine tests; game music/VO remain paused. |
| Gameplay framework | The Gameplay Ability System combines abilities, attributes, effects, gameplay tags and events. | Astral needs a generic engine-side event/tag/attribute/serialization contract, but no game abilities or combat content while game work is paused. |
| Observability | Unreal Insights/Trace captures timing, counters, bookmarks, frame data, screenshots and channel-controlled events. | Astral must make every engine acceptance claim observable: structured trace events, CPU/GPU/memory counters, captures, reproducible scenario IDs, and exportable receipts. |

## Official-source register

| ID | Source | Planning use |
| --- | --- | --- |
| UE5-R01 | [Supported Features by Rendering Path for Desktop](https://dev.epicgames.com/documentation/unreal-engine/supported-features-by-rendering-path-for-desktop-with-unreal-engine) | Renderer/path/API capability matrix; DX12/SM6 and fallback discipline. |
| UE5-R02 | [Rendering Features Reference](https://dev.epicgames.com/documentation/unreal-engine/rendering-features-reference) | Taxonomy for renderer, lighting, post, reflection, shadow and platform coverage. |
| UE5-R03 | [Rendering Settings](https://dev.epicgames.com/documentation/unreal-engine/rendering-settings-in-the-unreal-engine-project-settings) | Settings/configuration provenance and runtime feature gating. |
| UE5-R04 | [Virtual Shadow Maps](https://dev.epicgames.com/documentation/unreal-engine/virtual-shadow-maps-in-unreal-engine) | Shadow-page/cache, movable-light, Nanite/Lumen/world-streaming dependencies. |
| UE5-R05 | [Shadowing](https://dev.epicgames.com/documentation/unreal-engine/shadowing-in-unreal-engine) | Shadow technique taxonomy and per-light policy. |
| UE5-R06 | [World Partition](https://dev.epicgames.com/documentation/unreal-engine/world-partition-in-unreal-engine) | Cell streaming, streaming sources, OFPA and data ownership. |
| UE5-R07 | [World Partition Data Layers](https://dev.epicgames.com/documentation/unreal-engine/world-partition---data-layers-in-unreal-engine) | Authoring/runtime data-layer separation. |
| UE5-R08 | [World Partition HLOD](https://dev.epicgames.com/documentation/unreal-engine/world-partition---hierarchical-level-of-detail-in-unreal-engine) | Hierarchical visibility and large-world representation. |
| UE5-R09 | [PCG with World Partition](https://dev.epicgames.com/documentation/unreal-engine/using-pcg-with-world-partition-in-unreal-engine) | Procedural content partition interaction and deterministic regeneration questions. |
| UE5-R10 | [Motion Matching](https://dev.epicgames.com/documentation/unreal-engine/motion-matching-in-unreal-engine) | Pose database/query and trajectory-driven animation planning. |
| UE5-R11 | [Physics in Unreal Engine](https://dev.epicgames.com/documentation/unreal-engine/physics-in-unreal-engine) | Physics/Chaos feature family and optional-subsystem boundaries. |
| UE5-R12 | [Physics Settings](https://dev.epicgames.com/documentation/unreal-engine/physics-settings-in-the-unreal-engine-project-settings) | Fixed-step/substep, collision, async and determinism configuration questions. |
| UE5-R13 | [MetaSounds](https://dev.epicgames.com/documentation/unreal-engine/metasounds-the-next-generation-sound-sources-in-unreal-engine) | Event-driven/sample-accurate audio graph reference. |
| UE5-R14 | [Unreal Insights Trace Quick Start](https://dev.epicgames.com/documentation/unreal-engine/trace-quick-start-guide-in-unreal-engine) | Trace channels, timing, bookmarks, snapshots and screenshot evidence. |
| UE5-R15 | [Gameplay Ability System](https://dev.epicgames.com/documentation/unreal-engine/gameplay-ability-system-component-and-gameplay-attributes-in-unreal-engine) | Ability/event/attribute/tag separation for an engine-owned framework. |
| UE5-R16 | [Hardware and Software Specifications](https://dev.epicgames.com/documentation/unreal-engine/hardware-and-software-specifications-for-unreal-engine) | Record actual target hardware, toolchain and API assumptions before budgets. |
| UE5-R17 | [UE5 Migration Guide](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-migration-guide) | Compatibility/version transition risks; document contract migrations and conversion tests. |
| UE5-R18 | [UE5.0 Release Notes](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5.0-release-notes?application_version=5.0) | Historical feature taxonomy; do not use as current behavior proof. |

## Gaps this survey does not hide

The official pages are a capability map, not an exhaustive implementation
specification. Before a packet enters execution, the assigned research or
architecture packet must additionally capture:

1. the current local Astral source and test evidence;
2. target GPU/driver/OS/toolchain and minimum supported configuration;
3. public licenses/provenance for any format, SDK or dependency considered;
4. a small original or synthetic fixture and a deterministic oracle;
5. a measurable CPU, GPU, RAM, VRAM, load, latency or authoring budget;
6. fallback behavior for unavailable hardware or unsupported content;
7. independent acceptance criteria and regression coverage.

## Update protocol

When a worker researches a new UE5 feature, append a short dated row here or
to a follow-on survey with: canonical URL, version/date, claim boundary,
affected roadmap IDs, local decision, and a proposed test. Do not replace this
survey with an unbounded prose recap, and do not revive a deferred feature
without satisfying its recorded re-entry condition.
