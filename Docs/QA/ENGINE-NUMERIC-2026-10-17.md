# Numeric repair QA — October 17, 2026

Task: [ENGINE-NUMERIC-2026-10-17](../../Tasks/ENGINE-NUMERIC-2026-10-17.md).
Base: `c644eec79752b9dc101aeca6b03141cf26854c96`.
Owned branch: `engine/numeric-repair-20261017`.
Worktree: `C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/astral-numeric`.

Status: implementation and focused native Debug/Release verification complete;
independent review, full repaired-build matrix, native smokes and integration
remain coordinator gates. This record does not establish engine workflow,
performance, GPU or 24-hour soak acceptance.

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

Independent substantive diff/commit review, complete repaired Debug/Release
builds and deterministic CTest, serialized repaired native smokes, representative
performance comparison, proposed-integration/current-main verification and any
portable sanitizer evidence remain pending with the coordinator. GPU rendering
and the 24-hour soak are outside this bounded numeric packet and remain open.
