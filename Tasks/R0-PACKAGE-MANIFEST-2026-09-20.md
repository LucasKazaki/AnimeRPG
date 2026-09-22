# R0 package manifest integrity and provenance, 2026-09-20

## Why this packet exists

Issue #7 still requires manifest accuracy and exact package provenance before the historical R0 path can be trusted. PR #9 has already bounded paths/processes and added Windows runtime prerequisite evidence, but `MANIFEST-M10-RC.json` has no independent verifier. A hash list that is never checked cannot detect later byte changes, missing files, unmanifested files, unsafe relative names, or a manifest bound to the wrong revision/executable.

This packet advances E15 packaging verification only. It does not invoke R0, merge or publish a package, install prerequisites, run interactive GUI tests, modify Engine/Game/CMake, change architecture, or claim clean-machine compatibility.

Baseline: PR #9 head `8ed1cb6996b794a797ddbe9bbce52c6f18d60c03`, stacked on PR #6.

## Allowed paths

- `Scripts/release_manifest.py`
- `Scripts/test_release_manifest.py`
- `.github/workflows/release-manifest-validation.yml`
- `Tasks/R0-PACKAGE-MANIFEST-2026-09-20.md`
- `Docs/QA/R0-PACKAGE-MANIFEST-2026-09-20.md`

## Primary-source basis

Accessed September 20, 2026:

1. Epic Games, **Packaging Unreal Engine Projects**, UE 5.8: https://dev.epicgames.com/documentation/unreal-engine/packaging-your-project . Epic separates build, cook, stage, package, deploy, and run, and describes staging as a directory outside the development tree. Astral should likewise treat packaged bytes as a distinct artifact that can be verified before launch.
2. Epic Games, **Build Operations: Cook, Package, Deploy, and Run**, UE 5.8: https://dev.epicgames.com/documentation/unreal-engine/build-operations-cooking-packaging-deploying-and-running-projects-in-unreal-engine . Packaging and running are separate operations, so manifest verification must not claim launch success.
3. Unity 6.0, **BuildReport**: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/Build.Reporting.BuildReport.html . Unity exposes generated files, build steps, packed assets, and summary information from builds. This is a capability reference for evidence-rich packaging, not source to copy.
4. Unity 6.0, **DetailedBuildReport**: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/BuildOptions.DetailedBuildReport.html . Detailed build reporting can include additional build-time/content information, reinforcing that build artifacts and their provenance should be inspectable.
5. CPython 3.13 `pathlib`: https://docs.python.org/3.13/library/pathlib.html . Path walking does not follow symbolic links by default and the documentation warns about link traversal/concurrent tree mutation. Astral's verifier rejects symlinked package entries rather than treating them as ordinary package bytes.

## Acceptance contract

Add a dependency-free manifest creator/verifier with bounded input/file counts. It must use an explicit package root, never trust the manifest's recorded absolute root, reject absolute/traversal/backslash/drive-qualified paths, Windows case-collisions, symlinked entries, malformed hashes/sizes, missing files, extra files, and changed bytes. It must bind an expected Git commit and expected `AstralGame.exe` SHA-256 when supplied.

New manifests use schema v2 with an aggregate digest over the ordered file-entry records. The verifier remains compatible with the historical R0 v1 shape so it can check a future authorized R0 output without rewriting that evidence first.

A successful report may claim only package-manifest integrity. `package_launch_verified`, `clean_machine_compatibility_verified`, and `independent_acceptance` must remain false.

Hosted Windows CI may build `AstralGame.exe`, stage a disposable package fixture under the runner temp directory, create its manifest, and verify exact bytes. This is hosted artifact-integrity evidence only. It is not an interactive package launch, clean target, code-signing check, runtime-prerequisite install, or local acceptance result.

## Stop and rollback

Stop on malformed-input acceptance, path escape, symlink acceptance, hash/size mismatch acceptance, unexpected unmanifested files, wrong revision/executable binding, CI build failure, or tracked-tree mutation. Do not weaken the manifest check to make CI green. Rollback is the manifest-integrity commit(s) on PR #9. Native package launch, clean-machine prerequisite/install evidence, 24-hour soak, and independent review remain separate gates.
