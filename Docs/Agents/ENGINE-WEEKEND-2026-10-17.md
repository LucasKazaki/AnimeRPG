# Astral Engine weekend checkpoint

Updated: 2026-10-17 13:41 America/New_York. Owner: Codex chat
`01a14ae5-1646-7c80-b8a8-c76ba4ab5747`, coordinator `/root`.
Scope/authority: `Tasks/ENGINE-WEEKEND-COORDINATION-2026-10-17.md` and Lucas's
October 17 pasted request. Deadline: 2026-10-18 23:59 Eastern.

## Current baseline and ownership

- Remote main: `c644eec79752b9dc101aeca6b03141cf26854c96`, refreshed locally.
- Primary `C:/AI/projects/AnimeRPG`: main at `771b61ac116dfa4a70d81b53a390f672aaf3bf4e`,
  926 commits behind; tracked tree clean; three untracked files preserved.
- Other existing worktrees: local-test at `96c25cbb6d56094b49a3e60717d150db42a9b873`,
  director recovery at `a13c3dae28bf1745d4c13bd48269c07fd3318fc6`, R0 recovery at
  `f43b3d385a4e6a90e7a178fcc00be57e97b7ed01`. Do not edit them.
- Session root: `C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you`.
- Coordinator worktree: `<session-root>/work/astral-integration`, branch
  `engine/weekend-coordinator-20261017`, created from exact current remote main.
- Numeric owner worktree: `<session-root>/work/astral-numeric`, branch
  `engine/numeric-repair-20261017`, same base; implementation agent
  `/root/numeric_implementation`. Frozen repair `ec3822df1966e50c944ab064c9c694f26639445e`;
  clean independent review and full native checks passed; coordinator cherry-pick
  `94d4e8a7b6437bc99a64898407361dbfa1dff285`. No remote merge yet.
- Framework worktree: `<session-root>/work/astral-framework`, branch
  `engine/framework-containment-20261017`, deliberately based on unmerged PR 80.
  Owner `/root/containment_implementation`. Exact PR80 full Debug build and
  31/31 deterministic CTest passed (56.05 s). Old importer root escape locally
  reproduced with harmless fixtures (exit 3); scoped repair is in progress.
  Focused post-fix Debug asset suite passes; Release and independent review pending.
- Full builds/native/GPU tests belong to coordinator and run serially. No Astral,
  CMake, MSBuild, or R0 process was observed before the owned baseline build.
- Chat inventory shows other active projects on this host; leave their processes,
  RAM/GPU workloads and schedulers untouched.

## Refreshed candidates (unmerged)

| PR | Exact head | Current state |
|---|---|---|
| 80 | `efac9db54e8f2781c20fbb1bb9be9a0ab85f4f5c` | Framework/assets/host/players; CI green, no submitted reviews; defects B–F require repair |
| 76 | `fa76e04f0cf3c88471bd266677589e39e26f8b01` | Conflicts; adapt engine-only numeric repairs, reconcile IsFinite signature |
| 52 | `1b7a80715b33080372405ff357b1dafb3979d178` | CI fails before configure, run 36026761079; R0 not admitted |
| 13 | `daf872debbba5fb775c57ab9e2e16c82bc32c13f` | Conflicting editor smoke candidate; native evidence pending |

Existing local-test editor candidate requires read-only inspection before any
new editor implementation. Do not assume PR 80 or that local branch is main.

## Machine and toolchain observed

Windows 11 Home 10.0.26300; AMD Ryzen 9 7950X3D (16 cores/32 threads);
31.16 GiB visible RAM, approximately 9.24 GiB initially available. RTX 5080,
16,303 MiB VRAM (1,633 MiB used), NVIDIA driver 617.14. C: approximately 471 GiB
free. Visual Studio 2022 Community 17.14.22, MSVC 19.44.35222, Windows SDK
10.0.26100.0. CMake/CTest bundled with VS, not on ordinary PATH. GitHub CLI
authenticated as repository owner; no token material retained in repository.
Build with two jobs initially. No optional models or competing GPU tests started.

## Commands, results and next action

Coordinator baseline configure and full Debug/Release builds succeeded (VS2022
x64) in `<session-root>/work/build-main-baseline`. Deterministic CTest: Debug
25/25, 53.67 seconds; Release 25/25, 4.87 seconds. Debug native smokes: 6/6,
19.45 seconds. Release smokes also pass 6/6 (see retained log for duration).
Logs: `<session-root>/work/baseline-{configure,debug-build,debug-test,
release-build,release-test,debug-smoke,release-smoke}.log`.
`test_test_safety.py`: 3/3 pass after adding VS CMake to this process's PATH;
milestone verifiers 1–3 pass. Initial safety invocation failed because CMake was
not on PATH; preserve both failure and `baseline-path-fixed-*` success logs.
Numeric reproduction harness compiled successfully and exited 1 on exact main:
member/free length infinity, normalized vector zero, NaN projection succeeds,
idle NaN time poisons position, hierarchy translation infinity. Harness/logs:
`<session-root>/work/numeric-evidence/`. Repaired harness passes all six checks.
Numeric full Debug/Release builds and deterministic CTest pass 25/25 each
(52.23/4.90 s); native smokes pass 6/6 each (18.96/19.29 s). No surviving owned
Astral processes observed. Fixed original 320x180 CPU fixture p95 rose from
9.1984 to 10.0937 ms (1.09733 ratio, under the predeclared 1.25 guard); images
visually inspected, 10 changed pixels. This is a robustness/cost check, not
general real-time or GPU acceptance. Full evidence: `Docs/QA/ENGINE-NUMERIC-2026-10-17.md`.
Next coordinator action: final integration documentation review, push scoped
numeric PR, verify actual head/base/merge trees and CI, merge only passing work,
then verify integrated main. Do not blindly merge original PR76 or PR80.

One read of historical PR 52 job logs via `gh api` failed because the response
contained terminal escape sequences; a subsequent explicitly escape-tolerant
read succeeded and retained `work/pr52-failed-job.log`. It confirms 31 tests,
one failure: CI exercises only PR head, not both head and merge ref. Do not
treat old CI logs as current build evidence.

App heartbeat `astral-engine-weekend-implementation` is active in this chat,
hourly at :59 through the deadline. It resumes this checkpoint and must stop
implementation at the deadline. Initial creation without destination was
rejected; corrected `destination=thread` creation succeeded. No other scheduler
was changed. The app must be running for local execution.

## Prioritized queue

1. Establish exact-main native baseline; numeric failure reproductions and repair A.
2. Independently review and integrate numeric repair after applicable gates.
3. Bound adoption of PR 80 with fixes for explicit UI/modal consumption (B),
   transactional dependency reload (C), diagnostics/recovery (D), canonical glTF
   dependency containment (E), finite input accumulation (F). Keep original sample
   only as existing engine validation; create no game content.
4. Repair actual head/merge execution coverage (G), reconcile editor smoke candidate.
   Existing editor-only adoption candidates after repaired PR80:
   `1c3d2ddb732b48d2129a5a0150747dbe3aa55978` then
   `96c25cbb6d56094b49a3e60717d150db42a9b873`. Skip game commits `bffe363` and
   `abadca8` wholesale. Preserve dirty local-test work. Confirmed source gaps:
   failed project-open loses document, untitled save confirmation, capture-loss
   cancellation, Unicode/surrogates, keyboard traversal/DPI, project creation,
   Play audio and dependency reload. PR13 shell predicates are obsolete for this
   editor; preserve its process containment but test a current neutral fixture.
5. Connect existing editor to runtime world and save/play restoration; then GPU
   spike/ADR/backend, asset/animation integration, external projects/packaging,
   starter walkthrough and measured hardening, dependency ordered.

Workflow gate: **not passed**. 24-hour immutable-candidate soak: **not started**.
No performance target has been measured. Long-term roadmap remains open.
