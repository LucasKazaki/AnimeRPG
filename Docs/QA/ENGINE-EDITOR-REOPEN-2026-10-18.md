# Explicit editor reopen evidence

Owner `/root/demo_editor`; independent reviewer and execution coordinator `/root`.
Task `Tasks/ENGINE-EDITOR-REOPEN-2026-10-18.md`.
Exact base `4ca4a31368d5858da4a90e998bf7c84f48a47f35`.

Status: four actual OLD failure witnesses, passing controls and fixed
Debug/Release affected-target regressions; final documentation review/commit
and combined candidate acceptance remain.

Source OpenScene/OpenProject accept Ready JSON without consulting the current
asset attempt's Error, while AssetHandle explicitly permits retained last-good
Ready data after failure. A separate actual Host ingress failure is context,
not proof of this editor hypothesis. Initial SceneEditor.cpp physical SHA256
`a572a630d25097b582243a13d87ca9c7a80c9e047887f477a1e9d3ec555d40ae`
remained unchanged through the reviewed OLD execution recorded below.

The Task defines four independently runnable failure fixtures and two positive
controls. Each failure first proves a failed current asset reload retaining good
data, then requires Open false BEFORE any prior editor-state access. Only a
correct rejection permits document/paths/selection/camera/preview/history and
actual Play project/config/input checks. Fixtures use only owned neutral files;
current demonstration project, original model fixture and older logs stay intact.

Root substantively reviewed fixture patch SHA256
`caa893b91b32537e46dc58f3ffb9e5402536774c09f0b189ee6dc5ee1d0c0113`,
Tests `b199f6bfea8783db78d6df52d92e1e6577cc18d10e082ed47040a0d54cfc17e8`,
six immutable raw JSON payload proof
`4abeccbe92a2e7571ed03657b97d75c8a9c86433babcb0c23d293f08d3eab856`
and the full bounded runner
`6bd1db74a9efb58e3158e2815c712a8cc27c41252253a120d01d6cdfb02a8ef0`.
After its explicit CPU grant, the fresh memory gate recorded 12.778 GiB.
Configure/build child exits were zero, 3.797/38.656s; only Debug EditorTests was
built, at two jobs with MSBuild node reuse disabled.

All four independently named malformed/missing scene/project children actually
exited 3 at the safe `!opened` assertion, line 323, after their Ready/current Error/
Version/Attempt preconditions. Thus old explicit reopen returned true, and no
prior-state access followed it. No assertion in the rejection-only history/
settings tail ran on those old failures. The separate current-good and repaired
controls each exited zero with one selected case. Their 2+4 exclusively owned
fixture directories were absent after normal completion.

Each failed fixture's four actual .initial hashes matched the reviewed source
payloads. Both original failed directories and exclusive evidence copies are
preserved, without deleting unknown files. Exact source/fixture hashes and
paths, selected child argv/cwd/PID/exits and logs are retained under
`work/editor-reopen-evidence/old-first`. The old test binary SHA256 is
`755116cb04972a1eae43a273980179c960b79ef7a1e0796f117e924f9ff3c5a3`.
All 268 physical source records matched before/after, JSON SHA256
`71b30d4667341e3ed0d9a2d18f4f8479a9b02671bf1781b6765996e7b1285bcd`.
SceneEditor.cpp remained byte-identical with empty production Git diff.
Session 77031 closed wrapper 0; cleanup at 05:30:58 UTC found zero relevant
compiler/test/editor processes before releasing the CPU slot. No GUI/GPU ran.

Root inspected all six actual child receipts and safe assertions, then
authorized exactly the two explicit entry guards to reject !Ready or nonempty
current Error with existing path diagnostics. All six fixtures stay unchanged.
At that point the correction still needed independent review and a separate
fixed CPU slot, while root owned the frozen-candidate package slot. Subsequent
results are recorded below; integration/native/full workflow gates are separate.

## Actual fixed affected-target acceptance

Root substantively reviewed four-path diff SHA256
`83bdb95f7c73b09b8333c9411b7b6d7a9e72559ac773e64eebb30c22034e4df7`
and full fixed runner SHA256
`284c399e8da8676a61baac9c64c778cf67471ccd78d6f00a4296efebf9b4759a`.
Production changes only the two OpenScene/OpenProject Ready/Error guards;
existing path diagnostics and all six fixtures remain unchanged. Fixed
SceneEditor.cpp SHA256:
`c1c536db92c7f9b7ca07bc5023b38cab5602db131fe63bf541b7537d536e4e34`.
Tests still SHA256 b199f6bf above.

| Gate | Debug | Release |
| --- | --- | --- |
| Five affected targets | child 0, 11.156s | child 0, 61.109s |
| Four anchored suites | 4/4, CTest5.82s | 4/4, CTest0.86s |
| Editor/Framework/Sample | 30/14/20 cases | 30/14/20 cases |
| Layout and link fixtures | pass, no SKIP | pass, no SKIP |
| Fresh memory before build | 13.117 GiB | 12.981 GiB |

The attributable cache reused the same owned source, with two jobs/no node
reuse. CTest argv explicitly used --stop-on-failure -j1 --no-tests=error and
the four-suite anchored expression. Safety passed 3/3, wrapper 5.484s;
milestones 1-3 and both diff checks exited zero. Exact argv/cwd/PID/exits,
per-build memory, full source/scope and binary receipts are retained in
`work/editor-reopen-evidence/fixed-first`.

Unlike the deliberately aborting old children, the actual fixed failure cases
all traversed their rejection tails: preserved document/paths/dirty/selection/
preview/camera; functional contents of both Undo/Redo histories; original
project settings consumed through actual Play and W/D action input; then
repaired asset reload/reopen publishing the intended scene/project/settings.
All 20 private fixture paths from both full suites were absent afterward.
The four earlier failed old fixture originals and checked copies are preserved.

All 268 physical source records matched before/after, source JSON SHA256
`07039318cec00444629f06bde047799e55c6d6a91c17f3b2bca9f07eddf1b579`.
Base 4ca4 and all four frozen scope hashes stayed unchanged during execution.
The harness session 31593 closed wrapper 0; fresh 05:37:13 UTC inventory found
zero compiler/test/editor processes, then the CPU slot was released. Only final
Task/QA evidence was edited afterward. No native GUI/GPU was launched.

Actual Debug editor: 7,064,576 bytes, SHA256
`59bad5935e7f7e5b4fa78afe3923d3a4c2383cf1fef60559d6154bbe7a35e9df`.
Release editor: 1,792,512 bytes, SHA256
`20ad2ab1396e4962501375bf09a643411ee7784a8505cb8d809614693d98b45c`.
The old failing test executable is preserved separately as
`work/editor-reopen-evidence/old-first/EngineEditorTests-old.exe`.

The bounded core API correction is locally verified; final documentation
review/commit and combined candidate checks remain. These tests establish no
new native UI, package, GPU presentation, Unicode/general path-hardening,
clean-machine or soak acceptance. Full engine workflow acceptance remains with
the coordinator.
