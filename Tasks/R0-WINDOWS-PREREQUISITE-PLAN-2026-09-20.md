# R0 Windows prerequisite policy record, 2026-09-20

## Why this packet exists

The preceding dependency inventory proved that the hosted Release `AstralGame.exe`
imports `MSVCP140.dll`, `VCRUNTIME140.dll`, and `VCRUNTIME140_1.dll`, with no Debug
CRT imports. That establishes a deployment requirement but does not yet turn it into
an explicit package policy. A release candidate should not be able to forget the
runtime decision between dependency inspection and clean-machine testing.

This packet remains inside E15 packaging/platform verification. It does not invoke
R0, download/install/copy a redistributable, switch the C++ runtime linkage model,
modify Engine/Game/CMake, publish a package, merge a PR, or claim clean-machine
compatibility. Baseline: PR #9 head `3a27de91e2cfecf40b19639134a39e93ac807bb1`,
stacked on PR #6.

## Allowed paths

- `Scripts/plan_windows_prerequisites.py`
- `Scripts/test_windows_prerequisite_plan.py`
- `.github/workflows/windows-ci.yml`
- `Tasks/R0-WINDOWS-PREREQUISITE-PLAN-2026-09-20.md`
- `Docs/QA/R0-WINDOWS-PREREQUISITE-PLAN-2026-09-20.md`

No release-runner execution or dependency installation/copy is authorized by this packet.

## Primary-source research

Accessed September 20, 2026:

1. Microsoft, **Choose a deployment method**:
   https://learn.microsoft.com/en-us/cpp/windows/choosing-a-deployment-method?view=msvc-170
   Microsoft recommends central deployment using the Visual C++ Redistributable,
   documents app-local deployment as an exception with serviceability costs, and
   recommends dynamic rather than static linking for redistributable libraries.
2. Microsoft, **Visual C++ runtime lifecycle FAQ**:
   https://learn.microsoft.com/en-us/lifecycle/faq/visual-c-faq
   Visual C++ v14 redistributables are cumulative and a target must have a runtime
   version at least as new as the build tools used by the application. The latest
   supported redistributable is therefore the preferred central prerequisite.
3. Microsoft, **Redistribute Visual C++ Files**:
   https://learn.microsoft.com/en-us/cpp/windows/redistributing-visual-cpp-files?view=msvc-170
   Microsoft recommends central package deployment over deprecated merge modules so
   runtime files can be serviced independently.
4. Epic Games, **Unreal Engine 5.8 Project Settings, Prerequisites**:
   https://dev.epicgames.com/documentation/en-us/unreal-engine/project-section-of-the-unreal-engine-project-settings
   UE exposes prerequisite-installer and app-local-prerequisite packaging controls.
   This is a capability reference only, not source or a design copied into Astral.
5. Epic Games, **Packaging Your Project, Unreal Engine 5.8**:
   https://dev.epicgames.com/documentation/unreal-engine/packaging-your-project
   UE treats build, cook, stage, package, deploy, and run as distinct operations.
   Astral's current recovery package is much narrower and still requires explicit
   prerequisite and clean-machine acceptance evidence.

Unity 6.0's public system-requirements documentation remains useful for supported
player platform baselines, but it does not define Astral's MSVC redistribution policy.
Do not infer a Visual C++ deployment strategy from that Unity page.

## Acceptance contract

Add a dependency-free planner that consumes only the JSON produced by
`inspect_pe_dependencies.py`. It must validate report provenance fields and
classification consistency, map PE machine types to x86/x64/arm64, and convert the
current VC-runtime dependency into an explicit `central_vc_redist` policy. It must
never download, copy, or run an installer, and must keep clean-machine verification
false until a separate supported-host launch actually occurs.

A report containing a Debug CRT dependency must remain a Release rejection. A report
with no VC-runtime import may say that no VC redistributable is required *by the import
table*, but it still may not claim clean-machine compatibility because dynamically
loaded libraries and OS availability remain separate concerns.

Portable tests cover normal VC imports, no-VC imports, architecture mapping, Debug CRT
rejection, malformed/unknown machines, inconsistent classification, false clean-machine
claims, and CLI behavior. Hosted Windows CI must run those tests, inspect the actual
Release executable, create its prerequisite plan, and assert that the current Release
build resolves to `central_vc_redist` without claiming an installer was bundled or run.

## Stop conditions and next gate

Stop on malformed-report acceptance, policy inconsistency, Debug CRT acceptance, a
hosted Release plan other than the expected current central v14 redistributable, or
tracked-tree mutation. Do not switch to `/MT`, app-local copying, or an installer just
to make a test green.

The next local E15 acceptance step remains external to this packet: choose the approved
central redistributable delivery/bootstrap method, test the finished package on a
supported clean Windows machine, and retain exact installer/runtime/package receipts.
Interactive RuntimeSmoke, independent review, and the long soak remain separate gates.
