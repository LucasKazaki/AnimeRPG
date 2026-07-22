# Architecture Decision 0001 — Production Engine

Date: 2026-07-21
Status: Accepted for implementation; Lucas approval still required for any later switch to Unreal.

## Decision
Proceed with the PRD's custom C++ Astral Engine as the production runtime. The Unreal Specialist role is retained as the game-development coordinator and reviewer, not as a runtime dependency. Unreal may be used only for throwaway reference work if explicitly approved later.

## Why
The PRD names custom C++ as a product pillar, calls Unreal/Unity out as excluded, and defines engine-specific acceptance criteria. The current workspace contains no Unreal project, so adopting Unreal now would be a deliberate product change rather than an implementation detail.

## Consequences
- Milestone 1 uses a native Windows application and no Unreal plugins/assets.
- The first renderer is intentionally a GDI clear-color stub; DirectX/Vulkan selection is deferred until a toolchain and renderer spike are available.
- Gameplay, art, and world agents remain blocked behind engine/runtime contracts instead of creating incompatible assets.
- A future Unreal proposal must be a separate Lucas-approved PRD change.

## Smallest safe work while broader direction is validated
Build and test a minimal Windows window, loop, timing, input, logging, clear renderer, and math library. Do not start full combat, art production, multiplayer, or external asset acquisition.
