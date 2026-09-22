# R0 package runtime-smoke receipt evidence, 2026-09-20

Status before publication: portable wrapper contracts pass; repository-level manifest integration is intentionally deferred to hosted CI because this sandbox contains only a partial reconstructed fixture. Native `M10RuntimeSmoke.exe` was not run here.

## Candidate and scope

Baseline: PR #9 head `d67b5582225927b77afe49a6014698e808201117`.

This packet adds evidence plumbing for the existing native GUI smoke. It does not change engine/game code, invoke R0, install prerequisites, publish or merge a package, claim an owned interactive desktop, claim a clean supported target, or provide independent acceptance.

## Sandbox evidence

Executed in `/tmp/astral-pass8` against the exact proposed `run_package_runtime_smoke.py` and `test_package_runtime_smoke.py`:

- `python -m py_compile run_package_runtime_smoke.py test_package_runtime_smoke.py`: PASS, exit 0.
- `python test_package_runtime_smoke.py`: **11 discovered, 10 passed, 1 skipped**, exit 0.
- The single skip is the test that imports the repository's real `release_manifest.py`; that file was not copied into this partial sandbox fixture. Hosted CI is required to run the same test with the real repository module.
- The real subprocess timeout fixture passed. On this Linux sandbox it started a descendant that would write a delayed sentinel if it escaped the wrapper's process-group cleanup; the sentinel remained absent after timeout.

Covered contracts include native-vs-synthetic claim separation, refusal to set launch acceptance from injected test executors/verifiers, M10 smoke hash/name/prefix/PASS-marker restrictions, package/smoke argument binding, nonzero/timeout/oversize-output rejection, fresh/disjoint/non-symlink runtime-root enforcement, smoke symlink/package-location rejection, post-launch package-mutation failure, timeout bounds, and process cleanup.

## Research/evidence interpretation

UE 5.8 separates package creation from run/test operations, and its packaging tutorial explicitly runs, tests, and exits the packaged executable. Unity 6.0 likewise exposes programmable player-build/run options. The Astral wrapper follows that evidence boundary: package-manifest integrity must pass before and after the runtime smoke, and a build alone never implies launch success.

The wrapper deliberately runs `M10RuntimeSmoke.exe` with a fresh working directory outside the package. Astral's current logger uses a relative `astral.log` path, so this prevents test-generated writable state from silently becoming part of or mutating the immutable package artifact. This is acceptance-harness behavior, not a claim that Astral already has a production per-user writable-data policy.

## Hosted/local handoff

The modified release-manifest workflow must run the new tests on `windows-2022`. That hosted run may establish Windows contract portability and real `release_manifest.py` integration only. It must not execute the GUI smoke or set package-launch acceptance.

A real native package-launch receipt requires the registered local Windows executor to use the exact manifest-bound package and a separately hashed Release `M10RuntimeSmoke.exe`, on an exclusively owned interactive desktop, after the existing VC-runtime prerequisite chain. The result must retain exact commands, hashes, stdout/stderr, exit state, runtime logs and post-launch manifest verification. Even a successful native receipt leaves clean-machine support, independent review, stress/performance evidence, and the required 24-hour soak unresolved.
