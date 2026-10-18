# Engine framework: a workable game engine on the portable runtime

Authorized by Lucas's September 29 request: "Implement more ue5 and unity and
other top engine features into this engine to make sure its a workable game
engine". It follows the September 28 uplift
([ENGINE-UPLIFT-2026-09-28](ENGINE-UPLIFT-2026-09-28.md), merged as #79).
Base: `main` at `c644eec`. Branch: `claude/loving-clarke-el24od`, one cloud
container, one owner. Pull request: #80.

The engine stays custom C++17 (ADR-0001) with the portable runtime and opt-in
presentation of ADR-0002. This packet adds the layer a game is built from
(ADR-0003):

- services: JSON, CVars/console, delegates/events, reflection;
- assets: glTF 2.0, PNG/JPEG/WAV, an asset manager with hot reload;
- a gameplay framework: world, components, behaviours, timers, JSON
  scenes/prefabs;
- physics: static triangle-mesh colliders;
- content tools: tweens, sequencer timelines, retained UI, in-game console,
  automatic LODs;
- a game host with headless and Win32 players, and a sample project.

It adds no third-party code, dependencies, plugins, downloads or network access.

## Preserved behavior (non-negotiable)

- `AstralGame` is unchanged. The GDI default renderer, titles, HUD text, key
  bindings, the opt-in F2 Astral mode and every runtime smoke contract and
  verifier marker are unchanged.
- The special features stay authoritative and untouched:
  - Shadowblade: dash, guard, fatal strike, resource.
  - Thought Commands: focus ×0.35 and the typed parser.
  - National Mall landmarks, discovery and encounter, and the training dummy.
- Engine changes that affect existing behaviour are bug fixes with tests: the
  kinematic speed clamp, character teleports and MSVC segment clamping. The
  showcase and gameplay suites still pass unchanged.

## Allowed paths

- New: `Engine/Core/{Json,Console,Delegate,Reflection}.*`, `Engine/Assets/**`,
  `Engine/Framework/**`, `Engine/UI/**`, `Engine/Animation/{Tween,Timeline}.*`,
  `Engine/Graphics/MeshSimplify.*`, `Engine/Physics/TriangleMesh.*`
- New: `Game/Samples/**`, `Content/Samples/**`, `Tools/AstralPlayer.cpp`,
  `Tools/AstralPlayerWin32.cpp`
- New tests: `Tests/Engine{Services,Assets,Framework,Timeline,UI,Sample}Tests.cpp`
- Edits to existing engine modules, needed by the above:
  - `Engine/Physics/*` (shapes, collision, world, character controller)
  - `Engine/Graphics/Material.h`, `Engine/World/Registry.h`
  - `Engine/Input/InputSystem.*`, `Engine/Math/Geometry.h`
- Existing suites extended: Math, Graphics, Physics.
- Build and CI:
  - `cmake/AstralEngineSources.cmake`, `CMakeLists.txt` (new suites through
    `astral_add_test`, player targets)
  - `Tests/EngineRuntime/CMakeLists.txt`, `Tests/EngineTestSupport.h`
  - `.github/workflows/{engine-runtime,windows-ci}.yml` (trigger paths, player
    demo step)
- Docs:
  - this packet, `Docs/Architecture/ADR-0003-*`,
    `Docs/QA/ENGINE-FRAMEWORK-2026-09-29.md`
  - `Docs/Research/ENGINE-CAPABILITIES.json`, `README.md`

## Acceptance

1. All 15 portable suites pass with `-Wall -Wextra -Wpedantic -Werror` in these
   Linux configurations: GCC Debug, GCC Release, Clang Release, GCC ASan+UBSan
   and GCC TSan.
2. Hosted MSVC Debug and Release builds (including both players) and
   deterministic CTest pass in `windows-ci`.
3. The Playground sample plays through the real host:
   - walking, sprinting, jumping, pushing crates;
   - pickups, the win timeline and the automatic restart;
   - the jump pad;
   - console commands, hot reload, scene requests;
   - deterministic replays.
   `AstralPlayer` runs the demo script with PNG captures in CI.
4. `Scripts/test_test_safety.py` and the milestone verifiers still pass.
5. **Local acceptance gates (not claimed):**
   - interactive use of `AstralPlayerWin32` on Lucas's machine (window, audio,
     input feel, frame rate);
   - GPU and frame-time budgets;
   - the soak;
   - independent review.

## Stop conditions

Stop at the first deterministic blocker. Never weaken or skip an existing test,
marker or smoke to make new work pass. Record exact commands and exits in the
QA record.
