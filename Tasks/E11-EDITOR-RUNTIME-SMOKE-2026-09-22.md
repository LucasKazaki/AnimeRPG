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

## Hosted evidence and exact reviewed receipt

For implementation source commit `4582d637a36d8d55981bda0886ef31d8643eef1b`:

- Windows build and deterministic tests: run `35902292907`, job `107321254906`, completed success at `2026-09-23T18:27:06Z`.
- Profiling capture portability: run `35902292991`, success.
- Release manifest integrity: run `35902292936`, success.

For exact final receipt/source head `0e7564982ca4a61f84840c1b9e6da6d9a3d7ef9d`:

- current observed `main` at the tested integration point: `4c3308051c910b66dd0aebcf513956926beab505`;
- GitHub synthetic PR merge actually tested: `5fd749061e2d146a03240232366f17d611327aef`, whose parents are base `4c3308051c910b66dd0aebcf513956926beab505` and source `0e7564982ca4a61f84840c1b9e6da6d9a3d7ef9d`;
- Windows build and deterministic tests: run `35902826792`, job `107323032434`, completed `success` at `2026-09-23T18:31:47Z`;
- profiling capture portability: run `35902826812`, `success`;
- release manifest integrity: run `35902826744`, `success`;
- fresh independent Codex review completed `2026-09-23T18:38:09Z` on exact commit `0e756498...` and reported no major issues. No new inline finding was produced for that exact head.

The Windows job passed repository/R0 safety contracts, VS2022 x64 configure, Debug build/tests, Release build/tests, runtime/prerequisite checks, static milestone verifiers, and clean tracked-tree verification.

Hosted deterministic CTest intentionally excludes the interactive GUI `EditorRuntimeSmoke`. These results are compilation/deterministic/integration evidence only and do not establish native maximize/restore, GPU/desktop behavior, screenshots, package launch, performance, stress, or soak acceptance.

## Retained gates

`native_evidence` remains empty. The exact receipt tree has now satisfied the fresh independent implementation-review gate, but E11 final acceptance remains false until the registered Windows executor produces the required native interactive evidence on that exact reviewed tree. Issue #7 is still open, so the historical R0 runner remains blocked.

The wider engine capability catalogue remains unresolved, including runtime/jobs/memory, scene ownership/serialization, full asset pipeline, GPU rendering/materials, lighting/shadows/reflections, large-world streaming/detail, animation, physics/collision, AI/navigation, audio, genuine 2D, networking, profiling/budgets, packaging/platforms, terrain/foliage, VFX/particles, cinematics, scripting/reflection, input/replay, accessibility/localization, additional platforms, comparative performance/reliability, clean-machine packaging, stress/recovery, and the required 24-hour soak. No UE5/Unity parity claim follows from this packet.

## Registered native handoff, now unblocked by hosted/review gates

The registered Windows executor should use one owned interactive desktop on exact reviewed source SHA `0e7564982ca4a61f84840c1b9e6da6d9a3d7ef9d`:

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

Run the registered native Debug/Release `EditorContainmentTests` and interactive `EditorRuntimeSmoke` handoff on exact reviewed SHA `0e7564982ca4a61f84840c1b9e6da6d9a3d7ef9d`, retain the required machine/toolchain/GPU/desktop/process evidence and screenshots, then launch `AstralGame` separately as the same-build no-regression check. Do not start another dependent editor feature while this native QA gate is unresolved.

## Bounded continuation: exact restore placement, 2026-09-23

This continuation supersedes the earlier handoff SHA above until the new source is independently reviewed. While auditing the already-automated maximize/restore acceptance path, the coordinator reproduced a false-pass condition in the test logic: after `SW_RESTORE`, the smoke accepted any window with the original width and height, even if the restored top-left position was wrong. Microsoft documents `SW_RESTORE` as restoring a maximized/minimized/arranged window to its original **size and position**, and `GetWindowRect` returns the screen-space bounding rectangle, so width/height alone was weaker than the Windows behavior being claimed.

Source repair `3020590f091c68cc802016c02fcbbe54e5c7cebf` changes only `Tests/EditorRuntimeSmoke.cpp`, 5 additions and 3 deletions. New smoke blob: `3bb8774e33113e3e50357ed77c7054f486d339a9`. The restore poll now requires all four `GetWindowRect` members, left/top/right/bottom, to equal the pre-maximize rectangle before it accepts restored state. The timeout diagnostic now says `outer rectangle` rather than `outer size`. No production editor source, CMake, workflow, dependency, graphics API, scheduler configuration, game content, release/deployment state, or R0 code changed.

Primary sources rechecked for this continuation:

- Microsoft `ShowWindow` / `SW_RESTORE`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow . `SW_RESTORE` restores the original size and position.
- Microsoft `GetWindowRect`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowrect . The returned rectangle contains screen-coordinate upper-left and lower-right corners, making an exact outer-rectangle comparison directly measurable.
- Microsoft `ShowWindowAsync`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindowasync . The asynchronous request still requires the existing bounded positive completion poll.
- Epic Unreal Engine 5.8 `Using Editor Viewports` and Unity 6.0 `EditorWindow.maximized` remain workflow references for maximized/restored editor use. This repair copies no proprietary source and adds no dependency.

Disposable C++17 restore-rectangle fixture SHA-256 `ab2d1952a2e30036e396b4652bcff0a3190fdf629c9c43606ed760eb7cbe4d15` passed warning-clean GCC C++17 and Clang C++17 with ASan+UBSan plus leak detection. The fixture demonstrates that the previous same-size predicate accepts shifted rectangles while the new exact-rectangle predicate rejects them. This is source-logic evidence only, not native Win32 GUI evidence.

Source-associated hosted workflows for `3020590f...` were started as runs `35915703271` (Windows), `35915703261` (profiling), and `35915703309` (release manifest). At this checkpoint profiling had completed successfully; Windows and release-manifest were still running, so they are not counted as passed yet. The prior clean review of `0e756498...` does not cover this new source change. Fresh independent review is required before any native handoff may treat `3020590f...` or a later evidence-only descendant as reviewed.

Updated stop condition: do not run the registered native handoff on the superseded `0e756498...` if the goal is to accept current E11. First require hosted checks plus fresh independent review on the new exact-restoration source/evidence tree. After those gates are clean, the registered Windows executor should run the same Debug/Release `EditorContainmentTests` and interactive `EditorRuntimeSmoke` matrix and specifically retain evidence that maximize followed by restore returns the editor to the same pre-maximize outer screen rectangle, in addition to the existing containment, semantic continuity, screenshots, cleanup, and `AstralGame` no-regression evidence.
