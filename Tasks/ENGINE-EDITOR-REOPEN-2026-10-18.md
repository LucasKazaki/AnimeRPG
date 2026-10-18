# Reject failed current asset attempts on explicit editor reopen

Owner `/root/demo_editor`; independent reviewer/coordinator `/root`.
Packet prepared before test edits, following fresh clock 2026-10-18 05:19:30 UTC.
Dedicated worktree `work/astral-editor-reopen`, branch
`engine/editor-reopen-20261018`, deliberately based on exact reviewed Save As
commit `4ca4a31368d5858da4a90e998bf7c84f48a47f35`.

## Problem, source evidence and boundaries

Independent runtime review identified a source hypothesis: SceneEditor
OpenScene/OpenProject check only AssetHandle::Ready(). The documented asset
contract keeps last-good data Ready after a failed replacement while Error()
describes the current attempt. Explicit reopen may therefore accept stale
cached scene/project JSON. At admission this had no actual old execution result.
The analogous Host ingress repair does not establish this editor result.

Allowed paths only:

- `Engine/Editor/SceneEditor.cpp`
- `Tests/EngineEditorTests.cpp`
- this Task
- `Docs/QA/ENGINE-EDITOR-REOPEN-2026-10-18.md`

Do not edit Host, AssetManager, Tool, CMake, Game, original model/demo content,
policy or dependencies. The initial checkout's physical SceneEditor.cpp SHA256
is `a572a630d25097b582243a13d87ca9c7a80c9e047887f477a1e9d3ec555d40ae`.
Keep it byte-identical while preparing and executing the reviewed OLD fixtures.
No production change before the actual old assertions. Root owns every native,
compiler and runtime slot; this initial authorization is source-only.

## Safe OLD fixtures and controls

Use only exclusively created private neutral fixture directories. Author and
alternate scenes/projects have distinct names, transforms, config and input
bindings. Explicitly open the alternate scene/project to cache good JSON, then
open the author project. Establish a dirty author rename, a transform edit and
Undo so both real undo/redo histories are populated; retain selection/preview
entity, camera and document/path snapshots. Never touch demonstration files.

Four independently named children cover scene/project by malformed/deleted
alternate JSON. Force Assets.Reload on the disposable alternate file; prove
Ready, nonempty current Error, unchanged good Version and advanced Attempt.
Then the FIRST assertion after OpenScene/OpenProject must require false. No
previous world/selection/history/settings access follows an unexpected true
return. This avoids stale-state/pointer access while reproducing old acceptance.
Expected old abort 3 was a hypothesis until those commands actually ran.

After a correct rejection, verify AuthorState/camera and functional Undo/Redo,
then Play/Stop to check the original project name/config and its actual W/D
input mapping. Restore valid alternate bytes, Reload, and require successful
reopen publishes repaired scene/project/settings. Separate current-good and
repaired-before-reopen controls should pass unchanged old production.

Only after actual old results and independent review, change the two explicit
entry guards to reject not Ready OR nonempty current Error, preserving existing
path diagnostics and transaction. Do not change last-good caching semantics or
silently expand into automatic hot-reload policy.

## Commands, stop condition and final gates

External attributable cache: `C:/AI/astral-build/editor-reopen-first`.
Use VS2022 installed CMake and bundled Python through the reviewed bounded
run_owned helper. Fresh >=8 GiB before every configuration build, two jobs,
MSBuild no node reuse. Preserve exact argv/cwd/PID/exits and all source/fixture
hashes before/after. Stop immediately on unexpected result; do not rerun an
identical failure or weaken assertions. Root grants OLD/fixed slots separately.

```text
cmake -S <work/astral-editor-reopen> -B C:/AI/astral-build/editor-reopen-first -G "Visual Studio 17 2022" -A x64
cmake --build C:/AI/astral-build/editor-reopen-first --config Debug --target EngineEditorTests --parallel 2
<Debug/EngineEditorTests.exe> EditorOpenSceneRejectsRetainedMalformedJson
<Debug/EngineEditorTests.exe> EditorOpenSceneRejectsRetainedMissingJson
<Debug/EngineEditorTests.exe> EditorOpenProjectRejectsRetainedMalformedJson
<Debug/EngineEditorTests.exe> EditorOpenProjectRejectsRetainedMissingJson
<Debug/EngineEditorTests.exe> EditorExplicitReopenAcceptsCurrentGoodJson
<Debug/EngineEditorTests.exe> EditorExplicitReopenAcceptsRecoveredJson
```

After the bounded correction, compile AstralEditor, EngineEditorTests,
EditorLayoutTests, EngineFrameworkTests and EngineSampleTests in Debug/Release.
Run their four anchored suites with `--stop-on-failure -j1 --no-tests=error`,
then safety/milestones1-3 and diff checks. Native/integrated candidate acceptance
and full engine workflow gates remain separately owned by root. No GPU,
clean-machine, general path-hardening or soak claim belongs to this packet.

## Actual OLD result before production edits

Root independently reviewed the OLD fixture diff
`caa893b91b32537e46dc58f3ffb9e5402536774c09f0b189ee6dc5ee1d0c0113`,
Tests SHA256 `b199f6bfea8783db78d6df52d92e1e6577cc18d10e082ed47040a0d54cfc17e8`,
immutable six-payload proof
`4abeccbe92a2e7571ed03657b97d75c8a9c86433babcb0c23d293f08d3eab856`,
and bounded runner
`6bd1db74a9efb58e3158e2815c712a8cc27c41252253a120d01d6cdfb02a8ef0`.
It then separately granted the exclusive OLD CPU slot. Fresh free memory was
12.778 GiB before the sole Debug build; configure/build child exits were zero
in 3.797/38.656s, with two jobs and no MSBuild node reuse.

All four old children actually exited 3 at exactly
`EngineEditorTests.cpp:323: CHECK failed: !opened`. Reaching that assertion proves
the Ready/nonempty current Error/retained Version/advanced Attempt preconditions
passed. No prior editor state was accessed after the unexpected successful
reopen. The two separate current-good/repaired-before-reopen controls each
exited zero with one selected case; all 2+4 positive owned directories were
absent afterward. No post-rejection state/history/settings result is claimed
from an old child that deliberately stopped before those accesses.

Exact argv/cwd/PID/exits, actual assertion logs, original/copy fixture paths and
hashes are in `work/editor-reopen-evidence/old-first`. All four .initial files
per failed fixture matched the immutable source payload proof; failed original
owned directories remain preserved, with copies in the evidence directory.
The old Debug test binary SHA256 is
`755116cb04972a1eae43a273980179c960b79ef7a1e0796f117e924f9ff3c5a3`.
All 268 production/test/Tool physical source records matched before/after,
source JSON SHA256
`71b30d4667341e3ed0d9a2d18f4f8479a9b02671bf1781b6765996e7b1285bcd`.
SceneEditor.cpp stayed at its initial physical SHA256 a572a630 above, with no
production Git diff. Session 77031 closed wrapper 0; fresh cleanup 05:30:58 UTC
found no relevant compiler/test/editor process. CPU slot released; no GUI/GPU
run occurred. Root then independently inspected all six actual child receipts
and the first safe assertions, and authorized only the two explicit entry
guards to reject !Ready or nonempty current Error. All six fixtures remain
byte-identical. At that point source review/fixed execution were pending and
root owned the separate package slot on frozen candidate 8867.

## Actual fixed gates and handoff

Root independently reviewed the exact two-guard production correction and
four-path diff SHA256
`83bdb95f7c73b09b8333c9411b7b6d7a9e72559ac773e64eebb30c22034e4df7`,
plus full bounded fixed runner
`284c399e8da8676a61baac9c64c778cf67471ccd78d6f00a4296efebf9b4759a`.
The guards now reject either not Ready or nonempty current Error, retaining
existing path diagnostics. No other caching/reload/settings policy changed.
Production SHA256 remains
`c1c536db92c7f9b7ca07bc5023b38cab5602db131fe63bf541b7537d536e4e34`;
all six fixtures remain byte-identical at Tests b199f6bf above.

The separately granted fixed slot built all five affected targets in
Debug/Release with actual child exits zero, 11.156/61.109s. Fresh memory gates
immediately before those builds recorded 13.117/12.981 GiB, two jobs and no node
reuse. Four anchored suites passed 4/4 in each configuration: Editor30,
Framework14, Sample20 and Layout, no SKIP. CTest elapsed 5.82/0.86s (wrappers
5.875/0.906s), with explicit --stop-on-failure -j1 --no-tests=error.
Safety passed 3/3 (wrapper 5.484s); milestones 1-3 and both diff checks exited
zero. Exact argv/cwd/PID/exits, logs, memory/source/binary/cleanup receipts are
in `work/editor-reopen-evidence/fixed-first`.

The actual fixed rejection tails now verify document/path/dirty/selection/
preview/camera preservation, contents of both Undo/Redo histories, real Play
project name/config and W/D input mapping, and consumer recovery after restored
bytes. All 20 private fixture paths across Debug/Release were absent after
normal completion; the earlier old-failure originals/copies remain preserved.
All 268 physical source records matched before/after, source JSON SHA256
`07039318cec00444629f06bde047799e55c6d6a91c17f3b2bca9f07eddf1b579`.
Base 4ca4 and all four then-frozen scope hashes stayed unchanged during gates.
Session 31593 closed wrapper 0; fresh cleanup at 05:37:13 UTC found zero relevant
compiler/test/editor processes, then the CPU slot was released.

Debug editor SHA256:
`59bad5935e7f7e5b4fa78afe3923d3a4c2383cf1fef60559d6154bbe7a35e9df`.
Release editor SHA256:
`20ad2ab1396e4962501375bf09a643411ee7784a8505cb8d809614693d98b45c`.
The exact old test binary remains at
`work/editor-reopen-evidence/old-first/EngineEditorTests-old.exe`.

Only final Task/QA evidence follows these gates. This core API repair is locally
verified, pending final documentation review and the four-path commit. No native
GUI/GPU ran for this packet. Combined candidate, package, native workflow and
full engine acceptance remain separately owned; no broader path/Unicode,
clean-machine or soak result follows from these component tests.
