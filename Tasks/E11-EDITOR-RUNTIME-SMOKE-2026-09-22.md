# E11 Editor Runtime Smoke task, 2026-09-23

## Scope and ownership

Bounded verification-only packet for the integrated Win32 `AstralEditor`. This packet may harden the native Windows smoke, its containment/recovery supervisor, deterministic containment tests, and evidence. It must not add editor/game features, change rendering/API architecture, add dependencies, alter workflows outside the packet, merge/rebase, deploy/release, invoke R0, or restart paused game content.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Admitted baseline from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Allowed paths only: `CMakeLists.txt`, `Tests/EditorRuntimeSmoke.cpp`, this task, `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`, and `Docs/Research/ENGINE-CAPABILITIES.json`.
One active writer only. Do not rebase, merge, force-push, or absorb unrelated game-worker work.

## Current finding selected from independent review

Fresh Codex review of source/evidence head `d709017977c11a2f0cd9d9498d63a9980cbf5e3c` identified two P2 verification defects in the newly automated maximize/restore path:

1. The advertised three-second show-state deadline was not enforced across nested `ValidateShellState` cross-process message waits. A slow but individually responsive editor could finish validation after the deadline and still be accepted.
2. `ShowWindowAsync(SW_MAXIMIZE)` and `ShowWindowAsync(SW_RESTORE)` were reached after earlier ownership checks plus intervening window queries. The saved HWND therefore was not revalidated against the launched process immediately before each new side-effecting show-state post.

These are verification-harness findings. They do not authorize production editor changes or another dependent feature.

## Bounded repair

Implementation commit `4582d637a36d8d55981bda0886ef31d8643eef1b` changes only `Tests/EditorRuntimeSmoke.cpp`. Compare against `d709017...` reports 97 additions and 35 deletions in that one file. New smoke blob: `797eebbc3c4e587efa1c7f14b62230c532a52e15`.

The repair:

- adds a scoped local message deadline layered under the existing 135-second work budget;
- caps each `SendMessageTimeoutW` call during maximize/restore validation to the remaining show-state phase budget rather than the generic one-second message budget alone;
- caps polling sleeps to the remaining show-state phase budget;
- establishes the maximize and restore deadlines before the corresponding asynchronous show-state request;
- repeats process ownership, visibility, and enabled-state validation immediately before each `ShowWindowAsync` request;
- after a complete shell validation succeeds, rechecks the show-state deadline before accepting maximized or restored state;
- retains startup, 800x600, 1280x720, 1440x900, 420x260, child-HWND continuity, semantic shell checks, positive-area containment, bounded interaction, process identity hardening, job containment, and safe final close.

No production editor source, CMake registration, dependency, workflow, graphics API, scheduler configuration, game content, release/deployment state, or R0 code changed in this implementation commit.

## Primary-source basis, rechecked 2026-09-23

- Microsoft `ShowWindowAsync`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindowasync . The call posts a show-window event and returns when the operation is started, so completion must be positively polled and bounded.
- Microsoft `SendMessageTimeoutW`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendmessagetimeoutw . `uTimeout` bounds an individual cross-thread message wait and `SMTO_ABORTIFHUNG`/`SMTO_ERRORONEXIT` provide the existing fail-closed behavior; a phase deadline therefore has to cap each nested timeout if the overall three-second claim is to be real.
- Microsoft `GetWindowThreadProcessId`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowthreadprocessid . It provides the creating process/thread identity used to fail closed on stale or recycled HWNDs.
- Epic Unreal Engine 5.8, Using Editor Viewports: https://dev.epicgames.com/documentation/unreal-engine/using-editor-viewports-in-unreal-engine . Perspective 3D, orthographic 2D, multi-viewport layouts, and maximized/immersive authoring states remain comparison points.
- Unity 6.0, `EditorWindow.maximized`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/EditorWindow-maximized.html . Unity exposes maximized editor-window state as an editor workflow property.

Public behavioral/API documentation only. No proprietary engine source was copied and no dependency was imported.

## Portable source-logic evidence

Disposable C++17 fixture SHA-256 `b8c196a51b372982922165b5dc4d5044445d2279559ba3312505fec14a8f9dbb` models the new phase-budget and side-effect gate. It passed:

```text
g++ -std=c++17 -Wall -Wextra -Werror -pedantic e11_show_state_deadline_fixture.cpp -o e11_show_state_deadline_fixture_gcc
./e11_show_state_deadline_fixture_gcc
clang++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer e11_show_state_deadline_fixture.cpp -o e11_show_state_deadline_fixture_clang
ASAN_OPTIONS=detect_leaks=1 ./e11_show_state_deadline_fixture_clang
```

Both executions printed `e11 show-state deadline fixture: PASS`. The fixture proves only source-logic properties, not Win32 GUI behavior.

## Hosted evidence for implementation commit

For source commit `4582d637a36d8d55981bda0886ef31d8643eef1b`:

- Windows build and deterministic tests: run `35902292907`, job `107321254906`, completed success at `2026-09-23T18:27:06Z`. Repository/R0 safety checks, VS2022 x64 configure, Debug build/tests, Release build/tests, runtime/prerequisite checks, static verifiers, and clean tracked-tree verification all passed.
- Profiling capture portability: run `35902292991`, success.
- Release manifest integrity: run `35902292936`, success.

Hosted deterministic CTest intentionally excludes the interactive GUI `EditorRuntimeSmoke`. These results are compilation/deterministic evidence only and do not establish native maximize/restore, GPU/desktop behavior, screenshots, package launch, performance, stress, or soak acceptance.

## Retained gates

`native_evidence` remains empty. Independent final acceptance remains false until the exact post-evidence receipt tree receives fresh independent review and the registered Windows executor retains native evidence. Issue #7 is still open, so the historical R0 runner remains blocked.

The wider engine capability catalogue remains unresolved, including runtime/jobs/memory, scene ownership/serialization, full asset pipeline, GPU rendering/materials, lighting/shadows/reflections, large-world streaming/detail, animation, physics/collision, AI/navigation, audio, genuine 2D, networking, profiling/budgets, packaging/platforms, terrain/foliage, VFX/particles, cinematics, scripting/reflection, input/replay, accessibility/localization, additional platforms, comparative performance/reliability, clean-machine packaging, stress/recovery, and the required 24-hour soak. No UE5/Unity parity claim follows from this packet.

## Registered native handoff

Only after green hosted checks and a fresh clean review of the exact post-evidence receipt tree, the registered Windows executor should use one owned interactive desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact reviewed source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, and process inspection proving zero owned contained processes after any failure/interruption. Capture untouched startup, 800x600, 1280x720, 1440x900, actual maximized desktop, and 420x260 or closest OS-permitted narrow state. Confirm containment/positive area, viewport clipping, semantic continuity, Cube selection/Inspector synchronization, disabled pending tools, bounded maximize/restore, and safe close. Separately launch `AstralGame` from the same source/build as a no-regression check.

## Rollback and stop conditions

Rollback only this verification repair if native evidence proves the phase-budget/ownership contract itself is invalid under the existing owned-desktop requirements. Do not weaken or delete the maximized-state acceptance requirement to make the test pass. Stop before production-runtime changes, dependency/API changes, workflow changes outside packet authority, rebase/merge, R0 execution, scheduler operations, release/deployment, or game-content work.

## Single next useful action

Refresh the QA/capability receipts and PR checkpoint around `4582d637...`, obtain fresh independent review of the exact final receipt head, then, if clean, hand that exact reviewed tree to the registered Windows executor. Do not start another dependent editor feature while the native QA gate is unresolved.
