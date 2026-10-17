# Current-main Windows CI provenance evidence

Owner `/root`; independent reviewer `/root/containment_implementation`.
Task: `Tasks/ENGINE-CURRENT-CI-PROVENANCE-2026-10-17.md`.
Dedicated base126e6cfafedc47d1a2266f1adf27d6b23293f329.

## Source and local inspection

Current main had one conventional PR merge checkout. The existing reviewed
PR83/6a1ddc implementation actually executed separate head/merge jobs and a manual
single-commit job, all passing. This packet ports that prefix only; PR52 and
its downstream historical recovery/Job Object changes are not adopted.

At2026-10-17 19:06 UTC, the exact reviewed prefix was copied through the boundary
immediately before `Set external build root`. Every downstream current-main byte
was compared and remains unchanged. The actual extracted provenance script
parsed with the installed PowerShell AST with zero errors. The external copy
is `work/current-ci-evidence/provenance.ps1`. `git diff --check` passed. Only the
workflow and this task/QA are allowed. No local compiler, test executable,
native window, recovery run or editor/GPU process was started for this check.

The prefix validates event/revision shape, exact checked-out HEAD and ordered
base/head parents for a merge; it prints identities and a job summary receipt.
Pull requests execute head and merge independently with fail-fast false;
push/manual events execute the exact event commit. The checkout has depth2 so
the merge's parent identities are available. Metadata enters through environment
values and must match40-hex commit identities. Existing build/test steps remain.

## Pending gates

Actual diff/source independent review, committed binding, hosted PR head and
merge jobs, manual commit job and downloaded provenance logs are still pending.
Do not infer that a local syntax check executed the workflow. Historical R0
remains unadmitted; ordinary existing runner-fixture steps stay unchanged.
No framework/editor, native/GPU, package or soak acceptance is established here.

Primary behavior references:
[GitHub pull_request event semantics](https://docs.github.com/en/actions/reference/workflows-and-actions/events-that-trigger-workflows#pull_request)
and [actions/checkout HEAD example](https://github.com/actions/checkout#checkout-pull-request-head-commit-instead-of-merge-commit).
