# E11 Editor Runtime Smoke

## Scope and authority

Verification-only packet for Astral Editor native runtime acceptance. Preserve the custom C++17/Win32/GDI Astral Engine. No game-content work, dependency changes, graphics-API changes, scheduler/runtime control, merge, release, deployment, or historical R0 execution is authorized here. Issue #7 remains open, so the historical R0 runner remains blocked.

Allowed paths remain:

1. `CMakeLists.txt`
2. `Tests/EditorRuntimeSmoke.cpp`
3. `Tasks/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`
4. `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`
5. `Docs/Research/ENGINE-CAPABILITIES.json`

## Current bounded implementation

Source repair: `20cc436569cfcebcec0d8b9c195380df5a2a5b1b`

`Tests/EditorRuntimeSmoke.cpp` blob: `5bae7e024eed7ab88be772775abc2b1fa9479c14`

GitHub commit diff: 35 additions, 12 deletions, only `Tests/EditorRuntimeSmoke.cpp`.

The repaired `SelectCubeAndNotify` carries the same three-second selection deadline into `PostSelectionMutationWhileOwnerPinned`. Both asynchronous selection mutations now fail closed if the phase is exhausted before the owner pin or immediately before `PostMessageW`. The polling loop also rejects `LB_GETCURSEL == 3` when Cube is first observed at or after the deadline. Exact launched PID/TID ownership, retained process/thread liveness, zero-prior-count suspension, `GetThreadContext(CONTEXT_CONTROL)`, Outliner identity/style checks, asynchronous posting, immediate `ResumeThread`, the 135-second global budget, shell semantics, containment, resize deadlines, maximize/restore validation, exact restore rectangle and cleanup contracts remain intact.

The preceding evidence-control review finding was also repaired in `2a8604f52cbb52c91f125a91b1fe7b01de4c4d17`: `benchmark_proposals` and `progression_rules` were restored to the capability map. Review thread `PRRT_kwDOTo2Ig86lYyrl` was answered with exact evidence and resolved.

## Reproduction and portable regression evidence

The prior defect allowed two out-of-budget transitions:

- `LB_GETCURSEL == 3` could be observed exactly at the three-second boundary and accepted before the deadline check.
- Cube could be observed in-budget, then owner pinning/context validation could consume the remaining time and allow `WM_COMMAND/LBN_SELCHANGE` to be queued after the phase deadline.

Disposable C++17 source-logic fixture SHA-256:

`b02ed747e51817b150bfe7e9d0adca255a36efc97c61d5f5ba3860042736ee25`

Checks executed in the sandbox:

- GCC 14.2.0, `-std=c++17 -Wall -Wextra -Werror -pedantic`: PASS.
- Clang 17.0.0, `-std=c++17 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer`, with ASan leak detection and halt-on-error: PASS.

The fixture rejects an observation at/after deadline, rejects a post whose owner-pin phase crosses the deadline, and retains normal in-budget admission. This is source-logic evidence only, not Win32 GUI evidence.

## Primary research basis, rechecked 2026-09-24

- Microsoft Learn `PostMessageW`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-postmessagew . It posts to the creating thread's queue and returns without waiting for processing, so the phase budget must gate the enqueue itself.
- Microsoft Learn `GetTickCount64`: https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-gettickcount64 . It supplies elapsed milliseconds; this packet treats `now >= deadline` as exhausted.
- Microsoft Learn `GetThreadContext`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadcontext . A valid context cannot be obtained for a running thread, which is the existing post-suspend execution barrier.
- Microsoft Learn `SuspendThread`: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-suspendthread . Microsoft describes it as debugger-oriented, not general synchronization. E11 therefore uses it only for the short verification-only identity-and-enqueue interval and performs no target-dependent wait while suspended.
- Unreal Engine 5.8 editor viewport workflows: https://dev.epicgames.com/documentation/en-us/unreal-engine/using-editor-viewports-in-unreal-engine . Behavioral reference only.
- Unity 6.0 `EditorWindow.maximized`: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/EditorWindow-maximized.html . Behavioral reference only.

No proprietary engine source was copied and no dependency was imported.

## Hosted evidence for source `20cc436...`

All three source-associated pull-request workflows completed successfully:

- Windows build and deterministic tests: run `35942799798`, job `107454197090`, completed `2026-09-24T01:26:40Z`, success.
- Profiling capture portability: run `35942799775`, success.
- Release manifest integrity: run `35942799781`, success.

The Windows job passed repository/R0 safety contracts, VS2022 x64 configuration, Debug build and deterministic tests, Release build and deterministic tests, release dependency/prerequisite checks, static milestone verifiers and clean-tree verification. Hosted deterministic CI intentionally does not establish the interactive GUI behavior of `EditorRuntimeSmoke`.

At this checkpoint, current `main` is `e38760070e0d021293e9f907834e61e6a6edca23`. GitHub's current PR synthetic merge for source `20cc436...` is `45c3685897ca8c31cdca62052f97d6d738752045`, whose commit message records merge of `20cc436...` into `e387600...`. Keep source association distinct from the integration tree exercised by default pull-request checkout.

## Native acceptance matrix retained

Native evidence remains empty. The registered Windows executor must use one owned interactive desktop and retain source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, stdout/stderr, exit codes, UTC timestamps, hashes and screenshots where applicable.

Required native checks, after fresh independent review of the exact handoff tree:

1. Debug `EditorContainmentTests`.
2. Release `EditorContainmentTests`.
3. Debug and Release interactive `EditorRuntimeSmoke` as required by the packet controls.
4. Startup untouched shell and Cube selection/Inspector synchronization.
5. 800x600, 1280x720, 1440x900, actual maximized desktop plus exact restore to the pre-maximize outer rectangle, and 420x260 or the closest OS-permitted narrow state.
6. Zero owned processes after normal success and every exercised failure/cleanup path.
7. Separate `AstralGame` launch from the same build as a no-regression check.

Do not shrink this matrix without explicit approved scope change.

## Stop and rollback conditions

Stop on the first deterministic regression, review finding requiring source changes, native ownership/cleanup ambiguity, or any request outside the allowed paths. Do not weaken an acceptance test to obtain green evidence. Roll back only this bounded packet if a repair cannot be validated; do not reset or overwrite concurrent work.

## Remaining gates and single next action

Fresh independent review of the exact post-repair receipt tree is still required. Native GUI/GPU evidence, clean-machine packaging, comparative frame/RAM/VRAM measurement, stress/recovery, the remaining E00-E17 catalogue, and the required 24-hour soak remain unresolved. `parity_claim` remains false.

Single next useful action: reconcile a fresh independent review of the exact receipt head containing source `20cc436...` and the synchronized records. If it is clean, hand that exact reviewed SHA to the registered Windows executor for the retained native acceptance matrix above. If review finds a source defect, repair only that bounded finding before native execution.
