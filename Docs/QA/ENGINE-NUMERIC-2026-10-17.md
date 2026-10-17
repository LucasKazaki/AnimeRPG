# Numeric repair QA — October 17, 2026

Task: [ENGINE-NUMERIC-2026-10-17](../../Tasks/ENGINE-NUMERIC-2026-10-17.md).
Base: `c644eec79752b9dc101aeca6b03141cf26854c96`.
Owned branch: `engine/numeric-repair-20261017`.
Worktree: `C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/astral-numeric`.

Status: implementation, independent review, full native Debug/Release builds,
deterministic tests and native smokes complete. The bounded CPU cost check passes
its declared relative guard. Hosted source/merge-result checks, remote merge and
integrated-main verification remain pending. This is not complete engine workflow,
general performance, GPU or 24-hour soak acceptance.

## Environment and ownership

Windows 10.0.26300, Visual Studio 2022 Community, MSVC 19.44.35222.0
(toolset directory 14.44.35207), MSBuild 17.14.23+b0019275e, Windows SDK
10.0.26100.0, C++17, x64. No dependency or system configuration was changed.
The coordinator owns full builds and the interactive desktop. The implementation
owner ran only the authorized one-process disposable harness, three focused
targets with `--parallel 2`, their CTest filter, and repository gates.

External evidence directory: `../numeric-evidence` relative to the worktree.
External root build directory: `../build-numeric`. Build outputs and the
disposable harness stay outside the source tree and normal source commits.

Before source edits the coordinator reported full baseline Debug/Release builds
passing, deterministic CTest 25/25 in each configuration, and six native smokes
passing in each configuration. Those baseline results do not validate this diff.

## Reproduced old failures and repaired behavior

An external six-check executable included the real `VectorMath.h` and compiled
the actual `Camera.cpp`, `PlayerController.cpp`, and `Transform.cpp` with one
MSVC compiler process. It did not traverse a cyclic baseline hierarchy.

| Observation | Exact baseline | Repaired source |
|---|---|---|
| Member/free length `{1e20f,1e20f,0}` | both `inf` | both `1.41421e+20` |
| Normalized direction | `(0,0,0)` | `(0.707107,0.707107,0)` |
| NaN perspective projection | success, `(nan,300)` | failure, unchanged `(12,34)` |
| Idle controller, NaN time | `(nan,nan,0)` | unchanged `(0,0,0)` |
| Two maximum-float hierarchy translations | `(inf,0,0)` | documented failure `(nan,nan,nan)` |

Baseline harness compilation exited 0; execution exited 1 with six failed
checks. Repaired compilation/execution exited 0; all six passed.
Logs: `compile-baseline.log`, `reproduce-baseline.log`,
`compile-repaired.log`, `reproduce-repaired.log`; harness: `reproduce.cpp`.

From the external evidence directory, both harness compilations ran the following
command through `cmd /d /c`, after expanding the full worktree source paths:

```text
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul && cl /nologo /std:c++17 /EHsc /W4 /UNDEBUG /I"C:\Users\lucas\Documents\Codex\2026-10-17\files-pasted-by-the-user-you\work\astral-numeric" /Fe:reproduce.exe reproduce.cpp "C:\Users\lucas\Documents\Codex\2026-10-17\files-pasted-by-the-user-you\work\astral-numeric\Engine\Scene\Camera.cpp" "C:\Users\lucas\Documents\Codex\2026-10-17\files-pasted-by-the-user-you\work\astral-numeric\Engine\Scene\PlayerController.cpp" "C:\Users\lucas\Documents\Codex\2026-10-17\files-pasted-by-the-user-you\work\astral-numeric\Engine\Scene\Transform.cpp"
reproduce.exe
```

## Focused registered regressions

The root CMake build already registers all three suites through
`astral_add_test`. `AssertionsEnabled.cpp` compiled in each configuration;
Release logs show the expected `/DNDEBUG` overridden by `/UNDEBUG`. New scene
checks also use the existing always-active `ASTRAL_CHECK` helper. No CMake,
test-safety contract or existing assertion was weakened.

- `AstralMathTests`: ordinary norm, large representable norm, tiny norm, zero,
  genuinely unrepresentable norm, shared finite-vector predicate.
- `EngineMathTests`: exponent-swept norms against a double reference and agreement
  between member/free APIs; large normalization including an unrepresentable
  float norm; retained zero/epsilon fallback and nonfinite component rejection.
- `AstralSceneTests`: invalid controller configuration/time/position, finite
  extreme midpoint/displacement/clamp, zero speed and opposing input; checked
  overflow/nonfinite hierarchy failures with unchanged output, valid cancelling
  sums, self/multi-node/prefix cycles, a stable 50,000-node chain; invalid camera
  parameters/data/unsafe raster output, finite offscreen acceptance, checked
  orthographic failures and preservation of last good follow state.

Exact working directory for the following commands is the owned worktree.
`cmake.exe` and `ctest.exe` were invoked by full path under
`C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin`.

```text
cmake.exe -S . -B ../build-numeric -G "Visual Studio 17 2022" -A x64
cmake.exe --build ../build-numeric --config Debug --parallel 2 --target AstralMathTests EngineMathTests AstralSceneTests
ctest.exe --test-dir ../build-numeric -C Debug --output-on-failure -R "^(AstralMathTests|EngineMathTests|AstralSceneTests)$" --no-tests=error
cmake.exe --build ../build-numeric --config Release --parallel 2 --target AstralMathTests EngineMathTests AstralSceneTests
ctest.exe --test-dir ../build-numeric -C Release --output-on-failure -R "^(AstralMathTests|EngineMathTests|AstralSceneTests)$" --no-tests=error
```

Configure and both builds exited 0. Debug and Release focused CTest each passed
3/3, exit 0, 0.09 seconds. Logs: `configure-numeric.log`,
`build-focused-debug.log`, `ctest-focused-debug.log`,
`build-focused-release.log`, `ctest-focused-release.log`.

## Repository gates and scope

Python executable:
`C:/Users/lucas/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe`.
For `test_test_safety.py` only, the Visual Studio CMake bin directory was
prepended to the child process PATH. No persistent environment change occurred.

| Command from worktree | Result | Evidence log |
|---|---|---|
| `python Scripts/test_test_safety.py` | exit 0, 3/3, 5.074 s | `test-safety.log` |
| `python Scripts/verify_milestone1.py` | exit 0 | `milestone1.log` |
| `python Scripts/verify_milestone2.py` before marker adaptation | exit 1, missing `parent->WorldPosition` | `milestone2-before-marker-update.log` |
| `python Scripts/verify_milestone2.py` after marker adaptation | exit 0 | `milestone2-after-marker-update.log` |
| `python Scripts/verify_milestone3.py` | exit 0 | `milestone3.log` |
| `git diff --check` | exit 0 | command transcript |
| `git status --short` / staged file listing | approved paths only | command transcript |

The M2 static gate failed only because correct iteration removed its recursion
string. The approved conditional repair now checks interface/traversal presence;
actual hierarchy acceptance is performed by the registered scene regression.

Changed paths are the two math headers, the Camera/PlayerController/Transform
headers and implementations, the three existing test files, that one static
marker and the task/QA records. No combat, game content, dependency, policy or
source-root build output is included.

## Review and remaining gates

The existing epsilon cutoff is intentional compatibility for near-zero
directions. Vector lengths still return float infinity when the true norm is
outside float range. Double normalization retains a valid direction in that
case; its hot-path cost requires the coordinator's fixed representative workload
comparison before performance acceptance. No performance conclusion is drawn
from the tiny focused tests.

Raw parent pointers remain borrowed and require live, stable ownership during
queries. Cycle handling does not validate dangling pointers. Checked outputs
are atomic; compatibility wrappers use documented `{0,0}`/NaN sentinels.
The signed 32-bit raster limit is tied to actual `lround`/GDI/HUD consumers and
does not reject ordinary finite offscreen points.

At the implementation-owner checkpoint, independent review, the full native
matrix, representative cost comparison and integration checks were pending.
The coordinator results below supersede that checkpoint: review, full native
matrix and bounded cost comparison are complete. Hosted source/merge-result
checks, remote merge, integrated-main verification and local portable sanitizer
evidence remain pending. GPU rendering and the 24-hour soak remain open.

## Coordinator verification and independent review

Source commit: `ec3822df1966e50c944ab064c9c694f26639445e` on the numeric-owner
branch. Coordinator cherry-pick: `94d4e8a7b6437bc99a64898407361dbfa1dff285`.
`git diff ec3822d HEAD -- Engine Tests Scripts` is empty: integration did not
change the reviewed production, regression or verifier code. Full native runs
use the numeric worktree and external `work/build-numeric` cache.

Independent reviewer `/root/candidate_review` verified the clean exact source
commit against `c644eec79752b9dc101aeca6b03141cf26854c96`, inspected the full
14-path diff, actual Renderer.cpp rounding/GDI/HUD consumers, API compatibility,
atomic failure behavior, finite extreme movement, cycle/deep/overflow handling,
borrowed pointer limits, test registration/assertions and retained reproduction
logs. **No blocking correctness findings.** The reviewer did not execute tests
or edit source. Its one nonblocking suggestion is a future regression directly
around the raster conversion margin; existing checks cover gross rejection and
accepted finite offscreen output. The 4096 margin exceeds current HUD offsets
and float rounding. Review acceptance was conditional on the following gates,
which the coordinator then completed:

| Command | Exit/result | Log under `<session>/work/` |
|---|---|---|
| `cmake --build work/build-numeric --config Debug --parallel 2` | 0, all configured targets | `numeric-full-Debug-build.log` |
| `ctest --test-dir work/build-numeric -C Debug --output-on-failure -E RuntimeSmoke --no-tests=error --stop-on-failure` | 0, 25/25, 52.23 s | `numeric-full-Debug-test.log` |
| Same build/test commands with Release | 0, 25/25, 4.90 s | `numeric-full-Release-{build,test}.log` |
| `ctest --test-dir work/build-numeric -C Debug --output-on-failure -R RuntimeSmoke --no-tests=error --stop-on-failure` | 0, 6/6, 18.96 s | `numeric-full-Debug-smoke.log` |
| Same smoke command with Release | 0, 6/6, 19.29 s | `numeric-full-Release-smoke.log` |

Desktop smokes were serialized; process inspection afterwards found no surviving
Astral/smoke processes. No game-content implementation was changed. Main merge
and hosted source/merge-result SHA verification are still pending at this record.

## Fixed workload cost and image check

Before measurement the coordinator declared a relative p95 guard of **at most
1.25 times exact-main baseline** for this numeric repair. This is a bounded cost
check, not a general engine performance budget or a 60 Hz acceptance result.

Same original static plane/box/sphere fixture, 320x180, renderer preset 1,
512 shadow resolution, three draws/331 rasterized triangles/32,521 shaded pixels,
no job workers, 10 warm-up frames and 100 measured frames. Same MSVC 19.44 `/O2
/MD /std:c++17` harness is linked separately against each exact-source Release
runtime library. CPU/OS/GPU identity is the baseline machine in the session QA
record; GPU timing/VRAM allocation measurement is not supplied by this CPU path.

Harness SHA-256: `58aa7ba47312f301438e5e91db81c5a4443e39ebef536cad0fe72441822563fd`.
External source/command: `work/numeric-render-benchmark.cpp` and
`work/run-numeric-benchmark.cmd` (run from `work` via `cmd /d /c`). Both compiles
and both runs exit 0; compile/run logs are `benchmark-{baseline,numeric}*`.

| Measurement | Main c644eec | Numeric ec3822d |
|---|---:|---:|
| Mean ms | 8.500648 | 9.664544 |
| p50 ms | 8.265100 | 9.649400 |
| p95 ms | 9.198400 | 10.093700 |
| p99 ms | 10.522400 | 10.296200 |
| Measured duration ms | 850.064800 | 966.454400 |
| Process working set bytes | 16,990,208 | 17,199,104 |
| Private bytes | 12,795,904 | 12,967,936 |
| Renderer-reported memory bytes | 4,429,752 | 4,429,752 |

p95 ratio **1.09733**, passing the declared 1.25 guard. There is a measured
cost for robust normalization; do not describe this change as a speedup. This
short single-process fixture is not evidence about GPU, high-resolution scenes,
other machines or whole-engine worst-case performance.

Both PNGs were visually inspected. Geometry, shadow and composition are intact;
floating-point precision changes affect 10/57,600 pixels, four by more than one
channel value (maximum channel delta 43 at a sphere boundary; mean absolute
channel delta 0.000798611). Hashes differ (`dbc92f74aab8369a` versus
`76d234329c17828a`); bit identity is not claimed. Existing semantic renderer
regressions pass. PNGs remain in external evidence, not source commits.
