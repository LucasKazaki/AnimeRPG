# R0 Windows central-runtime preflight, 2026-09-20

## Why this packet exists

The preceding E15 passes proved that the hosted Release `AstralGame.exe` imports
Microsoft Visual C++ v14 runtime DLLs and established `central_vc_redist` as the
reviewed deployment policy. They did not bind that policy back to the exact package
image on a target host or retain concrete evidence about the central runtime files
that are actually present there.

This packet adds that evidence-only preflight. It remains inside R0/E15 packaging
verification and does not invoke R0, download/install/repair/copy a redistributable,
launch the game, alter Engine/Game/CMake, change runtime linkage, publish a package,
merge a PR, or claim clean-machine compatibility. Baseline: draft PR #9 head
`5241384e5fe8f063abe1a68106d1d40cc7c3c0e2`, stacked on PR #6.

## Allowed paths

- `Scripts/probe_windows_runtime_environment.py`
- `Scripts/test_windows_runtime_environment.py`
- `.github/workflows/windows-ci.yml`
- `Tasks/R0-WINDOWS-RUNTIME-PREFLIGHT-2026-09-20.md`
- `Docs/QA/R0-WINDOWS-RUNTIME-PREFLIGHT-2026-09-20.md`

No installer execution, package launch, release-runner execution, or external output
outside the owned CI/temp evidence paths is authorized by this packet.

## Primary-source research

Accessed September 20, 2026:

1. Microsoft, **Microsoft C++ Build Tools, Redistributable, and runtime libraries FAQ**:
   https://learn.microsoft.com/en-us/lifecycle/faq/visual-c-faq
   Visual C++ v14 Redistributables are cumulative. Microsoft states that the installed
   Redistributable must be equal to or newer than the Build Tools version used by the
   application, and recommends the latest available supported Redistributable.
2. Microsoft, **Audit Visual C++ Runtime version usage**:
   https://learn.microsoft.com/en-us/cpp/windows/redist-version-auditing?view=msvc-170
   Microsoft documents auditing actual VC Runtime DLL usage and the system locations
   used by centrally installed runtimes. Astral uses this only as a target-evidence
   reference; the packet does not alter Windows auditing policy.
3. Epic Games, **Unreal Engine 5.8 Project Settings, Prerequisites**:
   https://dev.epicgames.com/documentation/en-us/unreal-engine/project-section-of-the-unreal-engine-project-settings
   UE exposes prerequisite-installer and app-local-prerequisite controls for packaged
   games. This is a comparison target only. No UE source or dependency is copied.

The existing Astral plan still does not record an exact minimum Redistributable package
version derived from its build environment. Therefore this preflight records runtime
DLL versions but explicitly keeps `runtime_version_compatibility_verified` false.
File presence is not a substitute for the required clean-machine launch.

## Acceptance contract

Add a dependency-free Windows preflight tool that consumes the exact dependency report,
prerequisite plan, and package image together. Before reading host runtime files it must:

- hash the package and require the dependency report to match it;
- require the prerequisite plan to point to the same image hash, PE machine, architecture,
  and VC import set;
- reject a Release already marked rejected or falsely marked clean-machine verified;
- accept only the reviewed `central_vc_redist` strategy when VC runtime imports exist;
- reject path-like or otherwise unsafe DLL names so evidence lookup cannot escape the
  expected central runtime directory.

For the current x64 package, the probe checks required runtime basenames under
`%SystemRoot%\System32`, retaining path, SHA-256, byte size, and fixed file-version
metadata. x86 maps to `SysWOW64`; ARM64 maps to `System32`. Missing files or unreadable
metadata fail the preflight when `--fail-on-missing` is requested.

The output must keep package-launch, clean-machine, runtime-version-compatibility, and
independent-acceptance claims false. Hosted Windows CI may use the probe to prove that
the script works against a real freshly built Astral image and a real Windows host,
but that host is not a clean supported end-user machine.

Portable unit tests cover exact evidence chaining, missing files, metadata failures,
image hash changes, stale/import-mismatched plans, path-like DLL names, unreviewed
strategies, false acceptance claims, and architecture-directory mapping.

## Stop conditions and next gate

Stop on stale-evidence acceptance, package-hash mismatch acceptance, path traversal,
missing-runtime success, false launch/clean-machine claims, or tracked-tree mutation.
Do not install a redistributable or switch linkage to make hosted CI green.

After this packet, the next E15 work remains local/independent: bind the build toolchain
to an approved minimum/latest supported Redistributable delivery, then exercise the
finished package on a supported clean Windows host and retain installer/runtime/package
receipts. Interactive RuntimeSmoke, independent review, and the long soak remain open.
