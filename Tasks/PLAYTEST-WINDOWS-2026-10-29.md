# Windows playtest delivery — October 29, 2026

Lucas directly requested getting Astral Engine and AnimeRPG working in GitHub,
downloading to this PC, and running/testing them as quickly as possible. This
authorizes this task's local builds, native tests and playtests despite historical
game-playtest pauses and expired execution plans. It does not claim full engine
acceptance, a 24-hour soak, multiplayer, or completion of the RPG roadmap.

Owner: current playtest chat. Dedicated branch: playtest/windows-ready-20261029.
Base: c2ff2ce, two reviewed tooling commits above origin/main 09ab28f.
Preserve all previous worktrees and their evidence.

Allowed scope: integrate reviewed entry-recovery and prompt-release changes;
repair blockers in native input, build, tests and launch/packaging; add reproducible
Windows playtest launch/package scripts and concise instructions. Keep the custom
C++17/CMake/Win32 architecture and existing content. New unrelated features,
third-party dependencies and architecture migrations are excluded.

Build output: sibling astral-playtest-build. Evidence: sibling playtest-evidence.
Package: C:/Users/lucas/Games/AstralPlaytest-2026-10-29.

Validation: configure root CMake using VS2022 x64 with D3D11 and hardware test
targets enabled; build Debug and Release with at most four compiler workers;
run each complete CTest inventory, serializing native/GPU tests. Run repository
Python static/build contract checks. Inspect the game, editor and engine samples
in the actual desktop, validate package launches, and retain exact output.
Stop a failing test, diagnose, repair and rerun the affected checks. Bound native
tests with existing CTest timeouts. Independent review is required before delivery.

GitHub delivery: push a reviewable branch/PR with verification and limits. Do not
silently merge unrelated branches or overwrite the existing main checkout.
