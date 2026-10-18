# Project ingress evidence

Owner `/root/demo_runtime`; Task
`Tasks/ENGINE-PROJECT-INGRESS-2026-10-18.md`; exact base
`1804a3cba77dace7eb04e7f14c052e796adb5128`.

OLD phase: four actual failures and three passing controls, before any
production correction. The coordinator granted an exclusive OLD-only CPU
slot after reviewing the plans. Fresh available RAM was12.784GiB. Configure
returned0 (3.703s), then Debug EngineSampleTests built with two jobs, exit0
(32.609s). Every filter ran separately on unchanged production1804a3c.

| Filter | Actual child exit and observation |
| --- | --- |
| ProjectLoadRejectsCachedSyntaxFailure | 3 at `!accepted`; cached Ready=true, Version1, Attempt2, real syntax Error, yet LoadProject returnedtrue and applied Candidate |
| ProjectLoadRejectsCachedDeletion | 3 at `!accepted`; cached Ready=true, Version1, Attempt2, cannot-open Error, yet LoadProject returnedtrue and applied Candidate |
| ProjectPositiveFloatBoundaryLoads | 3 at `accepted`; exact +FLT_MAX JSON was rejected by config.project.edge before any Float getter |
| ProjectNegativeFloatBoundaryLoads | 3 at `accepted`; exact -FLT_MAX JSON was rejected by config.project.edge before any Float getter |
| ProjectCurrentCachedDataAndRecoveryLoad | 0; current good cache and successful recovery publication loaded |
| ProjectOrdinaryFloatUnderflowAndClampingLoad | 0; ordinary0.1, tiny double, Float subnormal and both useful range clamps loaded |
| ProjectOutOfRangeFloatRejectsWithoutMutation | 0; both truly out-of-range values rejected while prior state remained |

Exact argv/cwd, times/exits, stdout/stderr and source/cache binding are retained
in `work/project-ingress-evidence`; external runner
`work/run_project_ingress_old.py`. Engine production diff versus1804a3c was zero
before and after. Source/test fixture patchSHA256 stayed
`673780a718006037976a6ce0cc7484c2a11461b419f1c92c55228b73dc1444da`;
complete Sample fixture file-byte SHA256 stayed
`b331ee7f9a2bdde36f0b3d1b315cf8f5f74f91537548de0474b3be63d54901c9`.
Each OLD failure stopped at its intended safe assertion, so later fixed-acceptance
assertions were not observed. Failed-child fresh roots remain identified in the
retained stdout; successful roots clean only their own reserved directories.
The wrapper completed0, the CPU slot was released, and no affected compiler/test
process remained. Root read/accepted the actual safe failures and cleared the
bounded four-path correction. LoadProject now checks both Ready and current
Error, matching LoadScene while leaving last-good asset data/version intact.
ConfigValue delegates numeric text to Core::WriteJson's existing shortest-round-
trip formatter. Bool/string behavior and guarded numeric destination/clamping
policies remain unchanged. All seven original fixtures are retained, with extra
preserved metadata assertions behind the safe failed-load predicate. This final
source was independently reviewed clean and admitted for a first fixed attempt.
Its configure/build passed (1.047/9.500s), but Debug CTest stopped with exit8
after Services28 and Framework21 passed: the unchanged
ProjectFilesParseAndReportErrors assertion at Sample line204 failed because a
numeric config spelling no longer equaled"1". Core::WriteJson's default pretty
mode appends a newline. No Release/safety/milestone gates ran in that attempt.
Actual logs and identical stopped source binding315e496b88f15c187e527b68f3fa4dfc634b1ef3b949850738c6f3e872c9f395
remain in `work/project-ingress-final-evidence`; no record was overwritten.

Root read the actual failure and formatter implementation, accepted the cause,
and cleared the one-line compact JsonWriteOptions{false} correction. The original
spelling assertion and all seven fixtures remain unchanged. Corrected source
was independently reviewed clean, then admitted for a new, separately logged
fixed attempt. The failed attempt's CPU slot was released with no affected
process remaining.

The corrected fixed attempt completed0 in
`work/project-ingress-corrected-evidence`; external runner
`work/run_demo_runtime_fixed.py project-ingress --corrected` created that new
directory exclusively, preserving every first-attempt receipt. Fresh available
RAM was13.177GiB, all builds used two jobs and disabled MSBuild node reuse, and
the attributable cache remained `work/build-project-ingress-old`, with exact
CMAKE_HOME_DIRECTORY matching this worktree. Installed CMake was prepended only
to the runner child's PATH. Actual configure returned0 in1.640s.

| Corrected gate | Actual result |
| --- | --- |
| Debug affected build | 0,4.078s |
| Debug anchored CTest | 3/3 suites,81 cases,0 failures,3.079s |
| Release affected build | 0,56.328s |
| Release anchored CTest | 3/3 suites,81 cases,0 failures,0.562s |
| Test safety | 3/3,0 skips,exit0,5.422s runner time |
| Milestone1,2,3 | each exit0,0.047/0.062/0.063s |

Each configuration explicitly reported Sample32, Services28 and Framework21
cases passed. All seven ingress fixtures and the unchanged existing numeric
spelling assertion passed. The source/test patch versus1804a3c remained exactly
`65e25fdf2b95cecc785c403fad31cc8220eb19d96bb30b5555a7fe89e8981e58`
before and after. Receipts retain exact argv/cwd/start/elapsed/exit, cache HOME
and hash, memory, stdout/stderr, and source bindings. Session39653 completed0;
the exclusive CPU slot was explicitly released after an empty affected
compiler/test process check. Task/QA updates followed execution; no production
or fixture edit occurred during it.

No GUI/GPU, package, candidate adoption or demo claim is made. Existing reviewed
transaction/numeric/physics source and tests remain intact. This four-path
packet is commit-ready, pending coordinator review; nothing has been pushed.
