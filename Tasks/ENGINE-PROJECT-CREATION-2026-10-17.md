# Create a native engine project and consume Runtime independently

Owner `/root/demo_editor` (transferred from `/root/containment_implementation`), sole writer in
`work/astral-project-creation`, branch `engine/project-creation-20261017`.
Deliberate base `2c69888774c074a69ec12c5f659e3632280db420`, distinct from main.
Independent reviewer/compiler scheduler: `/root`. Lucas's current engine-only
weekend authority applies; older per-change/local-only limits are superseded,
technical gates remain. Local time checked October17, 15:38-04:00.
Root approved source implementation after reading this contract. The final
Debug/Release project/runtime consumer gates now pass as recorded below and in
QA; native UI/import/package gates remain separate. The user released the
desktop/GPU hold for the noon October18 demonstration deadline; all execution
still needs the coordinator's serial slot.
Editor and parented-physics source remain separately frozen. Root CMake ownership
was explicitly released after GPU candidate `e6885ae` froze; do not change that
candidate or adopt its source silently into this deliberate base.

## Exactly 13 allowed paths

1. `Engine/Projects/ProjectCreation.h`
2. `Engine/Projects/ProjectCreation.cpp`
3. `Tools/AstralProject.cpp`
4. `Tools/AstralPlayer.cpp`
5. `cmake/AstralRuntime.cmake`
6. `cmake/AstralEngineSources.cmake`
7. `CMakeLists.txt`
8. `Tests/EngineRuntime/CMakeLists.txt`
9. `Tests/EngineProjectTests.cpp`
10. `Tests/ExternalProject/CMakeLists.txt`
11. `Tests/ExternalProject/Main.cpp`
12. This packet
13. `Docs/QA/ENGINE-PROJECT-CREATION-2026-10-17.md`

No Host/Scene/Assets/Editor/Game/Win32 player, policy, workflow, packaging,
dependency or license edits. No new framework/dependency. No 2D template,
native New Project button, packaging, clean-machine or soak acceptance claim.

## Proposed API, CLI and exact generated document contract

One shared C++ creator, reused later by native UI, takes an **explicit existing
approved parent directory**, a **single new directory name**, display name and
`Blank3D`/`Basic3D` enum. No cwd fallback. Result paths are assigned only after
complete success; failure returns a diagnostic and preserves prior result data.
Proposed public types: `ProjectTemplate { Blank3D, Basic3D }`,
`ProjectCreationRequest { approvedParent, directoryName, displayName, templateKind }`,
and `CreatedProject { root, contentRoot, projectFile }`, with string paths and
`bool CreateProject(const ProjectCreationRequest&, CreatedProject&, std::string&)`.
An empty displayName uses directoryName; otherwise it must contain no NUL/control
characters. Path conversion failures return diagnostics; no silent truncation.
Successful result paths are absolute resolved paths; projectFile names the
generated Content/project.json. README and JSON retain the chosen display name.
CLI: `AstralProject create <parent> <directory> --template blank3d|3d [--name <display>]`.
Unknown/missing options fail; no launch, editor replacement or shell execution.

Generated tree: `<parent>/<directory>/Content/project.json`,
`Content/scenes/main.scene.json`, and root `README.md` with exact run commands
and original-fixture provenance, without a new redistribution-license grant.
Project JSON has only:
`{"format":"astral-project","version":1,"name":<display>,"startupScene":"scenes/main.scene.json","input":{"contexts":[]},"config":{"r.Quality":2}}`.
No game behavior names, imported files or absolute resource paths.

Both scenes have `format:"astral-scene"`, `version:1`, environment object and
entities array. Blank3D has only Main Camera at position[0,3,-8], Euler[14,0,0],
Camera(fieldOfView60,nearPlane0.1,farPlane100) and AudioListener. Basic3D adds
Sun Euler[50,30,0], Directional Light(intensity2.5); Ground with scale[10,1,10],
MeshRenderer(mesh`primitive:plane`,material`neutral`) and Collider(shape`Mesh`);
Box at[0,2,0], MeshRenderer(mesh`primitive:cube`,material`neutral`),
Collider(shape`Box`,halfExtents[0.5,0.5,0.5]), RigidBody(type`Dynamic`,mass1).
Basic3D defines one neutral material(shading`Toon`,baseColor[0.55,0.58,0.62]).
No scripted controls are implied. Play means actual GameHost simulation/render.
Validate JSON via real ParseProject and SceneSerializer before writing it.

## Proposed destination reservation and rollback contract

Reject empty/NUL-containing roots, invalid template/name data, and directory
names containing separators, colon, control characters, dot/dotdot, trailing
dot/space or Windows device basenames. Do not truncate names. Explicitly catch
path-conversion/filesystem errors; Unicode/native-path acceptance is not assumed.
Canonicalize the selected existing parent (a deliberately selected parent alias
resolves to the actual approved root), then use whole-component containment.
Refuse any existing final entry, including an empty directory, file, broken
symlink or junction/reparse alias. Never traverse an existing destination.

Reserve the final directory with exclusive `create_directory`: success means
this call created it; false/error never gives ownership. Generate only fixed
relative subpaths below that reservation. Before each write check existing
ancestors still resolve below the owned root and approved parent; reject any
unexpected entry or symbolic/reparse redirection at known output/.tmp paths.
Reuse the atomic JSON writer only for known absent new paths; do not truncate
pre-existing files or temporary aliases. Write project.json last as the commit
marker. Report success only after all files are written. The new folder can be
visible during creation; this is **not atomic whole-directory publication**.

Track exactly which files/directories this call created. On failure, remove
only still-owned ordinary files/temps and then empty owned directories, deepest
first, after canonical/entry checks; never recursive-delete unknown contents or
follow a replacement link. If cleanup is unsafe/incomplete, retain that path and
report it. Existing destinations and caller outputs remain untouched. This is
a bounded single-writer creation transaction, not a race-proof sandbox against
concurrent filesystem mutation. No fault-injection/test-only public API.

## Runtime factoring and legacy preservation

New `astral_add_runtime()` creates the existing AstralEngineRuntime target from
the same source list, PUBLIC absolute engine include root, PUBLIC Threads and
C++17 requirement; add Astral::Runtime alias for external consumers. Root and
portable builds call it, preserving global standard/runtime-library settings.
Keep caller-owned warnings/sanitizers/TSAN options and mutual-exclusion check
unchanged: portable still calls astral_portable_options on Runtime and every
existing target. Do not move platform definitions to Runtime or change Game,
Editor, Win32 player, Showcase, MSVC optimization or test timeout behavior.

Reuse Tools/AstralPlayer.cpp for new AstralEnginePlayer linked only to Runtime;
compile Playground include/registration only under an explicit legacy-target
definition. Define it for existing AstralPlayer in **both** root and portable
builds; current AstralPlayer/Win32/Showcase behavior and registration stay intact.
AstralProject CLI and EngineProjectTests also link only Runtime. New native
tests use the established root astral_add_test/AssertionsEnabled helper in both
build surfaces. Portable currently uses add_test for legacy suites: retain those
registrations/options, and include the established helper for the new suite
only. The new suite must link only Runtime even in the portable foreach, whose
other suites retain Showcase. The external fixture includes
only the runtime helper and compiles its own Main/neutral native behavior. It
loads the generated project, writes its own separate sibling scene through the
real atomic writer, then loads that file through GameHost. It checks actual
OnCreate/Update/OnDestroy and restores the original scene in the running host,
with the original disk bytes unchanged. It refuses any existing sibling/.tmp,
and normal completion removes only its checked ordinary, single-link, unchanged
owned scene after confirming its temporary path is absent; unsafe or failed
cleanup retains the path with a diagnostic. No generated template is overwritten.
Copy the fixture outside the repository before its independent configure/build;
no root add_subdirectory, GameShowcase or National Mall object dependency.

## Original planned gates and commands — historical

After source approval, prepare creation success/refusal/result-preservation and
real parser/host/render tests, including existing destination/symlink/junction
controls where supported. Keep original Playground suites unchanged. Future
execution needs a fresh explicit resource/compiler slot; stop on unexpected
deterministic failures and retain actual logs/source SHA. No source-only pass
may be relabeled runtime evidence. Independent review before final acceptance.

```text
cmake -S <worktree> -B <external-build> -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
cmake --build <external-build> --config Debug --target AstralProject AstralEnginePlayer AstralPlayer EngineProjectTests EngineSampleTests --parallel 2
ctest --test-dir <external-build> -C Debug --output-on-failure -R "^(EngineProjectTests|EngineSampleTests)$" --no-tests=error
# Repeat matching Release targets/suites with assertions retained.
AstralProject.exe create <owned-parent> NeutralBox --template 3d --name "Neutral Box"
# Copy only the authored Tests/ExternalProject fixture into a new outside source directory.
cmake -S <outside-source> -B <outside-build> -G "Visual Studio 17 2022" -A x64 -DASTRAL_ENGINE_ROOT=<worktree>
cmake --build <outside-build> --config Debug --parallel 2
# Repeat Release; run its bounded consumer against generated Content/project.json.
AstralEnginePlayer.exe <owned-parent>/NeutralBox/Content project.json --frames 120 --size 320x180 --capture <outside-captures>
python Scripts/test_test_safety.py
python Scripts/verify_milestone1.py
python Scripts/verify_milestone2.py
python Scripts/verify_milestone3.py
git diff --check
git diff --name-only
```

Resolve all placeholders to task-owned disjoint paths and installed tools before
any granted run, retaining exact commands/times/exits. Proposed cache/evidence:
`work/build-project-creation`, `work/project-creation-evidence`. Win32 adapter
extraction, native New Project integration, staging/notices/runtime dependencies,
clean Windows launch and 24-hour soak remain separate future packets/gates.

## Source-ready status before execution — historical

Approved implementation is staged in exactly the allowed13 paths, with seven
project test cases and the independent outside consumer fixture. The coordinator
reviewed the initial source and requested the file-based consumer proof above,
now staged for re-review. No configure, build, test, process fixture, UI or GPU
execution occurred for this packet. All runtime acceptance remains pending a
coordinator slot. See QA for
implementation facts and unverified limits; no final acceptance commit yet.

## Actual execution outcome — passes retained, earlier failures preserved

The coordinator independently reviewed the13-path implementation and external
file-consumer strengthening. Under granted serial CPU slots, Debug/Release
five-target builds pass, as do Project7+Sample20 cases (2/2 suites each,
2.77/0.47s). Real CLI Basic3D creation succeeds. No filesystem SKIP is reported.
The copied outside consumer builds and runs D/R from an unrelated working
directory using only the public Runtime target, actually writes/loads its own
native-behavior scene and prints create1/update3/destroy1 while preserving the
original scene bytes. Its owned probe/temp are absent after checked cleanup.
Both engine-only120-frame player runs and bounded legacy sample runs pass;
the Box rests atY0.490 with2 bodies and matching320x180 captures. Safety3/3 and
milestones1–3 pass. Actual target metadata lists50 Runtime sources, all Engine,
and no Showcase dependency for the new engine player/outside consumer.

Two earlier tooling failures remain documented: a manifest-order preflight
stopped before CMake (all13 individual hashes matched), and the first deep
outside build's compiler test could not access its generated .tlog directory.
The latter passed at the coordinator-approved shorter new build path
`C:/AI/astral-build/creator-external-first`, with identical source bytes and no
system setting or production edit. The failed old tree is retained; passed
root matrices were not repeated. See QA for actual commands, times, exits and
the bounded path inference rather than an invented threshold.

Full270-file source binding stayed59f25cacd83b83565b84a779ae3caebef8c184f9582441ce66476b0162d7a521,
scope manifest04370f4c... before/after execution. The successful tail's session84381
closed0 and normalized owned-process inventory was empty before slot release.
Captures have fileSHA256 e9fb04ef7aa330764e031f6214162745a220de8f73607ee3b04beda3a443bc6c;
root has now visually inspected both actual captures: visible box on ground/sky,
coherent shadow, no apparent missing geometry in this bounded scene. No native
wizard, model import, GPU, package,
clean-machine or soak acceptance is claimed. Only Task/QA prose changed after
the frozen source matrix. Root reviewed final source/docs/link evidence and
both images as clean and authorized the exact13-path commit; combined
integration and the remaining workflow gates are separate.
