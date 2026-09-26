# Astral Engine milestones, acceptance gates and historical slice record

**Active scope:** engine-first, local-only. This file supersedes its former
game-content-oriented backlog for planning purposes. The controlling source is
`GAME_DEVELOPMENT_CONTROL.md`; the detailed UE5-reference capability ledger is
[`ASTRAL_UE5_CAPABILITY_ROADMAP.md`](ASTRAL_UE5_CAPABILITY_ROADMAP.md).

## Active milestones

| ID | Milestone | Required exit evidence | Dependency |
| --- | --- | --- | --- |
| E0 | Evidence and reproducibility baseline | Current Present/Partial/Missing ledger; exact CMake/Debug/Release test census; hardware/config capture; trace/receipt schema | None |
| E1 | Runtime, jobs, ownership and serialization | Deterministic jobs/tick/lifetime/serialization tests; clean shutdown and error evidence | E0 |
| E2 | World, entities and streaming | Stable entity/cell lifecycle, load/unload, layers, scene precision and HLOD/culling seam evidence | E1 |
| E3 | GPU renderer and image foundation | D3D12 capability matrix, render graph, shader/material/mesh lifecycle, camera, direct lights, baseline shadows/post, 3D capture | E1 |
| E4 | Asset and package pipeline | Import/reimport, registry/dependencies, cache, async data/package read, validation, standalone package launch | E1, E3 |
| E5 | Motion, interaction and sound systems | Skeletal graph, collision/physics, navigation/AI framework, audio mixer, action mapping, save/replay evidence | E1–E4 as applicable |
| E6 | Genuine independent 2D and developer tooling | Separately runnable 2D sprite/tile/UI fixture; editor inspection; automation/trace/validation tools | E1, E3, E4 |
| E7 | Mature capability paths | Each eligible lighting/streaming/VFX/animation/physics/cinematic Compatible row has measured adoption or an explicit maintained deferment | E2–E6 |
| E8 | Engine stress and independent acceptance | Full Debug/Release regressions; 3D and 2D integration; budget distributions; package launch; mixed-workload 24-hour soak; independent verdict | E0–E7 |

## Execution rule

The active director must select work through the roadmap's packet-selection
algorithm. A milestone is not completed by a task title, a build or a report.
It exits only with exact source, test, runtime, performance and independent
review evidence. When the current work queue empties, the Loop Operations
Supervisor and the director must create a repair, acceptance or next eligible
capability packet according to the roadmap's no-frontier recovery ladder.

Game milestones, game art/audio, gameplay tuning and player playtests remain
paused. Engine test fixtures may use original procedural/synthetic content.

## Historical vertical-slice record — non-authoritative while engine-first

The entries below preserve historic receipt references; they do not authorize
game work or establish current engine readiness.

| Historical ID | Retained claim | Current use |
| --- | --- | --- |
| M1 | Native window/core loop, input, logs, timing and clear color were developed. | Regression evidence only; audit under E0/ARC-001/005/007. |
| M2 | Static mesh/camera/transforms/assets/debug grid were explored. | Partial renderer/scene seed; audit under E2/E3. |
| M3 | Third-person controller/camera packet has retained evidence. | Historical input/physics regression only; no game progression. |
| M4 | Deterministic combat sandbox packet has retained evidence. | Historical test code only; no combat development. |
| M5 | Perspective wireframe blockout packet has retained evidence. | Synthetic 3D fixture seed only; audit as E2/E3 evidence. |
| M7 | Shadowblade action kit packet has retained evidence. | Historical regression only; no game ability/content work. |

The historic M6 and M8–M15 game-content backlog is deliberately parked rather
than deleted. It cannot be selected until E8 has an independently accepted
engine candidate and the operator explicitly reopens game production.
