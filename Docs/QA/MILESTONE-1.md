# Milestone 1 Acceptance Matrix

| ID | Feature | Test | Expected result | Current evidence |
|---|---|---|---|---|
| M1-01 | Clean configure | `cmake -S . -B Build -G "Visual Studio 17 2022" -A x64` | Configure succeeds | PASS — CMake 4.4.0 selected Windows SDK 10.0.26100.0 and MSVC 19.44.35228.0 |
| M1-02 | Build | `cmake --build Build --config Debug --parallel` and Release equivalent | AstralGame and AstralMathTests build | PASS — both Debug and Release executables built with MSBuild 17.14.51 |
| M1-03 | Math | `cmake --build Build --config Debug --target RUN_TESTS` | Vec3 length and identity matrix tests pass | PASS — 1/1 CTest passed in 0.71 seconds |
| M1-04 | Window | Launch AstralGame | Native 1280x720 window appears | PASS — Win32 class `AstralEngineWindow` found; client measured 1264x681 within the 1280x720 framed window |
| M1-05 | Loop/timing | Observe title for 10 seconds | FPS title updates; no runaway delta | PASS — title updated to `Astral Engine | Milestone 1 | FPS: 64`; timing logs continued for 60 seconds |
| M1-06 | Input | Press Escape; close window | Process exits cleanly | PASS — automated `WM_CLOSE` exercised the close path and process exited with code 0; Escape branch remains source-verified |
| M1-07 | Logging | Inspect `astral.log` after launch | Startup and frame-timing records exist | PASS — startup plus 60 frame-timing records written |
| M1-08 | Clear renderer | Launch and inspect client area | Stable dark blue clear color | PASS — Win32 `GetPixel` at client center returned RGB `(12, 18, 36)` |
| M1-09 | Static source gate | `python Scripts/verify_milestone1.py` | Required files and implementation markers present | PASS — 7 source files and required system markers verified |

## Release gate
M1-01 through M1-09 passed on Windows on 2026-07-21. A clean-clone reproduction remains required after the approved baseline commit before M2 begins.
