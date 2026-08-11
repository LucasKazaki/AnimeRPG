# BLOCKER-0001 — Native C++ Build Toolchain

Status: resolved
Opened: 2026-07-21
Resolved: 2026-07-22
Last reconciled: 2026-08-11

## Original evidence

- The first environment used for M1 verification did not have `cmake` or a discoverable C++ compiler.
- The Windows SDK was present, but the compiler/linker environment required by the CMake project was not available in that shell.

## Resolution evidence

The blocker was superseded by later native verification recorded in `Docs/Decision-Log.md` and the milestone QA reports:

- M2 completed Visual Studio 2022 Debug and Release builds, CTest, and a live Win32 rendering probe.
- M3 through M7 completed native builds and automated runtime-smoke evidence.
- The M10 task record states that Visual Studio 2022 x64 configure, Debug and Release builds, and the complete 14/14 CTest suite passed, including M4, M5, M7, M8, M9, and M10 runtime smokes.

This file must no longer stop new work merely because the original July 21 shell lacked a toolchain.

## Current handling rule

Before a new implementation packet begins, run a fresh toolchain probe in its dedicated worktree. When the current machine cannot configure or build, create a new dated blocker containing the exact shell, command, exit code, and missing executable. Do not reopen this historical blocker without new evidence.

No agent may install or modify a toolchain, accept a license, or change system configuration without Lucas's explicit approval.
