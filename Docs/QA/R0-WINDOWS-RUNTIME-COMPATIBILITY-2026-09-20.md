# R0 Windows runtime/toolset compatibility evidence, 2026-09-20

Status: portable contract tests and hosted Windows version-floor verification passed for
the exact implementation candidate. No package launch, installer execution, clean-machine
acceptance, R0 invocation, merge, or independent review occurred.

## Scope and research result

Implementation baseline: draft PR #9 head `bbbd22e08e88d0119f3d48f2904e120b6995d396`.
Implementation commit: `1a081d4545792133ad5fa9cd05096281dafc12d8`.
This packet adds only the missing comparison between the Visual C++ Build Tools version
used by the build and the fixed file versions of the already inventoried runtime DLLs.
Microsoft's September 20, 2026 lifecycle guidance says the v14 Redistributable/runtime
must be equal to or newer than the MSVC Build Tools version used by the application.
Microsoft's compiler-version table separately maps VS 2022 17.14 / compiler 19.44 to
MSVC toolset 14.44. The checker therefore consumes the actual `14.xx...` VC Tools
version rather than incorrectly comparing the compiler's `19.xx...` executable version.

Primary sources:
- https://learn.microsoft.com/en-us/lifecycle/faq/visual-c-faq
- https://learn.microsoft.com/en-us/cpp/overview/compiler-versions?view=msvc-170
- https://learn.microsoft.com/en-us/cpp/windows/redist-version-auditing?view=msvc-170
- https://dev.epicgames.com/documentation/en-us/unreal-engine/project-section-of-the-unreal-engine-project-settings

## Portable evidence

Disposable Linux fixture, real proposed script/test source:

```text
python -m py_compile audit_windows_runtime_compatibility.py test_windows_runtime_compatibility.py
python test_windows_runtime_compatibility.py
```

Result: **7/7 passed**, exit 0. Covered compatible/equal-or-newer runtime versions,
older runtime rejection, numeric major/minor ordering, malformed versions,
inconsistent and false preflight evidence, duplicate runtime entries, and CLI success
versus `--fail-on-incompatible` exit 2.

Pre-publication SHA-256:
- `Scripts/audit_windows_runtime_compatibility.py`: `5d846789fa9724377d5e3a9e8272bed025d5d1f4b726352e488fa0d18b162d9f`
- `Scripts/test_windows_runtime_compatibility.py`: `b9184bb1581aa6d5b9fb600238277438a858e3d00c60f0d0ea62fa9d58ab2753`

## Hosted Windows receipt

GitHub Actions run `35528776408`, job `106125525634`, completed **SUCCESS** on
`windows-2022`, Windows Server 2022 `10.0.20348`, runner image `20260913.307.1`.
The pull-request workflow checked merge ref
`8fa953819e61dd3d7df27f85926f035921c0e895`, containing implementation head
`1a081d4545792133ad5fa9cd05096281dafc12d8` stacked on PR #6.

The workflow passed:
- R0 runner safety: **13/13**;
- PE dependency inspector: **5/5**;
- Windows prerequisite planner: **8/8**;
- Windows central-runtime preflight: **8/8**;
- runtime/toolset compatibility audit: **7/7**;
- Release assertion/CTest safety: **3/3**;
- full MSVC Debug build and deterministic domain CTest: **8/8**;
- full MSVC Release build and deterministic domain CTest: **8/8**;
- all three static milestone verifiers and clean tracked-tree verification.

CMake selected Windows SDK `10.0.26100.0`, compiler `MSVC 19.44.35228.0`, and
`cl.exe` from `VC/Tools/MSVC/14.44.35207`. The separate `vswhere`/default-toolset
probe recorded Visual C++ Build Tools version **14.44.35207** and passed that exact
value into the compatibility auditor.

The freshly built Release `AstralGame.exe` was AMD64 / PE32+, 69,632 bytes, SHA-256
`e864987cd9253997d5ef73419223291e55c873b2b5c51af4dccf47ca636a4cf7`.
Its VC runtime imports were exactly `MSVCP140.dll`, `VCRUNTIME140.dll`, and
`VCRUNTIME140_1.dll`; no Debug CRT imports were present.

The hosted central-runtime preflight bound those imports to that exact package image
and observed all three required files in `C:\Windows\System32`, each with fixed file
version **14.51.36247.0**. The compatibility audit compared those numeric versions to
the recorded Build Tools floor **14.44.35207.0** and reported, for each imported DLL,
`satisfies_version_floor=true`. Aggregate results were:

```text
runtime_file_version_floor_satisfied = true
runtime_version_compatibility_verified = true
supported_redist_installation_verified = false
package_launch_verified = false
clean_machine_compatibility_verified = false
independent_acceptance = false
```

Observed file receipts from that hosted image:
- `MSVCP140.dll`: version `14.51.36247.0`, SHA-256 `7c26614e1d733892c2deac7e245ce115504b1d80592dd0a01b08e3e5a55f89ca`, 643,512 bytes;
- `VCRUNTIME140.dll`: version `14.51.36247.0`, SHA-256 `d1f4225df2cd877dbf130d5668a021dce3f94118455ff5ec952061c30afc9ce7`, 178,616 bytes;
- `VCRUNTIME140_1.dll`: version `14.51.36247.0`, SHA-256 `a7146c08f89fe5b04541ab507cdb59ff7b44534d4ba3c668a426c6450a03434e`, 50,112 bytes.

This establishes only that the observed hosted central runtime DLL files meet the
numeric version floor for the exact recorded build toolset. It does not establish
Redistributable installer package provenance, supported-installation state, or an
end-user-machine configuration. The GitHub-hosted image is not a clean target machine.

## Remaining gates

The remaining E15 work is delivery and execution evidence: preserve an approved,
supported central Visual C++ Redistributable bootstrap/installer route and launch the
finished package on a supported clean Windows host with OS, runtime, package, command,
timestamp, exit, and artifact receipts. Interactive RuntimeSmoke, independent review,
and the required long soak remain open. Do not invoke R0 from this evidence alone.
