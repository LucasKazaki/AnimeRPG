# Milestone 2-1 QA Evidence — Debug Scene Foundation

Date: 2026-07-21
Branch: `task/m2-planning`
Baseline: `15de0d2`

## Automated verification

| Check | Command | Result |
|---|---|---|
| Debug configure/build | `"C:/Program Files/CMake/bin/cmake.exe" -S . -B Build -G "Visual Studio 17 2022" -A x64 && ... --build Build --config Debug --parallel` | PASS; MSBuild 17.14.51; AstralGame, AstralMathTests, AstralSceneTests built |
| Debug CTest | `"C:/Program Files/CMake/bin/ctest.exe" --test-dir Build -C Debug --output-on-failure` | PASS; 2/2 tests |
| Release build/CTest | `cmake --build Build --config Release --parallel` and CTest equivalent | PASS; 2/2 tests |
| Scene/math tests | `AstralSceneTests`, `AstralMathTests` through CTest | PASS; transform, camera, mesh success/failure paths |
| Static gate | `python Scripts/verify_milestone2.py` | PASS; required source/fixture markers |
| Python syntax | `python -m py_compile Scripts/verify_milestone2.py` | PASS |

## Runtime evidence

A local Python `ctypes` Win32 probe found the live Debug window, read its client surface, sampled pixels, and posted `WM_CLOSE`:

- Title: `Astral Engine | Milestone 1 | FPS: 64`
- Client size: `1264x681`
- Clear-color sample: `RGB(12,18,36)` — PASS
- Triangle-edge sample: `RGB(168,92,255)` — PASS
- Runtime visual checks: PASS
- Process wait after close: exit code `0`
- `astral.log`: mesh-loaded startup record and ongoing frame-timing records present
- Temporary runtime probe removed after execution

## Independent review inputs

- Architecture reviewer (`deleg_b7d9cfaa/task-0`): scope coherent; retain GDI as a bounded spike and avoid premature ownership/threading/render-backend expansion.
- QA reviewer (`deleg_b7d9cfaa/task-1`): independently required transform, camera, asset failure, runtime pixels, and M1 regression evidence; those checks are covered above.
- Review-planning reviewer (`deleg_b7d9cfaa/task-2`): incomplete; the child misparsed the packet path and did not inspect the task. No approval is inferred from that child.

## Acceptance decision

M2-1 is GREEN for this branch. It is a debug-scene contract spike, not production renderer completion. Next queued packet: M3 third-person controller, after Lucas-approved review/merge of this task branch.
