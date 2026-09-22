# E11 Editor Runtime Smoke task, 2026-09-22

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. This packet may harden the native Windows smoke and its evidence, but it must not add scene mutation/serialization, transform gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Current independently moving `main` re-read in this pass: `f68e0917b38a0b4780d94a409874bb94df818163`; do not rebase, merge, or absorb unrelated game-worker work in this packet.
Current code candidate: `625e744e8c20bdb5c768e643e6cbaf4113808212`.
Current smoke blob: `b0799149ed0794adbb32c0653414deb9bda5ecd4`.
Integrated editor source fixture blob re-read for expected strings: `Tools/AstralEditorMain.cpp` `5142e632a79c89d0d0ce3efe87e752456f802185`; that production editor file is not owned by this packet and was not modified.

Allowed paths only:

- `CMakeLists.txt`
- `Tests/EditorRuntimeSmoke.cpp`
- `Tasks/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`
- `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`
- `Docs/Research/ENGINE-CAPABILITIES.json`

One active writer only. No force push, destructive cleanup, merge, release, deployment, dependency import, repository-permission change, architecture/graphics-API change, or Company Runtime mutation.

## Selected false-pass and repair

The prior smoke required exactly five direct `Static` controls and exact Scene Root Inspector text, but did not identity-check the remaining four shell statics. A shell with a renamed panel header, duplicated label, or stale status line could still pass the count-based static-control contract.

Candidate `625e744e8c20bdb5c768e643e6cbaf4113808212` now requires all four non-Inspector shell statics to exist with exact integrated text and to be visible:

- `OUTLINER`
- `INSPECTOR`
- `ASSETS / DEFAULT PRIMITIVES`
- `E11.0 editor shell | Outliner selection works | viewport transform tools, undo/redo, save/reopen, Play and real asset import pending`

The exact visible Scene Root Inspector fixture remains required separately. Because the direct `Static` count remains exactly five, requiring these four identities plus the Inspector identity rejects same-count rename, duplicate, missing-label and stale-status cases without adding any new editor/process side effect.

## Acceptance contract

The smoke may report PASS only when all of the following hold:

1. The same visible top-level HWND owned by the launched `AstralEditor` process is the only visible process-owned top-level window for 20 consecutive 50 ms observations at startup and again after interaction.
2. Top-level class and title are exactly `AstralEditorWindow` and `Astral Editor 0.1`.
3. Exactly 12 direct child controls exist with five Buttons, two ListBoxes and five Statics.
4. All five pending toolbar buttons have exact expected captions, remain visible, and remain disabled.
5. All four non-Inspector shell Statics listed above exist with exact text and are visible; the Scene Root Inspector static also exists with exact full text and is visible.
6. Outliner count is five, row 0 is exactly `Scene Root`, row 3 is exactly `Cube`, and initial selection is row 0.
7. Assets count is four and rows are exactly, in order, `Primitive/Cube`, `Primitive/Plane`, `Camera`, `DirectionalLight`.
8. Before selection, notification and post-notification reads, the Outliner HWND remains owned by the launched PID, directly parented by the owned editor HWND, and addressable as control ID 1001. After the bounded notification, current selection must still be index 3, selected row text must be `Cube`, and the Inspector must exactly match the Cube fixture.
9. Before each side-effecting resize, the saved editor HWND is revalidated against the launched PID. The 800x600 and 420x260 resizes use `SWP_ASYNCWINDOWPOS`, must complete within the existing bounded poll, and every direct child must remain within the actual client rectangle.
10. Before shutdown, editor HWND ownership is revalidated; exactly one `WM_CLOSE` is posted, exit is bounded and must be code 0, and failure cleanup may terminate only the retained process handle created by the smoke.

Cross-process synchronous messages remain bounded through `SendMessageTimeoutW` with the existing one-second timeout. `EditorRuntimeSmoke` remains registered through `astral_add_test`, `RUN_SERIAL`, and the existing 180-second CTest timeout. Do not weaken assertions, timeout, Release-assertion protection, or exclusive-desktop requirements to make a gate green.

## Primary-source research rechecked 2026-09-22

Behavioral/API references only. No proprietary engine source, artwork, assets or dependency was copied or imported.

- Epic Games, Unreal Engine 5.8, Unreal Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
  - Applicability: Epic identifies separate, named editor surfaces including the Level Viewport, Outliner, Details panel and Content Drawer. Identity of the shell surfaces is part of the observable editor contract, not merely their count.
- Epic Games, Unreal Engine 5.8, Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
  - Applicability: the Outliner is an identifiable hierarchical editor panel used for selection.
- Unity Manual, Unity editor interface / Hierarchy and Inspector concepts: https://docs.unity3d.com/Manual/UsingTheEditor.html
  - Applicability: Unity likewise presents distinct named Hierarchy, Scene, Inspector and Project surfaces rather than interchangeable anonymous controls.
- Microsoft `WM_GETTEXT`: https://learn.microsoft.com/windows/win32/winmsg/wm-gettext
  - Applicability: text static controls return their text through the existing bounded `WindowText` helper; list-box rows continue to use the dedicated LB_GETTEXT path.

Access date for this packet: 2026-09-22.

## Verification evidence

Portable source-logic fixture SHA-256: `6dbe2d7e13203e0b796cc5d8844d2b322959a1912d327123713bccbf555cf304`.

Executed in the coordinator sandbox:

```bash
g++ -std=c++17 -Wall -Wextra -Werror /tmp/e11_static_identity_fixture.cpp -o /tmp/e11_static_identity_fixture
/tmp/e11_static_identity_fixture
clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /tmp/e11_static_identity_fixture.cpp -o /tmp/e11_static_identity_fixture_san
ASAN_OPTIONS=detect_leaks=1 /tmp/e11_static_identity_fixture_san
sha256sum /tmp/e11_static_identity_fixture.cpp
```

Both executions exited 0 and printed `static identity contract fixture: PASS`. The fixture proves only the source-level identity rule: the previous model accepts five Statics with a wrong panel label, stale status or duplicate label as long as the Inspector exists, while the repaired rule rejects those cases. It is not Win32/native editor execution.

Hosted exact-code evidence for `625e744e8c20bdb5c768e643e6cbaf4113808212`:

- Windows Server 2022 run `35740208183`, job `106787679557`: PASS. This includes R0 parser-only and safety-contract checks, VS2022 x64 configure, Debug build/tests, Release build/tests, dependency/prerequisite checks, static verifiers and clean-tree verification. The historical R0 runner itself was not executed.
- Profiling portability run `35740208392`: PASS.
- Release manifest integrity run `35740208157`, job `106788552728`: PASS, including two Release builds classified byte-identical and hosted package-manifest checks.

Hosted deterministic CTest intentionally excludes every `RuntimeSmoke`, so none of these hosted results is native GUI execution evidence.

A fresh independent Codex code review was requested for exact code head `625e744e8c20bdb5c768e643e6cbaf4113808212`. Until it completes, the older independent review of `c8e0bd2e3952f7b77a3f9701cf69af8544f39620` is stale for the latest code and must not be counted as current acceptance.

## Registered-local handoff

Run the exact reviewed code candidate on one owned interactive Windows desktop using external build output:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine and Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, full stdout/stderr, exit codes, UTC timestamps and screenshots at normal and narrow/short sizes. Human-visible acceptance must confirm the exact four asset entries, exact OUTLINER/INSPECTOR/ASSETS labels, truthful E11 status text, Outliner visibly remains on Cube when Inspector shows Cube, viewport `Selected:` text reflects Cube, and panels do not bleed during resize.

## Stop, rollback and next action

Stop on unexpected edits outside the allowed paths, stale ownership, a failing introduced regression, a changed architecture/dependency requirement, or a native gate requiring the registered Windows desktop. Rollback is branch-local revert of this packet; never rewrite shared history.

E11 remains partial. `native_evidence` remains empty and `independent_acceptance` remains false. Do not start dependent scene-document, transform-gizmo, undo/redo or save/reopen implementation based on hosted compilation alone. Issue #7 remains separate and open; do not invoke the historical R0 runner.

Single next useful action: complete the fresh independent review of exact code head `625e744...`, then run Debug and Release `EditorRuntimeSmoke` on the registered Windows desktop with retained receipts/screenshots.