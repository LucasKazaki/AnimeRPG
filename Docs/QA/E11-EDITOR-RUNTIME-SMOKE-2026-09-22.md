# E11 Editor Runtime Smoke evidence, 2026-09-23

## Current checkpoint

Branch: `engine/2026-09-22-editor-runtime-smoke`.
Current source repair: `4582d637a36d8d55981bda0886ef31d8643eef1b`.
Exact clean-reviewed receipt/source tree: `0e7564982ca4a61f84840c1b9e6da6d9a3d7ef9d`.
`Tests/EditorRuntimeSmoke.cpp` blob: `797eebbc3c4e587efa1c7f14b62230c532a52e15`.
`CMakeLists.txt` remains unchanged at blob `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.
Production editor/game source remains unchanged by this evidence update.

The previous receipt `d709017977c11a2f0cd9d9498d63a9980cbf5e3c` automated native maximize/restore. Independent Codex review completed at `2026-09-23T17:35:14Z` with two P2 findings in that new verification path. Source repair `4582d637...` addressed both findings. A later fresh independent Codex review completed at `2026-09-23T18:38:09Z` on exact receipt/source tree `0e756498...` and reported no major issues; no new inline finding was produced for that exact head.

## Review findings repaired

### 1. Show-state deadline could be exceeded during validation

The old helper allowed each nested `SendMessageTimeoutW` to consume up to the generic one-second message timeout even when the three-second maximize/restore phase was nearly exhausted. `4582d637...` adds a scoped message-phase deadline. Every bounded cross-process send during maximize/restore now uses the smaller of the global work budget, normal one-second message ceiling, and remaining show-state budget. Poll sleeps are capped to the remaining phase budget, and a successful shell validation is followed by an explicit deadline check before state acceptance.

### 2. Show-state posts lacked adjacent ownership revalidation

The earlier implementation performed ownership checks, then intervening window queries, then side-effecting `ShowWindowAsync` through a cached HWND. `4582d637...` establishes each phase deadline first, then repeats launched-process ownership plus visible/enabled-state validation immediately before `ShowWindowAsync(SW_MAXIMIZE)` and `ShowWindowAsync(SW_RESTORE)`. Subsequent polling keeps process-identity checks.

GitHub compare `d709017...` -> `4582d637...` reports exactly one modified file, `Tests/EditorRuntimeSmoke.cpp`, with 97 additions and 35 deletions. No production editor source, CMake registration, workflow, dependency, graphics API, architecture, scheduler setting, game content, release, deployment, or R0 code changed.

## Primary-source research

Accessed 2026-09-23:

- Microsoft Learn, `ShowWindowAsync`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindowasync . It sets show state without waiting for completion and posts the show-window event to the target window queue.
- Microsoft Learn, `SendMessageTimeoutW`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendmessagetimeoutw . `uTimeout` bounds each cross-thread wait; the phase deadline therefore must cap nested waits if the three-second phase claim is to remain true.
- Microsoft Learn, `GetWindowThreadProcessId`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowthreadprocessid . It supplies the creating process/thread identity used for fail-closed HWND checks.
- Epic, Unreal Engine 5.8, Using Editor Viewports: https://dev.epicgames.com/documentation/unreal-engine/using-editor-viewports-in-unreal-engine . Perspective 3D, orthographic 2D, multi-viewport layouts, maximized viewports, and immersive mode remain editor comparison workflows.
- Unity 6.0, `EditorWindow.maximized`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/EditorWindow-maximized.html . Unity exposes maximized editor-window state.

Behavioral/API comparison only. No proprietary Epic/Unity source copied and no dependency imported.

## Portable fixture

Disposable source-logic fixture SHA-256 `b8c196a51b372982922165b5dc4d5044445d2279559ba3312505fec14a8f9dbb` passed warning-clean GCC C++17 and Clang C++17 ASan+UBSan with leak detection. It covers remaining-phase timeout capping, rejection after late validation, fail-closed ownership before a simulated side effect, and exhausted-deadline rejection. This is source-logic evidence only, not native Win32 GUI evidence.

## Hosted verification and provenance

Implementation source `4582d637a36d8d55981bda0886ef31d8643eef1b` passed Windows run `35902292907` / job `107321254906`, profiling `35902292991`, and release-manifest `35902292936`.

Exact clean-reviewed receipt/source tree `0e7564982ca4a61f84840c1b9e6da6d9a3d7ef9d` has:

- source-associated Windows run `35902826792`, job `107323032434`, `success`, completed `2026-09-23T18:31:47Z`;
- source-associated profiling run `35902826812`, `success`;
- source-associated release-manifest run `35902826744`, `success`;
- tested synthetic pull-request merge `5fd749061e2d146a03240232366f17d611327aef`;
- tested base parent `4c3308051c910b66dd0aebcf513956926beab505` and source parent `0e7564982ca4a61f84840c1b9e6da6d9a3d7ef9d`;
- fresh independent Codex review completed `2026-09-23T18:38:09Z`, result: no major issues and no new inline finding for that exact head.

The Windows workflow passed repository/R0 safety contracts, prerequisite/runtime policy checks, VS2022 x64 configuration, Debug build/tests, Release build/tests, static milestone verifiers, and clean tracked-tree verification. Hosted CTest intentionally excludes interactive `EditorRuntimeSmoke`. These runs establish hosted compilation, deterministic-test, and PR-integration compatibility only, not native GUI/GPU acceptance.

## Retained acceptance state

- `EditorContainmentTests` remains the registered deterministic containment gate.
- Interactive `EditorRuntimeSmoke` remains the native desktop gate.
- The smoke requires one stable process-owned top-level editor window, original 12 child HWND/class identities, semantic Static/Button bindings, exact rows and Inspector state, `LBS_NOTIFY`, disabled pending toolbar actions, bounded cross-process reads, positive-area containment, safe process-tree cleanup, and hardened exact-owner final close.
- Automated state matrix remains untouched startup, 800x600, 1280x720, 1440x900, native maximize+restore, and 420x260.
- Human-visible screenshots at actual desktop dimensions remain required.

`native_evidence`: empty.
Fresh exact-head independent implementation review: clean on `0e756498...`.
E11 final acceptance: false pending registered native execution.
UE5/Unity parity claim: false.
Issue #7: open. Historical R0 runner not invoked.

## Native handoff, now unblocked by hosted/review gates

The registered Windows executor should use one owned interactive desktop and exact reviewed SHA `0e7564982ca4a61f84840c1b9e6da6d9a3d7ef9d`:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, and zero-owned-contained-process proof after failure or interruption. Capture untouched startup, 800x600, 1280x720, 1440x900, actual maximized desktop, and 420x260 or closest OS-permitted narrow state. Confirm semantic continuity, positive area/containment, viewport clipping, Cube selection/Inspector synchronization, disabled pending tools, bounded maximize/restore, and safe final shutdown. Separately launch `AstralGame` from the same source/build as a no-regression check.

Clean-machine packaging, comparative frame-time/RAM/VRAM evidence, wider stress/recovery, all remaining capability rows/catalogue gaps, and the required 24-hour soak remain unresolved.

## Single next action

Execute the registered native Debug/Release containment and interactive editor smoke handoff on exact reviewed SHA `0e7564982ca4a61f84840c1b9e6da6d9a3d7ef9d`, retain the required evidence and screenshots, then separately launch `AstralGame` from the same build. Do not admit another dependent editor feature before this native QA gate resolves.

## Continuation evidence: exact restore placement repair

This section supersedes the earlier current handoff until the new source receives fresh hosted and independent review evidence.

Finding: the maximize/restore smoke retained the pre-maximize `GetWindowRect`, but its restore predicate compared only width and height. A shifted restored window with the same dimensions therefore passed. This under-tested the Win32 contract because Microsoft documents `SW_RESTORE` as restoring the original size **and position**, while `GetWindowRect` returns the screen-coordinate upper-left and lower-right corners.

Repair commit: `3020590f091c68cc802016c02fcbbe54e5c7cebf`.
New `Tests/EditorRuntimeSmoke.cpp` blob: `3bb8774e33113e3e50357ed77c7054f486d339a9`.
GitHub compare `0de9892a...` -> `3020590f...`: exactly one changed file, `Tests/EditorRuntimeSmoke.cpp`, 5 additions, 3 deletions. The new predicate requires left, top, right, and bottom to match the pre-maximize outer rectangle; failure output names the `outer rectangle`. CMake and production editor/game source are unchanged.

Primary-source basis, rechecked in this pass:

- Microsoft Learn `ShowWindow`, `SW_RESTORE`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow . Restores original size and position.
- Microsoft Learn `GetWindowRect`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowrect . Returns screen-coordinate upper-left/lower-right outer-window bounds.
- Microsoft Learn `ShowWindowAsync`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindowasync . Starts the asynchronous show-state operation without waiting, so the bounded positive polling remains necessary.
- Epic Unreal Engine 5.8 `Using Editor Viewports`: https://dev.epicgames.com/documentation/unreal-engine/using-editor-viewports-in-unreal-engine . Maximized/restored viewport workflows remain a behavioral comparison point.
- Unity 6.0 `EditorWindow.maximized`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/EditorWindow-maximized.html . Maximized editor state remains a first-class workflow property.

No proprietary source or dependency was imported.

Portable reproduction fixture SHA-256: `ab2d1952a2e30036e396b4652bcff0a3190fdf629c9c43606ed760eb7cbe4d15`. It passed:

```text
g++ -std=c++17 -Wall -Wextra -Werror -pedantic e11_restore_rect_fixture.cpp -o e11_restore_rect_fixture_gcc
./e11_restore_rect_fixture_gcc
clang++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer e11_restore_rect_fixture.cpp -o e11_restore_rect_fixture_clang
ASAN_OPTIONS=detect_leaks=1 ./e11_restore_rect_fixture_clang
```

Both executions printed `e11 restore-rectangle fixture: PASS`. The fixture proves the old same-size predicate accepts shifted same-size rectangles and the new exact-rectangle predicate rejects them. It is source-logic evidence only.

Source-associated hosted runs: Windows `35915703271`, profiling `35915703261`, release manifest `35915703309`. Profiling completed `success`; Windows and release manifest were still in progress at the evidence-write checkpoint and are not recorded as passed here. The earlier clean Codex review on `0e756498...` does not review this new source change. `native_evidence` remains empty, current-source independent review is pending, and E11 final acceptance remains false.

Native acceptance for the current source must additionally prove that after the actual maximize state, restore returns the editor to the same pre-maximize outer screen rectangle before continuing to the narrow-state check. The existing machine/toolchain/GPU identity, complete outputs/exits/timestamps, screenshots, zero-contained-process cleanup proof, and separate `AstralGame` launch requirements remain unchanged.
