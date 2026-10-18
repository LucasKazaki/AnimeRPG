# QA record — Engine framework 2026-09-29

Task: [Tasks/ENGINE-FRAMEWORK-2026-09-29.md](../../Tasks/ENGINE-FRAMEWORK-2026-09-29.md)
Decision: [ADR-0003](../Architecture/ADR-0003-gameplay-framework-and-game-host.md)
Branch: `claude/loving-clarke-el24od` (pull request #80); base `main` at `c644eec`.
Environment:

- Linux cloud container with 4 vCPU.
- GCC 13.3.0, Clang 18.1.3, CMake 3.28.3, Ninja, Python 3.11.
- No Windows toolchain, GPU or desktop.

Everything below is **portable execution**, plus hosted MSVC results where
stated. None of it is native interactive, GPU or performance evidence.

## Portable engine build (Tests/EngineRuntime)

```bash
CXX=<g++|clang++> cmake -S Tests/EngineRuntime -B <build> -G Ninja \
  -DCMAKE_BUILD_TYPE=<type> [-DASTRAL_SANITIZE=ON | -DASTRAL_TSAN=ON]
cmake --build <build>
ctest --test-dir <build> --output-on-failure
```

| Configuration | Compiler | Flags | Result |
|---|---|---|---|
| Debug | GCC 13 | `-Wall -Wextra -Wpedantic -Werror` | 15/15 suites pass, exit 0 |
| Release | GCC 13 | same | 15/15 pass, exit 0 |
| Release | Clang 18 | same | 15/15 pass, exit 0 |
| RelWithDebInfo | GCC 13 | ASan+UBSan, no recover | 15/15 pass, exit 0 |
| RelWithDebInfo | GCC 13 | TSan | 15/15 pass, exit 0 |

The 15 suites contain 153 test cases. Six suites are new in this phase:

| Suite | Cases | Covers |
|---|---|---|
| Services | 9 | JSON, CVars/console, delegates and events, reflection, atomic file writes |
| Assets | 10 | glTF 2.0 round-trip, skins, animations, PNG/JPEG/WAV, async loads, hot reload |
| Framework | 12 | tick phases, components, behaviours, timers, physics sync, scenes/prefabs, LOD extraction |
| Timeline | 7 | tweens, eases, sequences, timeline tracks, events, world binding |
| UI | 6 | layout, input, focus, navigation, text boxes, console overlay |
| Sample | 11 | the game host and the Playground sample |

Other suites were extended:

- Graphics has 16 cases (+3 for LODs).
- Physics has 19 cases, covering:
  - mesh colliders;
  - a fuzz test;
  - the cast tolerance contract;
  - the MSVC regression;
  - kinematic follow.
- Math has 11 cases (+1 for the MSVC segment clamping).

Python verifiers, run from the repository root: `Scripts/test_test_safety.py`
(OK) and `verify_milestone1.py`, `verify_milestone2.py`, `verify_milestone3.py`
(PASS). All exit 0.

## The Playground sample through the game host

`EngineSampleTests` plays `Content/Samples/Playground` through `GameHost` with
the project's own input map:

- **Movement:** walking, facing, sprinting (at least 1.3× the walk distance),
  a buffered jump that lands, the follow camera, and crates pushed by the
  character.
- **Winning:**
  - Each of the five stars is collected on the same frame the player reaches it.
  - The HUD text updates and the win banner shows.
  - The gate timeline raises the gate more than 2.5 m.
  - A `LoadSceneRequest` restarts the scene after the delay.
  - The old HUD is cleared.
- **Jump pad:** it launches the character past 4.5 m.
- **Console:**
  - The toggle key takes the keyboard.
  - `spawn`, `entities`, `pause`, `slomo` and `r.Quality` all work.
  - A missing scene is reported while the running scene continues.
  - `restart` and `quit` work.
- **Other host features:**
  - Hot reload of an edited scene; a broken edit is reported once and keeps
    the running scene.
  - Scene switching and reload by event.
  - Frame rendering with the HUD and stats overlay, and a placeholder frame
    without a camera.
  - Two identical 150-frame playthroughs finish bit-identical.

Headless player (GCC Release), the same command as the CI step:

```bash
AstralPlayer Content/Samples/Playground project.json \
  --script Content/Samples/Playground/demo.play --capture <dir> --every 120 --track Player
```

Exit 0. Frame lines, with render times removed:

```text
loaded project project.json (35 entities)
frame   120 hash 0a3ca29b061862f5  draws   21 tris     970 entities   34 bodies  20
  Player            2.077    0.020    1.835
frame   240 hash 8e2a2dde380b9952  draws   12 tris     597 entities   34 bodies  20
  Player            5.640    0.020    7.538
> spawn Crate 0 6 -2
spawned Crate
frame   360 hash f680872cb3cab4a9  draws   13 tris     687 entities   35 bodies  21
frame   372 hash bd81efed7c3eed4a  draws   13 tris     681 entities   35 bodies  21
summary: frames 372 scene loads 1 hot reloads 0 entities 35 | update 0.04 ms/frame
```

- Two consecutive runs produced identical frame hashes.
- By frame 120 the player has collected the centre star; the HUD reads
  "Stars 1 / 5".
- The capture at frame 120 was inspected. It shows the HUD panel, the
  character, crates with shadows, the ramp and platform, the lamps and the
  gate.

`Tools/AstralPlayerWin32.cpp` is built by the hosted MSVC job. On Linux it was
syntax-checked with `-Wall -Wextra -Wpedantic` against minimal Win32
declarations. It has not been run interactively.

## Defects found and fixed in this phase

1. **MSVC x64 Release miscompiled segment-segment clamping.**
   - *Symptom:* the mesh cast fuzz test failed only in MSVC Release. A capsule
     beside a triangle measured 0.1999 instead of 0.0303, and casts sank into
     the triangle.
   - *Isolating it:*
     - Bit-exact diagnostics showed an identical mesh on MSVC.
     - Dropping the edge candidates locally reproduced MSVC's value bit for bit.
     - The per-edge output from the next run showed
       `Math::ClosestPointsSegmentSegment` returning s = 1 in both clamped
       branches, where the answer is 0.665. That is consistent with the
       divisor `a` being read as zero.
   - *Fix:* the clamping is now straight-line alternating projections. Over
     2,000,000 random general, point, near-point and parallel cases, the
     distances agree with the old form to 1e-6 and the parameters are
     identical in every non-parallel case.
   - *Regression tests:* `ClosestPointsClampToTheSharedCorner` (Math) and
     `CapsuleBesideATriangleMeasuresItsNearestVertex` (Physics) pin the exact
     inputs.
2. **Kinematic bodies lagged their targets.** The dynamic speed limit
   (120 m/s) also clamped `MoveKinematic` velocities, so a teleported character
   reached its new position several steps late and triggers fired late. Only
   dynamic bodies are clamped now, and character teleports move the body
   directly (`KinematicBodiesFollowFastMovesExactly`).
3. **Hot reload lost the scene path.** `LoadScene(scenePath_)` passed the
   member by reference, and activation cleared it mid-load. The path is now
   copied first; `HotReloadPicksUpSceneEdits` covers it.

## Hosted MSVC (windows-ci)

Toolset: MSVC 14.44.35207 (Visual Studio 2022, windows-2022 runner). The job
builds Debug and Release, runs deterministic CTest (`-E RuntimeSmoke`) and the
milestone verifiers, and checks that the tracked tree stays clean.

- `7a30cf9` (the segment-segment workaround): Debug and Release both pass. In
  Release, 30/30 tests pass, including `EnginePhysicsTests` and
  `EngineMathTests`, which had failed on every earlier commit of this phase;
  the milestone verifiers pass and the tree stays clean.
- Earlier Release-only failures of the mesh cast fuzz test on this branch ran
  from `ed9f6d1` to `48e8504`.
  - `81b46fe` fixed a genuine cast early-out bug that the fuzz also exposed.
  - The failures that remained were this defect.
  - `14bece0`, `d3216cc`, `6c21c75` and `48e8504` added the diagnostics that
    isolated it.
- Runs for later commits, which add the players and the sample suite: see
  pull request #80.

RuntimeSmoke tests are built but not run by hosted CI. They need an owned
interactive desktop.

## Not claimed (local acceptance gates)

- Interactive `AstralPlayerWin32` on Lucas's machine: window, audio, input
  feel and frame rate.
- GPU and frame-time budgets, the soak, and independent review.
- Parity with Unreal Engine 5 or Unity. See the gaps listed in
  `Docs/Research/ENGINE-CAPABILITIES.json`.
