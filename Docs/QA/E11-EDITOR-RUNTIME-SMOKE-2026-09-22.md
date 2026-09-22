# E11 Editor Runtime Smoke evidence, 2026-09-22

## Checkpoint

This record covers the bounded `EditorRuntimeSmoke` packet admitted from `main` at `e2c0cbe3c7bbdea646888bf31f25cfeb394693e1` on branch `engine/2026-09-22-editor-runtime-smoke`.

The packet adds a native Windows smoke executable for the already-integrated `AstralEditor`; it does not add editor authoring features. The smoke launches the exact built editor process and checks the existing E11 shell contract: top-level identity, the 12 required direct child controls, disabled pending tools, initial Outliner/assets/Inspector state, Outliner-to-Inspector selection synchronization, containment after normal and narrow/short resizes, and bounded clean close. Failure cleanup is limited to the process launched by the smoke. Cross-process synchronous messages use `SendMessageTimeoutW` with a one-second deadline.

No scene mutation or serialization, transform gizmo, Play-in-Editor, asset import, renderer/API change, game-content work, dependency, scheduler operation, deployment, release, or R0 invocation is part of this packet.

## Research basis

Primary sources read or rechecked on 2026-09-22:

- Epic Games, Unreal Engine 5.8, Unreal Editor Interface: https://dev.epicgames.com/documentation/unreal-engine/unreal-editor-interface
- Unity Manual, current editor/search provider documentation: https://docs.unity3d.com/jp/current/Manual/search-use-provider.html
- Microsoft Win32 `EnumChildWindows`: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-enumchildwindows
- Microsoft Win32 `GetClassNameW`: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-getclassnamew
- Microsoft Win32 `IsWindowEnabled`: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-iswindowenabled

These are behavioral/API references only. No Epic or Unity source, UI artwork, assets, or external dependency was copied or imported.

## Exact published implementation

Hosted candidate with complete code/task/capability state: `210da4bb58b200e6acc6b34173ae3d6c5f9ad80d`.

Relevant published blobs at that candidate:

- `CMakeLists.txt`: `945e0f5e28a0ecd91338541984cdc240bf986f82`
- `Tests/EditorRuntimeSmoke.cpp`: `6fba3f7d78aa3fdfb69b61359774285f5bbeb8c4`
- `Tasks/E11-EDITOR-RUNTIME-SMOKE-2026-09-22.md`: `0886d7d4f35fe162b70bac466876ba65c0bfd8bc`
- `Docs/Research/ENGINE-CAPABILITIES.json`: `f81aad73667aec5d7398ef0752e0af3626030bdf`

`CMakeLists.txt` registers the target through `astral_add_test`. Because its name ends in `RuntimeSmoke`, the existing project helper applies Release assertion protection, `RUN_SERIAL`, and the 180-second CTest timeout. Hosted deterministic test execution still excludes RuntimeSmoke targets.

## Hosted Windows evidence

GitHub Actions Windows Server 2022 run `35690534783`, job `106626511914`, executed against exact head `210da4bb58b200e6acc6b34173ae3d6c5f9ad80d` and completed successfully at 2026-09-22T05:26:26Z.

Passed steps include:

- checkout and external build-root setup;
- R0 parser-only step and runner-safety contracts;
- Windows dependency/prerequisite/runtime-policy verifier suites;
- Release assertion and CTest safety contracts;
- Visual Studio 2022 x64 configure;
- MSVC Debug build;
- deterministic Debug tests excluding RuntimeSmoke;
- MSVC Release build;
- hosted release dependency/prerequisite checks;
- deterministic Release tests excluding RuntimeSmoke;
- static milestone verifiers;
- clean tracked-tree verification.

The successful Debug and Release builds establish that the real Win32 `EditorRuntimeSmoke` source and its `AstralEditor` dependency compile under the project's hosted MSVC configuration. They do **not** establish that `EditorRuntimeSmoke` itself ran, because hosted deterministic CTest intentionally excludes all RuntimeSmoke targets.

Two additional final-candidate workflows also completed successfully:

- Profiling capture portability run `35690534830`.
- Release manifest integrity run `35690534790`.

An earlier Windows run for superseded code-only head `0dc4d44a2ab26d0fd9c7d00012a277453c96e4a1` reached successful test-safety, VS2022 configure, Debug build and deterministic Debug tests before GitHub cancelled it after later branch commits. It is not used as final-head acceptance evidence.

## Environment limitation and deferred evidence

The coordinator environment does not provide the Windows SDK/Win32 headers required to compile or run this native smoke locally. No synthetic Win32 runtime result is substituted for that missing environment. The hosted runner compiled the target but did not execute its interactive process/window checks.

Therefore E11 still has no registered-local native GUI evidence and no independent acceptance. The capability map remains partial and explicitly records native runtime as pending. This packet is not evidence of UE5/Unity editor parity.

Registered-local verification remains:

```powershell
cmake -S . -B ../AnimeRPG-e11-runtime-build -G "Visual Studio 17 2022" -A x64
cmake --build ../AnimeRPG-e11-runtime-build --config Debug --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Debug --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
cmake --build ../AnimeRPG-e11-runtime-build --config Release --parallel
ctest --test-dir ../AnimeRPG-e11-runtime-build -C Release --output-on-failure -R "^EditorRuntimeSmoke$" --no-tests=error
```

The registered executor must retain exact source SHA, machine and Windows identity, MSVC/CMake versions, GPU/driver identity, UTC timestamps, full commands, stdout/stderr, exit codes, and screenshots at the normal and narrow/short sizes. A human-visible pass should also confirm that the viewport and neighboring panes render without bleed. A separate reviewer must inspect the exact candidate; review by the implementation author is not independent.

## Result and next action

Status: **hosted Debug/Release compile and deterministic regression gate passed; native editor smoke execution pending; independent review pending**.

The single next useful action is to run the exact `EditorRuntimeSmoke` in Debug and Release on the registered owned Windows desktop, preserve the required receipts/screenshots, then obtain independent review. Do not start dependent scene-document, transform-gizmo, save/reopen, or undo/redo work on the strength of hosted compilation alone. Issue #7 remains a separate open local/native/package acceptance gate and the R0 runner was not executed by this packet.
