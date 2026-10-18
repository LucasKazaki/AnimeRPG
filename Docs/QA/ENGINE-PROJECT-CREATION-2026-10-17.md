# Project creation / runtime-only consumer — planned QA

Current status: **reviewed source passes native Debug/Release project and legacy
sample suites, real CLI creation, independent Runtime-only consumer builds/runs,
engine-only and legacy player runs, safety and milestones1–3**. The generated
project has real falling/resting-box physics and matching rendered captures.
One runner manifest-order preflight and one outside-consumer configure failure
were retained and resolved without production edits. Actual native project UI,
model import, packaging and clean-machine acceptance remain separate gates.
Earlier source-readiness/planned sections below are chronological context;
the final execution section records the actual results.

Owned worktree `work/astral-project-creation`, branch
`engine/project-creation-20261017`, exact base
`2c69888774c074a69ec12c5f659e3632280db420`. Root CMake ownership was explicitly
released following frozen GPU candidate `e6885ae`; no GPU source adoption is
part of this base. Editor and parented-physics worktrees remain unchanged/frozen.
The matching task defines exactly13 paths and the proposed schema, destination
reservation/rollback, runtime factoring and future command shapes.

## Source reuse facts

Runtime already has no Game include dependency in its listed source directories.
GameHost loads projects/scenes from an explicit content root. Current headless
player supports bounded frames and captures but unconditionally registers
Playground and links Showcase in both build surfaces. NewScene is usable scene
JSON, not project creation. Sprite2D is a real subsystem, but scene Camera and
GameWorld render assembly are perspective-only; no 2D template is proposed.

The shared creator/CLI, Runtime-only player/consumer and their tests are now
implemented in source and **uncompiled/unexecuted**. Existing package manifest
and PE/prerequisite tools are useful
for a later staging packet; existing M10 package runners remain out of scope.
No package, prerequisite installation, clean-machine, native editor creation,
full create/import/edit/play/save/package or soak completion is claimed.

## Frozen source implementation before execution — historical

Root approved the exact13-path contract. The creator reserves a new root with
exclusive create_directory, tracks fixed outputs, checks ordinary directories,
canonical root identity and Windows reparse attributes, permits no pre-existing
output/.tmp entry, and writes project.json last. The original atomic JSON writer
is reused, with completed JSON read back for equality. Returned native path
strings must round-trip before any reservation, rather than silently substituting
characters unsupported by the current runtime path encoding. Rollback attempts
only known ordinary files with a single hard link and empty owned directories;
unsafe cleanup is retained and reported. No recursive unknown cleanup occurs.

Both build surfaces use the shared Runtime helper with public absolute headers,
C++17 and Threads. Caller warning/sanitizer/TSAN settings remain. Legacy
AstralPlayer retains its explicit Playground definition; the new engine player,
creator and project tests link Runtime only. Portable legacy suites still use
their existing add_test registrations; only the new project suite includes the
established root safety helper/AssertionsEnabled. No legacy flags were weakened.

Seven source tests cover real Blank3D camera/no geometry, Basic3D box falling
then resting and box/ground render IDs, invalid inputs/result preservation,
existing file/empty/nonempty directories, destination junction/broken-link
refusal, deliberately selected parent aliases, and absence of game registration.
No fixture has run and no link facility is assumed available. The outside source
consumer loads a generated project, clones its actual scene, authors its own
registered neutral behavior into that clone, writes a separate owned
`scenes/external-owned-probe.scene.json` through the atomic writer, then loads
that real file through GameHost. It checks OnCreate/Update/OnDestroy and restores
the original scene in the host, verifying the original disk bytes and JSON are
unchanged. Existing sibling/.tmp entries are refused before writing. Normal
completion removes only the owned ordinary single-link sibling after verifying
its canonical parent, unchanged authored JSON and absent temporary path. Unsafe
or failed cleanup is retained with an exact-path diagnostic; unknown entries
are never deleted. This is a bounded fixture ownership contract, not protection
against concurrent adversarial file replacement. The coordinator requested this
strengthening after the initial source review; it is now staged and unexecuted.
Compilation alone is not its acceptance. It is not manually constructed behavior
proof and does not add native behavior to the original templates.

Post-reservation I/O fault recovery, path/Unicode details, actual filesystem link
support, compile/link validity and rendering/physics assertions remain runtime
gates. No failures or passes are invented for these unexecuted cases.

## Evidence that must be collected after approval and resource release

- Real blank3D/basic3D documents parse, load and render through GameHost; the 3D
  box participates in actual physics. Original generated content only.
- Existing file/empty directory/nonempty directory/broken symlink/junction
  destinations are refused unchanged; caller result is transactional. Entry,
  canonical-root and temporary-file checks precede writes; cleanup only touches
  known owned outputs and never recursively deletes unknown files.
- Root and portable target options/test safety remain intact. New test targets
  use astral_add_test with AssertionsEnabled; legacy player registration/default
  behavior and original Playground suites remain unchanged.
- A copied outside CMake/Main fixture really builds Debug/Release from Runtime
  only, registers its own neutral native behavior and loads generated content
  from its own authored on-disk scene while running from an unrelated working
  directory. It then reloads the original scene, retains its exact disk bytes
  and removes only its checked owned sibling. Inspect actual target/link
  evidence for absence of Showcase/National Mall objects.
- Exact source SHA, commands, configurations, toolchain, times, stdout/stderr,
  actual exits and unsupported/skipped filesystem conditions are retained.
  Stop/debug an unexpected failure; no unexecuted or source-only claim is a pass.

Independent reviewer: coordinator `/root`; execution results follow.

## Actual granted execution — Debug and Release passed

Root independently reviewed the exact13-path source, including the strengthened
outside consumer, before granting serial CPU execution. The frozen physical
scope manifest was `04370f4cbeb6d22b8451545a85ea11f414a67b408e7578a10afe6b29de576f43`.
Fresh available RAM was12.656GiB before the first attempt and12.713GiB at the
actual build start. Builds used installed VS2022 x64, MSVC19.44.35222 and SDK
10.0.26100, two jobs, no installation or system setting change.

The first prepared runner used Python ordinal ordering instead of the reviewed
PowerShell case-insensitive ordering and stopped before creating fixtures or
running CMake. All13 physical hashes were checked individually and matched;
only runner ordering changed. Receipt `preflight-ordering-correction.receipt.json`
retains the diagnostic. Corrected runner SHA256 was85249a820151687e3a12a5eb70af588243f94954c9b1f9415e66026aa0e066d2.

`run-creation-acceptance.py --attempt first` then ran with a fresh owned receipt
directory. Its actual root configure/build/tests and CLI creation passed:

| Step | Actual result | Log under `acceptance-first` |
|---|---|---|
| Fresh root configure | exit0, 4.234s | `configure.log` |
| Debug five-target build | exit0, 41.016s | `build-Debug.log` |
| Debug Project/Sample CTest | exit0, **2/2**, 2.77s | `tests-Debug.log` |
| Release five-target build | exit0, 53.047s | `build-Release.log` |
| Release same CTest | exit0, **2/2**, 0.47s | `tests-Release.log` |
| Real Basic3D CLI creation | exit0 | `create-basic3d-cli.log` |

Five targets were AstralProject, AstralEnginePlayer, preserved AstralPlayer,
EngineProjectTests and EngineSampleTests. Each configuration ran Project7 and
Sample20 cases; no filesystem SKIP was reported. New project tests exercise
actual Blank3D/Basic3D parser/host/render/physics and existing destination/link
refusal controls. The CLI created the new owned project at
`acceptance-first/generated-projects/NeutralBox`, with Content/project.json.

The copied outside consumer's first configure failed before the Runtime helper
was loaded, during the generated compiler test. MSBuild MSB6003 could not access
the nested TryCompile `.tlog` directory. That directory string was220 characters;
a specific MAX_PATH threshold is not asserted. The failure and complete command
are retained in `acceptance-first/outside-configure.log` and its receipt/matrix.
The wrapper stopped; no outside build/run, player or safety step followed in
that attempt. Its session66759 closed1, no owned process remained, and the CPU
slot was explicitly released. No source correction was inferred from this alone.

Root authorized resuming only the uncompleted tail at the new shorter owned
build path `C:/AI/astral-build/creator-external-first`. The old failed tree was
retained. The exact copied CMake/Main bytes and complete source binding matched
the first attempt; no root matrix was repeated. Fresh available RAM was13.142GiB.
`run-creation-tail.py` reserved only the absent approved target, with no delete,
and ran all consumer/player commands from the original unrelated working folder:

| Step | Actual result | Log under `acceptance-external-tail` |
|---|---|---|
| Same outside configure at short build path | exit0, 2.937s | `outside-configure-short.log` |
| Outside Runtime-only Debug/Release build | exit0, 27.391/39.984s | `outside-build-Debug/Release.log` |
| Outside consumer Debug/Release run | exit0, 0.234/0.063s | `outside-consumer-Debug/Release.log` |
| Engine-only player D/R,120 frames each | exit0 each | `engine-player-Debug/Release.log` |
| Legacy player D/R, bounded original sample | exit0 each | `legacy-player-Debug/Release.log` |
| Safety | exit0, **3/3**, 5.488s harness time | `test_test_safety.log` |
| Milestones1/2/3 and whitespace | exit0 each | matching named logs |

Both outside consumers print actual `create=1 update=3 destroy=1`. Their always
active checks write/load their own authored on-disk scene, run the registered
native behavior, restore the original scene, preserve its exact bytes and remove
only the checked owned sibling. Probe and `.tmp` are absent afterward.

Actual generated VS target metadata confirms the outside consumer and new
engine player depend on AstralEngineRuntime plus Windows system libraries,
with no Showcase/Playground/National Mall project reference. The outside Runtime
compiles50 sources, all under Engine; no non-engine source is listed. The legacy
player retains its Playground define and Showcase link and actually loads its
original sample. Inspection is recorded in `actual-runtime-link-inspection.json`
and the generated vcxproj files; this is source/target evidence, not a packaged
binary SDK or clean-machine dependency result.

Engine-only runs load4 entities and simulate2 bodies. At frame120 the Box rests
atY0.490, with2 visible draws and9 rasterized triangles. Both captures are320x180,
image hash973791a64d79e61e, file SHA256
`e9fb04ef7aa330764e031f6214162745a220de8f73607ee3b04beda3a443bc6c`:
`acceptance-external-tail/engine-player-Debug/frame_00120.png` and the matching
Release path. Root visually inspected both actual files after the slot release:
the box is visible on the ground beneath the sky, with a coherent shadow and
no apparent missing geometry. This is bounded visual inspection of this320x180
scene; one capture timing is not a performance benchmark or frame-rate claim.

The full270-file Engine/Game/Tests/Tools/cmake/root-CMake binding remained exact
before/after both attempts:59f25cacd83b83565b84a779ae3caebef8c184f9582441ce66476b0162d7a521.
Scope04370f4c... also matched. The first matrix began October18
00:26:01.464751-04:00; the successful tail ran00:29:41.763265–00:30:58.620149-04:00.
Session84381 closed0 and normalized-path inventory found no remaining owned
compiler/player/test process before explicit slot release. No GUI, GPU, historical
R0 or package operation ran. Only Task/QA prose changes after this frozen source
execution. Root reviewed the final source/docs/link evidence and both images
as clean, then authorized the exact13-path commit. Combined integration and
the remaining native import/package gates still require their own checks.
