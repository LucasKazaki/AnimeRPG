# Guard console numeric destinations — October 17, 2026

Owner `/root/demo_runtime` (transferred from `/root/candidate_review`); independent reviewer `/root` or
`/root/numeric_implementation`. Dedicated worktree `work/astral-cvar-numeric`,
branch `engine/cvar-numeric-20261017`, exact base
`2c69888774c074a69ec12c5f659e3632280db420` (original Console API, no project
batch API). Lucas's October 17 engine-only authority applies; technical,
independent review and execution gates remain required.

## Problem, ownership and current boundary

Source inspection found that Float SetFromString accepts a finite double
such as 1e100, then stores it beyond the matching GetFloat destination range.
Void Set(double) can likewise store values outside the declared Int or Float
destination. SetRange may force an otherwise valid current value into an unsafe
range, and RegisterFloat can publish nonfinite defaults. The integral-float
spelling of the legitimate -2147483648 boundary is also rejected by an absolute
max-value check. Local OLD execution confirmed these on October18 at03:51:20Z.

Allowed paths only: `Engine/Core/Console.{h,cpp}`,
`Tests/EngineServicesTests.cpp`, this packet and
`Docs/QA/ENGINE-CVAR-NUMERIC-2026-10-17.md` (five paths). This owner remains the
sole Console/Services writer across explicitly separate packets. Keep the
eight-path project-transaction worktree frozen; do not silently add numeric
policy to it. No GameHost, Scene, input/math/physics/audio, editor/native, asset,
game content, CMake, dependency or policy edits here. The separate LoadProject
stale Ready/Error finding remains queued elsewhere.

Lucas released the desktop/GPU hold and moved the demonstration deadline to
October18 noon Eastern (16:00Z). The coordinator accepted local OLD receipts
from assembled source `afd4a21`, whose Engine bytes equal base2c698887, and
authorized the bounded production fix. All nine OLD failure filters aborted
with child exit3, before unsafe typed reads; both positive controls returned0.
Root retained exact commands/output in `work/cpu-repro-evidence/receipts.json`.
Configure returned0 in5.0s and the two-job Debug Services/World build returned0
in34.344s. Original Services file hash8b74ac7501cdfd263cb9806b17763611ef6c5b0161ac9f2203a468717aac06a0 matched.
The first corrected source passed affected Debug/Release suites, but independent
review found a Float archive boundary issue. A new safe SaveConfig/LoadConfig
fixture actually aborted with child exit3 on that first correction, at accepted
load=false before any destination getter (03:58:41Z). Nine-digit formatting wrote
3.40282347e+38 beyond exact FLT_MAX. Root accepted this failure and authorized
conditional17-digit formatting only when nine-digit text cannot parse safely or
crosses Float bounds. Ordinary display remains unchanged. Positive controls
cover 0.1, tiny finite doubles/Float subnormals, and both finite Float boundaries.
This final source was independently reviewed clean and passed fresh Debug/
Release Services/Framework/Sample3/3 (66 cases each), safety3/3 and M1-3 at04:10Z.
Exact source-before/after patchSHA256
`de048e3b7ef294e637ce6fa4edaca1386a506955ccede1509ad45bf7ecbfde9b`
matched. Logs are in `work/cvar-numeric-final-evidence`; final independent
logs/docs/commit binding remains required. Root schedules all execution slots.

## Approved bounded ingress contract

1. Normalize every numeric candidate through one path: reject nonfinite input,
   apply meaningful declared range clamp, preserve Int rounding/Bool
   normalization, then check the declared destination before publishing. Int
   limits include the full lower boundary; Float uses finite +/-FLT_MAX.
   Do not demand exact Float representation: normal rounding, subnormal values
   and underflow from a valid finite double remain permitted.
2. Parsed writes return false/error without mutation or notification if the
   final value is unsafe. Void Set(double) silently preserves its representable
   original and emits no change callback on rejection. A Float range [0,10]
   still accepts 1e100 as 10; do not reject before the useful range clamp.
3. Stage SetRange before changing range metadata. Endpoints must be finite and
   ordered, and the current value normalized under that range must remain safe.
   Reject the entire range/value change on failure. Wide finite endpoints can
   remain valid if effective values pass destination checks. Reset follows the
   same safe ingress and configured range policy.
4. Reject a new invalid Float default with nullptr and no published variable.
   Preserve existing same-name/type lookup semantics, where a new ignored
   default does not replace a valid registered variable. Deferred invalid
   config must leave the new representable default intact.
5. Preserve API signatures, permissions, callbacks, valid typed values and
   config behavior. No general getter redesign or new semantics for callers
   asking a mismatched typed getter of another CVar type. Project batch
   preparation will later reuse this ingress through a reviewed integration.

## Safe tests and acceptance

Old fixtures must first assert false/null or zero notifications. They may read
the matching numeric getter only after the assertion proves the valid original
was preserved. Callback bodies count changes without casting the candidate.
Never invoke GetFloat on an old out-of-range Float or GetInt on an old unsafe
Int, even to print diagnostics. The old defaults fixture checks nullptr before
any getter. Original native failures must be retained separately before any
production edit; stop/report each expected or unexpected result.

For the deferred Float config path, GetString formats the stored double directly
without narrowing; compare that to the representable default before any
GetFloat call. Do not apply that witness to Int GetString, whose formatter
converts to an integer and could itself be unsafe on old out-of-range storage.

Prepare filters for Float parsing, direct Int and Float Set, unsafe SetRange,
invalid Float registration and the valid Int lower-bound spelling. Positive
controls preserve finite declared-range clamping, Float underflow/rounding and
ordinary Int rounding. After clearance, verify direct setters, console/config
consumers, deferred config/default/reset and both valid boundaries under
scheduled Debug/Release Services checks, with adjacent project checks only
after reviewed integration. Safety/static gates require coordinator slots.

Exact planned commands, from the owned worktree, only after the hold releases:

```powershell
$cvarCmakeBin = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin'
& "$cvarCmakeBin/cmake.exe" -S . -B ../build-cvar-numeric -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON
& "$cvarCmakeBin/cmake.exe" --build ../build-cvar-numeric --config Debug --target EngineServicesTests --parallel 2
& '../build-cvar-numeric/Debug/EngineServicesTests.exe' CVarFloatParsingRejectsUnrepresentableBeforeRead
& '../build-cvar-numeric/Debug/EngineServicesTests.exe' CVarDirectIntSetRejectsUnrepresentableBeforeRead
& '../build-cvar-numeric/Debug/EngineServicesTests.exe' CVarDirectFloatSetRejectsUnrepresentableBeforeRead
& '../build-cvar-numeric/Debug/EngineServicesTests.exe' CVarRangeRejectsUnsafeEffectiveValuesBeforeRead
& '../build-cvar-numeric/Debug/EngineServicesTests.exe' CVarRegistrationRejectsNonfiniteFloatDefaultsBeforeRead
& '../build-cvar-numeric/Debug/EngineServicesTests.exe' CVarIntAcceptsTheFullLowerBoundarySpelling
& '../build-cvar-numeric/Debug/EngineServicesTests.exe' CVarConfigRejectsUnrepresentableFloatBeforeRead
& '../build-cvar-numeric/Debug/EngineServicesTests.exe' CVarDeferredFloatConfigKeepsRepresentableDefaultBeforeRead
& '../build-cvar-numeric/Debug/EngineServicesTests.exe' CVarRangesRejectNonfiniteBoundsWithoutLosingTheOldRange
```

Retain exact argv/cwd, stdout/stderr, actual child exits, source patch, machine
and times externally in `work/cvar-numeric-evidence`. No push/merge or runtime,
GPU, performance, package or soak claim by this packet. Independent final
source/docs/log/commit review is required before adoption.

Primary language references read October 17:
[floating to integer conversion](https://eel.is/c++draft/conv.fpint) and
[floating destination conversion](https://eel.is/c++draft/conv.double).
These motivate checking destination range before a getter conversion; they do
not establish any test result. The implementation must remain C++17.
