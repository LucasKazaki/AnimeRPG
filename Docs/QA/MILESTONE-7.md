# Milestone 7 QA — Shadowblade Action Kit

Date: 2026-07-30
Branch: `task/m7-shadowblade`
Worktree: `C:/AI/worktrees/AnimeRPG/m7-shadowblade`

## Implemented contract

- `ShadowbladeActions` owns deterministic capped shadow resource, positive-finite-delta regeneration, dash/fatal cooldown countdown, held guard state, and explicit activation/rejection outcomes.
- `Q` performs a six-unit forward dash for 25 resource. The live path applies the destination through `PlayerController::SetPosition`, retaining the existing world bounds and camera follow.
- `L` performs an 80-damage fatal strike for 50 resource when the live training dummy is within 3.5 units; damage is applied through `CombatSandbox::ApplyDamage` and cannot exceed remaining health.
- Held left Shift visibly guards, blocks J/K combat activation and Q/L Shadowblade conflicts, and presents a yellow GDI guard ring plus title state.
- The existing M5 perspective grid, landmarks, player, dummy, movement, combat controls, and Escape exit remain active.
- The title exposes resource, guard, cooldown tenths, last Shadowblade action/result, damage, dummy health, and player position. A cyan GDI resource bar and guard label provide in-client feedback.

## Automated evidence

Out-of-source build tree: `C:/AI/builds/AnimeRPG/m7-shadowblade`.

Configure:

`cmake -S C:/AI/worktrees/AnimeRPG/m7-shadowblade -B C:/AI/builds/AnimeRPG/m7-shadowblade -G "Visual Studio 17 2022" -A x64`

Result: PASS with Visual Studio 2022 x64 and Windows SDK 10.0.26100.0.

Debug:

- `cmake --build C:/AI/builds/AnimeRPG/m7-shadowblade --config Debug --parallel` — PASS.
- `ctest --test-dir C:/AI/builds/AnimeRPG/m7-shadowblade -C Debug --output-on-failure -V` — PASS, 8/8.

Release:

- `cmake --build C:/AI/builds/AnimeRPG/m7-shadowblade --config Release --parallel` — PASS.
- `ctest --test-dir C:/AI/builds/AnimeRPG/m7-shadowblade -C Release --output-on-failure -V` — PASS, 8/8.

`ShadowbladeActionsTests` covers dash destination/cost/cooldown, resource regeneration and cap, non-positive/non-finite delta rejection, fatal range/damage/cooldown/defeat, guard conflicts, and insufficient resource rejection for both dash and fatal strike.

## Automated native M7 runtime smoke

`M7RuntimeSmoke` launches the configuration-matched actual `AstralGame`, finds its process-owned window, drives left Shift, Q, L, and Escape through `SendInput` into the real `GetAsyncKeyState` loop, reads title state, and captures live GDI pixels.

Observed in both Debug and Release:

1. Initial title reported `M7 Shadowblade`, 100/100 resource, guard off, zero cooldowns, dummy 100/100, and position `(0,0)`.
2. Held left Shift changed title to `Guard: ON` and rendered a yellow guard ring (1,669 matching client pixels).
3. Q while guarding reported `Dash Blocked by Guard`, consumed no resource, and did not move.
4. L after guard release reported `Fatal Activated -80`, reduced resource to 50 and dummy health to 20, and started the fatal cooldown.
5. Q reported `Dash Activated`, moved the live bounded controller from Z 0 to Z 6, consumed its defined resource, and started the dash cooldown.
6. A second edge-triggered Q reported `Dash Cooldown` without moving.
7. The cyan resource bar was visible (2,520 matching client pixels at full resource).
8. Escape terminated the actual game with exit code 0.

Result: `M7 AUTOMATED NATIVE RUNTIME SMOKE: PASS` in Debug and Release. Existing M4 and M5 native runtime smoke tests also passed in both configurations.

## Scope and quality gates

- No dependencies, downloads, plugins, assets, networking, framework replacement, or source-tree CMake outputs were introduced.
- All M7 source, test, build, packet, planning, decision, and QA changes are within the packet allowlist.
- Existing untracked `.hermes/` continuity-supervisor artifacts were preserved and excluded from the M7 commit.
- `python Scripts/verify_milestone3.py`: PASS.
- `git diff --check`: PASS.
- Changed-file scope audit: PASS.

## Honest capability and risks

This milestone is a deterministic playable Shadowblade layer over the prototype world, not animation, VFX, directional facing, invulnerability, enemy AI, or production combat balancing. Dash currently uses the prototype world's fixed positive-forward axis and controller clamp. Guard is an active conflict state and visual stance; no incoming enemy damage system exists yet, so damage mitigation is outside this packet.
