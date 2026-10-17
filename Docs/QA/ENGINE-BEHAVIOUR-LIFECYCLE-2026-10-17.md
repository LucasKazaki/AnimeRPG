# Behaviour component teardown verification

Owner: `/root`; independent reviewer `/root/numeric_implementation` pending.
Base `92e4880489d2478f2fa45fbb690fabd8caca63dd`, source commit
`63fe8ebd426c3ffd2530cbe21b974c965dbf962f`, task
`Tasks/ENGINE-BEHAVIOUR-LIFECYCLE-2026-10-17.md`. Branch-only candidate; not main.

## Reproduction and repair

The three original tests were written before production changes. VS2022 x64
configure and Debug EngineWorldTests build exited0 with Engine unchanged from
the base. Every test then exited3 as expected:

| Safe public-API fixture | Observed original result |
|---|---|
| Remove Behaviours while entity remains alive | destroyed0, disposed1, timer fired1 |
| Replace Behaviours with an empty component | destroyed0, disposed1, timer fired1 |
| Earlier OnUpdate removes another entity's Behaviours | destroyed0, disposed1, removed behaviour updated1 |

Counters are shared external objects; these tests never deliberately dereference
freed memory. The surviving timers nevertheless demonstrate the lifetime bug
that could affect a callback capturing its deleted behaviour.

GameWorld now detaches the component before callbacks, marks the entire shared
snapshot destroyed before any callback, cancels the entity's timers, then invokes
OnDestroy exactly once for each created behaviour. Entity destruction uses the
same helper. A generation-scoped teardown guard makes recursive removal harmless
and rejects same-entity attachment during teardown: typed reference-returning
APIs throw invalid_argument; the named API returns null with a diagnostic.
Attachment to another entity remains allowed, and the guard ends on scope exit.
Destroyed Behaviour Invoke/InvokeRepeating return null handles.

This is a deliberate GameWorld API contract, not a change to raw Registry
mutation. Callback exceptions and direct manipulation of Behaviours::list remain
outside this packet. Entity-owned timers are shared by the behaviour component;
removing the component cancels all timers carrying that entity owner.

## Native execution

Same observed weekend machine: Windows11, Ryzen7950X3D, VS2022/MSVC19.44,
installed SDK10.0.26100, external `work/build-behaviour-lifecycle`, two jobs.
Logs are external under `<session>/work/behaviour-lifecycle-evidence/`.
Every exit was checked; no tests/assertions were disabled.

| Commands/result | Exit/evidence |
|---|---|
| Old configure and Debug EngineWorldTests build | 0, `old-{configure,build}.log` |
| Three original filtered tests | 3 each, `old-<full-test-name>.log` |
| Debug build targets EngineWorldTests EngineFrameworkTests EngineSystemsTests EngineSampleTests | 0, `fixed-build-Debug.log` |
| Debug CTest `-R '^Engine(World\|Framework\|Systems\|Sample)Tests$' --output-on-failure --no-tests=error` | 4/4 pass,45.36s, `fixed-test-Debug.log` |
| Same Release targets and CTest | 0,4/4 pass,1.03s, `fixed-{build,test}-Release.log` |
| Release filtered EngineWorldTests `Behaviour` | six lifecycle cases pass, `fixed-focused-world.log` |
| Existing Scripts/test_test_safety.py with installed CMake on process PATH | 3/3 pass,5.316s, `test-safety.log` |

The fixed six cases also cover recursive entity destruction during replacement,
all three same-entity attachment APIs, timer creation during OnDestroy, attachment
to another entity, later reattachment, and removal of a pending uncreated
behaviour retained by a shared reference. The latter must never create/start.

Exact command shapes are in the task; CMake/CTest use the bundled VS path from
the weekend coordination packet. No post-fix deterministic failures occurred.
Full combined framework all-target builds, complete suites and native smokes
remain a later integration gate; this packet ran its four affected suites.
Hosted sanitizers and exact proposed-main verification are pending.

## Review and limits

The source is frozen for independent review. No clean review, push or integration
is claimed yet. Broader Rigidbody synchronization, reflected numeric ingress,
prefab expansion budgets and sparse glTF integer accessors were separate source
review findings; they are not repaired here. Editor/GPU/package/soak acceptance
and the complete development workflow remain open.

The existing shared snapshot mechanism follows documented shared ownership
semantics; object lifetime alone does not prevent a logically destroyed object
from receiving callbacks, which is why the explicit destroyed state is needed.
Primary references: [MSVC shared_ptr](https://learn.microsoft.com/en-us/cpp/standard-library/shared-ptr-class?view=msvc-170)
and [MSVC RAII/smart pointers](https://learn.microsoft.com/en-us/cpp/cpp/smart-pointers-modern-cpp?view=msvc-170).
