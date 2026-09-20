# R0 Windows runtime/toolset compatibility evidence, 2026-09-20

Status: portable contract tests passed. Hosted Windows evidence for the exact candidate
is pending at this pre-publication checkpoint. No package launch, installer execution,
clean-machine acceptance, R0 invocation, or independent review occurred.

## Scope and research result

Baseline: draft PR #9 head `bbbd22e08e88d0119f3d48f2904e120b6995d396`.
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

These fixtures do not prove Windows toolset discovery, the hosted runner's runtime file
versions, installer provenance, a supported Redistributable installation, or package
launch. CI must exercise the same scripts against a freshly built real `AstralGame.exe`.

## Hosted contract to verify

The workflow must restrict `vswhere` to Visual Studio 17.x, read the default v143 tools
version from the selected VS 2022 installation, and bind that exact value to the
compatibility JSON. It then requires each runtime DLL already bound to the exact package
SHA to meet the numeric version floor. Hosted CI is allowed to set
`runtime_version_compatibility_verified=true` only for that observed file-version floor.
It must keep `supported_redist_installation_verified`, `package_launch_verified`,
`clean_machine_compatibility_verified`, and `independent_acceptance` false.

## Remaining gates

Even a green hosted version-floor audit is not deployment acceptance. The registered
local executor still needs an approved Redistributable delivery/bootstrap path and a
finished-package launch on a supported clean Windows host with OS, runtime, package,
command, timestamp, exit, and artifact receipts. Interactive RuntimeSmoke, independent
review, and the long soak remain open. Do not invoke R0 from this evidence alone.
