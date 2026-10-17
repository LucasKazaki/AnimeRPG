# Engine weekend coordination, October 17–18, 2026

Owner: Codex chat `01a14ae5-1646-7c80-b8a8-c76ba4ab5747` (`/root`).
Deadline: **2026-10-18 23:59 America/New_York** (2026-10-19 03:59 UTC).
Authority: Lucas's attached October 17 implementation assignment in this chat.
It expressly covers engine source, isolated worktrees, local execution, commits,
pushes, PRs and merges after independent review and applicable technical gates;
older local-only/per-change approval wording does not block this scope. It does
not waive tests, native/performance gates, or repository protections. No policy
files are changed. Engine-only; preserve existing game behavior and vision.

## Problem and approach

Main and unmerged framework/editor/safety candidates need fresh local evidence
and bounded repairs before the create/import/edit/play/save/package workflow can
be accepted. Refresh exact SHAs, establish a native baseline, then admit one
coherent repair per implementation owner and worktree. Reuse existing code.
Independent review must examine the actual resulting diff/commit. Never use the
historical R0 runner until its current safety/provenance gates pass.

## Owned paths and dependencies

Coordinator owns this packet, `Docs/Agents/ENGINE-WEEKEND-2026-10-17.md`,
`Docs/QA/ENGINE-WEEKEND-BASELINE-2026-10-17.md`, and the session field in
`Docs/Research/ENGINE-CAPABILITIES.json`. Dedicated repair packets own source.
Build output and full logs remain external to every source worktree.
No game/art branch integration, dependency installation, or unrelated changes.

## Acceptance and risks

Retain exact source/build/test/review/integration identities and failures.
Compile relevant native Debug and Release targets, run deterministic tests and
applicable serialized desktop smokes. Keep integrated-but-unverified, locally
verified and packaged/accepted states distinct. Unavailable tools or desktop
ownership block only dependent checks. Resource contention is an observed risk:
initial available RAM is approximately 9 GiB, so start at two build jobs and
serialize full builds. Preserve 6–8 GiB headroom where possible.

## Commands

Use the installed CMake/CTest under Visual Studio 2022 Community:

```powershell
$cmake = 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$ctest = 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe'
& $cmake -S <owned-source> -B <external-build> -G 'Visual Studio 17 2022' -A x64
& $cmake --build <external-build> --config Debug --parallel 2
& $ctest --test-dir <external-build> -C Debug --output-on-failure -E RuntimeSmoke --no-tests=error
& $cmake --build <external-build> --config Release --parallel 2
& $ctest --test-dir <external-build> -C Release --output-on-failure -E RuntimeSmoke --no-tests=error
```

Check every command exit and retain logs. RuntimeSmoke runs separately only on
an owned interactive desktop; no concurrent GUI invocation. Supplemental portable
and sanitizer commands follow `Tests/EngineRuntime/CMakeLists.txt` tool support.

## Stop and handoff

At deadline stop admitting packets, checkpoint owned work, and report merged vs
branch-only work, real workflow status, exact launch commands and remaining gates.
Checkpoint after each packet and before session limits. Supported chat heartbeat
continuation may resume this owner; do not change another worker's scheduler.
