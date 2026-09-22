# R0 Windows runtime/toolset compatibility audit, 2026-09-20

## Why this packet exists

The preceding E15 packets bound a fresh Release image to its PE imports, selected the
reviewed `central_vc_redist` deployment strategy, and inventoried the required central
Visual C++ runtime files on a Windows host. They intentionally left runtime-version
compatibility unverified because the evidence did not record the build toolset version.

This packet closes only that evidence gap. It does not download, install, repair, copy,
or bundle a Redistributable; launch the package; invoke R0; modify Engine/Game/CMake;
change linkage; publish or merge anything; or claim clean-machine compatibility.
Baseline: draft PR #9 head `bbbd22e08e88d0119f3d48f2904e120b6995d396`, stacked on PR #6.

## Allowed paths

- `Scripts/audit_windows_runtime_compatibility.py`
- `Scripts/test_windows_runtime_compatibility.py`
- `.github/workflows/windows-ci.yml`
- `Tasks/R0-WINDOWS-RUNTIME-COMPATIBILITY-2026-09-20.md`
- `Docs/QA/R0-WINDOWS-RUNTIME-COMPATIBILITY-2026-09-20.md`

No installer execution, package launch, release-runner execution, dependency change, or
external output outside owned CI/temp evidence paths is authorized by this packet.

## Primary-source research

Accessed September 20, 2026:

1. Microsoft, **Microsoft C++ Build Tools, Redistributable, and runtime libraries FAQ**:
   https://learn.microsoft.com/en-us/lifecycle/faq/visual-c-faq
   Microsoft states that the cumulative v14 Redistributable is compatible when its
   version is equal to or higher than the MSVC Build Tools version used to build the
   application. The same page identifies MSVC/Redistributable 14.44 as the supported
   Visual Studio 2022 v17.14 final line through January 13, 2032.
2. Microsoft, **Microsoft C++ compiler versioning**:
   https://learn.microsoft.com/en-us/cpp/overview/compiler-versions?view=msvc-170
   The version table maps Visual Studio 2022 17.14 and `_MSC_VER` 1944 to MSVC toolset
   14.44. This prevents confusing CMake's compiler executable version (`19.44.x`) with
   the redistributable/build-tools version family (`14.44.x`).
3. Microsoft, **Audit Visual C++ Runtime version usage**:
   https://learn.microsoft.com/en-us/cpp/windows/redist-version-auditing?view=msvc-170
   This remains the source basis for retaining concrete central runtime DLL evidence.
4. Epic Games, **Unreal Engine 5.8 Project Settings, Prerequisites**:
   https://dev.epicgames.com/documentation/en-us/unreal-engine/project-section-of-the-unreal-engine-project-settings
   UE exposes prerequisite packaging controls. It is a capability reference only; no
   Epic code or dependency is copied into Astral.

## Acceptance contract

Add a dependency-free compatibility auditor that consumes the previous runtime-preflight
JSON and an explicit VC Build Tools version from the same build environment. It must:

- reject unsupported/malformed preflight evidence, unreviewed prerequisite strategy,
  missing/unversioned files, duplicate or mismatched runtime entries, or prior false
  clean-machine/runtime-compatibility claims;
- parse numeric three- or four-component tool/runtime versions, never compare them as
  strings;
- require every runtime DLL actually imported by the package to have a file version
  greater than or equal to the recorded VC Build Tools version;
- fail with a distinct nonzero result when `--fail-on-incompatible` is requested and a
  runtime file is older than the toolset;
- bind the output to the package SHA and exact recorded toolset version;
- keep Redistributable installer provenance/support, package launch, clean-machine
  compatibility, and independent acceptance false.

Hosted Windows CI must discover the default VS 2022 C++ toolset actually selected by the
unversioned `Visual Studio 17 2022` generator from
`VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt`, after restricting `vswhere`
to the 17.x installation family. It then runs the existing real Release dependency and
runtime preflight before applying this version-floor audit. This is hosted evidence for
the checker and current runner image, not clean-machine acceptance.

Portable tests cover equal/newer and older runtimes, numeric major/minor ordering,
malformed versions, mismatched/false preflight claims, duplicate entries, and CLI exit
behavior.

## Stop conditions and next gate

Stop on acceptance of an older runtime, lexical version comparison, stale/mismatched
preflight evidence, inability to identify the VS 2022 toolset, false installer/launch
claims, or tracked-tree mutation. Do not alter linkage or install a Redistributable to
make hosted CI green.

After this packet, the remaining E15 gate is delivery and execution evidence: preserve
an approved supported central Redistributable bootstrap/installer route and launch the
finished package on a supported clean Windows host. Interactive RuntimeSmoke,
independent review, and the required long soak remain separate gates. Issue #7 remains
open and R0 remains uninvoked until those gates are accepted.
