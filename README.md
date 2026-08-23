# Astral Engine / AnimeRPG

[![Windows build and deterministic tests](https://github.com/LucasKazaki/AnimeRPG/actions/workflows/windows-ci.yml/badge.svg)](https://github.com/LucasKazaki/AnimeRPG/actions/workflows/windows-ci.yml)

Astral Engine is a from-scratch C++17 Windows action-RPG prototype built directly on Win32 and GDI. It explores a supernatural Washington, DC National Mall setting without using Unreal Engine, Unity, or another commercial game runtime.

The current M10 systems slice is intentionally compact: it prioritizes a bounded native game loop, deterministic gameplay domains, and automated native runtime checks over production art and content volume.

## What is implemented

- Native Win32 window, bounded game loop, keyboard input, frame timing, window-title state telemetry, and file logging.
- GDI wireframe renderer with a perspective camera and a traversable National Mall blockout.
- Deterministic player movement and camera follow with explicit world bounds.
- Light/heavy combat, range and cooldown rules, a health-bearing training target, and terminal defeat state.
- Shadowblade resource mechanics: dash, guard, fatal strike, cooldowns, and regeneration.
- Five bounded Thought Commands covering dash, fatal strike, guard state, and focus mode.
- Landmark selection and discovery for the Lincoln Memorial, Reflecting Pool, and Washington Monument.
- A Lincoln Memorial training encounter with activation, completion, and capped resource reward states.
- Fourteen CTest targets spanning domain tests and native Win32 runtime-smoke executables.

## Architecture

| Area | Responsibility |
|---|---|
| `Engine/Core` | Clock and logging services |
| `Engine/Platform` | Win32 window lifecycle, message pump, and input sampling |
| `Engine/Renderer` | GDI scene, landmark, combat, and HUD rendering |
| `Engine/Scene` | Camera, transforms, movement, combat, abilities, commands, landmarks, and encounters |
| `Engine/Assets` | Dependency-light text mesh loading |
| `Game` | Executable entry point and local prototype assets |
| `Tests` | Deterministic unit/domain tests and native runtime smokes |

Gameplay rules live outside the Win32 message loop where practical. That keeps combat, abilities, interaction, and encounter transitions independently testable while the runtime smokes exercise the real executable, input path, window state, and renderer.

## Controls

| Input | Action |
|---|---|
| `W` `A` `S` `D` | Move |
| `J` / `K` | Light / heavy attack |
| `Q` | Shadow Dash |
| `L` | Fatal Strike |
| Left `Shift` | Guard |
| `1` | Thought Command: dash |
| `2` | Thought Command: fatal strike |
| `3` / `4` | Thought Command: guard on / off |
| `5` | Thought Command: focus mode |
| `E` | Discover or interact with the selected landmark |
| `Escape` | Exit |

## Build and test

Requirements:

- Windows 10 or newer
- CMake 3.25+
- Visual Studio 2022 with the Desktop development with C++ workload and a Windows SDK
- Python 3 for the static milestone verifiers

Configure into a sibling directory so generated files stay outside the source tree:

```powershell
cmake -S . -B ../AnimeRPG-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-build -C Debug --output-on-failure -E RuntimeSmoke
```

Run the game:

```powershell
& ../AnimeRPG-build/Debug/AstralGame.exe
```

Repeat the deterministic build and test commands with `Release`, then run:

```powershell
python Scripts/verify_milestone1.py
python Scripts/verify_milestone2.py
python Scripts/verify_milestone3.py
```

The native runtime smokes are an explicit opt-in on an interactive Windows desktop:

```powershell
ctest --test-dir ../AnimeRPG-build -C Debug --output-on-failure -R RuntimeSmoke
```

These smokes create and inspect a real window, bring it to the foreground, and send keyboard input. GitHub Actions therefore builds Debug and Release configurations and runs only the deterministic, non-interactive tests.

## Project status

This is a playable systems prototype, not a content-complete RPG. The visuals are deliberately programmer-facing wireframes; there is no production asset pipeline, save system, audio layer, distribution installer, or claim of engine completeness. The repository's `Tasks/` and `Docs/QA/` records preserve the incremental implementation and verification history.

## License

No project-level open-source license is currently included. The repository is available for source review, but reuse rights have not been granted.
