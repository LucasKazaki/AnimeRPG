# QA record — Engine uplift 2026-09-28

Task: [Tasks/ENGINE-UPLIFT-2026-09-28.md](../../Tasks/ENGINE-UPLIFT-2026-09-28.md)
Branch: `claude/loving-clarke-el24od`; base `main` at `2958741`.
Environment: Linux cloud container, 4 vCPU, GCC 13.3.0, Clang 18.1.3, CMake
3.28.3, Ninja, Python 3. No Windows toolchain, GPU or desktop.

The evidence below is **portable execution only**. It is not native Windows,
GPU, interactive or performance evidence.

## Portable engine build (Tests/EngineRuntime)

The same script runs for every configuration:

```bash
CXX=<g++|clang++> cmake -S Tests/EngineRuntime -B <build> -G Ninja \
  -DCMAKE_BUILD_TYPE=<type> [-DASTRAL_SANITIZE=ON | -DASTRAL_TSAN=ON]
cmake --build <build>
ctest --test-dir <build> --output-on-failure
```

| Configuration | Compiler | Flags | Result |
|---|---|---|---|
| Debug | GCC 13 | `-Wall -Wextra -Wpedantic -Werror` | 9/9 suites pass, exit 0 |
| Release | GCC 13 | same | 9/9 pass, exit 0 |
| Release | Clang 18 | same | 9/9 pass, exit 0 |
| RelWithDebInfo | GCC 13 | `-DASTRAL_SANITIZE=ON` (ASan+UBSan, no recover) | 9/9 pass, exit 0 |
| RelWithDebInfo | GCC 13 | `-DASTRAL_TSAN=ON` | 9/9 pass, exit 0 |

Clang's sanitizer runtimes are not installed in this container, so the
sanitizer legs use GCC.

The nine suites contain 83 test cases:

| Suite | Cases |
|---|---|
| Math | 10 |
| Core | 11 |
| Graphics | 13 |
| 2D | 6 |
| World | 7 |
| Physics | 11 |
| Animation | 8 |
| Systems | 10 |
| Showcase | 7 |

The slowest suite is Showcase: about 6.2 s in GCC Debug, 1.7 s in Release and
9.2 s under TSan.

## Root project (the CMake used by windows-ci), configured on Linux

The Win32 targets are declared but not built on Linux.

```bash
cmake -S . -B <root-build> -G Ninja
ninja -C <root-build> <9 engine suites> <gameplay domain tests>
ctest --test-dir <root-build> -R 'Engine|Tests$' \
  -E 'Smoke|Editor|FrameTiming|FramePhase|ProcessMemory|Simulation|Benchmark|Profiling|AstralScene|AssetValidation'
```

Result: 16/16 pass. That is the 9 engine suites plus 7 existing gameplay domain
suites, built through `astral_add_test` with assertions enabled. Exit 0.

## Contracts and static verifiers

| Command | Result |
|---|---|
| `python3 Scripts/test_test_safety.py` | 3 tests OK, exit 0. Every testing-block `add_executable` uses `astral_add_test` |
| `python3 Scripts/verify_milestone1.py` | PASS |
| `python3 Scripts/verify_milestone2.py` | PASS |
| `python3 Scripts/verify_milestone3.py` | PASS |
| `git diff --check` | clean |

## Win32 code

No Windows SDK is available here. `Engine/Platform/Win32Application.cpp`,
`Game/Showcase/Win32AstralPresenter.cpp` and `Game/Main.cpp` were checked with
`clang++ -std=c++17 -fsyntax-only -Wall -Wextra -DUNICODE -D_UNICODE -DNOMINMAX
-DWIN32_LEAN_AND_MEAN` against a minimal stub `windows.h`/`mmsystem.h`. The stub
was written from the documented signatures and is not committed. There were no
errors. The one warning is the pre-existing unused `fps` parameter in
`UpdateTitle`. **This is not an MSVC build.** Run `windows-ci`.

## Headless Astral-mode capture

```bash
<build>/AstralCapture <out-dir> 1280 720
```

GCC 13 Release. The scripted route mirrors the Win32 frame order:

- arrival
- light attack
- guard
- typed prompt, Thought Focus on and off
- dash
- Lincoln prompt and discovery (encounter becomes Active)
- Reflecting Pool
- Washington Monument (3/3 visited)
- return
- heavy attack, then Fatal Strike, which defeats the target and completes the
  encounter
- stats overlay
- low quality preset

Every route check held. Exit 0.

Output: 15 PNG frames, a 31.0 s stereo WAV (5,955,244 bytes) and a Chrome trace
(336,256 bytes). Two consecutive runs produced identical image hashes for 14 of
the 15 frames. The stats-overlay frame prints live timings, so it differs by
design.

| Frame | Hash (FNV, GCC 13 Release) |
|---|---|
| 01_arrival | 73c7bb37d1a3a0a2 |
| 06_shadow_dash | b9e43e22290edd98 |
| 08_lincoln_discovered | f3c6db53f0547f06 |
| 10_washington_monument | 43de0a970e43e077 |
| 12_fatal_strike | 7edc11c22990506a |
| 13_encounter_complete | 733290313e099f5b |

Frames were inspected visually during development. That inspection led to these
fixes:

- HUD overflow and overlapping banners were fixed.
- The Reflecting Pool was calmed to a mirror.
- Flag poles no longer block the camera: prop collision layers plus the new
  camera-occlusion dither fade.
- The spring arm no longer clips into the Monument.

Timing at 1280x720 on this VM with 3 workers, averaged over the 15 shots:

| Pass | Time |
|---|---|
| Full frame | 169 ms |
| Post | 60 ms |
| Shade | 51 ms |
| Shadows | 37 ms |
| Visibility | 7 ms |

At 640x360 the frame averages 75 ms. These are sandbox figures, not budgets or
claims about Lucas's hardware.

## Not verified (local acceptance gates)

- MSVC Debug/Release build and deterministic CTest (`windows-ci` on this branch).
- The six RuntimeSmokes on an interactive Windows desktop (GDI default path).
- Native F2 / `ASTRAL_RENDER_MODE=astral`, the typed prompt and waveOut audio.
- Frame-time percentiles on target hardware, the 24-hour soak and independent review.
