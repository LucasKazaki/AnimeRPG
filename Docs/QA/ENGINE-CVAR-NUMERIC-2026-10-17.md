# Console numeric ingress QA — October 17, 2026

Owner `/root/candidate_review`; worktree `work/astral-cvar-numeric`, base
`2c69888774c074a69ec12c5f659e3632280db420`.
Packet: `Tasks/ENGINE-CVAR-NUMERIC-2026-10-17.md`.

Source-only old-test preparation. Float parsing, direct numeric setters,
SetRange and default registration have candidate safety gaps; no local
reproduction has executed. Tests assert rejection/callback count/null before
any getter could cast an unsafe stored candidate. Language references and
the coordinator-approved consistent destination/range contract are in the task.

Eleven existing-API cases are prepared: six core old predicates for parsing,
direct Int/Float writes, SetRange, defaults and the legitimate Int lower-bound
spelling; three real config/deferred/range consumers; two positive controls for
clamping/rounding/underflow and ignored duplicate defaults/reset. The deferred
Float witness compares GetString's direct double formatting with its valid
default before any matching numeric getter; Int GetString is never used on
unsafe old state. No old failure is claimed until the filter actually runs.

Frozen `Tests/EngineServicesTests.cpp` file-byte SHA256 is
`8b74ac7501cdfd263cb9806b17763611ef6c5b0161ac9f2203a468717aac06a0`.
This identifies the complete fixture file, not a git diff/source-patch hash.
Console production remains unchanged. The separate project transaction branch
remains frozen and is not this packet's base.

The coordinator authorized supplemental hosted OLD reproduction on a separate,
root-owned receipt branch from the same base, combining these unchanged eleven
cases with separately reviewed Physics fixtures. Hosted execution has not yet
run. This WIP commit records only OLD fixtures and task/QA preparation; it does
not claim a reproduced defect or repaired production. Hosted evidence must
identify the exact checkout, source and individual child exits. The nine OLD
filters listed in the task are expected to fail with child exit 3; that is an
unobserved expectation. Loops stop at their first failed assertion, so an OLD
filter receipt cannot establish every later loop variant.

The two OLD positive controls are expected to return 0, also unobserved:

```powershell
& '../build-cvar-numeric/Debug/EngineServicesTests.exe' CVarNumericIngressKeepsUsefulClampRoundingAndUnderflow
& '../build-cvar-numeric/Debug/EngineServicesTests.exe' CVarExistingFloatRegistrationIgnoresAnUnusedInvalidDefault
```

Lucas requested desktop/GPU availability, and the coordinator's RAM hold pauses
all new local build/test slots until explicit release. No local compilation,
tests, native/GPU work, source repair, push, hosted result or adoption result is
claimed. Supplemental hosted execution does not waive pending local OLD/fixed
gates. Actual OLD receipts and explicit clearance must precede production changes.
