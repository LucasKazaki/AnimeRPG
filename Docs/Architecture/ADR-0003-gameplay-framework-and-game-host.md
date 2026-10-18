# Architecture Decision 0003 — Gameplay framework, data-driven scenes and the game host

Date: 2026-09-29
Status: Proposed on branch `claude/loving-clarke-el24od`. Lucas approves any merge.
Builds on: ADR-0001 (custom C++17 engine) and ADR-0002 (portable runtime, CPU
renderer, opt-in presentation).

## Context

After ADR-0002 the engine had subsystems (rendering, physics, animation, audio,
VFX, input, AI, an ECS) but nothing a game could be *made from*. There was no
world that ran them together, no components or scripts, no scene files, no
asset import and no player. Every UE5 or Unity project relies on that layer: a
`UWorld` or Unity scene; actors or GameObjects with components; Blueprint or
MonoBehaviour scripting; levels and prefabs; glTF/FBX import; Sequencer or
Timeline; UMG or uGUI; a developer console; a packaged player. The request of
2026-09-29 was to make Astral "a workable game engine" with those features.

## Decision

1. **Services.** `Engine/Core` gains JSON, console variables and commands
   (CVars), delegates and an event bus, and runtime reflection (`TypeRegistry`).
   Reflection is what makes components, materials and scripts authorable as data.
2. **Assets.** `Engine/Assets` imports and exports glTF 2.0 (meshes,
   materials, textures, skins, animations) and decodes PNG, JPEG and WAV.
   `AssetManager` handles:
   - typed, cached, reference-counted loads, synchronous or async on the job
     system;
   - hot reload via change polling, and collection of unreferenced assets;
   - path containment (no absolute paths or `..`).
3. **Gameplay framework.** `Engine/Framework` provides:
   - `GameWorld`, which owns the registry, physics, particles, timers and events.
     Its `Tick` phases are fixed and documented: start, fixed steps with physics
     and collision callbacks, timers, update, animation, late update,
     presentation, deferred destroy.
   - Reflected components: MeshRenderer, Light, Camera, Collider, RigidBody,
     CharacterMover, AudioSource/Listener, ParticleSystem, Animator.
   - Native `Behaviour` scripts (lifecycle and collision callbacks, `Invoke`),
     registered by name with reflected properties.
   - `SceneSerializer`: JSON scenes with inline or file prefabs, key-wise
     overrides and glTF model expansion. Parsing is transactional, errors name
     the JSON path, and `Save` round-trips.
   - Physics gained static triangle-mesh colliders (BVH, internal-edge
     handling) so imported levels collide.
4. **Content tools inside the runtime.**
   - Tweens with 31 eases.
   - Sequencer timelines (float/vector/rotation/event/activation tracks)
     bound to entities by name.
   - A retained widget UI with layout, focus and navigation.
   - An in-game console over the CVar registry.
   - QEM mesh simplification with screen-size LOD groups.
5. **Game host and players.** `Framework::GameHost` is the shell around a world
   (UE's GameInstance and viewport client, Unity's player loop).
   - A project file names the startup scene, the input map and config.
   - Each frame routes input console → UI → input actions, then ticks.
   - Scene changes (`LoadSceneRequest`) and hot reloads apply between frames,
     transactionally.
   - Rendering draws the world, then the HUD and console.
   - Two thin platform layers use it: `Tools/AstralPlayer` (headless, scripted
     input and recordings, PNG captures) and `Tools/AstralPlayerWin32`
     (window, keyboard, mouse, waveOut, F9 input recording).
6. **A sample project proves the loop.** `Content/Samples/Playground` (data)
   and `Game/Samples/Playground` (seven behaviours) make a small third-person
   game using only engine features. `EngineSampleTests` plays it through the
   real host.

## Consequences

- The National Mall game, its Win32 loop, the GDI default renderer and every
  smoke contract are unchanged. None of this code runs in `AstralGame`. The
  sample behaviours are compiled into the tools/test library, never the game.
- Two engine behaviour fixes came out of the sample and apply everywhere:
  - Kinematic bodies are no longer clamped to the dynamic speed limit when
    driven by `MoveKinematic`, and teleported characters move their bodies
    directly. Before, a teleport lagged for several steps and triggers fired late.
  - `CharacterController::Launch` (UE's LaunchCharacter) was added.
- MSVC x64 Release (14.44) miscompiled the branchy clamping in
  `Math::ClosestPointsSegmentSegment`. It is now straight-line code, validated
  against the old form on two million random cases. Regression tests pin the
  exact inputs.
- Behaviours are native C++. There is no script VM and no visual scripting; the
  reflected-property registry is the hook a future editor or script binding
  would use.
- The editor (E11) is not yet a scene editor. Scenes are authored as JSON, with
  console, hot reload and players as the iteration loop. A UE/Unity-class
  editor, a GPU backend, streaming and networking remain the largest gaps
  (`Docs/Research/ENGINE-CAPABILITIES.json`).
- No dependency, plugin, network access or downloaded asset was added.
