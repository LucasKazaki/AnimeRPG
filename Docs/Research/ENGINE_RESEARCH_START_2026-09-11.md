# Engine research starting evidence — 2026-09-11

Engine-only research under `GAME_DEVELOPMENT_CONTROL.md`. These official pages were opened on 2026-09-11. This bibliography is a starting assignment, not a completed Astral audit or readiness proof. Record page versions when revisiting these living documents.

| Reference | Investigation |
| --- | --- |
| [Unreal Engine 5 World Partition](https://dev.epicgames.com/documentation/en-us/unreal-engine/world-partition-in-unreal-engine) | Study spatial streaming and world-loading workflows; measure load/unload, memory and correctness on synthetic 3D fixtures. |
| [Unreal Automation Test Framework](https://dev.epicgames.com/documentation/unreal-engine/automation-test-framework-in-unreal-engine) | Epic distinguishes unit, feature, smoke, stress and screenshot-comparison tests. Adapt these layers to Astral's native runner and keep tests independent of prior state. |
| [Godot internal rendering architecture](https://docs.godotengine.org/en/stable/engine_details/architecture/internal_rendering_architecture.html) | Compare renderer/backend boundaries, rendering methods and 2D/3D paths. Choose based on measured hardware/workload tradeoffs. |
| [Unity profiling and native debugging tools](https://unity.com/how-to/profiling-and-debugging-tools) | Study complementary engine/native profiling and frame debugging; define CPU, GPU and memory evidence for the benchmark matrix. |

Next local packet: inspect current engine sources and references; produce a dated Present/Partial/Missing matrix with measurable acceptance criteria; select the first unresolved dependency. Expand with official UE5 rendering, animation and asset-pipeline documentation when the corresponding decision needs it. Carry findings into bounded implementation/test packets instead of repeating surveys. New web evidence uses the registered source-browser role. Local planning can use this saved starting evidence immediately.
