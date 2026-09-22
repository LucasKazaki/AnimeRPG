# Windows central Redistributable bootstrap evidence, 2026-09-20

Status: portable planner tests and the hosted Windows bootstrap decision passed.
Finished-package launch on a supported clean Windows machine, installer provenance
when installation is required, interactive RuntimeSmoke, independent review, and
the required long soak remain pending.

Implementation commit: `e4e2bd71961ac47297807ef1b1c69c81dcdb3aa8`.
Hosted-workflow integration commit: `8bf1aca1b9aea82941aef214afdf04c87dd36732`.
This packet does not invoke R0, download or execute a Redistributable, launch the
game, alter Engine/Game/CMake, merge, release, or touch the local Company Runtime.

## Research basis

Primary sources read September 20, 2026:

- Microsoft, **Redistribute Visual C++ files**:
  https://learn.microsoft.com/en-us/cpp/windows/redistributing-visual-cpp-files?view=msvc-170
  Microsoft recommends the central Redistributable package, documents the v14
  registry record and Version/Major/Minor/Bld/Rbld values, says not to install an
  older package over a newer registered version, and documents `/install`,
  `/passive`, `/quiet`, `/norestart`, and `/log` command options.
- Microsoft, **Latest supported Visual C++ Redistributable downloads**:
  https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170
  Microsoft publishes architecture-specific v14 permalinks and requires the
  installed Redistributable to be at least as new as the MSVC Build Tools used to
  compile the application. Redistribution remains subject to license terms.
- Unreal Engine 5.8, **Project settings / Prerequisites**:
  https://dev.epicgames.com/documentation/en-us/unreal-engine/project-section-of-the-unreal-engine-project-settings
  UE exposes prerequisite-installer and app-local-prerequisite packaging controls;
  this is a comparison target, not source copied into Astral.

## Portable evidence

Implementation was exercised in a disposable Linux directory before publication:

- `python -m py_compile plan_windows_redist_bootstrap.py test_windows_redist_bootstrap.py`: PASS, exit 0.
- `python test_windows_redist_bootstrap.py`: **9/9 tests passed**, exit 0.

The cases cover same/newer registered v14 versions selecting `skip_install`,
older/missing registration selecting `install_latest_supported`, x64/x86/ARM64
Microsoft permalink mapping, stale package evidence, false acceptance claims,
registry component mismatch, architecture mismatch, CLI JSON output, and refusing
a live-registry claim off Windows.

The planner is intentionally non-mutating. It does not download or execute the
installer and leaves `supported_redist_installation_verified`, package launch,
clean-machine compatibility, and independent acceptance false. The install command
is a template only and includes `/passive /norestart /log`; it is not an executed
or approved installer receipt.

## Hosted Windows receipt

GitHub Actions run
https://github.com/LucasKazaki/AnimeRPG/actions/runs/35532005942,
job `106134167852`, completed **SUCCESS** on `windows-2022`, Windows Server 2022
10.0.20348, runner image `20260913.307.1`.

The exact candidate passed:

- R0 runner safety: **13/13**.
- PE dependency inspector: **5/5**.
- Windows prerequisite planner: **8/8**.
- Windows runtime preflight: **8/8**.
- Runtime/toolset compatibility auditor: **7/7**.
- Redistributable bootstrap tests: **8 passed, 1 expected non-Windows-only test skipped**.
- Release assertion/CTest safety: **3/3**.
- Full MSVC Debug build and deterministic non-GUI CTest: **8/8**.
- Full MSVC Release build and deterministic non-GUI CTest: **8/8**.
- All three static milestone verifiers and the clean tracked-tree check: PASS.

Hosted build/runtime evidence for this run:

- MSVC compiler: `19.44.35228.0`.
- VC Build Tools: `14.44.35207`.
- Windows SDK: `10.0.26100.0`.
- Fresh Release `AstralGame.exe` SHA-256:
  `54f7145835281754d8ec3503fd37892343cefa5a2150b98f219223c7fe5137e8`.
- Image: AMD64 / PE32+, 69,632 bytes.
- Required VC imports: `MSVCP140.dll`, `VCRUNTIME140.dll`,
  `VCRUNTIME140_1.dll`; no Debug CRT imports.
- Observed central runtime DLL versions: `14.51.36247.0`, satisfying the
  recorded Build Tools floor `14.44.35207.0`.

The new live registry probe found the x64 v14 registration at
`HKLM\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64` in the 32-bit
registry view, version `14.51.36247.0`. Because this is newer than the build-tools
floor, the hosted bootstrap result is:

- strategy: `central_vc_redist`;
- architecture: `x64`;
- action: `skip_install`;
- registered version floor satisfied: `true`;
- current Microsoft latest-supported permalink retained for a machine that does
  require installation: `https://aka.ms/vc14/vc_redist.x64.exe`;
- installer download/execution/receipt: all **false**;
- supported Redistributable installation, package launch, clean-machine
  compatibility, and independent acceptance: all **false**.

The hosted runner therefore demonstrates that the planner can make a bounded,
version-aware skip/install decision on a real Windows registry and bind it to the
same package/toolchain evidence chain. It is not a clean end-user acceptance host.

## Registered local handoff

The registered local executor must run the same evidence chain against the exact
candidate/package on a supported clean Windows target and retain OS, package SHA,
Build Tools/runtime/registry state, commands, UTC timestamps, outputs, exit codes,
and launch/runtime evidence.

If the planner returns `install_latest_supported`, do not infer permission to run
an installer from the planner. Use an separately approved redistribution/license
route, retain the exact installer URL/source, downloaded file SHA-256, version and
signature/provenance, execute only under the approved local workflow, record its
exit/restart result, then re-probe the v14 registry/runtime evidence before launch.
If the planner returns `skip_install`, retain the registry/version receipt and
proceed only to the separately authorized package-launch gate.

Interactive RuntimeSmoke, finished-package launch, independent review,
stress/failure-recovery evidence and the long soak remain open. R0 remains
uninvoked by this packet, and issue #7 should remain open until those gates are
separately accepted.
