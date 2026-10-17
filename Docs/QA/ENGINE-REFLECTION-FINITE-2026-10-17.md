# Floating reflection ingress verification

Owner `/root`; independent reviewer `/root/numeric_implementation`.
Base92e4880489d2478f2fa45fbb690fabd8caca63dd; source commit
`cad986de6b3ef61cabd6286df149e81e86446269`; branch
`engine/reflection-finite-20261017`, task of the same date/name.
Branch-only scoped repair; no main adoption.

## Old behavior and finite-range contract

Three public API regressions were added while Engine remained identical to the
base. Fresh VS2022 x64 configure and Debug EngineServicesTests build exited0.
Each filtered run then exited3 before production changes:

- `ReflectionRejectsUnrepresentableScalarWithoutMutation`: SetField accepted a
  finite1e100 for an unbounded float member instead of rejecting it.
- `SceneRejectsUnrepresentableReflectedRuntimeScalars`: actual SceneSerializer
  accepted an unrepresentable RigidBody.gravityScale (the fixed test also checks
  CharacterMover.gravity) instead of preserving the supplied document.
- `ReflectedBehaviourRejectsUnrepresentableScalarBeforeAttachment`: the named
  GameWorld AddBehaviour API attached a behaviour with that invalid float field.

The floating reflection writer now retains existing declared Range clamping,
checks the resulting finite value against the destination member type's limits
using wider comparison arithmetic, and only then casts/assigns. Unbounded
out-of-range results fail without mutating caller state. Values that become
valid through an explicit Range remain accepted. Double and extended member
limits are not narrowed to float/double for the check. Normal precision rounding
and underflow remain allowed. This bounds numeric ingress; it does not promise
that every representable float is a useful simulation setting or that later
arithmetic cannot overflow.

The tests cover both signs, exact float limits, adjacent double values just
outside those limits, denormal/zero, double maximum, positive/negative Range
clamping, transactional multi-field FromJson, actual scene components and named
behaviour attachment. No implementation marker assertions were introduced.

## Native results and commands

Observed weekend Windows11/Ryzen7950X3D, VS2022/MSVC19.44/SDK10.0.26100,
two jobs, external `work/build-reflection-finite`. All logs under external
`<session>/work/reflection-finite-evidence/`; no binaries/logs committed.

| Check | Result | Log |
|---|---|---|
| Fresh configure and old Debug Services build | exit0 each | `old-configure.log`, `old-build-Debug.log` |
| Three original filters on old source | exit3 each | `old-<full-test-name>.log` |
| Fixed Debug Services/Framework/Sample targets | exit0 | `fixed-build-Debug.log` |
| Debug CTest Services/Framework/Sample | 3/3 pass,2.32s | `fixed-test-Debug.log` |
| Same Release build/test | exit0,3/3 pass,0.29s | `fixed-{build,test}-Release.log` |
| Existing test-safety fixture | 3/3 pass,5.099s | `test-safety.log` |

Actual fixed command shape, with installed VS CMake/CTest paths from the weekend
packet, executed sequentially for Debug and Release:

```powershell
& $cmake --build work/build-reflection-finite --config $configuration --target EngineServicesTests EngineFrameworkTests EngineSampleTests --parallel 2
& $ctest --test-dir work/build-reflection-finite -C $configuration --output-on-failure -R '^Engine(Services|Framework|Sample)Tests$' --no-tests=error
& $python work/astral-reflection/Scripts/test_test_safety.py
```

Every exit was checked; tests retain the established safety helper/assertions.
No post-fix deterministic failure occurred. Full combined all-target/native
smokes and hosted sanitizers remain separate integration gates.

## Independent review and limits

Reviewer performed read-only source/test/task inspection of exactcad986d, found
no actionable issue, and ran no builds/tests or edits. Final QA/evidence binding
is pending this record's commit. Allowed paths are Reflection.h, Services tests
and task/QA only; no integer/vector/quaternion, Scene, UI, Assets or game edit.
Existing behavior and public method signatures are retained. No performance,
native input, GPU, packaging, soak or complete workflow acceptance claim.

Primary references: [C++ floating conversions](https://eel.is/c++draft/conv.double)
and [MSVC numeric limits](https://learn.microsoft.com/en-us/cpp/standard-library/numeric-limits-class?view=msvc-170).
