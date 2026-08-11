# R0 — Loop Recovery and M10 Release Candidate

Target date: **Friday, August 14, 2026**

## Objective

Re-establish a trustworthy, low-waste game-development loop and produce a reproducible Windows release-candidate package of the current M10 playable prototype. This packet verifies and packages the existing baseline. It does not authorize a new gameplay feature, engine migration, dependency installation, or broad refactor.

## Starting point

- Repository: `LucasKazaki/AnimeRPG`
- Required base: latest integrated `main` containing merge commit `181298f69b42899db83040d2d5b6c3e67804e242` or a later explicitly reviewed recovery commit.
- Worktree: `C:/AI/worktrees/AnimeRPG/r0-loop-recovery-2026-08-11`
- External build tree: `C:/AI/builds/AnimeRPG/r0-loop-recovery-2026-08-11`
- Release output: `C:/AI/releases/AnimeRPG/2026-08-14-m10-release-candidate`

Do not build in the source root. Do not reuse a worktree that has an active writer or unknown modifications.

## Agent allocation

1. One coordinator owns git state, heartbeat, packet compliance, and the final decision.
2. One build/release worker runs commands and packages artifacts.
3. One independent reviewer/QA worker reads the packet, diff, logs, and package without editing.

Do not launch parallel coding agents. A source-code change requires a separate, narrowly scoped fix packet created only after a deterministic failure is captured.

## Allowed files

The recovery pass may create or update only:

- `Docs/Agents/LOOP_HEARTBEAT.json`
- `Docs/Agents/LOOP_STATUS_2026-08-11.md`
- `Docs/QA/MILESTONE-8.md`
- `Docs/QA/MILESTONE-9.md`
- `Docs/QA/MILESTONE-10.md`
- `Docs/QA/RECOVERY-2026-08-11.md`
- `Docs/Decision-Log.md`
- `Docs/Planning/MILESTONES.md`
- `Docs/Reviews/R0-independent-review.md`
- `Release/README-M10-RC.md`
- `Release/MANIFEST-M10-RC.json`
- a narrowly scoped packaging script under `Scripts/` when needed

No `Engine/`, `Game/`, `Tests/`, or `CMakeLists.txt` change is authorized by this packet.

## Phase 1 — Prove a clean starting state

Run and record the complete output of:

```powershell
git fetch --all --prune
git status --short
git branch --show-current
git rev-parse HEAD
git worktree list --porcelain
Get-Process cmake, ctest, msbuild, devenv, cl -ErrorAction SilentlyContinue
where.exe cmake
where.exe git
```

Open a new dated blocker and stop when:

- the worktree is dirty with unexplained changes;
- another writer is using the same worktree;
- the required base commit is missing;
- CMake or a supported Visual Studio C++ toolchain is unavailable;
- the coordinator cannot write the heartbeat.

Do not install or repair system software under this packet.

## Phase 2 — Fresh configure, build, and test

From the repository worktree, use the external build directory only:

```powershell
cmake -S . -B C:/AI/builds/AnimeRPG/r0-loop-recovery-2026-08-11 -G "Visual Studio 17 2022" -A x64
cmake --build C:/AI/builds/AnimeRPG/r0-loop-recovery-2026-08-11 --config Debug --parallel
ctest --test-dir C:/AI/builds/AnimeRPG/r0-loop-recovery-2026-08-11 -C Debug --output-on-failure
cmake --build C:/AI/builds/AnimeRPG/r0-loop-recovery-2026-08-11 --config Release --parallel
ctest --test-dir C:/AI/builds/AnimeRPG/r0-loop-recovery-2026-08-11 -C Release --output-on-failure
python Scripts/verify_milestone1.py
python Scripts/verify_milestone2.py
python Scripts/verify_milestone3.py
git diff --check
git status --short
```

Acceptance requires:

- Visual Studio 2022 x64 configure succeeds;
- Debug and Release builds succeed;
- complete CTest passes in both configurations, including all native runtime smokes;
- all available static milestone verifiers pass;
- no source-root CMake artifacts appear;
- no unexplained source change appears.

On the first deterministic failure, save the exact command, exit code, and smallest useful log excerpt, update the heartbeat to `blocked`, and stop. Do not repeatedly ask models to reinterpret the same log.

## Phase 3 — Reconcile evidence without inventing it

Create or repair QA records for implementation tasks M8, M9, and M10 using fresh command output plus the existing task and test contracts. Each report must distinguish:

- fresh August 2026 evidence;
- historical evidence already recorded in the repository;
- anything not directly verified.

Append a dated recovery entry to `Docs/Decision-Log.md`. Update `Docs/Planning/MILESTONES.md` so that the original product backlog and the later implementation-task labels cannot be confused. Preserve both histories rather than silently rewriting old decisions.

The reconciliation must explicitly state that the later implementation labels are:

- implementation M8: Thought Commands;
- implementation M9: Landmark Interaction;
- implementation M10: Landmark Encounter Loop.

These labels do not automatically replace the original product-backlog definitions for Shadow Summon, Enemy Set, or later dungeon work.

## Phase 4 — Package the current product

Create `C:/AI/releases/AnimeRPG/2026-08-14-m10-release-candidate` from the verified Release build. Include only required runtime files and documentation. At minimum include:

- `AstralGame.exe`;
- any file that the executable demonstrably requires at runtime;
- `README-M10-RC.md` with controls, launch steps, verified commit, build date, and known limitations;
- `MANIFEST-M10-RC.json` with relative paths, byte sizes, and SHA-256 hashes;
- the final recovery QA report.

Launch the packaged executable from the package directory, not the source tree. Re-run the M10 acceptance path or a package-specific smoke that proves the package is self-contained enough for its documented launch procedure. Exit through Escape and record the result.

Current controls to verify and document:

- `W`, `A`, `S`, `D`: movement;
- `J`, `K`: light and heavy attacks;
- `Q`: Shadow Dash;
- `L`: Fatal Strike;
- left `Shift`: Guard;
- `1` through `5`: bounded Thought Commands;
- `E`: landmark interaction;
- `Escape`: exit.

## Phase 5 — Independent review and next packet

The independent reviewer must confirm:

- allowed-file compliance;
- exact commit and package provenance;
- Debug and Release evidence;
- runtime-smoke evidence;
- package manifest accuracy;
- no unsupported product claim;
- no hidden source or dependency change;
- no unresolved milestone-number ambiguity.

Only after the reviewer recommends acceptance may the coordinator mark the heartbeat `complete` and queue one next feature packet. The next packet must reference the reconciled product backlog and choose one coherent prerequisite-aware increment. Do not begin implementation in the same recovery run.

## Heartbeat cadence

Update `Docs/Agents/LOOP_HEARTBEAT.json` at state transitions and after commands that materially change the result. Do not spend model calls updating it on a timer. A `running` heartbeat older than 30 minutes is stale unless the named process is still active and its log is growing.

## Stop conditions

Stop immediately for:

- destructive cleanup or history rewriting;
- dependency, plugin, SDK, or toolchain installation;
- credential access or external API calls;
- public deployment;
- source changes not authorized by a separate fix packet;
- multiple agents editing the same worktree;
- fabricated or inferred test evidence;
- a time or token budget that cannot finish the current acceptance gate.

A blocked report with one exact next action is preferable to repeated retries or broad speculative changes.
