# E11 Editor Runtime Smoke evidence, 2026-09-23

## Current checkpoint

Branch: `engine/2026-09-22-editor-runtime-smoke`.
Current source repair: `4582d637a36d8d55981bda0886ef31d8643eef1b`.
`Tests/EditorRuntimeSmoke.cpp` blob: `797eebbc3c4e587efa1c7f14b62230c532a52e15`.
`CMakeLists.txt` remains unchanged at blob `4fd471151acb4b5919ccef4a92f49da12bd8d1f1`.
Production editor/game source remains unchanged by this packet.

The previous receipt `d709017977c11a2f0cd9d9498d63a9980cbf5e3c` automated native maximize/restore, but fresh independent Codex review completed at `2026-09-23T17:35:14Z` with two P2 findings in that new verification path. Independent acceptance therefore remained false.

## Review findings repaired

### 1. Show-state deadline could be exceeded during validation

The old helper allowed each nested `SendMessageTimeoutW` to consume up to the generic one-second message timeout, even after the three-second maximize or restore deadline had nearly expired. A slow but still responsive editor could complete `ValidateShellState` after the advertised phase deadline and be accepted.

`4582d637...` adds a scoped message-phase deadline. During maximize/restore validation, every bounded cross-process send now uses the smaller of the existing global work budget, the normal one-second message ceiling, and the remaining show-state budget. Polling sleeps are likewise capped to the remaining show-state budget. A successful complete shell validation is followed by an explicit deadline check before the maximized/restored state is accepted.

### 2. Show-state posts lacked adjacent ownership revalidation

The earlier implementation performed ownership checks, then executed `IsZoomed`/`GetWindowRect`, then posted a side-effecting maximize request through the cached HWND. The restore path had the same class of gap.

`4582d637...` establishes each show-state deadline first, then repeats launched-process ownership plus visible/enabled-state validation immediately before `ShowWindowAsync(SW_MAXIMIZE)` and `ShowWindowAsync(SW_RESTORE)`. Subsequent polling retains the same process-identity checks. This does not turn a numeric HWND into a lifetime capability; it closes the review-identified newly introduced gap and retains the existing fail-closed process-handle checks.

GitHub compare `d709017...` -> `4582d637...` reports exactly one modified file, `Tests/EditorRuntimeSmoke.cpp`, with 97 additions and 35 deletions. No production editor source, CMake registration, workflow, dependency, graphics API, architecture, scheduler setting, game content, release, deployment, or R0 code changed.

## Primary-source research

Accessed 2026-09-23:

- Microsoft Learn, `ShowWindowAsync`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindowasync . It sets show state without waiting for completion and posts the show-window event to the target window's message queue.
- Microsoft Learn, `SendMessageTimeoutW`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendmessagetimeoutw . `uTimeout` bounds each cross-thread wait; `SMTO_ABORTIFHUNG` and `SMTO_ERRORONEXIT` are retained. This supports capping each nested message to the remaining phase budget rather than summing multiple independent one-second waits beyond the phase deadline.
- Microsoft Learn, `GetWindowThreadProcessId`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowthreadprocessid . It returns the creating thread identifier and optionally the creating process identifier for a valid HWND, and zero for an invalid HWND.
- Epic, Unreal Engine 5.8, Using Editor Viewports: https://dev.epicgames.com/documentation/unreal-engine/using-editor-viewports-in-unreal-engine . UE documents Perspective 3D, orthographic 2D, multi-viewport layouts, maximized viewports, and immersive mode.
- Unity 6.0, `EditorWindow.maximized`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/EditorWindow-maximized.html . Unity exposes whether a docked custom editor window is maximized.

These are behavioral/API comparison references only. No proprietary Epic/Unity source was copied and no dependency was imported.

## Portable fixture

Disposable source-logic fixture SHA-256:

`b8c196a51b372982922165b5dc4d5044445d2279559ba3312505fec14a8f9dbb`

Executed in the sandbox with:

```text
g++ -std=c++17 -Wall -Wextra -Werror -pedantic /mnt/data/e11_show_state_deadline_fixture.cpp -o /mnt/data/e11_show_state_deadline_fixture_gcc
/mnt/data/e11_show_state_deadline_fixture_gcc
clang++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer /mnt/data/e11_show_state_deadline_fixture.cpp -o /mnt/data/e11_show_state_deadline_fixture_clang
ASAN_OPTIONS=detect_leaks=1 /mnt/data/e11_show_state_deadline_fixture_clang
```

Both executions printed `e11 show-state deadline fixture: PASS`. The fixture covers remaining-phase message timeout capping, rejection after a late full validation, fail-closed ownership before a simulated side effect, and no message admission when the local deadline is exhausted. It is source-logic evidence only, not Win32 GUI evidence.

## Hosted verification for implementation source

For `4582d637a36d8d55981bda0886ef31d8643eef1b`:

- Windows build and deterministic tests: run `35902292907`, job `107321254906`, completed `success` at `2026-09-23T18:27:06Z`.
- The Windows job passed repository/R0 safety contracts, prerequisite/runtime policy checks, VS2022 x64 configuration, Debug build, deterministic Debug tests, Release build, deterministic Release tests, static milestone verifiers, and clean-tree verification.
- Profiling capture portability: run `35902292991`, `success`.
- Release manifest integrity: run `35902292936`, `success`.

Hosted CTest intentionally excludes interactive `EditorRuntimeSmoke`. These runs therefore establish hosted compilation and deterministic-test compatibility only. They do not establish native Win32 maximize/restore behavior, screenshots, GPU behavior, clean-machine package launch, measured performance, stress/recovery, or soak acceptance.

## Retained acceptance state

- `EditorContainmentTests` remains the registered deterministic containment gate.
- Interactive `EditorRuntimeSmoke` remains the native desktop gate.
- The smoke still requires one stable process-owned top-level editor window, the original 12 child HWND/class identities, semantic Static/Button bindings, exact rows and Inspector state, `LBS_NOTIFY`, disabled pending toolbar actions, bounded cross-process reads, positive-area containment, safe process-tree cleanup, and the hardened exact-owner final close.
- Automated state matrix remains untouched startup, 800x600, 1280x720, 1440x900, native maximize+restore, and 420x260.
- Human-visible screenshots at actual desktop dimensions remain required.

`native_evidence`: empty.
Independent final acceptance: false pending a fresh review of the exact post-evidence receipt and registered native execution.
UE5/Unity parity claim: false.
Issue #7: open. Historical R0 runner not invoked.

## Native handoff

After the exact final receipt tree has green hosted checks and a fresh clean independent review, the registered Windows executor should use one owned interactive desktop and run:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorContainmentTests$" --no-tests=error
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact reviewed SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, complete commands/stdout/stderr, exit codes, UTC timestamps, and zero-owned-contained-process proof after failure or interruption. Capture untouched startup, 800x600, 1280x720, 1440x900, actual maximized desktop, and 420x260 or closest OS-permitted narrow state. Confirm semantic continuity, positive area/containment, viewport clipping, Cube selection/Inspector synchronization, disabled pending tools, bounded maximize/restore, and safe final shutdown. Separately launch `AstralGame` from the same source/build as a no-regression check.

Clean-machine packaging, comparative frame-time/RAM/VRAM evidence, wider stress/recovery, all remaining capability rows/catalogue gaps, and the required 24-hour soak remain unresolved.

## Single next action

Pin the final evidence receipt head in PR metadata/checkpoint, complete its hosted checks, request fresh independent Codex review of that exact tree, then hand only a clean-reviewed exact tree to the registered Windows executor. Do not admit another dependent editor feature before the native gate is resolved.
