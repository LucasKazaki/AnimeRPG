# M4 — Combat Sandbox

## Objective

Turn the verified M3 controller scene into a small playable combat sandbox. The player must be able to perform light and heavy attacks against a visible training dummy, with deterministic hit detection, damage, cooldown behavior, and a clear defeated state.

This is a gameplay vertical increment. It is **not** a renderer rewrite, a 3D-engine claim, physics system, enemy-AI system, asset pipeline, networking feature, or content/art pass.

## Player-facing behavior

- `J` triggers a light attack.
- `K` triggers a heavy attack.
- The scene contains one training dummy at a deterministic world position.
- An attack lands only when the dummy is alive and inside the defined attack range.
- Light and heavy attacks use distinct damage and cooldown values.
- Attempts during cooldown do not apply extra damage.
- A defeated dummy remains defeated and no longer receives damage.
- The window title and/or existing GDI debug scene clearly communicates attack/dummy state without adding assets or dependencies.
- Escape and window close retain the M3 clean-exit behavior.

## Allowed files

- `Tasks/M4-combat-sandbox.md`
- `CMakeLists.txt`
- `Engine/Platform/Win32Application.h`
- `Engine/Platform/Win32Application.cpp`
- `Engine/Renderer/Renderer.h`
- `Engine/Renderer/Renderer.cpp`
- `Engine/Scene/CombatSandbox.h`
- `Engine/Scene/CombatSandbox.cpp`
- `Tests/CombatSandboxTests.cpp`
- `Docs/QA/MILESTONE-4.md`
- `Docs/Decision-Log.md`
- `Docs/Planning/MILESTONES.md`

Do not modify other files without creating an amended packet that explains the requirement and preserves the isolated-worktree workflow.

## Design constraints

- C++17 and existing Astral types only.
- No external dependency, plugin, asset, graphics API, download, networking, or global installation is needed for this packet.
- Keep combat logic deterministic and directly unit-testable outside Win32 input handling.
- Keep rendering to the existing Win32/GDI debug renderer. A small primitive/marker is sufficient for the dummy.
- Configure only out of source. The dedicated build directory may be used with `cmake --build`; if absent, configure it with `cmake -S . -B Build -G "Visual Studio 17 2022" -A x64`.

## Required automated evidence

1. `CombatSandboxTests` must cover:
   - light damage;
   - heavy damage;
   - target out of range;
   - cooldown rejection;
   - dummy defeat and no post-defeat damage.
2. Debug build and CTest pass.
3. Release build and CTest pass.
4. `git diff --check` passes.
5. A native automated runtime smoke launches `AstralGame`, records the combat-state/title behavior for a controlled attack path, and confirms clean exit. It must be labeled automated, not manual visual QA.

## Merge gate

The task branch may be committed and merged into `main` automatically only after every automated evidence item passes, the diff is confined to the allowed files, and the final worktree is clean except for the intended committed changes.

## Stop conditions

Stop and report the exact blocker rather than broadening scope if M4 requires an external dependency, new rendering API, renderer redesign, engine-wide refactor, human-only runtime proof, or any file outside this packet.
