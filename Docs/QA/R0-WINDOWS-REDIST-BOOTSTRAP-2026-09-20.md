# Windows central Redistributable bootstrap evidence, 2026-09-20

Status before hosted CI: portable planner tests passed; Windows registry evidence,
package launch, clean-machine acceptance and independent review remain pending.

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
is a template only and includes `/norestart`; Microsoft documents that central
Redistributable installation can require elevation and recommends checking the
v14 registry state before attempting installation when a newer version exists.

## Hosted/local boundaries

Hosted Windows CI must build a fresh Release image, reproduce the existing PE,
prerequisite, runtime-preflight, and toolset-compatibility evidence chain, then run
this planner against the host's central v14 registry registration. A missing or old
registration is a valid `install_latest_supported` decision, not permission for CI
to download or install anything. A same/newer registration is only a hosted-host
skip decision. Neither result proves a clean end-user machine or installer provenance.

Local acceptance remains: approved redistribution/license route, installer hash and
version provenance if an install is performed, supported target OS, finished-package
launch, interactive RuntimeSmoke, independent review, stress/recovery and the long
soak. R0 remains uninvoked by this packet.
