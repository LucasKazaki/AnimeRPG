# Astral Engine / AnimeRPG

[![Windows build and deterministic tests](https://github.com/LucasKazaki/AnimeRPG/actions/workflows/windows-ci.yml/badge.svg)](https://github.com/LucasKazaki/AnimeRPG/actions/workflows/windows-ci.yml)

Custom C++17, Win32, and GDI engine/action-RPG prototype. Unreal Engine and Unity
are not production dependencies. AnimeRPG-UE5 is a separate experiment.

## Direction and evidence

Read [Game Development Control](GAME_DEVELOPMENT_CONTROL.md) and
[agent rules](AGENTS.md) before selecting work. The engine lane retains its
engine-first, local-only, 3D plus genuine 2D acceptance requirements. The
September 22 operator update permits dependency-ready parallel GAME work under
its own ownership, review and merge rules. Older blanket content-pause paragraphs
and historical deadlines do not override that update or newer explicit requests.
A game feature does not establish engine acceptance.

Lucas's current product direction is one playable persistent protagonist, a
supporting NPC/AI-companion cast, English/Japanese offline-generated voices,
responsive expressive dialogue, localized damage and optional equipment/cosmetic
gacha rather than playable-character pulls. All required production tools must
have a free route; no required paid API or subscription is admitted by this README.
These are requirements, NOT claims that those features are playable. Draft
research and source foundations in PRs #68, #71 and #74 remain distinct from main.
No dependency installation, spending, new local scheduler or unreviewed merge is
authorized by this summary.

See the [capability survey](Docs/Research/ENGINE-CAPABILITIES-2026-09-20.md),
[capability register](Docs/Research/ENGINE-CAPABILITIES.json),
[September 25 audit](Docs/QA/REPO-AUDIT-2026-09-25.md), and
[numeric/asset-pipeline research](Docs/Research/NUMERIC-AND-ASSET-PIPELINE-2026-09-25.md).
[Project Status](Docs/Project-Status.md) is a dated September 19 snapshot, not a
live work assignment or current workstation heartbeat. Neither file counts nor
milestone names establish completion or Unreal/Unity parity.

## Existing systems prototype

- Native window, bounded loop, keyboard input, frame timing and logging.
- Perspective GDI wireframe scene, player movement/camera and landmark proxies.
- Training combat, Shadowblade actions, thought commands and game-domain
  exploration, progression, encounter and narrative logic.
- A separate editor shell, not a complete authoring editor.
- Registered native tests plus separate portable suites. Discover the actual
  configured tests with `ctest --test-dir <build> -C Debug -N`; do not rely on a
  hard-coded count in prose. A listed test is not evidence it executed.

This is not final anime art, a production GPU/material pipeline, full animation,
physics, audio, multiplayer or a finished engine. Domain logic and validated
source assets must still be connected to their real runtime consumers. The
original roadmap is in [Milestones](Docs/Planning/MILESTONES.md); implementation
M9/M10 refer to landmark features, not completion of the original summon/enemy
milestones. The September 25 repair preserves existing XY gameplay mapped to XZ
presentation; it does not silently change coordinates or migrate the renderer.

## Layout

`Engine/Core` provides timing/logging; `Engine/Platform` owns Win32/input;
`Engine/Renderer` provides GDI rendering; `Engine/Scene` contains domain code;
`Engine/Assets` loads bounded text meshes; `Game` hosts the entry point and
fixtures. `Tests`, `Scripts` and `Tools` provide testing and authoring support.

## Retained prototype controls

| Input | Action |
|---|---|
| W, A, S, D | Move |
| J / K | Light / heavy attack |
| Q / L | Shadow Dash / Fatal Strike |
| Left Shift | Guard |
| 1 / 2 | Command dash / fatal strike |
| 3 / 4 / 5 | Command guard on / guard off / focus |
| E | Landmark discovery or interaction |
| Escape | Exit |

## Build and test

The main application targets Windows 10+, CMake 3.25+, Visual Studio 2022 C++ and
a Windows SDK. Verification scripts use Python 3. Use an owned worktree, with
all generated outputs outside the source tree:

```powershell
cmake -S . -B ../AnimeRPG-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-build -C Debug --output-on-failure -E RuntimeSmoke --no-tests=error
cmake --build ../AnimeRPG-build --config Release --parallel
ctest --test-dir ../AnimeRPG-build -C Release --output-on-failure -E RuntimeSmoke --no-tests=error
python Scripts/test_test_safety.py
python Scripts/verify_milestone1.py
python Scripts/verify_milestone2.py
python Scripts/verify_milestone3.py
```

These are command references, not a fail-fast script. Stop and retain the error
on any nonzero exit. `astral_add_test` keeps assertions active in Release without
changing the game's own optimization settings. Old green runs do not replace
checks against the actual new revision.

The dedicated numeric audit suite reuses production source and the unchanged
legacy scene tests. Its own workflow executes the suite on Windows and Linux;
PR jobs distinguish exact-head from generated merge-result testing. It does not
replace the root build's broader combat/mission regression:

```sh
cmake -S Tests/NumericAudit -B ../AnimeRPG-numeric -DCMAKE_BUILD_TYPE=Debug
cmake --build ../AnimeRPG-numeric --config Debug
ctest --test-dir ../AnimeRPG-numeric -C Debug --output-on-failure --no-tests=error
```

Repeat in a separate Release directory. With Clang/GCC, a separate Debug build
can use `-DASTRAL_AUDIT_SANITIZERS=ON`; no sanitizer or dependency installation is
performed by these commands. Test logs must identify compiler/configuration and
source revision. CI that merely compiles an unrelated target is not this suite.

Only an owned interactive Windows desktop can supply native runtime evidence:

```powershell
ctest --test-dir ../AnimeRPG-build -C Debug --output-on-failure -R RuntimeSmoke --no-tests=error
ctest --test-dir ../AnimeRPG-build -C Release --output-on-failure -R RuntimeSmoke --no-tests=error
& ../AnimeRPG-build/Debug/AstralGame.exe
```

Runtime smokes send real window input. RUN_SERIAL covers one CTest process, not
competing invocations. Hosted jobs do not establish interactive GUI/GPU behavior.
The legacy scene test also uses a fixed temporary filename; do not run competing
copies on one host. Do not invoke the historical R0 packaging runner while its
current safety/review/native requirements are unresolved. Preserve the existing
registered executor and separate packaging, performance and 24-hour soak gates.

## License

No project-level open-source license is included. Source visibility is not a grant
of reuse rights. This audit does not introduce a license or third-party assets.
