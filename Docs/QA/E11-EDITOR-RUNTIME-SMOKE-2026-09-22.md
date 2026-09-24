# E11 Editor Runtime Smoke QA Receipt

## Candidate and scope

Current source repair: `20cc436569cfcebcec0d8b9c195380df5a2a5b1b`

`Tests/EditorRuntimeSmoke.cpp` blob: `5bae7e024eed7ab88be772775abc2b1fa9479c14`

`CMakeLists.txt` remains outside this repair and was not changed by source commit `20cc436...`.

Implementation diff: 35 additions, 12 deletions, only `Tests/EditorRuntimeSmoke.cpp`.

Task receipt update: `13573441d34d4a2e1d826a0486cfa59045025dfa`.

Capability-control repair preceding the source change: `2a8604f52cbb52c91f125a91b1fe7b01de4c4d17`, restoring `benchmark_proposals` and `progression_rules`. Review thread `PRRT_kwDOTo2Ig86lYyrl` is resolved.

## Defect repaired

The prior three-second selection phase had two admission gaps. It could accept `LB_GETCURSEL == 3` at the deadline before checking the clock, and it could observe Cube just before the deadline then spend the remaining budget owner-pinning before enqueuing `WM_COMMAND/LBN_SELCHANGE` after the phase expired.

The repair passes the phase deadline into `PostSelectionMutationWhileOwnerPinned`, fails before owner pinning if exhausted, rechecks `GetTickCount64() >= deadline` immediately before either asynchronous `PostMessageW`, and rejects Cube first observed at or after the deadline. The existing exact PID/TID owner pin, retained process/thread liveness, zero prior suspension count, `GetThreadContext(CONTEXT_CONTROL)` barrier, Outliner identity/style checks, immediate resume, global work budget, shell-state assertions, containment, resize/show-state deadlines and cleanup behavior remain intact.

Microsoft documents `PostMessageW` as returning after queueing without waiting for processing, so the phase budget must gate the enqueue itself. `GetTickCount64` is the elapsed-millisecond source used by the packet. `GetThreadContext` requires the target not be running to obtain a valid context, while `SuspendThread` is documented as debugger-oriented rather than a general synchronization primitive. Sources were rechecked 2026-09-24:

- https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-postmessagew
- https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-gettickcount64
- https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadcontext
- https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-suspendthread
- Unreal Engine 5.8 reference: https://dev.epicgames.com/documentation/en-us/unreal-engine/using-editor-viewports-in-unreal-engine
- Unity 6.0 reference: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/EditorWindow-maximized.html

No proprietary source was copied and no dependency was imported.

## Portable regression fixture

Fixture SHA-256: `b02ed747e51817b150bfe7e9d0adca255a36efc97c61d5f5ba3860042736ee25`.

Results:

- GCC 14.2.0 warning-clean C++17 build and execution: PASS.
- Clang 17.0.0 C++17 with ASan, UBSan and leak detection: PASS.

Covered cases: Cube first observed exactly at the deadline, Cube first observed after the deadline, an in-budget observation whose owner-pin work crosses the deadline before post, and normal in-budget admission. This is source-logic evidence only and does not substitute for native Win32 GUI execution.

## Hosted verification

Source-associated pull-request runs for `20cc436...`:

- Windows build and deterministic tests run `35942799798`, job `107454197090`: `completed/success` at `2026-09-24T01:26:40Z`.
- Profiling capture portability run `35942799775`: `completed/success`.
- Release manifest integrity run `35942799781`: `completed/success`.

The Windows job passed repository/R0 safety contracts, VS2022 x64 configure, Debug build/tests, Release build/tests, dependency/prerequisite checks, static milestone verifiers and clean-tree verification.

Current observed `main` at this checkpoint is `e38760070e0d021293e9f907834e61e6a6edca23`. Current PR synthetic merge is `45c3685897ca8c31cdca62052f97d6d738752045`, whose commit message records merging source `20cc436...` into base `e387600...`. Source-head association and pull-request integration identity must remain separate in later receipts.

Hosted CI does not establish interactive GUI behavior of `EditorRuntimeSmoke`. Native evidence remains empty.

## Deferred native acceptance

After a fresh independent review of the exact synchronized receipt head, the registered Windows executor must run Debug and Release `EditorContainmentTests`, then the interactive `EditorRuntimeSmoke` matrix on one owned desktop, retaining machine/Windows/MSVC/CMake/GPU/driver identity, commands, stdout/stderr, exit codes, UTC timestamps, hashes, screenshots and zero-owned-process cleanup evidence. The matrix retains startup, Cube selection/Inspector synchronization, 800x600, 1280x720, 1440x900, actual maximize plus exact restore, and the narrow state, followed by a separate `AstralGame` launch from the same build.

Issue #7 remains open, so the historical R0 runner was not invoked. Clean-machine packaging, measured frame/RAM/VRAM comparison, stress/recovery, remaining capability gaps and the 24-hour soak remain unresolved. Independent final acceptance is false and `parity_claim` is false.

## Next action

Obtain and reconcile fresh independent review of the exact synchronized receipt tree containing source `20cc436...`. If clean, hand that exact SHA to the registered Windows executor. If review identifies a source defect, repair only that bounded finding before native execution.
