# Engine audit and uplift: AnimeRPG + AnimeRPG-UE5

Date: 2026-09-28. Branch: `claude/loving-clarke-el24od` in both repositories.
Bases: AnimeRPG `main` at `2958741`, AnimeRPG-UE5 `master` at `b0a7062`.
Task packet: [Tasks/ENGINE-UPLIFT-2026-09-28.md](../../Tasks/ENGINE-UPLIFT-2026-09-28.md).
Evidence: [Docs/QA/ENGINE-UPLIFT-2026-09-28.md](../QA/ENGINE-UPLIFT-2026-09-28.md).
Decision record: [ADR-0002](../Architecture/ADR-0002-portable-runtime-and-astral-renderer.md).

## Summary

Lucas asked for the engine/game pair to be audited and for Astral Engine to be
brought toward Unreal Engine 5 and other top-tier engine levels, while keeping
what makes the game special.

**Verdict: this is not UE5 parity, and no parity claim is made.** Astral Engine
went from a GDI wireframe loop with small gameplay domains to a portable,
sanitizer-tested runtime. The runtime covers these areas:

- Jobs, profiling and time dilation.
- Entity-component-system (ECS) world and serialisation.
- A software 3D renderer with anime shading, shadows, reflections and post.
- Rigid-body physics, a character controller and destruction.
- Skeletal animation with state machines, IK and skinning.
- Particles and trails.
- An action-input layer.
- An audio mixer.
- Navigation and behaviour trees.
- A UI canvas and genuine 2D.

The game runs on it through an opt-in Astral mode (F2) over the unchanged
gameplay rules. The largest remaining gaps against UE5 and Unity are these:

- A GPU rendering backend.
- An asset import and cooking pipeline.
- World streaming.
- Editor integration.
- Comparative, measured acceptance.

The detail is in the matrix below.

## AnimeRPG (Astral Engine) at base

- **Runtime:** Win32 window, fixed-step `Clock`, GDI perspective wireframe renderer
  and window-title telemetry.
- **Gameplay:** well-tested, engine-free domains in `Engine/Scene`:
  - `CombatSandbox`, `ShadowbladeActions`, `ThoughtCommands`,
    `LandmarkInteraction` and `LandmarkEncounter`.
  - Many header-only progression, loadout, mission, puzzle and trial models.
  - Coverage: 9 domain CTest targets and 6 interactive runtime smokes. The
    smokes drive the real window and read its title and pixels.
- **Evidence tooling:** strong. Benchmark run control, frame, phase and memory
  capture, release-determinism checks, and a CTest safety contract with
  assertions enabled in every configuration.
- **Missing before this change:**
  - A job system, scene or ECS, and a rasteriser.
  - Textures, lighting and shadows.
  - Physics, skeletal animation, audio, VFX, navigation and AI.
  - A real 2D path.
- **Defects:** none were found in the gameplay domains, and none were modified.
  `UpdateTitle` still has an unused `fps` parameter. This is pre-existing and was
  left as is.
- **Layout constraint found:** the landmark footprints (Lincoln at x -14..-2,
  z 14.5..21.5) sit inside the Win32 movement bounds (x -18..18, y -4..72).
  Discovery uses a 3 m radius from the footprint edge. Solid landmarks in Astral
  mode therefore still leave every discovery reachable. The scripted route
  proves it for all three landmarks.

## AnimeRPG-UE5 at base

This is a minimal Unreal 5.8 C++ scaffold: a character with a spring arm, a game
mode, and a Python source-contract validator run in CI. Findings:

1. **Legacy input.** `BindAxis`/`BindAction` and `DefaultInput.ini` mappings are
   deprecated in favour of Enhanced Input since UE 5.1. Migrating would change a
   contract the validator checks, so it needs Lucas's approval.
2. **Dash does not match the game's rules.** It calls `LaunchCharacter(dir * 1200)`
   with no resource, cooldown or distance. Astral's Shadowblade dash costs 25 of
   100 resource, has a 1 s cooldown and travels 6 m.
3. **No content defaults.** No default map, mesh, anim blueprint or level is
   checked in, so Play In Editor shows an empty world.
4. **No special features.** Shadowblade, Thought Commands, the National Mall and
   the encounter are absent.
5. **No native build evidence** in this or the previous audit. There is only the
   source validator.

Recommendation: keep it as the reference experiment it is (ADR-0001). A UE5
track, if Lucas wants one, should compile the engine-free `Engine/Scene` domains
as a plain C++ module, so rules stay identical across both engines, and wrap them
in actors. Its audit note is in that repository at `Docs/AUDIT-2026-09-28.md`.
No source, config or engine-version change was made there.

## Capability matrix (IDs from Docs/Research/ENGINE-CAPABILITIES.json)

"Now" means implemented in portable C++17 and tested on Linux GCC and Clang,
including ASan+UBSan and TSan. It does not mean natively verified on Windows or
comparable to UE5.

| ID | Area | Before | Now | Largest remaining gap vs UE5/Unity |
|---|---|---|---|---|
| E01 | Runtime/jobs | Clock, logger | Dependency job graph, `ParallelFor`, waiter participation, TSan-clean; generational slot map; PCG32; scoped profiler with Chrome trace export; game time with dilation channels, hitstop and fixed steps | Memory budgets and allocators, task priorities, fibers |
| E02 | Scene/serialisation | Transforms | Sparse-set ECS, generational entities, cycle-safe hierarchy, transactional versioned text format | Reflection, prefabs, binary format. The showcase does not use the ECS yet |
| E03 | Assets | Text wireframes | PNG encode/decode (own inflate/deflate), procedural textures | glTF import, cooking, async streaming, dependency database |
| E04 | Rendering | GDI wireframe | CPU visibility-buffer rasteriser (tile-binned, 28.4 fixed point, top-left rule, homogeneous clipping); mipmapped textures; toon, GGX, unlit and water materials; outlines; sorted translucency; soft particles; sky LUT; scalability presets; dynamic resolution; camera-occlusion dither fade | **GPU RHI (D3D12/Vulkan)**, GPU culling, TAA/TSR, shader pipeline |
| E05 | Lighting | None | Sun and point lights, texel-snapped PCF shadows, SSR water, fog, bloom, neutral tonemap, FXAA, grading | Global illumination (Lumen-class), cascaded or virtual shadows, AO, volumetrics |
| E06 | Large worlds | 3 proxies | Hand-built National Mall scene | Streaming, LOD/HLOD, Nanite-class geometry |
| E07 | Animation | None | Skeletons; clips with notifies and root motion; masked and additive blending; state machine with buffered triggers, blend spaces and overlays; two-bone IK; linear blend skinning (LBS); procedural 21-joint humanoid | Import, retargeting, motion matching, Control Rig-class tools |
| E08 | Physics | Bounds | Analytic narrowphase including box-box SAT; dynamic AABB tree; sequential impulses with warm start and sleeping; triggers; raycasts, shape casts and overlaps; layers; kinematic capsule controller; fracture | Convex and mesh colliders, joints, continuous collision detection (CCD), Chaos-scale destruction |
| E09 | AI/navigation | Encounter state | Grid A* with smoothing and physics bake; behaviour trees with abort semantics; perception | Navmesh, EQS, crowds. The training dummy is not AI-driven |
| E10 | Audio | None | Mixer with buses, 3D pan and attenuation, voice stealing, ducking and a limiter; procedural synth; WAV output; Win32 waveOut stream | Codecs and streaming, HRTF, reverb, occlusion, authoring |
| E11 | UI/editor | Editor shell | Runtime canvas UI (bitmap font, shapes, rings, bars) | Editor integration, widget system, text shaping |
| E12 | Genuine 2D | Ortho math | 2D camera, sprite batch, tile maps with collision | 2D physics, animation and tools |
| E13 | Multiplayer | None | None | Everything; offline schema first |
| E14 | Profiling | CPU frame/phase capture | Plus profiler zones, Chrome trace, per-pass renderer stats, F3 overlay | GPU timings, memory tracking, Insights-class UI |
| E15 | Packaging | Unchanged | Unchanged | Clean-machine packaging evidence |
| E16 | Other catalogue | Unaudited | VFX (particles, ribbons, presets); input contexts, buffering, rebinding and record/replay | Cinematics, scripting, localisation, platforms |
| E17 | Comparative acceptance | None | None | Frozen scenes, hardware, budgets and a 24-hour soak |

## What makes the game special: preserved and elevated

The gameplay domains remain the only owners of rules. Astral mode observes them
and, when collision is on, sweeps the resulting positions against the Mall. It
never changes cost, cooldown, range, damage or reward. Evidence for that:
`EngineShowcaseTests.AstralModeKeepsGameplayRulesOnTheM10Route` plays the M10
smoke route in Astral mode and in raw-gameplay mode and compares them.

| Feature | Rules (unchanged) | Astral presentation | Evidence |
|---|---|---|---|
| Shadowblade dash (Q) | `ShadowbladeActions` | Afterimages, streaks, smoke, FOV kick; swept so it stops at walls | Test `DashIsSweptAgainstTheMallWithoutChangingItsCost`; capture 06 |
| Guard (Shift) | `ShadowbladeActions` via `ThoughtCommands::ApplyGuardState` | Guard bubble, shimmer, HUD pill | Capture 03 |
| Fatal Strike (L) | `ShadowbladeActions` | Heavy swing, 0.14 s hitstop, camera shake, shadow smoke, crystal shattering | Capture 12 |
| Light/heavy attacks (J/K) | `CombatSandbox` | Notify-timed hits, ribbon trails, sparks, damage numbers | Captures 02 and 11 |
| Thought Commands (1–5, typed) | `ThoughtCommands` parser | Enter opens a typed prompt that reaches the same `Submit`; focus x0.35 drives the presentation clock with a violet grade, aura and drone | Test `ThoughtFocusDrivesPresentationTimeAtGameplayMultiplier`; captures 04 and 05 |
| National Mall landmarks (E) | `LandmarkInteraction` | Lincoln (24 columns, cella, statue), Reflecting Pool mirror, Washington Monument (two-tone shaft, 50 flags), Capitol backdrop; prompts, banners, minimap | Captures 07–10 |
| Lincoln training encounter | `LandmarkEncounter` | Encounter banner and status line in the same colours the GDI smoke counts; target tint | Pixel-colour checks in the M10 route test; capture 13 |
| Vision items | — | Mana Reactor rift over the pool (reflected); destructible crystals that respawn | Test `HeavyHitsShatterCrystalsThatRespawn` |

The Shadow Crypt is not modelled; it remains a future scene.

## Risks and what is not claimed

- **MSVC:** the new code has not been compiled by MSVC in this session.
  - Linux GCC 13 and Clang 18 builds pass with `-Werror`.
  - The Win32 files were syntax-checked against a stub SDK header.
  - Every standard header the code uses directly is included.
  - `windows-ci` must run on the branch before any merge.
- **Native runtime:** F2 mode, waveOut audio and the six runtime smokes have not
  been run on a Windows desktop.
- **Performance:** the CPU renderer takes about 170 ms per 1280x720 frame on this
  4-vCPU VM with 3 workers.
  - Dynamic resolution trades pixels for time.
  - Real-time 1080p60 needs the GPU backend.
  - No frame-time claim is made for Lucas's hardware.
- **Duplicated frame order:** the Win32 loop and `ShowcaseSession` keep the same
  order in two places. `GameplayBridge` holds the only logic they share, and the
  M10 route test guards the headless copy.
- **Layering:** `Engine/Platform/Win32Application` now includes
  `Game/Showcase/Win32AstralPresenter.h`. ADR-0002 accepts this for the opt-in
  path.

## Recommended next steps

1. Dispatch `windows-ci` on `claude/loving-clarke-el24od`. On a desktop, run the
   RuntimeSmokes, then `AstralGame` with F2 or `ASTRAL_RENDER_MODE=astral`.
2. Add a GPU backend (D3D12 first) behind `RenderScene`/`DrawItem`. Keep the CPU
   renderer as the image-comparison oracle.
3. Add glTF 2.0 import for meshes, skeletons and clips, with a cooked cache.
4. Move the showcase to the ECS, then add world cells and streaming.
5. With director approval for content work: make the Rift Wraith fight back
   using the behaviour tree and navigation, and build the Shadow Crypt.
