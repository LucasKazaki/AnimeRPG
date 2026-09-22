# E11 Editor Runtime Smoke task, 2026-09-22

## Scope and ownership

Bounded verification-only packet for the already-integrated Win32 `AstralEditor`. This packet may harden the native Windows smoke and its evidence, but it must not add scene mutation/serialization, transform gizmos, Play-in-Editor, asset import, rendering/API changes, dependencies, game content, scheduler operations, deployment, release, or R0 execution.

Owned branch: `engine/2026-09-22-editor-runtime-smoke`.
Baseline admitted from `main`: `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1`.
Latest independently moving `main` observed in this pass: `755faabfb5f04d5bc07324d91cbceb261cdc1060`; do not rebase, merge, or absorb unrelated game-worker work in this packet.
Current code candidate: `46b9a018af5dd6a1ae01d414497ee1e6ef294dfb`.
Current smoke blob: `98ab2d576da85b5f298bd83bb5135c79fc21b2b0`.
Latest exact hosted evidence head: `0bb638949adc4a79c1053eb2102741139497ea56`.
Integrated editor source fixture blob: `Tools/AstralEditorMain.cpp` `5142e632a79c89d0d0ce3efe87e752456f802185`; that production editor file is not owned by this packet and was not modified.

Allowed paths only:

- `CMakeLists.txt`
- `Tests/EditorRuntimeSmoke.cpp`
- `Tasks/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`
- `Docs/QA/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`
- `Docs/Research/ENGINE-CAPABILITIES.json`

One active writer only. No force push, destructive cleanup, merge, release, deployment, dependency import, repository-permission change, architecture/graphics-API change, or Company Runtime mutation.

## Selected review finding and repair

The latest independent Codex review of exact evidence head `0bb638949adc4a79c1053eb2102741139497ea56` completed at `2026-09-22T16:39:32Z`. It reported one P2 evidence-traceability finding and no new runtime-smoke code finding: the durable capability/task/QA records still named the earlier `c3f1ea2...` evidence state and `357541...` workflow receipts even though exact head `0bb6389...` had already completed its own Windows, profiling, and release-manifest checks.

This pass repairs that review finding in evidence only. The runtime-smoke code remains candidate `46b9a018...`, smoke blob `98ab2d5...`; production editor code remains unchanged. The repaired records now distinguish:

- exact code candidate `46b9a018af5dd6a1ae01d414497ee1e6ef294dfb`;
- exact reviewed/hosted evidence head `0bb638949adc4a79c1053eb2102741139497ea56`;
- Windows Server 2022 run `35754595962`, job `106837143068`, successful on that exact head;
- profiling capture portability run `35754596189`, successful on that exact head;
- release manifest integrity run `35754596081`, successful on that exact head;
- fresh Codex review of `0bb6389...`, which raised only this stale-receipt finding;
- native `EditorRuntimeSmoke` and final independent runtime acceptance, which remain pending.

No assertion, timeout, test registration, dependency, graphics API, or execution authority was weakened or expanded.

## Acceptance contract

The smoke may report PASS only when all of the following hold:

1. The same visible top-level HWND owned by the launched `AstralEditor` process is the only visible process-owned top-level window for 20 consecutive 50 ms observations at startup and again after interaction.
2. Top-level class and title are exactly `AstralEditorWindow` and `Astral Editor 0.1`.
3. Exactly 12 direct process-owned child controls exist with five Buttons, two ListBoxes and five Statics.
4. The initial 12 child HWND plus class identities are retained. Every later selection, resize, containment, shell-state and final validation must observe the same set; a replacement control cannot satisfy the contract merely by matching text/class/ID/state.
5. All five pending toolbar buttons have exact expected captions, are visible, and remain disabled at initial state, after each resize, and at final validation.
6. All four non-Inspector shell Statics have exact integrated text and are visible; Inspector is a visible process-owned direct `Static` with the exact expected Scene Root or Cube fixture.
7. Outliner is the visible process-owned direct `ListBox` at control ID 1001; Assets is the visible process-owned direct `ListBox` at control ID 1002.
8. Every bounded list/text read revalidates the target immediately before its send; two-message text reads revalidate again before the second message so a stale/recycled HWND fails closed.
9. Outliner count is five, row 0 is exactly `Scene Root`, row 3 is exactly `Cube`, and selection is the expected row for the current state.
10. Assets count is four and rows are exactly, in order, `Primitive/Cube`, `Primitive/Plane`, `Camera`, `DirectionalLight`.
11. Cube selection plus bounded `LBN_SELCHANGE` must result in Outliner selection 3, selected row `Cube`, and exact Cube Inspector state without replacing any original shell child HWND.
12. Before and during each side-effecting resize, the saved editor HWND remains owned by the launched PID. The 800x600 and 420x260 resizes use `SWP_ASYNCWINDOWPOS`, complete within the bounded poll, keep every original direct child inside the client rectangle, preserve all original child handles, and preserve the entire required shell state.
13. Before shutdown, editor HWND ownership is revalidated; exactly one `WM_CLOSE` is posted, exit is bounded and must be code 0, and failure cleanup may terminate only the retained process handle created by the smoke.

Cross-process synchronous messages remain bounded through `SendMessageTimeoutW` with the existing one-second timeout. `EditorRuntimeSmoke` remains registered through `astral_add_test`, `RUN_SERIAL`, and the existing 180-second CTest timeout. Do not weaken assertions, timeout, Release-assertion protection, or exclusive-desktop requirements to make a gate green.

## Primary-source evidence rechecked 2026-09-22

The selected gap in this pass is evidence traceability, so the primary source is the repository and GitHub Actions/review state rather than a new engine design reference. No proprietary source, artwork, assets or dependency was copied or imported.

- GitHub Actions, exact head `0bb638949adc4a79c1053eb2102741139497ea56`:
  - Windows Server 2022 workflow `35754595962`, job `106837143068`, status `completed`, conclusion `success`, head SHA `0bb6389...`, completed `2026-09-22T16:33:01Z`.
  - Profiling capture portability workflow `35754596189`, status `completed`, conclusion `success`.
  - Release manifest integrity workflow `35754596081`, status `completed`, conclusion `success`.
- GitHub Codex review submission for exact head `0bb638949adc4a79c1053eb2102741139497ea56`, submitted `2026-09-22T16:39:32Z`.
- Codex P2 review thread created `2026-09-22T16:39:32Z` on `Docs/Research/ENGINE-CAPABILITIES.json`: record exact final evidence-head verification in the durable capability/task/QA receipts while keeping native runtime acceptance pending.

The existing behavioral/API basis remains Epic UE 5.8 Unreal Editor Interface, Unity 6.0 Hierarchy, and Microsoft Win32 window-identity APIs as recorded in the QA receipt. This pass does not change those behavioral requirements.

## Verification evidence

Disposable portable contract fixture SHA-256 remains `619092d28540a53ee81e93efa29c93efc6571845de6d631a38800f4460c9ff4e` from the prior code-hardening pass. No runtime-smoke code changed in this evidence-only repair, so that source fixture was not rerun merely to reproduce unchanged evidence.

Exact hosted evidence head `0bb638949adc4a79c1053eb2102741139497ea56` passed:

- Windows Server 2022 run `35754595962`, job `106837143068`: PASS. The job is recorded against exact head SHA `0bb6389...`; successful steps include R0 parser-only/safety contracts, VS2022 x64 configure, MSVC Debug build plus deterministic tests, MSVC Release build plus deterministic tests, Release dependency/prerequisite/runtime-policy checks, static milestone verifiers and clean tracked-tree verification. The historical R0 runner itself was not executed.
- Profiling capture portability run `35754596189`: PASS.
- Release manifest integrity run `35754596081`: PASS.

Hosted deterministic CTest intentionally excludes every `RuntimeSmoke`, so these results prove compilation and deterministic non-runtime regression status, not native GUI execution.

## Independent review state

Fresh independent Codex review completed on exact evidence head `0bb638949adc4a79c1053eb2102741139497ea56` at `2026-09-22T16:39:32Z`. It raised one new finding, limited to stale evidence-head/workflow references in the durable records. No new runtime-smoke code finding was reported in that review.

This pass remediates that evidence finding in the task, QA receipt, capability map, PR body/checkpoint, and review-thread reply. Because these evidence files changed after the review, the evidence-only remediation should be rechecked independently before the packet is treated as fully reviewed. Same-author inspection is not independent acceptance. Native runtime acceptance remains separate and pending.

## Registered-local handoff

Run exact code candidate `46b9a018af5dd6a1ae01d414497ee1e6ef294dfb` on one owned interactive Windows desktop using external build output:

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

Single next useful action: independently recheck the evidence-only receipt repair, then run Debug and Release `EditorRuntimeSmoke` on the registered Windows desktop with retained receipts/screenshots.
