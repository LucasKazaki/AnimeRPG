# Reusable engine project packaging evidence

Task: Tasks/ENGINE-PROJECT-PACKAGE-2026-10-18.md. Owner `/root`.
Status: five-path script/test/documentation source has passed sixteen local
filesystem, process and parser controls after independently identified defects
were reproduced and fixed. Real engine packaging remains unexecuted.
No package build, staging, runtime launch or acceptance exists yet. Existing R0/game-specific
packagers remain unchanged and uninvoked. The generated project and Runtime-only
native player must be independently verified and assembled before actual staging.

Independent plan review by /root/demo_runtime identified evidence-directory
overlap, whole-Content reinventory, actual project.json defaults, Windows names
and fail-closed x64/import parsing. The source now implements these boundaries.
Independent code/test review found two dependency-parser omissions. Before the
production fix, actual tests for a dependency containing a space and a DLL named
Summary.dll both failed because PackageError was not raised (child exit1,
two failures). The strict parser now walks every dependency line, requires an
exact Summary heading and rejects unknown or malformed entries. Fourteen local
controls then passed, exit0, no skips (0.359s unittest duration).

Further independent review found that a directory named project.json satisfied
membership, and dot/superscript device names were accepted as relative names.
Safe old-source tests retained seven string failures and one directory-case
error (child exit1); the absent-project control already passed. No device path
was created and no CMake process ran: the directory case incorrectly reached
the deliberately absent tool path. Production now requires a regular project
file, nonempty path components and the superscript COM/LPT device variants.
The full corrected suite passed **16/16**, exit0, no skips, in0.402s.

Commands used the bundled Python3.12 with `-B` and
`Scripts/test_package_engine_project.py`, with explicit test filters for the old
failures and the full suite afterward. External evidence is under
`work/package-engine-evidence`: parser-old.log/receipt, unit-first.log,
preflight-old.log and unit-final.log. Actual controls exercise ordinary copies,
empty directories, byte manifests/tamper detection, changed inputs, preserved
existing output, file/directory symlink and hard-link refusal, Content additions
and removals, overlap/budgets, dependency parsing and owned-child timeout/reaping.
Only source binding is substituted in the missing-project preflight control;
no mocked build or PE output constitutes engine package acceptance.

Independent runtime-owner final review of all five paths, retained failures,
final16-case log and source bindings is clean. Reviewed production physical
SHA256 fa113da2734c63d182b73b1853293e56c051f800293521be8516e1a2c5aa5f82;
tests24d764c4b12e648d71559036ff90f135a4eb3d89c7c5c0de95c18718754ac949.
Root subsequently ran the unchanged safety and milestone tools under a separate
fresh-memory serial CPU admission, with CMake on process-local PATH, two jobs and
node reuse disabled. Safety3/3 passed (5.594s unittest time); milestones1–3 all
returned0. Exact bounded child receipts/logs and identical script pre/post hashes
are under work/package-engine-evidence/safety-first. No compiler/test process
remained. Commit binding and real package gates remain pending.

Record real source/commit/binary identities, commands/exits, negative controls,
PE imports, byte manifest, outside-repository launch and observed captures here.
No clean-machine or24-hour soak result can be inferred from this record.
