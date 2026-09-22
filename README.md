# Astral Engine / AnimeRPG

[![Windows build and deterministic tests](https://github.com/LucasKazaki/AnimeRPG/actions/workflows/windows-ci.yml/badge.svg)](https://github.com/LucasKazaki/AnimeRPG/actions/workflows/windows-ci.yml)

Custom C++17, Win32, and GDI engine/action-RPG prototype. Unreal Engine and Unity
are not production dependencies. The separate AnimeRPG-UE5 repository is an experiment.

## Direction first

Read [Game Development Control](GAME_DEVELOPMENT_CONTROL.md) and
[Project Status](Docs/Project-Status.md) before selecting work. The September 17
director direction is **engine-first, local-only, 3D plus genuine 2D support**.
Game content, combat tuning, encounters, narrative, production art, audio content,
and game playtests are paused until engine acceptance. Existing game systems are
retained; their presence is not permission to start the next old backlog item.

The audit branch carries previously unintegrated director context. Its files only
become default-branch guidance after an approved merge. The August 14 recovery
deadline and old heartbeat are historical, not evidence of current PC activity.

See the [UE5/Unity capability survey](Docs/Research/ENGINE-CAPABILITIES-2026-09-20.md)
and [machine-readable work map](Docs/Research/ENGINE-CAPABILITIES.json) for the
engine comparison plan. No recurring scheduler or parity claim is established.

## Existing systems prototype

- Native window, bounded loop, keyboard input, frame timing, title telemetry, and logging.
- Perspective GDI wireframe scene, bounded player movement/camera follow, and three National Mall landmark proxies.
- Light/heavy training-target combat, range/cooldowns, and terminal target defeat.
- Shadowblade dash, guard, fatal strike, resources, cooldowns, and regeneration.
- Five key-driven Thought Commands, landmark discovery, and one Lincoln Memorial training encounter with capped rewards.
- Fifteen native CTest targets: nine domain tests and six interactive runtime smokes.

This is not the complete National Mall, final anime art, a natural-language AI
command system, three complete classes, multiple dungeons, multiplayer, or a
production engine. The original game roadmap is retained in
[Milestones](Docs/Planning/MILESTONES.md); implementation M9/M10 are landmark
features, not completion of the original summon/enemy milestones.

## Layout

`Engine/Core` provides timing/logging; `Engine/Platform` owns Win32/input;
`Engine/Renderer` provides GDI rendering; `Engine/Scene` contains independently
testable domains; `Engine/Assets` loads simple text meshes; `Game` hosts the
entry point and fixtures; `Tests` and `Scripts` contain verification tooling.

## Controls for the retained prototype

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

Windows 10+, CMake 3.25+, Visual Studio 2022 Desktop development with C++, a
Windows SDK, and Python 3 are required. Use an owned worktree under an admitted
task, with generated files in a sibling directory:

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

Record and stop on each nonzero exit; the block is a command reference, not an
automated acceptance script. CI checks each exit explicitly. The new CMake helper
keeps assertions active in every native test configuration without changing
AstralGame's Release settings. Old green Release runs do not replace fresh checks.

Only an owned interactive Windows desktop can provide native runtime evidence:

```powershell
ctest --test-dir ../AnimeRPG-build -C Debug --output-on-failure -R RuntimeSmoke --no-tests=error
ctest --test-dir ../AnimeRPG-build -C Release --output-on-failure -R RuntimeSmoke --no-tests=error
& ../AnimeRPG-build/Debug/AstralGame.exe
```

Smokes inspect real windows and send input. They are serialized and time-bounded
within one CTest invocation; do not launch competing test invocations. Hosted CI
builds them but intentionally does not claim interactive execution.

See [Audit Evidence](Docs/QA/AUDIT-2026-09-19.md) for verified and pending gates.
The [asset-loader repair report](Docs/QA/E0-ASSET-VALIDATION-2026-09-20.md) documents
portable Debug/Release/sanitizer tests and the separate local acceptance handoff.
Do not run the historical R0 packaging script without a current task and its
outstanding safety/provenance repairs. No local heartbeat or package was validated
by the connector audit.

## License

No project-level open-source license is included. Source visibility is not a grant
of reuse rights. This audit does not introduce a license or third-party assets.
