# E11 Editor Runtime Smoke evidence, 2026-09-22

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. The packet verifies the already-integrated Win32 editor shell and does not authorize scene mutation/serialization, transform gizmos, Play-in-Editor, asset import, graphics/API changes, dependencies, game content, scheduler operations, deployment, release, or R0 execution.

Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed during this pass: `755faabfb5f04d5bc07324d91cbceb261cdc1060`. No rebase, merge, force push or unrelated game-work absorption was performed.

Current code candidate: `46b9a018af5dd6a1ae01d414497ee1e6ef294dfb`.
`Tests/EditorRuntimeSmoke.cpp` blob: `98ab2d576da85b5f298bd83bb5135c79fc21b2b0`.
Integrated editor source fixture: `Tools/AstralEditorMain.cpp` blob `5142e632a79c89d0d0ce3efe87e752456f802185`; not modified by this packet.

## Independent finding repaired

The latest independent Codex review reported one current P2 false-pass gap: post-resize and final shell validation freshly enumerated controls, so a `WM_SIZE` regression that destroyed and recreated a control with matching class, caption, ID, contents and state could still pass. The task contract requires the original shell control identities to survive.

Candidate `46b9a018...` repairs that gap:

- `CaptureInitialControlInventory` records exactly twelve direct process-owned child HWND plus class identities after the top-level window is stable;
- `SameControlHandles` performs one-to-one, enumeration-order-independent equality of the later handle/class set with that initial set;
- `ValidateShellState` rejects replacement/missing/class-changed controls before semantic checks and rechecks the same inventory again after all bounded reads;
- `DirectChildrenContained` rejects an inventory change before geometry is accepted;
- the Cube selection path checks the initial inventory before `LB_SETCURSEL` and again before the bounded `LBN_SELCHANGE` notification;
- each 800x600 and 420x260 resize checks the original inventory immediately before the side effect and again through containment/full-state validation;
- final shell validation still requires the original twelve handles.

The previous per-send ownership/parent/class/visibility/control-ID checks remain in place, so current HWND identity and the original-inventory continuity requirement are both enforced. Production editor code was not changed.

## Primary-source research, rechecked 2026-09-22

References are behavioral/API references only. No proprietary source, artwork, asset or dependency was copied/imported.

- Epic Games, Unreal Engine 5.8, Unreal Editor Interface: https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-editor-interface
  - Defines the editor as cooperating Level Viewport, Outliner, Details and Content Browser/Drawer surfaces. Astral's twelve-control fixture remains a minimum verification target, not parity.
- Unity Technologies, Unity 6.0 (6000.0), The Hierarchy window: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html
  - Defines the Hierarchy as a scene-object management surface; retaining editor surface state/identity across layout changes is part of the verification contract.
- Microsoft `IsWindow`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindow
  - Warns that HWNDs can be destroyed and recycled to different windows. A fresh semantic match alone therefore does not establish continuity with a previously observed shell control.
- Microsoft `EnumChildWindows`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-enumchildwindows
  - Enumerates child HWNDs and documents that children destroyed before enumeration or created during enumeration are not enumerated, supporting repeated fail-closed inventory checkpoints rather than one snapshot.
- Microsoft `GetDlgItem`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getdlgitem
  - Returns the current child HWND for a unique control ID but does not prove it is the same HWND captured before resize.

Access date: 2026-09-22.

## Portable reproduction and compiler checks

Disposable C++17 handle-inventory fixture SHA-256:
`619092d28540a53ee81e93efa29c93efc6571845de6d631a38800f4460c9ff4e`

Commands executed in the coordinator sandbox:

```bash
g++ -std=c++17 -Wall -Wextra -Werror -pedantic /tmp/e11_handle_inventory_fixture.cpp -o /tmp/e11_handle_inventory_fixture
/tmp/e11_handle_inventory_fixture
clang++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer /tmp/e11_handle_inventory_fixture.cpp -o /tmp/e11_handle_inventory_fixture_san
ASAN_OPTIONS=detect_leaks=1 /tmp/e11_handle_inventory_fixture_san
sha256sum /tmp/e11_handle_inventory_fixture.cpp
clang++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsyntax-only -I/tmp/winstub /tmp/EditorRuntimeSmoke.new.cpp
```

Results:

- GCC warning-clean compile and execution: PASS, printed `initial HWND inventory preservation contract fixture: PASS`.
- Clang ASan+UBSan warning-clean compile and execution with leak detection: PASS, printed the same marker.
- Fixture hash: `619092d28540a53ee81e93efa29c93efc6571845de6d631a38800f4460c9ff4e`.
- Exact rewritten smoke SHA-256: `5b24e49f0b37f3a83f710e4147dcc1253679a07f7e5258de8e6190bbb0c49302`; Git blob `98ab2d576da85b5f298bd83bb5135c79fc21b2b0`.
- The exact rewritten smoke passed Clang C++17 `-Wall -Wextra -Werror -pedantic -fsyntax-only` against a disposable minimal Win32 declaration shim.

The fixture accepts identical handles in a different enumeration order and rejects a replaced handle, class change, duplicate replacement, and missing control. These are source-logic/compiler checks only. They are not native Win32 editor execution.

One public `git clone` attempt in the sandbox failed because DNS could not resolve `github.com`; it was not repeated without a changed condition. Exact repository source and writes were instead verified through the connected GitHub repository interface and hosted CI.

## Hosted exact-candidate evidence

Exact code candidate `46b9a018af5dd6a1ae01d414497ee1e6ef294dfb` passed Windows Server 2022 run `35754110237`, job `106835418831`, completed at `2026-09-22T16:28:22Z`. Successful steps included checkout/external build root, R0 parser-only and safety contracts, PE/prerequisite/runtime-policy contracts, Release assertion and CTest safety contracts, VS2022 x64 configuration, MSVC Debug build and deterministic Debug tests, MSVC Release build and deterministic Release tests, Release dependency/prerequisite/runtime checks, static milestone verifiers and clean tracked-tree verification. The historical R0 runner itself was not executed.

Additional exact-candidate checks:

- Profiling capture portability run `35754109941`: PASS.
- Release manifest integrity run `35754109935`: PASS.

Hosted deterministic CTest intentionally excludes every `RuntimeSmoke`. These hosted results therefore prove compilation and deterministic non-runtime regression status, not native editor GUI execution.

## Review state

The latest independent review finding is repaired by `46b9a018...`, but that review predates this code change. Fresh independent review of the exact final code/evidence head is required. Same-author inspection is not counted as independent acceptance.

## Native evidence and handoff

`native_evidence` remains empty. The coordinator did not access or claim a registered Windows interactive desktop.

Run exact code candidate `46b9a018af5dd6a1ae01d414497ee1e6ef294dfb` on one owned interactive Windows desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain source SHA, machine/Windows identity, MSVC and CMake versions, GPU/driver identity, exact commands, full stdout/stderr, exit codes, UTC timestamps and screenshots at normal and narrow/short sizes. Human-visible acceptance must confirm exact `OUTLINER`, `INSPECTOR`, and `ASSETS / DEFAULT PRIMITIVES` labels; exact four asset rows; truthful E11 status text; Outliner visibly remains on Cube while Inspector shows Cube; viewport `Selected:` text reflects Cube; and panels do not bleed during resize.

## Result

Status: **original twelve child HWND identities are now retained and checked across selection, both resizes, containment, full shell validation and final validation; portable warning-clean/sanitizer fixture passes; exact-candidate hosted Windows Debug/Release, profiling and release-manifest workflows pass; fresh independent review and native Debug/Release RuntimeSmoke remain pending**.

E11 remains a partial editor-shell candidate, not UE5/Unity parity. No native GUI, GPU/performance, clean-machine, stress/recovery, soak, or final independent runtime acceptance claim is made. Issue #7 remains separate and open, and R0 was not invoked.

Single next useful action: obtain fresh independent review of the exact final code/evidence head, then execute Debug and Release `EditorRuntimeSmoke` on the registered Windows desktop with the required receipts/screenshots.
