# Astral Engine / AnimeRPG playtest

Open **Play AnimeRPG.cmd** to start the National Mall combat prototype in Astral
rendering mode. No account, installer, Unreal Engine, API key or network is needed.
Keep the whole package folder together, including its DLLs and Content folder.

## First encounter

1. Use **WASD** to move. **J / K** are light / heavy attacks; **Q** dashes;
   **L** uses Fatal Strike; hold **Left Shift** to guard.
2. Walk north with **W** toward Lincoln Memorial until its interaction prompt
   appears, then press **E** to start the encounter. If you defeated the training
   target early, entry restores it automatically.
3. Return south with **S** to the target near your starting position and defeat it.
   Attacks have range, resource and cooldown requirements shown in the HUD.
4. After completion, **F5** starts another attempt. Progress and the one-time
   completion reward are preserved. F5 does not reset an active encounter.

**Enter** opens the Thought prompt. Try `dash`, `fatal strike`, `guard on`,
`guard off`, or `focus`, then Enter to submit. **Escape** cancels the prompt;
outside the prompt it exits. `focus` toggles slowed time. Commands use a local
rule-based parser; there is no LLM or speech input. Typing alone does not slow time.
Number keys **1–5** provide the same five commands without typing.

**F2** toggles the legacy wireframe view. **F3** toggles statistics. **F4** cycles
rendering quality. The game ignores keyboard input while another app has focus;
switching away cancels a partially typed command. Release held keys before resuming.

## Engine and editor

- **Play Engine Sample.cmd** opens the collect-a-thon sample. WASD/arrows move,
  Space jumps, Shift sprints, Q/E rotate, F5 restarts, F6 reduces resolution,
  F3 shows statistics, and Escape exits.
- **Open Astral Editor.cmd** opens that project for editing. Select an object in
  the Outliner, use W/E/R for move/rotate/scale, F to frame selection, and F5 to
  play/stop. Save changes only if you want to edit the packaged sample.
- **Open Engine Player.cmd** exercises the generic engine host. It does not
  register the Playground-specific gameplay behaviours; use Play Engine Sample
  for the complete sample game.
- **Open GPU Sample.cmd** opens the neutral graphics fixture. Choose **Renderer >
  D3D11 Opaque Unlit** to exercise the hardware renderer. The Playground uses
  materials outside that renderer's current scope and remains on CPU rendering.

## Current limits

This is an early training-target combat prototype and an engine sample, with
procedural art. The full enemy/health/death loop, persistent RPG progression,
multiplayer, raids, final anime art and natural-language AI are not connected to
this playable build. Unit-tested subsystems are not all exposed as game features.
The package is verified on the development PC; clean-machine and 24-hour soak
qualification are separate work. The game currently uses the CPU Astral renderer;
the optional D3D11 path belongs to the editor/generic engine player.

## Rebuild the package

Use VS2022 C++ tools, CMake 3.25+, and Python 3. Configure a sibling build folder
with `ASTRAL_BUILD_D3D11=ON` and `ASTRAL_ENABLE_HARDWARE_GPU_TESTS=ON`, build Debug
and Release, and run CTest for each configuration. Native tests must run alone
on an interactive desktop. The GPU suite uses the checked-in NativeGpu fixture
and creates a fresh `native-gpu-captures` directory under the build folder for each
run. Explicit ASTRAL_NATIVE_GPU_FIXTURE / ASTRAL_CAPTURE_DIR overrides remain
supported; an explicit capture directory must be fresh. Commit the tested source,
then run:

```powershell
python Scripts/package_playtest.py --build-dir ../astral-playtest-build --crt-dir "<Visual Studio>/VC/Redist/MSVC/<version>/x64/Microsoft.VC143.CRT" --output "<new package directory>"
```

The packager checks x64 Release binaries, bundles their imported Visual C++
runtime libraries, preserves existing output directories, and records source
identity plus file hashes in manifest.json. Build/test results are in
`Docs/QA/PLAYTEST-WINDOWS-2026-10-29.md`.
