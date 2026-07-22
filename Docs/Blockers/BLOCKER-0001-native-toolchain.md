# BLOCKER-0001 — Native C++ Build Toolchain

Status: blocking runtime verification of M1
Date: 2026-07-21

## Exact evidence

- `cmake -S . -B build -G "Visual Studio 17 2022"` returned `/usr/bin/bash: line 3: cmake: command not found` (exit 127).
- `where cl.exe`, `where clang++.exe`, and `where g++.exe` returned no compiler path.
- `C:/Program Files/Windows Kits/10` exists, but a Windows SDK alone does not provide the C++ compiler/linker environment needed by this CMake project.

## Impact

The M1 source and static gate are present, but the native executable, CTest binary, and runtime window/input/logging evidence cannot honestly be claimed yet. M2 and gameplay implementation should not start until M1 has real build/runtime evidence.

## Recommended next action

Lucas should approve one ordinary local toolchain installation path, with its license/terms handled explicitly by Lucas. Recommended options:

1. Install Visual Studio Build Tools with Desktop C++ workload and CMake support.
2. Install LLVM/clang-cl plus CMake from a trusted local package source.

After approval and installation, run the README configure/build/CTest commands, then execute the manual M1-04 through M1-08 playtest. Do not install both toolchains unless a build failure demonstrates the need.

## Safe work continuing now

No installation, license acceptance, firewall change, credential entry, external API call, or public deployment was performed. The source, task packet, acceptance matrix, and loop artifacts are ready for the approved toolchain verification pass.
