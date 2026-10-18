# Runtime-only native player QA

Owner `/root/demo_gpu`; reviewer/coordinator `/root`. Initial base
`2c69888774c074a69ec12c5f659e3632280db420`, dedicated
`engine/native-runtime-player-20261018` in `work/astral-native-runtime-player`.

The coordinator independently accepted the external eight-path source plan and
initial seven-path source preparation. The complete reviewed creator commit is
now integrated, and its owner explicitly released CMake before the coordinator
transferred that eighth path. The initial frozen eight-path CPU matrix now
passes Debug/Release, including real target/link/PE dependency evidence. The
subsequent narrow startup audio-status diagnostic passed independent review and
both affected native-target rebuilds. Root then used a separately granted native
slot for four actual Debug/Release default/explicit-root launches; all passed
with observed device availability, controls and normal cleanup, as recorded below.

Read the active task before production. The existing native player imports
Playground registration/defaults and the Showcase presenter solely for its audio
adapter. The existing WaveOutput block uses AudioMixer, Win32 waveOut, buffers
and a pump thread; its extraction retains the old Showcase name as an alias.
The complete presenter remains with the game. No game content is added.

This is a new target, so no old functional failure is claimed for it. Acceptance
requires actual Runtime-only target/link/PE inputs and real no-argument native
launch of staged Content/project.json beside the EXE from an unrelated CWD,
followed by explicit root/path overrides. Headless captures and marker strings
cannot establish that native behavior. Preserve the old game/player checks.

The mixer must outlive the output adapter. Existing no-device silence and
join-before-reset teardown are preserved; allocation-exception recovery is not
proven by moving source. Native pointer cancellation and other inherited input
issues are not silently repaired. This remains CPU StretchDIBits presentation.
No D3D11 player integration, clean-machine packaging, long soak or whole-engine
acceptance is inferred from this bounded runtime acceptance.

## Seven-path source preparation freeze

The authorized five production files are prepared, with this QA and task making
seven paths. Existing WaveOutput method bodies compare exactly to the original
block after line-ending normalization; the moved class declaration also matches
exactly. Removing that block leaves Win32AstralPresenter.cpp otherwise identical
to its base. The header preserves Showcase::Win32WaveOutput through an alias to
the Engine Platform adapter. No PCM/pump/device behavior changed.

The native player directly includes the engine audio adapter. Only the intended
ASTRAL_PLAYER_PLAYGROUND build variant includes/calls Playground registration
and retains its Content/Samples/Playground default. The other variant resolves
Content beside its actual EXE through checked GetModuleFileNameW only when no
root was provided, defaults to project.json, preserves explicit root/path
arguments, and reports excess arguments or module/path-resolution failures.
It changes neither CWD nor actual window/input/host/render/recording behavior.
The audio adapter is still reset before host destruction.

Root CMake diff is zero, so the new target/audio link and legacy macro are not
configured yet. This intermediate seven-path preparation is explicitly not
build-ready. No configure/build/test/audio/native/GPU execution occurred.
Source is frozen for substantive coordinator review before creator integration
and the separately authorized eighth CMake path. Exact source/whole patch and
physical-file hashes are retained externally in
`work/native-runtime-player-evidence/source-ready.receipt.json` and patches.

## Creator integration and eight-path source freeze

After the creator owner committed its clean thirteen paths and explicitly
released CMake, root supplied exact commit
`5c9fb44f52254fc38e63b6447f1ae0cd21f71aba`, tree
`0edfa59879b03bb70dc6de4625b435721629d58b`. A normal fast-forward integrated that
complete commit while preserving the existing seven-path WIP. This is now the
packet base. No creator-owned runtime helper, portable CMake, source list,
project implementation or thirteen-path content was edited.

The eighth-path root CMake delta adds AstralWin32Audio from only the moved Engine
Platform source, linked to Astral::Runtime and winmm. AstralGame keeps its actual
Showcase presenter and links the extracted adapter. The legacy native player
retains AstralShowcase and gets ASTRAL_PLAYER_PLAYGROUND plus the adapter; only
its redundant complete-presenter compilation is removed. The new
AstralEnginePlayerWin32 compiles only the real native player source, with
Runtime/audio/user32/gdi32 dependencies and no Showcase or Game target.

Production logic in the independently reviewed seven paths did not change.
Source, CMake and docs are frozen for another substantive review before exact
Debug/Release target builds, affected registered tests, actual generated target
and PE dependency inspection, unchanged safety/milestones, and root-owned native
launch/audio checks. No configuration, build, test, audio, GPU or UI ran during
this preparation. Source/whole patches and physical hashes are retained as
`work/native-runtime-player-evidence/eight-path-*.patch` and receipt JSON.

## Initial frozen eight-path CPU matrix: pass

Root independently reviewed production patch SHA256
`3CE0690818FAF6E3BF697F391F7A81730802284499801415B7B233E2967EDE27`, all eight
physical hashes and the bounded exact-argv runner before granting the exclusive
CPU slot. October 18 from 04:50:37 to 04:53:06 UTC, at creator base 5c9fb44:

| Step | Actual result |
| --- | --- |
| VS2022 x64 configure, BUILD_TESTING ON | exit 0, 3.758 seconds |
| Debug Game, legacy/new native players and four suites build | exit 0, 59.046 seconds, two jobs |
| Debug registered focused CTest | 4/4, 3.44 seconds; Framework21, Systems20, Sample20, Project7 cases |
| Release same targets build | exit 0, 74.495 seconds, two jobs |
| Release same CTest | 4/4, 0.63 seconds; same 68 cases |
| Actual Debug/Release PE dependency inspection | exit 0 each |
| Unchanged test safety | 3/3, 5.285 seconds |
| Unchanged M1/M2/M3 static verification | exit 0 each |

The runner verified all eight physical hashes before and after, plus actual
cache CMAKE_HOME_DIRECTORY identity. Fresh available memory was 13,857,501,184
bytes before the Debug build and 13,547,532,288 before Release, above 8 GiB
admission. All thirteen exact-argv commands exited 0, timedOut false, owned child
reaped true, with explicit cwd/PID/deadline/argv and selected environment receipts.
MSBUILDDISABLENODEREUSE was 1 and CMAKE_BUILD_PARALLEL_LEVEL was 2 for all; original
process environment values were restored in finally. The final owned
build/compiler/test process count was zero; session completed and slot released.

Actual generated new-player ProjectReference and both real link-command inputs
contain only AstralEngineRuntime, AstralWin32Audio and system libraries, with no
Showcase, Playground, National Mall or complete presenter object. PE imports
include USER32/GDI32/WINMM/KERNEL32 and MSVC/UCRT runtime libraries. Debug imports
debug CRT DLLs and is a development build. Release imports MSVCP140/MSVCP140_2,
VCRUNTIME140/VCRUNTIME140_1 and UCRT API-set DLLs; this is dependency evidence,
not a clean-machine redistributable/package acceptance claim.

Complete structured receipt, per-step exact-command JSON/raw logs, generated
project and actual link inputs are retained under
`work/native-runtime-player-evidence/cpu-20261018-045037`. Initial EXE SHA256:
Debug `B7845C6EC1664C6E503E8C0072C2AE56050C469F6D749ACF31F49A3163EF2AC2`,
Release `90446440B54BBA93BD34F7A850E22DC82723B6E696014F5D165A4A13F3ADB403`.
No native player/game launch, native smoke, device audio, graphics device or UI
occurred in this CPU matrix. Those initial binary receipts predate the
subsequent diagnostic below and do not validate its newly edited source.

## Normal startup audio diagnostic: reviewed and rebuilt

Root noted that an open player window cannot establish Win32WaveOutput::Active()
and explicitly authorized a narrow normal console diagnostic after the initial
matrix completed and released. Only the existing native player's adapter
construction block changes: when host audio exists, construct the same adapter
then print either `Audio output device active` or `Audio output unavailable`
according to its actual Active() result. There is no test flag, alternate
initialization, forced device or new UI. This is actual availability reporting,
not proof of sound playback, audio content correctness or no allocation failure.

The shared native Tool change affects old/new native executables, so the next
proposed CPU slot rebuilds those two targets Debug/Release with exact new source
binding before root's actual console/device observation. No unit suite calls
wWinMain; repeating the unchanged parser/world cases would not validate this
diagnostic. Native no-args EXE-adjacent Content from unrelated CWD, explicit root
with spaces, actual displayed availability, resize/F5/close and process cleanup
remain required in both configurations. Root independently reviewed the narrow
diagnostic and production patch
`873F32E8564888A894A4324376549FC78E4303B1EB3EE49A20966CFC6E8AA66C`, native Tool
physical SHA256 `1C18902499D0E1FBE6D35E6594DD253611D6C52CEB16EA83211BA480CF12E402`,
then granted the separate native-target-only CPU slot. The original matrix logs
stay immutable.

At 05:00:16 to 05:00:25 UTC, the existing attributable cache rebuilt only
AstralPlayerWin32 and AstralEnginePlayerWin32 in Debug/Release with two jobs:
exit 0 in 4.047/4.093 seconds. All four bounded exact-argv steps, including
head checks, exited 0 with timedOut false and owned child reaped. All eight
physical source hashes matched before/after at creator base 5c9fb44; available
host memory before Debug/Release was 14,298,370,048/14,329,458,688 bytes.
Node reuse was disabled, parallel level was 2, original environment was restored,
and zero owned build/compiler/test processes remained. Slot explicitly released.

The complete receipt, raw build logs and exact cwd/argv/PID/deadline evidence are
`work/native-runtime-player-evidence/audio-status-20261018-050016`. New engine
native EXE SHA256: Debug
`56B2153F7D32138A3F8825F59D6EF8B4E178618B297C51FC4BDAB1CEE2907B39`, Release
`C2CC35CEE4D8B3545DAE3EF4B671034A2007755497F7F4786810CB7846F2CFA1`.
No native launch, CTest, sound device, graphics device or UI ran in this slot.
Actual console availability observation and native runtime checks remain pending;
compilation does not establish an active device or audible sound.

## Actual native default and explicit-root acceptance: pass

Root supervised four actual launches on October 18, 05:15:14 to 05:21:10 UTC,
using the diagnostic-bearing binaries above without further production edits.
The stage receipt binds the actual copied EXEs and both complete Content trees:
`work/native-runtime-player-evidence/native-pilot-stage.receipt.json`.
The pilot directory is `C:\AI\astral-demo\Native Player Acceptance 20261018`.
Both Debug/Release Content copies matched the generated NeutralBox project with
the original authored FacetedMarker glTF, settled dynamic box and ground.
The marker binary SHA256 is
`7b522670ae131cc50db2978f6604468066d3c3c660caddb84f4685a2493e2e24`;
project.json is
`0294744c1424f4e2f960bcb9553fdcc62a959d39a4e407331eb33fc12deef6c2`.

Every launch used the unrelated session directory as cwd, outside both the
source worktree and pilot. Default launches passed no arguments and loaded
the actual EXE-adjacent Content/project.json, then scenes/main.scene.json.
Explicit launches selected the other configuration's Content directory, whose
path contains spaces, and scenes/imported-demo.scene.json; title and content
matched the explicit selection.

| Launch | UTC interval | Actual PID / elapsed | Result |
| --- | --- | --- | --- |
| Debug, no arguments | 05:15:14.254023–05:16:52.467047 | 5468 / 98.203 s | exit 0 |
| Release, explicit Debug/Content root | 05:16:52.854723–05:18:51.305622 | 23208 / 118.454 s | exit 0 |
| Release, no arguments | 05:18:51.696045–05:20:03.870699 | 57068 / 72.172 s | exit 0 |
| Debug, explicit Release/Content root | 05:20:04.249842–05:21:10.364737 | 41392 / 66.125 s | exit 0 |

Root visually inspected the actual original colored marker, settled body and
ground. In every run, the real grave-key console showed the selected scene and
`Audio output device active`; F5 produced a second loaded-scene line. The Debug
default run also verified Escape console close and actual 4K maximize/redraw
followed by correct restore; Release explicit repeated maximize/restore.
All four windows closed normally. Each exact-argv receipt records a 600-second
bound, timedOut false, actual child exit 0 and ownedChildReaped true. Final fresh
native-window inventory contained zero owned windows, and process inventory
contained zero editor/player/compiler processes. The native slot was released.

Exact cwd/argv/PID/timing/exit receipts and player logs are retained under
`work/native-runtime-player-evidence/native-{debug-default,release-explicit,release-default,debug-explicit}-first`.
The visual/console observations are root's direct UI inspection and chat images;
they are not inferred from headless output. The staged binary hashes remain
Debug 56B2153F and Release C2CC35CE in full above, and native Tool physical hash
1C189024 remains unchanged. This establishes actual device availability, not
audible-content playback. Window-title rates of roughly 4 fps Debug, 40–49 fps
Release at 1280x720 and 15 fps maximized are informal observations, not a fixed
workload benchmark or 60 Hz acceptance.

This packet's affected old game/player targets were built in both configurations;
no legacy game/player native launch is claimed by these four new-target checks.
CPU StretchDIBits, inherited focus/capture/minimize behavior, audible-content
acceptance, clean-machine runtime deployment/notices, native GPU presentation,
driver lifetime baseline and long soak remain separate gates.
