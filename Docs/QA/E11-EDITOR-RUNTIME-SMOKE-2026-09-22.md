# E11 Editor Runtime Smoke evidence, 2026-09-22

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. The packet verifies the already-integrated Win32 editor shell and does not authorize scene mutation/serialization, transform gizmos, Play-in-Editor, asset import, graphics/API changes, dependencies, game content, scheduler operations, deployment, release, or R0 execution.

Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Current independently moving `main` re-read during this pass: `f68e0917b38a0b4780d94a409874bb94df818163`. No rebase, merge, force push or unrelated game-work absorption was performed.

Current code candidate: `625e744e8c20bdb5c768e643e6cbaf4113808212`.
`Tests/EditorRuntimeSmoke.cpp` blob: `b0799149ed0794adbb32c0653414deb9bda5ecd4`.
Integrated editor source fixture: `Tools/AstralEditorMain.cpp` blob `5142e632a79c89d0d0ce3efe87e752456f802185`; not modified by this packet.

## Finding repaired

The previous smoke required five direct `Static` controls and exact Scene Root Inspector text, but did not prove the identities of the other four shell Statics. A shell with the same count but a renamed panel heading, duplicated label, or stale status line could therefore pass.

Candidate `625e744...` adds exact constants for the integrated shell text and requires all four non-Inspector Statics to be found by exact text and be visible:

- `OUTLINER`
- `INSPECTOR`
- `ASSETS / DEFAULT PRIMITIVES`
- `E11.0 editor shell | Outliner selection works | viewport transform tools, undo/redo, save/reopen, Play and real asset import pending`

The complete Scene Root Inspector fixture remains separately identity-checked and is now also required visible. Since the direct Static count is still exactly five, the repaired contract proves the identity of every direct Static while preserving the existing process/window side-effect boundaries.

## Primary-source research, rechecked 2026-09-22

References are behavioral/API references only. No proprietary engine source, artwork, assets or dependency was copied/imported.

- Epic Games, Unreal Engine 5.8, Unreal Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
  - Epic distinguishes named Level Viewport, Outliner, Details panel and Content Drawer surfaces and describes their separate editor roles. This supports identity-level shell evidence rather than anonymous class counts.
- Epic Games, Unreal Engine 5.8, Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
  - The Outliner is a named hierarchical selection surface.
- Unity Manual, editor interface family: https://docs.unity3d.com/Manual/UsingTheEditor.html
  - Unity likewise exposes distinct named Hierarchy, Scene, Inspector and Project surfaces; Astral's E11 fixture is far smaller, but its claimed surfaces should be identity-checked. This is a behavioral reference, not a copied implementation.
- Microsoft `WM_GETTEXT`: https://learn.microsoft.com/windows/win32/winmsg/wm-gettext
  - Text static controls return text through this message; the smoke's existing bounded `WindowText` helper uses `WM_GETTEXTLENGTH`/`WM_GETTEXT`. List-box rows remain on LB_GETTEXTLEN/LB_GETTEXT.

## Portable reproduction

Disposable C++17 fixture SHA-256:
`6dbe2d7e13203e0b796cc5d8844d2b322959a1912d327123713bccbf555cf304`

Commands:

```bash
g++ -std=c++17 -Wall -Wextra -Werror /tmp/e11_static_identity_fixture.cpp -o /tmp/e11_static_identity_fixture
/tmp/e11_static_identity_fixture
clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /tmp/e11_static_identity_fixture.cpp -o /tmp/e11_static_identity_fixture_san
ASAN_OPTIONS=detect_leaks=1 /tmp/e11_static_identity_fixture_san
sha256sum /tmp/e11_static_identity_fixture.cpp
```

Results:

- GCC compile: PASS.
- GCC execution: PASS, printed `static identity contract fixture: PASS`.
- Clang ASan+UBSan compile: PASS.
- Sanitized execution with leak detection: PASS, printed the same PASS marker.
- Fixture hash: `6dbe2d7e13203e0b796cc5d8844d2b322959a1912d327123713bccbf555cf304`.

This fixture is source-logic evidence only. It demonstrates that the previous five-Statics-plus-Inspector model accepts a wrong panel label, stale status or duplicate label, while the exact-identity model rejects those cases. It is not production Win32/native execution.

## Exact hosted evidence

Windows Server 2022 run `35740208183`, job `106787679557`, ran against exact code candidate `625e744e8c20bdb5c768e643e6cbaf4113808212` and completed successfully on 2026-09-22.

Successful steps included checkout/external build root, R0 parser-only plus runner-safety contracts, PE/prerequisite/runtime-policy contracts, Release-assertion/CTest safety contracts, VS2022 x64 configure, MSVC Debug build and deterministic Debug tests, MSVC Release build and deterministic Release tests, Release dependency/prerequisite checks, static milestone verifiers and clean tracked-tree verification. The historical R0 runner itself was not executed.

Additional exact-candidate hosted checks:

- Profiling capture portability run `35740208392`: PASS.
- Release manifest integrity run `35740208157`, job `106788552728`: PASS. Its two Release builds were classified byte-identical, then the hosted package bytes and manifest contracts passed.

These hosted results prove compilation of the real Win32 smoke and green deterministic non-runtime regressions. Hosted deterministic CTest intentionally excludes all `RuntimeSmoke` tests, so this is not native editor GUI execution evidence.

## Independent review and evidence reconciliation

Codex completed an independent review of exact code head `625e744e8c20bdb5c768e643e6cbaf4113808212` at `2026-09-22T14:31:35Z`. No new runtime-code finding was produced. The review produced one P2 evidence finding: this QA receipt still named the superseded `40d2e3f...` / `49391f9...` implementation. That stale implementation receipt was corrected in evidence commit `6d8f47ed9cb369337c1c585ea83821304e01316c`, which produced QA blob `5c40ad84fbc8c27e3e78720551ff33bd6ffe03ef` and task blob `359fadc388ca9a52a60e42535d693e02b9933c53`.

Codex then reviewed reconciliation head `bc6ad8f620d573f90cb24317d20e0a0ec3a5c433` and completed at `2026-09-22T14:38:51Z`. It found one P2 evidence-consistency issue, not a runtime-code defect: the repaired QA/task receipts still said the already-completed `625e744...` review was running. This QA update and companion task update correct that status and identify the remaining gates accurately.

A final independent recheck of the post-fix evidence head remains pending. Independent code/evidence review does not substitute for native runtime acceptance.

## Native evidence and handoff

`native_evidence` remains empty. The coordinator did not access or claim a registered Windows interactive desktop.

Run exact code candidate `625e744e8c20bdb5c768e643e6cbaf4113808212` on one owned interactive Windows desktop:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain source SHA, machine/Windows identity, MSVC and CMake versions, GPU/driver identity, exact commands, full stdout/stderr, exit codes, UTC timestamps and screenshots at normal and narrow/short sizes. Human-visible acceptance must confirm exact `OUTLINER`, `INSPECTOR`, and `ASSETS / DEFAULT PRIMITIVES` labels; exact four asset rows; truthful E11 status text; Outliner visibly remains on Cube while Inspector shows Cube; viewport `Selected:` text reflects Cube; and panels do not bleed during resize.

## Result

Status: **same-count shell-static false passes are covered; exact code candidate hosted Debug/Release and deterministic non-runtime checks are green; the code review found no new runtime-code issue; evidence receipt findings have been repaired; a final receipt recheck plus native Debug/Release RuntimeSmoke execution remain pending**.

E11 remains a partial editor-shell candidate, not UE5/Unity parity. No native GUI, GPU/performance, clean-machine, stress/recovery, soak, or final independent runtime acceptance claim is made. Issue #7 remains separate and open, and R0 was not invoked.

Single next useful action: independently recheck the repaired evidence head for internal consistency; after that is clean, execute Debug and Release `EditorRuntimeSmoke` on the registered Windows desktop with the required receipts/screenshots.