# Windows playtest QA — October 29, 2026

Result: the existing combat prototype, editor, sample game and supported GPU
fixture run on Lucas's PC. This is bounded playtest readiness, not completion of
the RPG roadmap, clean-machine certification or 24-hour soak acceptance.

## Tested source and machine

- Product binaries: commit `846f961c64f98dcd865923cd3ede7541d2a1047a`, branch
  `playtest/windows-ready-20261029`. The delivery follow-up changes only packaging
  and documentation; no compiled source differs from this tested revision.
- Windows x64; Ryzen 9 7950X3D, 32 GB RAM, GeForce RTX 5080.
- VS2022 / MSVC 19.44.35222, Windows SDK 10.0.26100, CMake 3.31.6.
- Dedicated source checkout: `astral-playtest`; sibling build:
  `astral-playtest-build`; retained logs and captures: `playtest-evidence`.
- Independent source/evidence audit: `Docs/Reviews/PLAYTEST-WINDOWS-2026-10-29.md`.

## Build and automated results

The configure and both complete builds exited 0. Every registered CTest target
passed in each configuration, including real hardware GPU tests and six native
interactive smoke tests. Native tests ran alone on this desktop.

| Configuration | Domain tests | Native/GPU tests | Total |
| --- | --- | --- | --- |
| Debug | 33/33 (47.99 s) | 7/7 (55.68 s) | 40/40 |
| Release | 33/33 (3.49 s) | 7/7 (30.94 s) | 40/40 |

Receipts: `configure.log`, `build-debug.log`, `build-release.log`,
`tests-debug-domain.log`, `tests-debug-native-fixed.log`,
`tests-release-domain.log`, `tests-release-native.log`. Reproduction commands,
from source with the installed CMake and Python on the process-local PATH
(domain-test worker count can be adjusted):

```powershell
cmake -S . -B ../astral-playtest-build -G "Visual Studio 17 2022" -A x64 -DASTRAL_BUILD_D3D11=ON -DASTRAL_ENABLE_HARDWARE_GPU_TESTS=ON
cmake --build ../astral-playtest-build --config Debug --parallel 4
cmake --build ../astral-playtest-build --config Release --parallel 4
ctest --test-dir ../astral-playtest-build -C Debug -E 'EngineD3D11Tests|RuntimeSmoke' --output-on-failure -j 2
ctest --test-dir ../astral-playtest-build -C Debug -R 'EngineD3D11Tests|RuntimeSmoke' --output-on-failure -j 1
ctest --test-dir ../astral-playtest-build -C Release -E 'EngineD3D11Tests|RuntimeSmoke' --output-on-failure -j 2
ctest --test-dir ../astral-playtest-build -C Release -R 'EngineD3D11Tests|RuntimeSmoke' --output-on-failure -j 1
python Scripts/verify_milestone1.py
python Scripts/verify_milestone2.py
python Scripts/verify_milestone3.py
python Scripts/test_test_safety.py
python Scripts/test_portable_build_contract.py
python Scripts/test_package_soak_telemetry_analysis.py
git diff --check
```

All six Python commands exited 0 (safety: 3 tests; portable contract: 4;
soak analyzer: 15). The three milestone structural checks also passed. Hardware
coverage includes pixel correctness, reference captures, native presentation,
resource lifetime/debug-layer checks and frame metrics. M10 now verifies retry
after completion, restored target HP, subsequent damage and no active-attempt
reset from holding F5. No tests or correctness thresholds were disabled.

## Package and desktop observations

The initial package's 28 file hashes, 8 executable hashes and all 12 EXE/DLL
dependency inventories were independently verified. The Release package includes
the four required VC runtime DLLs; none import Debug CRT. The final package adds
this QA document and corrects the generic host launcher. `manifest.json` records
the exact source identity, input build and file hashes for each package.

- AnimeRPG displays the National Mall combat scene and controls. Enter opens
  the Thought prompt; physical character keys followed by Enter submitted
  `focus`, producing ACCEPTED and local time x0.35. Escape cancels the prompt
  without closing the game. Text-paste automation did not insert characters;
  physical typing is the verified input path. No paste support is claimed.
- The editor loaded the 35-entity Playground, entered Play via its toolbar,
  displayed the running game and returned to editing with F5. A key sent to
  the foreground editor did not change the background game's position or target
  HP. No packaged scene was saved or edited.
- `Play Engine Sample.cmd` loaded Playground; F5 restarted its timer and Escape
  exited with launcher status 0.
- `Open GPU Sample.cmd` loaded the six-entity fixture. Selecting Renderer >
  D3D11 Opaque Unlit visibly rendered its textured floor, primitive and imported
  mesh, and the title confirmed D3D11. It exited with launcher status 0. The
  observed FPS is not a controlled performance benchmark.
- Packaged headless Playground executed its entire 372-frame demo script,
  collected a star, spawned a crate, wrote four PNG captures and exited 0.
  Final image hash: `dda01eacc2beab61`. Receipt: `package-playground-fixed.log`.

Headless command, with the capture directory created first:

```powershell
./AstralPlayer.exe Content project.json --script Content/demo.play --capture <existing-capture-directory> --every 120
```

## Failures found and repaired

1. The first GPU suite depended on an external fixture environment variable.
   The original authored fixture is now checked in and selected by default;
   each test run creates a unique capture directory. Both full native reruns
   passed. The failed receipt is retained as `tests-debug-native.log`.
2. The extended M10 check initially expected 20 damage; the current light attack
   does 25. Its assertion now correctly expects HP 75, without changing damage.
3. The test-safety script initially could not locate CMake. Adding the existing
   CMake installation to this process's PATH resolved it; no global PATH edit.
4. Headless capture requires a pre-existing destination directory. The first
   invocation exited 3; creating the directory and rerunning exited 0.
5. The generic engine launcher initially opened Playground, whose GameMode
   behavior is registered only by the sample game. Its live launch returned
   an explicit unknown-behavior error (exit 2). The corrected launcher opens
   the neutral fixture. Playground remains on the sample-game launcher.

## Remaining scope

Full RPG progression, reactive enemy/health/death gameplay, multiplayer/raids,
final anime art and LLM interpretation are not connected to this build. The
combat game uses the CPU Astral renderer; hardware rendering currently supports
the editor/generic host's opaque Unlit subset. Local subsystem tests do not prove
these unfinished features. Long soak and clean-machine qualification remain
separate work. GitHub CI is additional evidence; it does not replace the native
tests and desktop observations above.
