# Engine uplift: portable runtime subsystems and opt-in Astral renderer

Authorized by Lucas's September 28 request to audit the AnimeRPG / AnimeRPG-UE5
pair and bring Astral Engine toward UE5 and other top-tier engine capability
levels, while preserving the features that make the game distinct.
Base: `main` at `2958741188279a0b438dd489ad00cac546012c25`.
Branch: `claude/loving-clarke-el24od`, one cloud container, one owner.

This packet keeps the custom C++17 Astral Engine (ADR-0001). It does not
migrate to Unreal, import third-party code, install dependencies, or choose a
GPU API. New subsystems are written from scratch in portable C++17 so they can be
built and sanitizer-tested without Windows. The Win32 platform layer and
GDI remain the presentation path. The new renderer writes a CPU framebuffer that
is presented with `StretchDIBits`.

## Preserved behavior (non-negotiable)

- The default GDI wireframe renderer, window titles, HUD text, key bindings and
  every existing runtime smoke contract stay unchanged.
- Existing gameplay domains stay authoritative: `CombatSandbox`,
  `ShadowbladeActions`, `ThoughtCommands`, `LandmarkInteraction`,
  `LandmarkEncounter` and the rest of `Engine/Scene`. The new runtime only
  observes them and presents them, and it moves the player only through
  `PlayerController::SetPosition`.
- The Astral renderer is opt-in with F2 or `ASTRAL_RENDER_MODE=astral`.

## Allowed paths

- New: `Engine/Math/VectorMath.h`, `Engine/Math/Geometry.h`
- New: `Engine/Core/{JobSystem,SlotMap,Profiler,GameTime,Random}.*`
- New directories: `Engine/Graphics`, `Engine/World`, `Engine/Physics`,
  `Engine/Animation`, `Engine/VFX`, `Engine/Input`, `Engine/Audio`, `Engine/AI`
- New: `Game/Showcase/**`, `Tools/AstralCapture.cpp`
- New tests: `Tests/Engine*Tests.cpp`, `Tests/EngineRuntime/CMakeLists.txt`
- Edits: `CMakeLists.txt` (engine library, new tests via `astral_add_test`),
  `Engine/Platform/Win32Application.{h,cpp}` (opt-in presentation path only),
  `.github/workflows/engine-runtime.yml` (new), `README.md`
- Docs: this packet, `Docs/Audit/**`, `Docs/Architecture/ADR-0002-*.md`,
  `Docs/QA/ENGINE-UPLIFT-2026-09-28.md`, `Docs/Research/ENGINE-CAPABILITIES*`

## Acceptance

1. Portable Debug, Release, ASan+UBSan and TSan (job system) runs of all new tests
   pass on Linux GCC and Clang with `-Wall -Wextra -Wpedantic -Werror`.
2. `python Scripts/test_test_safety.py` and `verify_milestone{1,2,3}.py` still pass.
3. Headless captures of the showcase are written and inspected. Semantic pixel
   assertions cover depth order, shadows, outlines, SSR and the HUD.
4. Hosted MSVC Debug/Release build and deterministic CTest via `windows-ci`.
5. Native interactive F2 mode, frame-time budgets on Lucas's GPUs/CPUs, the 24-hour
   soak and independent review remain **local acceptance gates**. This packet
   does not claim them.

## Stop conditions

Stop at the first deterministic blocker. Never weaken an existing test, marker or
smoke to make new work pass. Record the exact commands and exits in the QA record.

## Status (2026-09-28)

Acceptance items 1–3 are met in the portable sandbox and recorded in
`Docs/QA/ENGINE-UPLIFT-2026-09-28.md`. Item 4 (hosted MSVC via `windows-ci`) and
item 5 (native and interactive gates) are open. Audit:
`Docs/Audit/ENGINE-AUDIT-2026-09-28.md`; decision: `Docs/Architecture/ADR-0002-*`.
