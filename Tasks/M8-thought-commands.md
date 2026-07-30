# M8 — Thought Commands

## Objective

Add a bounded, deterministic “Thought Commands” layer to the M7 playable prototype: the user issues a small explicit command vocabulary that safely routes to Shadowblade abilities and a local slow-time state. This is an in-client command interface, not voice control, generative planning, autonomous NPC control, networked services, or a general natural-language agent.

## Required behavior

- Add a pure C++ command parser/domain with an intentionally small case-insensitive grammar:
  - `dash` → request the existing M7 dash;
  - `fatal` → request the existing M7 fatal strike;
  - `guard on` / `guard off` → command held guard state;
  - `focus` → toggle a bounded local slow-time multiplier.
- Reject empty, ambiguous, or unsupported commands with deterministic result/reason codes and no gameplay mutation.
- Preserve existing M7 resource/cooldown/range/guard conflict gates; commands must call the existing domain rather than duplicate or bypass rules.
- Bind an explicit non-text entry control appropriate to the Win32 prototype (e.g. number keys or a compact command-entry mode) and show the submitted command, result/rejection reason, and focus state in title/GDI feedback.
- Focus must be local/offline, bounded, deterministic, and visibly stateful. It must not imply real-time global simulation, network time control, or unsupported AI behavior.
- Retain M5 movement/world and M4/M7 combat behavior.

## Verification gates

1. New focused parser/domain tests cover normalization, every supported command, unsupported/no-op behavior, resource/cooldown/guard delegation, and focus bounds.
2. Debug and Release out-of-source builds and all CTest pass.
3. Native M8 runtime smoke launches the real game, invokes at least one successful command, one rejected command, visible focus state, and Escape exit.
4. `git diff --check`, scope check, no source-root CMake output, and no downloads/dependencies.

## Integration

Commit only after all gates pass. Merge only after canonical `main` conflict safety is rechecked. The existing unrelated untracked main documents must remain untouched.
