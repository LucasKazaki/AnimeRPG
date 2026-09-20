# R0 package runtime-smoke receipt, 2026-09-20

## Why this packet exists

PR #9 now verifies R0 path/process safety, Windows runtime prerequisites, and exact package bytes, but issue #7 still lacks a bounded way to bind a native GUI smoke result to the exact packaged `AstralGame.exe` and then prove the package bytes were unchanged by the launch. The existing `M10RuntimeSmoke.exe` already launches the supplied game executable, finds its visible Win32 window, checks title/state transitions and pixels, sends input, and requires a clean Escape exit. This packet wraps that existing smoke with package provenance and failure-safe receipts. It does not add a new engine feature.

Baseline: PR #9 head `d67b5582225927b77afe49a6014698e808201117`, stacked on PR #6. E15 remains partial and issue #7 remains open.

## Allowed paths

- `Scripts/run_package_runtime_smoke.py`
- `Scripts/test_package_runtime_smoke.py`
- `.github/workflows/release-manifest-validation.yml`
- `Tasks/R0-PACKAGE-RUNTIME-SMOKE-2026-09-20.md`
- `Docs/QA/R0-PACKAGE-RUNTIME-SMOKE-2026-09-20.md`

No Engine/, Game/, CMake, R0-runner, runtime database, scheduler, dependency, package publication, or merge changes are authorized by this packet.

## Primary-source basis

Accessed September 20, 2026:

1. Epic Games, **Packaging Unreal Engine Projects**, UE 5.8: https://dev.epicgames.com/documentation/unreal-engine/packaging-your-project . Epic treats packaged output as a standalone target artifact and explicitly includes running, testing, and exiting the packaged executable as a production-testing workflow.
2. Epic Games, **Build Operations: Cook, Package, Deploy, and Run**, UE 5.8: https://dev.epicgames.com/documentation/unreal-engine/build-operations-cooking-packaging-deploying-and-running-projects-in-unreal-engine . Build, stage/package, deploy, and run are separate operations. Astral should preserve the same evidence boundary rather than treating a successful build as a successful run.
3. Unity 6.0, **BuildPipeline** and **BuildPlayerOptions**: https://docs.unity3d.com/6000.0/Documentation/ScriptReference/BuildPipeline.html and https://docs.unity3d.com/6000.0/Documentation/ScriptReference/BuildPlayerOptions.html . Unity exposes player-build automation and run-related build options; this is a capability reference, not source code to copy.
4. CPython 3.13 `subprocess`: https://docs.python.org/3.13/library/subprocess.html . `Popen`/`wait(timeout=...)` support bounded child execution; correct timeout handling requires terminating and waiting for the child. The wrapper retains output in a file to avoid pipe-buffer deadlocks and kills its owned process tree on timeout/interruption.

## Acceptance contract

Add a dependency-free wrapper that:

- verifies the existing package manifest immediately before launch against an expected 40-hex commit and independently recorded `AstralGame.exe` SHA-256;
- requires native acceptance to run on Windows with an exact expected SHA-256 for an externally built `M10RuntimeSmoke.exe`;
- forbids prefix-command indirection for native acceptance and requires the existing M10 PASS marker plus exit code zero;
- runs the smoke from a fresh runtime working directory disjoint from the package so logs/evidence are not written into the immutable package tree;
- bounds execution to 0.1-300 seconds, retains bounded stdout/stderr, and kills only its owned process tree on timeout/interruption;
- verifies the package manifest again after the smoke, so runtime mutation of packaged bytes fails acceptance;
- emits a JSON receipt with exact package/smoke hashes, command, timestamps, output, exit/timeout state, and explicit claim boundaries.

Synthetic contract mode is permitted only for wrapper tests and must never claim package launch. A native successful receipt may set `package_launch_verified=true` only on actual Windows when the production manifest verifier and production subprocess executor are used, the exact M10 smoke hash is bound, its native PASS marker is observed, it exits zero within the deadline, and post-launch manifest verification succeeds. Dependency-injected contract tests cannot set that claim. It must keep `clean_machine_compatibility_verified`, `owned_interactive_desktop_verified`, and `independent_acceptance` false because those require separate machine/ownership/review evidence.

Hosted CI runs the wrapper contract tests but must not run `M10RuntimeSmoke.exe` or claim an interactive desktop. The registered local executor owns the eventual real package smoke.

## Local acceptance handoff

After the existing package-manifest and VC-runtime/preflight chain passes on the registered supported Windows target, build the exact admitted source in external Debug/Release roots. Hash the freshly built Release `M10RuntimeSmoke.exe` independently. Use a fresh external runtime directory and receipt path, then run:

```powershell
python Scripts/run_package_runtime_smoke.py `
  <package>\MANIFEST.json <package> `
  --expected-commit <40-hex-admitted-revision> `
  --expected-executable-sha256 <independently-recorded-AstralGame-sha256> `
  --smoke-executable <external-build>\Release\M10RuntimeSmoke.exe `
  --expected-smoke-sha256 <independently-recorded-M10RuntimeSmoke-sha256> `
  --runtime-root <fresh-external-runtime-directory> `
  --timeout-seconds 60 `
  --json <external-evidence>\package-runtime-smoke.json
```

Retain the wrapper JSON, `runtime-smoke-output.log`, `astral.log`, package manifest verification, prerequisite/runtime receipts, machine/OS/toolchain identity, exact command, UTC/local timestamps, and hashes. The operator must separately record that the desktop was exclusively owned for the interactive smoke. Do not call a hosted or synthetic receipt local native acceptance.

## Stop and rollback

Stop on a manifest/hash mismatch, wrong smoke binary/hash, nonzero exit, missing M10 PASS marker, timeout, output-limit breach, package mutation, stale/nonempty runtime root, or inability to terminate the owned smoke process tree. Do not weaken the smoke or post-launch manifest check. Rollback is the runtime-smoke receipt commit(s) on PR #9. Clean-machine support, independent review, stress/performance work, and the required 24-hour soak remain separate gates.
