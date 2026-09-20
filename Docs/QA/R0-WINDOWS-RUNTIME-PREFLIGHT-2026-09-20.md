# R0 Windows central-runtime preflight evidence, 2026-09-20

Status: portable preflight tests passed before publication. Hosted Windows verification
for the exact GitHub candidate remains required after commit. This is not clean-machine
or package-launch evidence.

## Baseline and scope

Baseline draft PR #9 head: `5241384e5fe8f063abe1a68106d1d40cc7c3c0e2`.
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

## Hosted contract added

Windows CI now runs the portable contract tests. After building the real Release image,
it reuses the exact PE dependency report and prerequisite plan, probes the hosted
Windows central runtime files, and requires a successful preflight while still asserting
that runtime-version compatibility, package launch, clean-machine compatibility, and
independent acceptance remain false.

Hosted Windows results are pending for the published candidate. Even a green hosted run
will demonstrate tool behavior and real central-runtime file inventory only. It cannot
satisfy clean-machine package launch, registered local RuntimeSmoke, independent review,
or the required soak.
