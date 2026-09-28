# Architecture Decision 0002 — Portable runtime and opt-in Astral renderer

Date: 2026-09-28
Status: Proposed on branch `claude/loving-clarke-el24od`. Lucas approves any merge.
Builds on: ADR-0001 (custom C++17 Astral Engine stays the production runtime).

## Context

The runtime was Win32 plus GDI wireframes. The engine capability map
(`Docs/Research/ENGINE-CAPABILITIES*`) lists the subsystems a UE5- or Unity-class
engine provides and Astral lacked. ADR-0001 deferred the GPU API choice until a
toolchain and renderer spike exist, and this session has no Windows toolchain
or GPU. Gameplay domains are engine-free and must stay authoritative.

## Decision

1. **Portable runtime modules.** New subsystems live in `Engine/{Core,Math,
   Graphics,World,Physics,Animation,VFX,Input,Audio,AI}` as platform-independent
   C++17 with no third-party code.
   - One source list, `cmake/AstralEngineSources.cmake`, feeds both builds.
   - The root (MSVC) build links it into `AstralGame`.
   - The headless Linux build (`Tests/EngineRuntime`) runs with `-Werror`,
     ASan+UBSan and TSan.
2. **CPU visibility-buffer renderer first.** `SceneRenderer` consumes a
   backend-neutral `RenderScene` (draw items, materials, lights, sky, fog,
   shadows, post) and writes an sRGB framebuffer. Win32 presents it with
   `StretchDIBits`.
   - It ships the anime look now: toon ramps, rim, ink outlines and a neutral
     tonemap.
   - It is the deterministic reference oracle for the future GPU backend.
3. **Opt-in presentation over authoritative gameplay.** Astral mode is reached
   with F2 or `ASTRAL_RENDER_MODE=astral`; GDI stays the default.
   - Every Win32 hook is a no-op while Astral mode is off.
   - Movement and dashes are swept against the physical Mall only after the
     gameplay domain has decided them (`Game/Showcase/GameplayBridge`).
4. **Headless mirror for evidence.** `ShowcaseSession` reproduces the Win32
   frame order, so tests and `Tools/AstralCapture` exercise the real rules
   without a desktop.

## Consequences

- Existing smokes, titles, bindings and GDI output are unchanged. The milestone
  verifiers and test-safety contract still pass.
- `Engine/Platform/Win32Application` includes a `Game/Showcase` header for the
  opt-in path. This is an accepted inversion; move the presenter behind an
  interface if a second platform layer appears.
- The CPU renderer is not a real-time 1080p solution on its own. A GPU RHI
  behind `RenderScene` is the next rendering decision and needs its own ADR
  (API, toolchain, device-loss and resize contracts).
- The Win32 loop and `ShowcaseSession` duplicate the frame order. The shared
  bridge and the M10 route-parity test limit drift.
- No dependency, plugin, network or asset download was added. `winmm` (system
  library) is linked for optional waveOut audio, which fails silent.
