# E11 Editor Runtime Smoke evidence, 2026-09-22

## Checkpoint

Bounded verification-only work on `engine/2026-09-22-editor-runtime-smoke`. This packet verifies the already-integrated Win32 `AstralEditor`; it does not authorize scene mutation/serialization, gizmos, Play-in-Editor, asset import, graphics/API changes, dependencies, game content, scheduler operations, deployment, release, merge, or R0 execution.

Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed: `b3a2b1bf8f2b0c356d5b352006c48cb86532426b`.
Current code candidate: `006fabd386cd47937b5ce6f7eca627d6960d1c1f`.
`Tests/EditorRuntimeSmoke.cpp` blob: `ab0957df75fee0129797f7fff6a7fab6c871d30c`.
Integrated editor source fixture: `Tools/AstralEditorMain.cpp` blob `5142e632a79c89d0d0ce3efe87e752456f802185`; production editor source was not modified.

## Independent review findings repaired

Independent Codex review of exact evidence tree `e1b3b5a64c21fb47aba5980b66493b4795010580` completed on 2026-09-22 and raised two current P2 findings.

1. **Require the Outliner notification style.** The previous smoke programmatically set selection and manually sent `LBN_SELCHANGE`, so it could pass if the Outliner lost `LBS_NOTIFY`, even though human mouse/keyboard selection would no longer emit that notification.
2. **Bind shell texts to their intended controls.** The previous smoke searched all visible `Static` controls for required strings, so swapping captions/roles among Static HWNDs could preserve the text set and still pass.

Candidate `006fabd...` repairs both without touching production editor code:

- `ValidatedControlHasStyle` performs live PID/parent/class/control-ID/visibility/enabled validation around `GetWindowLongPtrW(..., GWL_STYLE)` and requires `LBS_NOTIFY` for the Outliner.
- Full shell-state validation requires `LBS_NOTIFY`, and the selection path checks the style again before `LB_SETCURSEL` and before bounded `LBN_SELCHANGE` dispatch.
- Initial shell capture binds five distinct semantic Static HWNDs: Outliner label, Inspector label, Inspector body, Assets label, and status.
- Every later validation reads expected text from those exact original handles rather than searching for any matching Static.
- Existing 12-child handle/class continuity, enabled interaction surfaces, disabled pending toolbar buttons, exact rows, exact Inspector fixtures, bounded reads/messages, resize containment/full-state checks, and process-owned shutdown remain intact.

The two review threads were replied to with the exact repair commit/blob and portable fixture hash. They are not counted as independently accepted until a fresh review is completed on the final reconciled evidence tree.

## Primary-source research, accessed 2026-09-22

- Microsoft Learn, `LBN_SELCHANGE`: https://learn.microsoft.com/en-us/windows/win32/controls/lbn-selchange
- Microsoft Learn, List Box Styles / `LBS_NOTIFY`: https://learn.microsoft.com/en-us/windows/win32/controls/list-box-styles
- Microsoft Learn, `GetWindowLongW` / `GWL_STYLE`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowlongw
- Microsoft Learn, `IsWindowEnabled`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindowenabled
- Epic Games, UE 5.8 Outliner: https://dev.epicgames.com/documentation/unreal-engine/outliner-in-unreal-engine
- Epic Games, UE 5.8 Selecting Actors: https://dev.epicgames.com/documentation/unreal-engine/selecting-actors-in-unreal-engine
- Epic Games, UE 5.8 Unreal Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
- Unity Technologies, Unity 6.0 Hierarchy: https://docs.unity3d.com/6000.0/Documentation/Manual/hierarchy-window.html

Microsoft documents that `LBN_SELCHANGE` from user input is sent only by a list box with `LBS_NOTIFY`, and that `GWL_STYLE` retrieves window styles. Epic documents interactive Outliner selection plus viewport/Details synchronization. Unity documents the Hierarchy as the scene-object management surface. These sources define behavior/API expectations only; no proprietary source was copied and no dependency was imported.

## Portable mutation fixture

Disposable coordinator-sandbox fixture: `/mnt/data/e11_semantic_notify_fixture.cpp`.
SHA-256: `469803e6556d87fc3f5139e0145cd7133aed1d9b9eda5e3b0f9e224464372bc9`.

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror /mnt/data/e11_semantic_notify_fixture.cpp -o /mnt/data/e11_semantic_notify_fixture
/mnt/data/e11_semantic_notify_fixture
clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer /mnt/data/e11_semantic_notify_fixture.cpp -o /mnt/data/e11_semantic_notify_fixture_san
ASAN_OPTIONS=detect_leaks=1 /mnt/data/e11_semantic_notify_fixture_san
sha256sum /mnt/data/e11_semantic_notify_fixture.cpp
```

GCC C++17 warning-clean compile/execution: PASS. Clang C++17 ASan+UBSan warning-clean compile/execution: PASS with no sanitizer finding. The fixture demonstrates that the former predicate accepts loss of the notification contract and semantic Static swaps, while the hardened predicate rejects both. This is source-logic evidence only, not native Win32 execution.

## Exact-candidate hosted verification

Exact code candidate `006fabd386cd47937b5ce6f7eca627d6960d1c1f` completed all available hosted workflows successfully:

- Windows build and deterministic tests `35787602365`, job `106948082419`: `completed/success`, exact head `006fabd...`, completed 2026-09-22T21:38:00Z. Passed repository/R0 safety contracts, VS2022 x64 configuration, MSVC Debug build/tests, MSVC Release build/tests, Release dependency/prerequisite/runtime-policy checks, static milestone verifiers, and clean-tree verification.
- profiling capture portability `35787602447`: `completed/success`.
- release manifest integrity `35787602218`: `completed/success`.

Hosted deterministic CTest intentionally excludes every `RuntimeSmoke`, so these are compile and deterministic non-runtime regression results only. The historical R0 runner itself was not executed.

## Native and independent acceptance state

`native_evidence` remains empty. This coordinator did not access or claim a registered Windows interactive desktop. Fresh independent source review is also pending until the final evidence-only reconciliation is complete; the review of `e1b3b5a...` is the review that produced the two findings repaired here.

Run the final candidate on one owned interactive Windows desktop using external build output:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

Retain exact source SHA, machine/Windows identity, MSVC/CMake versions, GPU/driver identity, exact commands, complete stdout/stderr, exit codes, UTC timestamps, and normal plus narrow/short screenshots. Human-visible acceptance must confirm exact Outliner/assets rows, actual mouse/keyboard Outliner selection causing Inspector/viewport synchronization, the required notification behavior, correct semantic label/body placement, truthful disabled pending tools, panel containment at both sizes, and original control-handle continuity.

## Result

Status: **two independent-review false-pass findings repaired; portable semantic/notification mutation fixture passed GCC and Clang ASan+UBSan; exact code candidate passed hosted Windows Debug/Release deterministic checks, profiling, and release-manifest workflows; fresh final-head independent review and native Debug/Release `EditorRuntimeSmoke` remain pending**.

E11 remains a partial editor-shell candidate, not UE5/Unity parity. No native GUI, GPU/performance, clean-machine, stress/recovery, soak, or final independent runtime acceptance claim is made. Issue #7 remains open and the historical R0 runner was not invoked.

Single next useful action: finish final evidence reconciliation and exact-head independent review, then execute Debug and Release `EditorRuntimeSmoke` on the registered Windows desktop with required receipts/screenshots.
