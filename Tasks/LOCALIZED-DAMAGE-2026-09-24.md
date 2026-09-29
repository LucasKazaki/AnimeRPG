# LD-001: shared localized-damage research and portable foundation

## Operator request and source baseline
Lucas explicitly requested research and GitHub work for configurable boss weak
spots and part-specific damage/impairment on enemies, players and friendlies.
Observed production baseline: `2958741188279a0b438dd489ad00cac546012c25`.
Astral Engine and AnimeRPG currently share `LucasKazaki/AnimeRPG`; the separate
`AnimeRPG-UE5` experiment is NOT the engine and is outside this request.
Read AGENTS.md and GAME_DEVELOPMENT_CONTROL.md at that baseline. This packet
advances a newly requested independent source component, not engine acceptance.

## Scope and ownership
Use a dedicated branch/worktree `feature/localized-damage-20260924`.
Only NEW paths below are admitted; no pre-existing file is modified:
- Engine/Gameplay/LocalizedDamage.h
- Game/Combat/LocalizedDamageProfiles.h
- Tests/LocalizedDamage/LocalizedDamageTests.cpp
- Tests/LocalizedDamage/CMakeLists.txt
- Docs/Design/LOCALIZED-DAMAGE.md
- Docs/Engine/LOCALIZED-DAMAGE-CONTRACT.md
- Docs/Game/LOCALIZED-DAMAGE-ENCOUNTERS.md
- Docs/QA/LOCALIZED-DAMAGE-2026-09-24.md
- Docs/Design/localized-damage-backlog.json
- Tasks/LOCALIZED-DAMAGE-2026-09-24.md

Do not edit root CMake, workflows, runtime, collision, graphics, active combat,
R0, existing hourly continuation records, or another worker's PR. Specifically
avoid concurrent PR #67's ShadowCryptSkirmish/ThoughtCommandsTests paths and
PR #13's root CMake/editor scope. No proprietary assets or dependencies.

## Deliverables and stop
Deliver source-backed design, generic C++17 opt-in component, original game
profiles, registered standalone native tests, and split worker backlog. Stop at
a draft PR plus engine/game issues. Do not merge under this packet. The source
component is NOT in AstralGame; native Windows, full regression, independent
review, collision/input/AI integration and playtests remain explicit gates.

## Commands (run from the repository/worktree root)
```
cmake -S Tests/LocalizedDamage -B ../localized-damage-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build ../localized-damage-build-debug
ctest --test-dir ../localized-damage-build-debug --output-on-failure
cmake -S Tests/LocalizedDamage -B ../localized-damage-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build ../localized-damage-build-release
ctest --test-dir ../localized-damage-build-release --output-on-failure
clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -I. Tests/LocalizedDamage/LocalizedDamageTests.cpp Tests/AssertionsEnabled.cpp -o ../localized-damage-sanitized
../localized-damage-sanitized
```
Standalone test CMake reuses the existing `astral_add_test` helper and its
assertion guard; it does not silently register into the production root build.
For MSVC use distinct external x64 build roots and `--config Debug`/`Release`
plus `ctest -C Debug`/`Release`. Keep native results separate from Linux receipts.

## Acceptance before eventual integration
Independent reviewer must inspect final exact source SHA without authoring it.
Engine/game workers agree a single health owner, hit identity and effect contract.
Register tests in root CMake only in an engine-coordinated follow-up. Run complete
existing combat, defense, training, scoring, replay and mission-accounting tests
in Windows Debug and Release. Then verify real collision, controls, audiovisual
feedback and performance on an owned native desktop. No acceptance from documents.
