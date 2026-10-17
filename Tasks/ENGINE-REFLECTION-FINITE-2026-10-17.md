# Bound floating-point reflection ingress

Owner: weekend coordinator `/root`, branch `engine/reflection-finite-20261017`,
base92e4880489d2478f2fa45fbb690fabd8caca63dd. Independent reviewer:
`/root/numeric_implementation`. Lucas's October17 engine-only authority applies.

## Problem, scope and approach

Independent review found the floating FieldTraits writer accepts finite JSON
doubles outside float range and narrows them after only optional field-range
clamping. Unbounded RigidBody.gravityScale and CharacterMover.gravity can receive
infinity through valid scene JSON. This is a separate ingress path from F's input
bindings. Reproduce public TypeInfo and actual SceneSerializer failure before
production edits using neutral in-memory fixtures.

Own only `Engine/Core/Reflection.h`, `Tests/EngineServicesTests.cpp`, this packet
and matching `Docs/QA/ENGINE-REFLECTION-FINITE-2026-10-17.md`. No Scene/GameHost,
UI, Assets, game, policy or CMake changes. Preserve existing declared Range
clamping, then reject a still-unrepresentable floating result before assigning.
Finite double fields and float limits remain valid. Ordinary underflow rounding
is allowed; this packet prevents nonfinite narrowing, not arithmetic overflow in
later simulation. Failed SetField and FromJson must preserve caller state.

## Acceptance, commands and risks

Old native filtered regressions fail, then fixed Services/Framework/Sample suites
pass Debug and Release with active checks and unchanged safety registration.
No new executable or test registration. Verify scalar signs, exact float bounds,
just-outside bounds, declared range clamping, double range and transactional
multi-field rejection. Scene tests reject both scalar runtime examples while
retaining the last good output document. Broader integer/quaternion/vector
contracts and simulation value limits are separate.

External build `work/build-reflection-finite`, VS2022 x64, two jobs; serialize
compiler/native use with B/C/D. Installed CMake/CTest paths follow weekend packet.

```powershell
& $cmake -S <session>/work/astral-reflection -B <session>/work/build-reflection-finite -G 'Visual Studio 17 2022' -A x64
& $cmake --build <session>/work/build-reflection-finite --config Debug --target EngineServicesTests --parallel 2
& <session>/work/build-reflection-finite/Debug/EngineServicesTests.exe ReflectionRejectsUnrepresentableScalarWithoutMutation
& <session>/work/build-reflection-finite/Debug/EngineServicesTests.exe SceneRejectsUnrepresentableReflectedRuntimeScalars
& <session>/work/build-reflection-finite/Debug/EngineServicesTests.exe ReflectedBehaviourRejectsUnrepresentableScalarBeforeAttachment
& $cmake --build <session>/work/build-reflection-finite --config Debug --target EngineServicesTests EngineFrameworkTests EngineSampleTests --parallel 2
& $ctest --test-dir <session>/work/build-reflection-finite -C Debug --output-on-failure -R '^Engine(Services|Framework|Sample)Tests$' --no-tests=error
```

Repeat fixed affected builds/tests in Release, check each exit, preserve logs in
`work/reflection-finite-evidence`, independently review/freeze before scoped push.
Full combined framework and workflow acceptance remain separate gates.
