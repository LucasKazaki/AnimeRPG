# Console numeric ingress QA — October 17, 2026

Owner `/root/demo_runtime`; worktree `work/astral-cvar-numeric`, base
`2c69888774c074a69ec12c5f659e3632280db420`.
Packet: `Tasks/ENGINE-CVAR-NUMERIC-2026-10-17.md`.

## Current local status — October18, 03:51Z onward

Root executed the original frozen fixtures from assembled `afd4a21`, whose
Engine bytes were verified identical to2c698887. Configure0 (5.0s), two-job
Debug Services/World build0 (34.344s). All nine CVar OLD failure filters in the
task ran individually and aborted with actual child exit3. The two positive
controls each returned0. Exact argv/cwd/time/exits and separate stdout/stderr
are retained in `work/cpu-repro-evidence/receipts.json`; wrapper
`work/run_demo_old_cpu.py`. Each failure stopped before an unsafe typed read.
The OLD Services file-byte SHA256 below matched before execution. These local
receipts supersede the historical pending-hosted plan retained below.

After accepting those receipts, root authorized production changes. A private
normalization path now clamps meaningful ranges, rounds Int/normalizes Bool,
then validates destination bounds before publication. Parsed writes report
failure; void writes preserve value/callback count; SetRange stages metadata
and effective value; new nonfinite Float defaults are rejected after existing
name/type lookup. Reset and deferred config reuse guarded ingress. Int parsing
avoids a narrowing cast before validation, including the full minimum boundary.

The initial correction (source/test patchSHA256
c17159fef640039881e975afe39e23020fac06a64cc502a0e715487e9476d1ab)
passed two-job Debug/Release builds0 (36.406/48.360s), affected Services/Framework/
Sample suites3/3 (2.640/0.422s). Source-before/after hashes matched.
The first safety attempt failed before compiling fixtures with WinError2 because
the external runner omitted CMake from child PATH; logs remain untouched in
`work/cvar-numeric-fixed-evidence`. After reporting and root authorization,
prepending only installed CMake to runner child PATH made safety3/3 pass in4.906s
and M1-3 each returned0. These corrected gate receipts and identical source
binding are in `work/cvar-numeric-fixed-corrected-gates-evidence`.

Independent root review then found a config boundary concern. A new safe
Archive SaveConfig/LoadConfig filter actually aborted on this initial correction
at03:58:41Z, child exit3: it saved FloatBoundary=3.40282347e+38, then LoadConfig
returnedfalse with out-of-float-range error before a destination getter. The
Debug Services rebuild returned0 (2.047s). Actual outputs, commands, fixture/
production hash are retained in `work/cvar-archive-old-evidence`. This receipt
establishes the positive boundary; the abort did not reach its negative loop.

Root accepted the failure and authorized conditional17-digit Float formatting
only when ordinary9-digit output fails safe parsing or crosses Float bounds.
Bounds stay unchanged. Thirteen numeric cases now retain all original eleven,
add wide finite range/reset and archive round-trip cases, with huge parsed Int
clamps, both Float endpoints, ordinary0.1 display, tiny doubles and Float
subnormal controls. Root independently reviewed the final source clean, then
fresh fixed native gates passed on October18 at04:10Z. Debug/Release Services25,
Framework21, and Sample20 cases all passed:66 cases across3/3 suites per
configuration (2.687/0.453s). Configure0 (1.062s), two-job builds0 (5.641/5.719s),
safety3/3 passed (5.391s), and M1-3 all returned0. Both Float extrema were
observed saving17-digit text and reloading successfully; all three finite
positive controls passed. Available RAM before admission was13.043GiB.
Receipts and separate outputs are in `work/cvar-numeric-final-evidence`.
Source-before/after patchSHA256 matched
`de048e3b7ef294e637ce6fa4edaca1386a506955ccede1509ad45bf7ecbfde9b`.
The coordinator still must review final logs/docs and bind an immutable commit.
No GPU, native workflow, package or demonstration result is claimed. Lucas
released the desktop/GPU hold, but root schedules every execution; the CPU slot
was explicitly released before the formatter source edit.

## Historical OLD fixture preparation

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
This hash describes OLD fixture preparation, not the corrected file with added
acceptance cases. The separate project transaction branch remains frozen and
is not this packet's base.

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

At that earlier checkpoint Lucas's desktop/GPU and the coordinator's RAM hold
paused local execution. The local OLD receipts and release above supersede
that status; no supplemental hosted result is claimed. Fixed native gates and
independent final source/docs/log/commit review remain required before adoption.
