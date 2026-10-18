# LD-001 author QA receipt - September 24, 2026

## Scope and environment
Portable source-component evidence only. Production baseline inspected through
GitHub connector: `2958741188279a0b438dd489ad00cac546012c25`.
A direct HTTPS clone failed because the sandbox could not resolve github.com.
A dedicated local git worktree was used for the authored slice, NOT a full
production checkout. The two existing test-support files were reconstructed from
exact connector-fetched contents. Their Git blob identities are:
- cmake/AstralTestSafety.cmake: `92b04634bf8f7214e80bdf495a815550413780e3`
- Tests/AssertionsEnabled.cpp: `8c87950f6c15ae8784718a93311498cdc6ff9b09`
They are NOT part of the proposed commit. No workstation or native Windows
execution occurred. No full-repository regression is claimed.

## Executed checks
Linux GCC 14.2.0, Clang 17.0.0, CMake 3.31.6.
All commands below exited 0 against the source identities listed below:
```
cmake -S Tests/LocalizedDamage -B ../localized-damage-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build ../localized-damage-build-debug
ctest --test-dir ../localized-damage-build-debug --output-on-failure
cmake -S Tests/LocalizedDamage -B ../localized-damage-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build ../localized-damage-build-release
ctest --test-dir ../localized-damage-build-release --output-on-failure
clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -I. Tests/LocalizedDamage/LocalizedDamageTests.cpp Tests/AssertionsEnabled.cpp -o ../localized-damage-sanitized
ASAN_OPTIONS=detect_leaks=1 ../localized-damage-sanitized
```
Debug and Release each passed 1/1 standalone CTest targets. Each target exercises
32 named cases, including 2,000 fixed-seed independently calculated armor,
integrity and vitality trials. Clang AddressSanitizer/UndefinedBehaviorSanitizer
with leak detection passed the same cases without a diagnostic. These are not
2,032 independent test targets and are not performance measurements.

Coverage includes profile validation/cycles/bounds, invalid atomic configuration,
armor overflow, weak multipliers, typed susceptibility/immunity, covered core,
removal, break/re-break reward flags, ability disabling, impairment floors,
friendly fire, malformed/stale/duplicate/out-of-order receipts, maximum counter,
integer saturation, all three defeat modes, non-reviving recovery and player/ally
profiles. The prototype boss action hook now returns None for an unconfigured or
defeated body; its live fallback is not confused with permission to act after death.

## Review boundary
This is AUTHOR verification and a self-review, NOT independent QA approval.
Independent exact-head review is pending. Request it on the draft PR. Do not merge
this cross-engine/game packet using the game worker's unrelated blanket authority.
Hosted default CI may run existing tests but does not automatically execute this
new standalone suite until registration is coordinated in LD-E04.

## Remaining acceptance
No actual collision/picking, runtime attack migration, controller/mouse UI,
animation/art/audio, timed wound/recovery, serialization, replication, full
Windows Debug/Release regression, native playtest, performance or engine soak
was verified. Ability efficiency remains data until production consumers exist.
The stateful Body MUST NOT be layered beside existing authoritative health and
charged in parallel. Publish as a portable foundation and integration handoff,
not an implemented playable boss or a finished War Thunder simulation.

## Authored file identity
Git blob hashes pin the tested source and companion documents (this receipt is
not self-hashed). New files only; no old path is replaced or removed.

- `Engine/Gameplay/LocalizedDamage.h`: `6d8d09554979ee1ba1aa64c0355bdcc0c8ab0edd`
- `Game/Combat/LocalizedDamageProfiles.h`: `33018c3d95963f3d952346bf6042ea8db49eb4e5`
- `Tests/LocalizedDamage/LocalizedDamageTests.cpp`: `21988213136c1b778028f51a36599f6a85d4043a`
- `Tests/LocalizedDamage/CMakeLists.txt`: `2891b1e691b9a97778ffaff4bfba271cc65274e8`
- `Docs/Design/LOCALIZED-DAMAGE.md`: `d9fb9bdc0d898da962956ddab081a4c30b2feb89`
- `Docs/Engine/LOCALIZED-DAMAGE-CONTRACT.md`: `1e661c7043e1f4046aebc925d3a483f1fcf10b28`
- `Docs/Game/LOCALIZED-DAMAGE-ENCOUNTERS.md`: `d12114cf8e727bb5446fa61f4875af3844c7d912`
- `Docs/Design/localized-damage-backlog.json`: `0785f278ac41d3043cbbbc692372b3092cfdb40a`
- `Tasks/LOCALIZED-DAMAGE-2026-09-24.md`: `f0eef4a04de7a743fa8c714e657b67651f010241`
