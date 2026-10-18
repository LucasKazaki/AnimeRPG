# Runtime-only Windows graphical player

Owner `/root/demo_gpu`; independent reviewer/coordinator `/root`.
Authority: Lucas's engine-only request and noon October 18 live-demonstration
deadline (America/New_York, 16:00 UTC). Dedicated worktree
`work/astral-native-runtime-player`, branch
`engine/native-runtime-player-20261018`, exact base
`2c69888774c074a69ec12c5f659e3632280db420`.
The complete independently reviewed project-creator commit
`5c9fb44f52254fc38e63b6447f1ae0cd21f71aba` was subsequently integrated by a
normal fast-forward, preserving the seven-path WIP. This is now the packet base.

## Problem and bounded result

The native player already drives GameHost but imports Playground registration
and compiles the full Showcase presenter just to obtain its waveOut adapter.
Add a graphical engine player that consumes Runtime and Windows libraries only.
Its no-argument launch must load Content/project.json beside the actual EXE,
independent of caller CWD. Preserve the legacy player and game target.

The coordinator substantively reviewed the external eight-path plan and the
initial seven-path preparation. After the creator's exact commit and explicit
reservation release, the coordinator transferred root CMake to this owner as the
eighth path. No build, test, audio device, graphics device or native action is
authorized without a separate exclusive coordinator slot. Creator-owned runtime
helper, portable CMake and other thirteen paths remain untouched.

## Allowed paths

- New Engine/Platform/Win32/Win32WaveOutput.h and .cpp.
- Game/Showcase/Win32AstralPresenter.h and .cpp, narrow extraction plus alias.
- Tools/AstralPlayerWin32.cpp, conditional legacy dependency and new defaults.
- This task and Docs/QA/ENGINE-NATIVE-RUNTIME-PLAYER-2026-10-18.md.
- Root CMakeLists.txt, now explicitly transferred after creator integration.

No game content, behavior redesign, runtime/host/UI input repair, D3D11 source,
source-list, dependencies, installations or other paths belong to this packet.

## Contracts

Move the existing Win32WaveOutput implementation into Astral::Platform, with a
direct AudioMixer header dependency. Keep Showcase::Win32WaveOutput as an alias.
Preserve its PCM format, four 1024-frame buffers, finite PCM conversion, no-device
silence, pump thread and join/reset/unprepare/close order. The mixer must outlive
the adapter. This extraction does not establish allocation-exception recovery.
After the initial frozen CPU matrix passed, root separately authorized one
normal startup console diagnostic reporting the existing adapter's actual
Active() result when host audio exists. It adds no test flag or alternate path.
Device availability is distinct from audible-content acceptance.

The CMake change adds AstralWin32Audio linked to Astral::Runtime/winmm,
links the old game and player to it, and removes only the unnecessary complete
presenter source from the old player's target. Give that legacy native player
the explicit ASTRAL_PLAYER_PLAYGROUND definition. Its defaults, controls,
registration and Showcase linkage remain. Add AstralEnginePlayerWin32 from the
same Tools source without that macro; link Runtime, the audio adapter, user32
and gdi32, with no Showcase or Game objects.

For the new target only, no explicit content-root means checked
GetModuleFileNameW resolution of the actual EXE directory plus Content, loading
project.json. Explicit root and optional project-or-scene arguments override the
default. Explicit relative roots intentionally retain caller-relative meaning.
Reject excess positional arguments with useful usage; report module/path
conversion failures. Never change CWD, truncate the module path, search for
Playground, or silently substitute demo content. Preserve actual GameHost input,
frame timing, hot reload, recording and CPU StretchDIBits presentation.

## Required evidence before completion

After exact source review, creator integration and CMake transfer: external
VS2022 x64 Debug/Release configure/build of AstralGame, AstralPlayerWin32,
AstralEnginePlayerWin32 and affected runtime suites. Record real target/link
inputs and PE imports: the new target must contain no Showcase, Playground or
National Mall object/registration dependency. Run unchanged safety and milestone
checks, bind source/commit, and obtain independent review.

Root owns actual native checks: generated Basic3D and original imported glTF
projects staged beside the EXE as Content/project.json, launch with no arguments
from an unrelated CWD, inspect the real authored scene/falling body, resize,
restart and close with classified process cleanup. Repeat explicit arguments and
paths with spaces. Verify old-player Playground and game behavior separately.
Record actual waveOut availability/startup/shutdown separately from mixer tests.

Keep first failures and stop deterministic blockers. Focus/capture cancellation,
minimized CPU rendering, Unicode/IME, clean-machine runtime dependencies,
package/notices, D3D11 swapchain integration and a long soak remain separate.
This packet provides a CPU-rendered native runtime player, not full GPU or engine
parity. Rollback is removing the new target and reverting the scoped extraction.

## Delivered evidence and bounded completion

The independently reviewed eight-path production patch passed an actual VS2022
x64 Debug/Release build of Game, both native players and four runtime suites;
all four registered suites passed with 68 cases per configuration. Actual target,
link-command and PE imports establish the new player's Runtime/audio/system
boundary. Unchanged safety passed 3/3 and M1/M2/M3 verification exited 0. The
separately reviewed normal Active() diagnostic then rebuilt both native targets
in each configuration. Complete source-before/after, bounded command, memory,
binary and cleanup receipts are recorded in the paired QA.

Root subsequently supervised four actual new-player launches on October 18,
05:15:14–05:21:10 UTC: Debug and Release each with no arguments from an unrelated
cwd, and each with an explicit Content root containing spaces plus an explicit
imported scene. The staged EXEs matched Debug SHA256 56B2153F and Release C2CC35CE
(full hashes and stage/exit receipts in QA). Both complete staged Content trees
matched the generated NeutralBox original-model project. Actual title/scene,
marker/body/ground, console device-active status and F5 reload were observed;
Debug default and Release explicit also passed maximize/restore. All four
bounded owned launches exited 0, were reaped without timeout and left no owned
window or editor/player/compiler process. No further production edit occurred.

This completes the bounded new native runtime player's implementation and
acceptance, subject to final independent review/commit. Existing old targets
were built; their native launch is not established by these four checks. Device
availability does not establish audible content. CPU rendering, clean-machine
packaging, GPU integration and other separate gates above remain explicit.
