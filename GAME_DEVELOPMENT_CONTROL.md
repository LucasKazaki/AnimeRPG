# Astral Engine Development Control

Operator direction, 2026-09-11. This supersedes older game-slice, art, audio, playtest and engine-and-game instructions, including retained packets and historical loop status.

ENGINE-FIRST, LOCAL-ONLY. Current operator direction (2026-09-11) supersedes older game-slice and blanket no-research instructions. Build and stress-test the custom native C++17 Astral Engine: 3D is primary, with genuine 2D game support. Low-level code such as C is desired; preserve the existing native implementation instead of inferring a whole-engine C-only rewrite. Use registered local specialist models with careful bounded reasoning and independent local QA; no ordinary cloud-model fallback. Research Unreal Engine 5, Unity and Godot through public primary sources and the registered source-browser; engine research is explicitly allowed. Read GAME_DEVELOPMENT_CONTROL.md and Docs/Research/ENGINE_RESEARCH_START_2026-09-11.md. Maintain a source-backed capability matrix and implement dependencies in order: runtime/jobs/memory; 3D scene/ECS/serialization; GPU rendering/materials/lighting/streaming; asset pipeline; animation; collision/physics/navigation; audio engine; input/actions/save-replay; 2D support; profiling/editor/tooling/packaging. Pause gameplay, combat tuning, encounters, narrative, game art/assets, audio content and game playtests. Existing 2D AI pictures are historical placeholders and cannot prove 3D readiness. Procedural 3D/2D fixtures are permitted for engine testing. Require Debug/Release tests, real 3D and 2D runtime evidence, measured frame-time/RAM/VRAM budgets, load/unload and failure-recovery stress tests, a final 24-hour soak and independent engine acceptance before game work; game work remains paused until the operator resumes it. Preserve registered worktree ownership, exact native task/job receipts, serial code writers and all existing merge, push, release, destructive-action, credentials, external-service and licensing gates. Do not import Unreal or claim UE5 parity. Use the existing durable scheduler and turn failures into concrete engine repairs.

The future game is a 3D anime action RPG inspired by the quality of Genshin Impact, Zenless Zone Zero and Solo Leveling. These are capability references, not permission to copy content. Flat images or slideshows cannot pass a 3D gate: require actual geometry, depth, camera motion, materials and animation in the native executable. Preserve old 2D placeholder art; stop generating more game art now. Synthetic meshes, rigs, signals and 2D sprites are allowed only as engine test fixtures.

Local-only means model inference stays local; public engine documentation research through the registered browser is authorized. Existing GDI output is a baseline, not the rendering ceiling. Improve the native C++17 engine incrementally, including GPU renderer development; do not infer a wholesale C-only rewrite or migration to Unreal/Unity.

Maintain a capability matrix with requirement, reference URL/version/date, local source evidence, Present/Partial/Missing, dependency, bounded packet, measurable correctness/performance/memory criteria and independent acceptance. Research each gap, decide and implement; avoid repeated surveys. Each builder and verifier must read this control before an older packet. Scope does not authorize a second scheduler or bypass external-action gates.

Engine completion requires every project-critical row implemented and independently accepted on the exact candidate. Capture Debug and Release builds, the full applicable test suite, true 3D integration and an independently runnable 2D fixture, reproducible native captures, and packaged-launch evidence. Define hardware/resolution/workload-specific CPU/GPU frame-time, RAM/VRAM and load-time budgets before benchmarking; report distributions and peaks.

Stress tests cover increasing geometry/entities/lights/animation and 2D batching, repeated scene load/unload and import/reimport, resize/minimize/restore, invalid inputs, supported failure recovery and resource leaks. Run short checks while iterating and at least a 24-hour final mixed-workload soak on the frozen candidate before final acceptance. Retain crash/hang/leak metrics; elapsed time alone proves nothing. Failures require repairs and exact reruns, never weaker assertions. These are required future gates, not claims that features or tests already exist.

Use one registered worktree per implementation packet with parent commit, explicit file ownership and separate external Debug/Release roots. Keep the local writer/reviewer relay, retained task/job identities and ordinary autonomous project debugging. Advance an independent engine dependency if another is blocked; never fall back to game art or content. Existing game tests may remain regression checks. Game work stays paused even after engine acceptance until the operator resumes it.

## Capability ledger and autonomous frontier

`Docs/Planning/ASTRAL_UE5_CAPABILITY_ROADMAP.md` is the active, detailed
capability ledger. It is paired with the dated source register in
`Docs/Research/UE5_CAPABILITY_REFERENCE_2026-09-18.md` and the active milestone
view in `Docs/Planning/MILESTONES.md`. The director, goal worker and verifier
must use those documents before selecting a packet. The ledger's **Core**,
**Compatible path**, **Deferred** and **Not adopted** dispositions explicitly
account for mature UE5 capability families without claiming parity or copying
Unreal technology.

The Company Runtime's Loop Operations Supervisor is Astral's unblocking agent.
It and the Project Director must keep an evidence-backed runnable frontier
while a Core capability or an engine-acceptance gate remains incomplete. On a
routine internal block they preserve the task and receipts, change the
hypothesis, make one bounded local repair in an admitted clean worktree, verify
it, or advance an independent eligible engine capability. A blocked/review row
is historical evidence, never a reason to mass-close work or replay an
unchanged command. Only genuine external/safety boundaries—credentials,
licensing, paid/external services, destructive actions, merges, pushes,
releases, publication, or a required human verification—may be parked for
operator action. Quiet event-driven monitoring is correct only when no material
local repair or eligible engine packet remains.
