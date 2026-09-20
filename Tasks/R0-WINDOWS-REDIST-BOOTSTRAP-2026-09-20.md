# R0 Windows Redistributable bootstrap planning

Bounded E15 packaging packet, September 20, 2026. Base candidate:
`ffd295ac9c30eda6a9787c9affd039e9745c3c0b` on draft PR #9. This packet does not
invoke R0, install/download/copy a Redistributable, launch AstralGame, modify
Engine/Game/CMake, change linkage, publish, merge, or touch the local Company Runtime.

## Research basis

Primary sources read September 20, 2026:

- Microsoft, **Redistribute Visual C++ files**:
  https://learn.microsoft.com/en-us/cpp/windows/redistributing-visual-cpp-files?view=msvc-170
  Microsoft recommends the central Redistributable package, documents the v14
  registry record and Version/Major/Minor/Bld/Rbld values, says to avoid running
  an older package over a newer installed version, and documents `/install`,
  `/passive`, `/quiet`, `/norestart`, and `/log`.
- Microsoft, **Latest supported Visual C++ Redistributable downloads**:
  https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170
  The page gives current v14 architecture permalinks, requires matching target
  architecture, and says target Redistributable version must be no older than the
  build tools used for the application. Redistribution is license-controlled.
- Unreal Engine 5.8, **Project settings / Prerequisites**:
  https://dev.epicgames.com/documentation/en-us/unreal-engine/project-section-of-the-unreal-engine-project-settings
  UE exposes both packaged prerequisite-installer and app-local-prerequisite controls.

## Allowed paths and acceptance

- `Scripts/plan_windows_redist_bootstrap.py`
- `Scripts/test_windows_redist_bootstrap.py`
- `.github/workflows/windows-ci.yml`
- this task and `Docs/QA/R0-WINDOWS-REDIST-BOOTSTRAP-2026-09-20.md`

Bind the exact prerequisite plan and runtime-compatibility report to the central
v14 registry state for the package architecture. A same-or-newer registered v14
version selects `skip_install`; missing/older registration selects
`install_latest_supported` and an architecture-correct Microsoft permalink plus
non-executed command template. Reject stale package hashes, inconsistent claims,
registry component mismatches, unsupported architecture, or pre-claimed install,
launch, clean-machine, or independent acceptance. Never execute the installer.

Portable fixture tests must cover newer/equal/older/missing registrations,
architecture mapping, stale evidence, false claims, component inconsistency and
CLI output. Hosted Windows CI may probe its own registry after building the exact
Release package, but the hosted image is not a clean end-user machine. The local
executor must separately preserve package hash, OS, registry/runtime state,
installer provenance if installation is required, exact commands, exit codes,
timestamps, launch/runtime evidence and independent review.

Stop after the planner and hosted evidence are recorded. Do not claim clean-machine
compatibility or permission to invoke R0 from this packet.
