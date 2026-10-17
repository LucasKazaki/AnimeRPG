# Native baseline, October 17, 2026

Owner: Codex coordinator, chat `01a14ae5-1646-7c80-b8a8-c76ba4ab5747`.
Packet: `Tasks/ENGINE-WEEKEND-COORDINATION-2026-10-17.md`.
Source: exact remote main `c644eec79752b9dc101aeca6b03141cf26854c96`.
Windows 11 Home 10.0.26300; Ryzen 9 7950X3D; RTX 5080, 16,303 MiB,
driver 617.14; approximately 31.16 GiB visible RAM and 9.24 GiB initially free.
VS2022 Community 17.14.22, MSVC 19.44.35222, SDK 10.0.26100.0.

Source directory: `<session>/work/astral-integration`; external build:
`<session>/work/build-main-baseline`. `<session>` is
`C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you`.
No primary-checkout tracked or untracked file was changed. Full worktree
inventory is retained in `<session>/work/worktree-inventory.txt`.

## Commands and results

`cmake` and `ctest` below resolve to the absolute executables in
`C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin`.

| Command (paths expanded at execution) | Exit/result | External log under `<session>/work/` |
|---|---|---|
| `cmake -S <source> -B <build> -G "Visual Studio 17 2022" -A x64` | 0 | `baseline-configure.log` |
| `cmake --build <build> --config Debug --parallel 2` | 0, all configured targets | `baseline-debug-build.log` |
| `ctest --test-dir <build> -C Debug --output-on-failure -E RuntimeSmoke --no-tests=error` | 0, 25/25, 53.67 s | `baseline-debug-test.log` |
| `cmake --build <build> --config Release --parallel 2` | 0, all configured targets | `baseline-release-build.log` |
| `ctest --test-dir <build> -C Release --output-on-failure -E RuntimeSmoke --no-tests=error` | 0, 25/25, 4.87 s | `baseline-release-test.log` |
| `ctest --test-dir <build> -C Debug --output-on-failure -R RuntimeSmoke --no-tests=error --stop-on-failure` | 0, 6/6, 19.45 s | `baseline-debug-smoke.log` |
| Same smoke command with `-C Release` | 0, 6/6, 19.51 s | `baseline-release-smoke.log` |
| `python Scripts/test_test_safety.py` | Initial exit 1: subprocess CMake absent from ordinary PATH | `baseline-test_test_safety.log` |
| Same safety script after adding installed VS CMake to process-local PATH | 0, 3/3 | `baseline-path-fixed-test_test_safety.log` |
| `python Scripts/verify_milestone1.py` | 0 | `baseline-path-fixed-verify_milestone1.log` |
| `python Scripts/verify_milestone2.py` | 0 | `baseline-path-fixed-verify_milestone2.log` |
| `python Scripts/verify_milestone3.py` | 0 | `baseline-path-fixed-verify_milestone3.log` |

Python is the existing bundled 3.x executable under
`C:/Users/lucas/.cache/codex-runtimes/codex-primary-runtime/dependencies/python`.
No system PATH change or installation was made. Release's `/UNDEBUG` override
warning is expected: the test-safety suite proves assertions execute and missing
assertion protection fails compilation. Product Release flags remain intact.

Smokes ran in one CTest invocation at a time on the local desktop after an
application-window inventory found no competing Astral window. They exercise
the retained GDI prototype, not the new PR 80 player, complete editor, GPU
backend, package, or long-duration acceptance. No R0 invocation occurred.

## Exact-source reproductions and reuse review

The numeric owner compiled a disposable MSVC C++17 harness against this base:
compile exit 0, execution exit 1 with all four audit leads reproduced. Member
and free lengths of `{1e20f,1e20f,0}` are infinity and Normalize returns zero;
NaN world projection reports success with NaN output; idle Update with NaN time
poisons position; large hierarchy translations return infinity. Harness and
logs: `<session>/work/numeric-evidence/`. Existing green suites miss these cases.

Independent read-only candidate reviewer `/root/candidate_review` confirms:

- PR 80 `efac9db54e8f2781c20fbb1bb9be9a0ab85f4f5c` has real framework, assets,
  host, UI and players, still unmerged. Host/UI input ownership, dependent scene
  reload, syntax-error diagnostics, glTF URI containment and float axis
  accumulation need behavior regressions and repair before adoption.
- Existing local branch `claude/astral-qa-fixes-and-editor` at
  `96c25cbb6d56094b49a3e60717d150db42a9b873` includes PR 80 plus additional
  commits. Editor commits `1c3d2dd` and `96c25cb` are reusable candidates:
  authored scene model, viewport, gizmos, undo, Play and native shell. Review
  and verify them rather than replacing the editor. Intervening game changes
  are outside this session and must not be blindly integrated.
- That local-test worktree has modified Canvas/Widgets and untracked UIDocument
  files. Preserve them; they are not part of the committed candidate review.
- PR 13's shell smoke has a different older editor contract; reconcile its
  assertions against the selected current editor rather than weakening gates.
- PR 52 actual failed run 36026761079/job107725163484 stops in
  `test_windows_ci_verifies_pr_head_and_merge_ref` (31 tests, one failure, two
  platform skips); workflow currently checks out only head. Future repair must
  run actual build/test jobs on both exact head and merge result, not add a
  matching text marker. Old log retained as `pr52-failed-job.log`.

These are source reviews, not independent acceptance of future patches.

## Limits

No current sanitizer build, complete editor workflow, GPU scene, packaged
external project, performance budget, or immutable 24-hour soak is accepted.
Native baseline success does not validate later commits. Workflow gate:
**not passed**. Next: numeric regression/repair and independent exact-commit
review; establish PR 80 native baseline separately before modifying it.
