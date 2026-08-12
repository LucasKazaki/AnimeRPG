# R0 One-Command Coordinator Entry Point

Use this only on the Windows Agent Studio host after this recovery tooling is merged into a clean local `main` checkout.

```powershell
python Scripts/invoke_r0_release_candidate.py
```

The runner implements `Tasks/R0-loop-recovery-release-candidate.md` without changing game source. It:

1. requires a clean source checkout and the integrated M10 baseline;
2. fetches `origin/main` and creates or reuses the dedicated R0 branch and worktree;
3. preserves nonempty prior build, release, and evidence directories by renaming them;
4. runs fresh Visual Studio 2022 x64 Debug and Release builds;
5. runs complete local CTest in both configurations, including native RuntimeSmoke tests;
6. runs all available static milestone verifiers and git diff checks;
7. launches the packaged executable through the M10 package smoke;
8. writes M8, M9, M10, recovery, decision, planning, review, manifest, and heartbeat evidence;
9. stops at `review` until a separate reviewer accepts the package.

The first deterministic failure changes the heartbeat to `blocked`, records the exact command and log, and exits. Do not rerun an identical failed command without changing a material condition.

Default output locations:

- worktree: `C:/AI/worktrees/AnimeRPG/r0-loop-recovery-2026-08-11`
- build: `C:/AI/builds/AnimeRPG/r0-loop-recovery-2026-08-11`
- evidence: `C:/AI/evidence/AnimeRPG/r0-loop-recovery-2026-08-11`
- package: `C:/AI/releases/AnimeRPG/2026-08-14-m10-release-candidate`
