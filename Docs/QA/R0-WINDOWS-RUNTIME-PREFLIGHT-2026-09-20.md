# R0 Windows central-runtime preflight evidence, 2026-09-20

Status: portable preflight tests and hosted Windows verification passed for the
published candidate. This is not clean-machine or package-launch evidence.

## Baseline and scope

Implementation baseline was draft PR #9 head
`5241384e5fe8f063abe1a68106d1d40cc7c3c0e2`. The runtime-preflight implementation
was published as `cc8c1e829e82710883c421793fa0c512130cab82`.
The preceding hosted package evidence established an AMD64 Release importing
`MSVCP140.dll`, `VCRUNTIME140.dll`, and `VCRUNTIME140_1.dll` and a reviewed
`central_vc_redist` policy. This pass adds no dependency and does not change engine,
game, build linkage, package contents, or the R0 runner.

## Implemented preflight

`Scripts/probe_windows_runtime_environment.py` binds three pieces of evidence before it
looks at a target host: the package bytes, the PE dependency report, and the prerequisite
plan. It rejects stale hashes, machine/architecture disagreement, changed VC import sets,
unreviewed prerequisite strategies, rejected Release images, false clean-machine input,
and unsafe/path-like DLL names.

On Windows, it inventories the central runtime files required by the package and retains
absolute path, SHA-256, byte size, and fixed file version. It deliberately reports:

- `runtime_version_compatibility_verified: false`
- `package_launch_verified: false`
- `clean_machine_compatibility_verified: false`
- `independent_acceptance: false`

Those false values are part of the contract. Microsoft requires a Redistributable equal
to or newer than the application Build Tools version, while the current Astral evidence
has not yet bound an exact minimum Redistributable package version to the build record.

## Portable executed evidence

Disposable Linux fixture, Python 3, no external packages:

```text
python -m py_compile probe_windows_runtime_environment.py test_windows_runtime_environment.py
exit 0

python test_windows_runtime_environment.py
8 tests, 8 passed, exit 0
```

The tests use owned temporary directories and an injected file-version reader. They do
not impersonate a Windows execution result. Covered contracts: successful evidence
binding, hashes/version records, missing DLL failure, metadata failure, changed image,
stale/import-mismatched plan, unsafe DLL name, unreviewed strategy, rejected/false-clean
plan, and x86/x64/ARM64 central-directory selection.

## Hosted Windows receipt

GitHub Actions run https://github.com/LucasKazaki/AnimeRPG/actions/runs/35525799852,
job `106117606175`, completed **SUCCESS** on `windows-2022` for candidate
`cc8c1e829e82710883c421793fa0c512130cab82`.

Verified steps included:

- R0 runner-safety contracts: pass.
- PE dependency inspector contracts: pass.
- Windows prerequisite planner contracts: pass.
- Windows runtime-environment probe contract tests: pass.
- Release assertion/CTest safety contracts: pass.
- Visual Studio 2022 x64 configure: pass.
- full Debug build and deterministic non-GUI tests: pass.
- full Release build: pass.
- real Release dependency inspection and prerequisite planning: pass.
- current x64 `central_vc_redist` policy assertions: pass.
- central VC runtime preflight against the freshly built Release image and hosted
  Windows environment: pass.
- hosted claim guard requiring required runtime files to be present and versioned while
  version compatibility, package launch, clean-machine compatibility, and independent
  acceptance remain false: pass.
- deterministic Release tests, static milestone verifiers, and tracked-tree cleanliness:
  pass.

The hosted preflight demonstrates that the candidate tool can chain the freshly built
package to its dependency report and prerequisite plan and can inventory every required
central VC runtime file on that GitHub-hosted Windows image with SHA-256, size, and
fixed file-version metadata. GitHub Actions is not a supported clean end-user machine,
and the workflow deliberately does not treat this result as package-launch evidence.

## Research basis and remaining acceptance

Primary Microsoft references read September 20, 2026:

- https://learn.microsoft.com/en-us/lifecycle/faq/visual-c-faq
- https://learn.microsoft.com/en-us/cpp/windows/redist-version-auditing?view=msvc-170

The Visual C++ v14 Redistributable is cumulative, and Microsoft requires the installed
Redistributable to be equal to or newer than the Build Tools version used to build the
application. This pass records DLL file versions but does not derive or assert that
minimum-version comparison. That remains the next prerequisite-evidence step.

Unresolved acceptance remains: bind the exact build-tool/runtime compatibility policy,
use an approved central Redistributable delivery/bootstrap method, launch the finished
package on a supported clean Windows machine, run the registered interactive
RuntimeSmoke checks, obtain independent review, and complete the required long soak.
Do not invoke R0, merge, or claim E15 completion from this hosted preflight alone.
