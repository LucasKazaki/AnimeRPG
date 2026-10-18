# Noon demonstration assembly evidence

Owner `/root`; Task ENGINE-NOON-DEMO-ASSEMBLY-2026-10-18.
Status: source assembly only; no combined configure/build/test/native/package
acceptance yet. Every input's earlier evidence remains limited to that input.

The branch deliberately started at reviewed Project1804a3c. Task commit10d4e59
preceded source integration. Creator5c9fb44 merged normally without conflicts.
The next normal editor1429312 merge conflicted in five approved paths:
GameHost.h/cpp, root/portable CMake and cmake/AstralEngineSources.cmake.

The initial resolution retained Project's exact Host.h/cpp bytes, including the existing
ApplyProjectSettings signature consumed by Editor Play; the superseded editor
hook's sequential config publication is not restored. CMake preserves Creator's
shared Runtime helper and Runtime-only ProjectTests, adds EditorCore and keeps
both real Project/Editor suites registered. Portable suites preserve their
existing warning/sanitizer/test options; Editor adds its core dependency after
the Runtime-only Project exception. Source list contains both suites once.

Independent editor-owner review identified lost unnamed-scene restart behavior
from the editor parent. Root reproduced it before changing production: fresh
native Debug EditorTests compiled, then existing unchanged
PlayRunsTheEditedSceneAndRestartsIt aborted with actual child exit3 at line513,
sceneLoads/LastError assertion. The exact-argv bounded runner and six physical
source hashes before/after are retained in work/noon-editor-old-evidence.

The corrected resolution preserves Project's settings transaction while
restoring only the editor parent's document-copy Activate branch for unnamed
scene requests and its matching restart command/LoadSceneJson comment. This is
an intentional combined Host, not byte-identical to either full parent.
Independent editor-owner re-review of the corrected Host/doc delta was clean;
the staged CMake/suite resolution also remained clean. Root rebuilt native Debug
EditorTests and ran its complete registered suite:24 cases,1/1 CTest pass in
2.97s, including the formerly failing unchanged restart case and real filesystem
controls. Host/header/test physical hashes matched before/after. Fixed receipts
are separate in work/noon-editor-fixed-evidence. Full combined Debug/Release
all-target acceptance remains pending; this focused result authorizes only the
reviewed merge resolution.
Native-player, Host-ingress and packaging packets remain separately owned.
No R0, GPU presentation, clean-machine or24hour-soak result exists here.
