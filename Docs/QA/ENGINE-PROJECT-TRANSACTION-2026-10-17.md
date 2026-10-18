# Project transaction QA — October 17, 2026

## Current integration status — October18

Owner `/root/demo_runtime`; new source-only worktree `work/astral-project-demo`,
branch `engine/project-transaction-demo-20261018`. Base
`3f6371c510a2bc3f7e969acded0ceff135bb153b` combines complete reviewed CVar
63d7230 and Physicsba89029 branches above2c698887. Their original fixtures and
corrections are retained. The original Project worktree/proof remains untouched;
its source/test patch still equals0824ff6f50811b77e451ce6245461c5d2ccacaf852d07201c8b240d0c68f8fed.

Transaction production hunks were adopted unchanged. Batch probes now naturally
reuse the reviewed private numeric normalization and safe config formatter.
The Services footer conflict was manually concatenated, retaining all25 existing
numeric/base cases plus all3 Project cases without duplicate test names.
Four finite invalid typed values were added to batch rejection controls; no
assertion was weakened. Sample transaction fixtures remain unchanged, and World
production/tests are byte-identical to the reviewed Physics branch.

Root independently reviewed the integrated source clean and granted an exclusive
CPU slot. Fresh available RAM was12.680GiB. VS2022 x64 configure returned0
(3.860s), and two-job Debug/Release builds of Services/World/Framework/Sample
returned0 (43.750/52.391s). Four anchored suites each passed: World31, Services28,
Framework21, Sample25, totaling105 cases per configuration (58.687/1.203s).
The actual failures/recovery, callback ordering, settings-only hook, aliased
project, original numeric/physics cases and finite batch guards all passed.
Safety3/3 passed in5.359s and milestone1/2/3 each returned0. Exact argv/cwd,
times/exits, external cache binding and separate stdout/stderr are retained in
`work/project-demo-final-evidence`. Source-before/after patchSHA256 matched:
`a288912bd0996920ec9fdb1e9c904f48561be6c66498e3ef697367da8576d37a`.

The wrapper completed0, the CPU slot was explicitly released, and a read-only
process inventory found no cl/cmake/ctest or affected test process. Final
independent logs/docs/immutable commit binding is pending. No native/GPU or
package acceptance is inferred. Original actual OLD failures below remain
preserved. Lucas released the desktop/GPU hold and set noon Eastern as the
demonstration deadline; root still schedules every execution slot.

The historical preparation below retains its original owner/base and pending
hold statuses; the current integration status above supersedes those fields.

## Original preserved packet

Owner `/root/candidate_review`; dedicated `work/astral-project-transaction`.
Base `5245a3dd079083a534b4afd16924a6a5496d1a0d`.
Packet: `Tasks/ENGINE-PROJECT-TRANSACTION-2026-10-17.md`.

Source inspection found that ApplyProject mutates config/input/metadata before
its startup scene can fail, and config values invoke callbacks sequentially
before later config validation. The coordinator approved the extra
Console/Services paths and granted a serialized old-source slot after the
audio packet released its compiler/test work. The editor core/native owner
retains its paths; only narrow GameHost ownership was transferred.

## Original real-host reproductions

On `DESKTOP-AKPLL81`, October 17, 19:16:55–19:17:48 UTC, Engine production was
unchanged from the exact base throughout:

| External `work/project-transaction-evidence` receipt | Result |
| --- | --- |
| `old-configure` | Exit 0, 4.109 s; VS2022 x64, external cache, BUILD_TESTING=ON |
| `old-debug-build` | Exit 0, 37.625 s; EngineSampleTests target only, two jobs |
| `old-scene-failure` | Actual child exit 3, 0.047 s; project metadata preservation failed |
| `old-config-failure` | Actual child exit 3, 0.016 s; CVar preservation failed |

The scene failure snapshot printed `worldSame=1 project=Candidate quality=0
observed=9 notifications=2 moveDown=0`: old world survived with new metadata,
changed CVars and cleared input state. The config failure printed
`worldSame=1 project=Original quality=0 observed=9 notifications=2 moveDown=1`:
the late unknown CVar preserved world/input but earlier settings and external
callback effects were already applied. Both are genuine public GameHost calls.
Original disposable empty scenes/projects only; no game content was changed.

Each filter ran in a separate command. The external exact-argv evidence helper
retains the actual child exit 3; PowerShell reported the nonzero wrapper result
as exit 1. Stdout/stderr, exact argv/cwd, source identity/patch, machine and
UTC/elapsed times accompany each receipt. The test-only Engine/Tests patch was
SHA256 `3c180a27f3ecb3173c8a18b091d0a4bf5ca63f555f6808342abb6c54930a299b`.
Post-run production diff was zero, and no compiler/linker/CMake/CTest/Sample
process remained when the coordinator was notified of slot release.

Exact commands are those in the task, invoked through bundled Python and
external `work/project-transaction-evidence/run_logged.py`. Both old receipts
were reported before any production edit. The coordinator accepted both old
failures and cleared production within the eight approved paths.

## Fixed source prepared; execution pending

ConsoleRegistry now provides pure ValidateBatch and atomic SetBatch using
private callback-free variable probes. Every entry's type/range/permissions is
validated before any live write; duplicate case-insensitive aliases use the
last valid value. All values commit before callbacks, once per changed target.
Existing single Set, registered variable identities and callbacks remain.
String values are staged directly and swapped at commit; no config serialization
roundtrip or shadow registry is used. Callback exceptions and reentrant
registry-lifetime/host mutation remain outside the bool-failure guarantee.

GameHost copies project metadata, stages input and validates config, then resolves
the complete startup SceneDocument/dependencies/watch snapshot before settings
publication. SetBatch can fail only before any CVar write. Successful config
callbacks run before staged input/metadata installation and scene activation,
so OnCreate sees final services/settings. The public ApplyProjectSettings hook
matches the editor adapter signature and commits validated settings without a
scene load. Existing C/D contained reads, attempted-dependency failure watches,
strict image policy and scene activation are preserved in the candidate.

Added fixed-source cases cover callback-free rejection, all CVar types/ranges,
last duplicate aliases and single-change notifications, settings-only failure
and success, invalid input, aliased project metadata, and OnCreate's actual
host/settings observations. The original eight scene/config failure variants
and recovery remain in the two old filters.

No fixed build or test has run. Lucas requested the desktop/GPU stay free; the
coordinator paused all new local build/test slots for RAM headroom at 19:19 UTC.
Source-only review can proceed. Fixed native gates, safety/verifiers and final
commit/evidence binding require a later explicit resource-cleared slot. These
source descriptions are implementation intent, not tested-behavior claims.

This preparation record makes no fixed behavior, native, performance, package,
GPU, soak, independent review, hosted or merge claim. Executed commands/results
and final evidence binding will be recorded after scheduled gates.
