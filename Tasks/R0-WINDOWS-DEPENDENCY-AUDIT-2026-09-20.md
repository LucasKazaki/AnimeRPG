# R0 Windows dependency inventory, 2026-09-20

## Why this packet exists

PR #9 repaired the historical R0 runner's path, provenance, timeout, and one-shot
behavior, but clean-machine MSVC runtime/dependency acceptance is still explicitly
pending. The current package recipe copies `AstralGame.exe` without recording its
PE import table. That leaves an important packaging question invisible until a
clean-machine launch is attempted.

This packet advances the E15 packaging/platform evidence gap without invoking R0,
changing Engine/Game/CMake, selecting a new graphics API, installing redistributables,
or claiming clean-machine compatibility. Baseline is PR #9 head
`190e7e627e88596dd47aa28ea9e7a46f966ac601`, stacked on PR #6. PR #8 remains a
separate asset-loader candidate.

## Allowed paths

- `Scripts/inspect_pe_dependencies.py`
- `Scripts/test_pe_dependencies.py`
- `.github/workflows/windows-ci.yml`
- `Tasks/R0-WINDOWS-DEPENDENCY-AUDIT-2026-09-20.md`
- `Docs/QA/R0-WINDOWS-DEPENDENCY-AUDIT-2026-09-20.md`

No release runner execution, package publication, dependency download/copy, build
runtime policy change, merge, or local Company Runtime modification is authorized.

## Primary-source research

Accessed September 20, 2026:

1. Microsoft, **Latest Supported Visual C++ Redistributable Downloads**:
   https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170
   Microsoft states that applications using the MSVC runtime need an appropriate
   Visual C++ Redistributable on the target system and that the installed version
   must be at least as new as the build tools used to build the application.
2. Microsoft, **Choose a deployment method**:
   https://learn.microsoft.com/en-us/cpp/windows/choosing-a-deployment-method?view=msvc-170
   Microsoft documents redistributable/install and application-local deployment,
   warns that incorrect runtime deployment can cause load failures, and recommends
   dynamic linking for serviceability rather than switching to static linking just
   to avoid deployment work.
3. Epic Games, **Project Settings > Prerequisites**, Unreal Engine 5.8:
   https://dev.epicgames.com/documentation/en-us/unreal-engine/project-section-of-the-unreal-engine-project-settings
   UE exposes explicit options to include a prerequisites installer or app-local
   prerequisites in packaged games. This is a packaging capability reference, not
   source to copy or proof that Astral should use the same installer design.
4. Unity, **System requirements for Unity 6.0**:
   https://docs.unity3d.com/6000.0/Documentation/Manual/system-requirements.html
   Unity documents supported Windows player OS/CPU/graphics requirements. The page
   does not establish Astral's runtime dependency deployment policy, so this packet
   does not infer one from it.

## Acceptance contract

Add a dependency-free Python PE inspector that reads bounded PE32/PE32+ normal and
delay-load import tables, reports the executable SHA-256, machine type, imported DLL
names, and Microsoft runtime categories, and rejects malformed/out-of-bounds input.
It must flag known Debug CRT imports and provide a CI mode that fails a Release image
when such a dependency is found.

The report must always keep `clean_machine_compatibility_verified` false. Import
inspection cannot prove target-machine availability, does not discover arbitrary
`LoadLibrary` plugin dependencies, and cannot replace launching the finished package
on a supported clean Windows machine. A release-runtime import such as `VCRUNTIME140`
or `MSVCP140` should request deployment review, not fail automatically and not cause
the tool to copy Microsoft's redistributable files.

Portable tests may use synthetic bounded PE fixtures to exercise parser/error cases.
Hosted Windows CI must additionally parse a real Windows Python executable and the
actual freshly built Release `AstralGame.exe`, with the latter using
`--fail-on-debug-runtime`. The normal Debug/Release deterministic engine gates remain
unchanged. Hosted dependency inventory is evidence only, not clean-machine proof.

## Stop conditions and rollback

Stop on parser regression, malformed-input acceptance, hosted Release dependency
inspection failure, unexpected tracked-tree changes, or revision ambiguity. Do not
weaken Debug-runtime detection to make CI green. Rollback is one commit on the PR #9
branch. Clean-machine/package launch and independent review remain separate gates.
