# Finite action input — October 17, 2026

Owner: sole finite-input implementation agent; weekend engine lead coordinates
all builds and native execution. Worktree:
`C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/astral-input`.
Branch: `engine/input-finite-20261017`, deliberately from **unmerged PR #80**
`efac9db54e8f2781c20fbb1bb9be9a0ab85f4f5c`.
Authority: Lucas's pasted October 17 weekend engine assignment, repair F.
The current instruction authorizes scoped source/tests/docs and commits;
older per-change/local-only approval limits are superseded within that scope.
Independent review and applicable technical gates remain. No game content.

## Problem, evidence and dependency

Two JSON Axis2D bindings each `[3e38, 0]` are accepted by `ParseInputMap`.
`InputSystem::Update` adds them as floats, then computes a float length; a
simultaneous key press can overflow and normalize infinity into NaN. Even
one sufficiently large finite vector loses its direction. `JsonValue::AsFloat`
clamps an out-of-float-range number such as `1e100`, hiding invalid input data.
Direct `AddContext` accepts nonfinite axes and timing values. Accumulated
`heldSeconds` and `sinceLastPress` can overflow on repeated finite large dt.

The coordinator has cleared the actual PR80 full native Debug baseline,
31/31 deterministic suites. The previous assets packet is isolated and frozen;
its tests do not prove this input repair. No source changes or compiler run
precede the neutral real-host regression for the exact audit defect.

## Allowed paths

- `Engine/Input/InputSystem.{h,cpp}`
- `Engine/Framework/GameHost.cpp`, input JSON parsing only
- `Tests/EngineSystemsTests.cpp`
- `Tests/EngineSampleTests.cpp`, neutral host fixtures only
- This packet and `Docs/QA/ENGINE-FINITE-INPUT-2026-10-17.md`

Preserve C++17, current input/context framework, existing `astral_add_test`
registration, and always-active assertions. No UI routing, Assets, Math,
game-content, CMake, workflow, policy or dependency edits.

## Approved finite/range contract

JSON axes must be numeric, finite and in the representable float range before
narrowing. Retain ordinary rounding and finite large values rather than clamp
their contribution. Existing JSON timing range `[0, 10]` remains. Make direct
context insertion return bool with an optional diagnostic and validate the
entire context transactionally: finite axes,
nonnegative finite timers, no partial context/state mutation on failure.
Permit the full finite float range through the direct API.

Accumulate axis components in double, normalize using a widened stable length,
then narrow only bounded components. Preserve the ordinary magnitude-below-one
behavior, direction, cancellation, priority and consumed-key semantics. Treat
nonfinite/nonpositive dt as zero as before. Saturate nonnegative elapsed timers
at the largest finite float, using widened arithmetic; countdowns stay finite.
Private prior-press/elapsed-range metadata prevents first-ever double taps and
prevents an actually expired tap/double-tap interval from qualifying merely
because its elapsed float saturated. Verify behavior through existing public
axis/hold/edge/tap/buffer APIs; do not add a test-only state accessor.
Prior-press history ages exactly once for every stored action on every Update,
including disabled or shadowed contexts. A short disabled interval preserves
history; an interval beyond the window expires it. Detect range overflow by
comparing the smaller operand with `FLT_MAX - larger` near the limit, so a
small elapsed interval followed by FLT_MAX cannot disappear through rounding.
Exactly FLT_MAX remains a valid, finite window boundary.
The narrow `ApplyProject` call site stages validated contexts before its
existing assignment/configuration path, so direct invalid contexts surface a
diagnostic and preserve the previous project/input. Broader scene-load and
configuration transaction behavior is outside this numerical packet.

Inspect rebind/load-bindings and recordings for relevant finite ingress only.
Rebinding text carries integer keys, while recording deserialization already
rejects invalid float dt/mouse fields. Preserve their formats and behavior.
Controller/analog absence and Unicode/IME/modifier/focus/subframe/native gaps
are outside this bounded numerical repair and remain unverified.

Primary references read October 17:
[C++ floating conversion contract](https://eel.is/c++draft/conv.double) and
[MSVC cmath](https://learn.microsoft.com/en-us/cpp/standard-library/cmath?view=msvc-170).
Guard the narrowing domain explicitly; double arithmetic and `std::hypot`
provide ample range for sums of finite float contributions in an in-memory
binding vector. No new dependency is needed.

## Acceptance, commands and stop

1. Reproduce the two-binding overflow through a real GameHost loading original
   disposable neutral scene/project files; retain the failing command/log.
2. Verify direct context insertion and rejection are transactional. Reject
   nonfinite/out-of-range JSON axes before narrowing and preserve prior parsed
   input/project outputs on failure.
3. Check all ActionState timing/axis fields with largest finite positive dt,
   invalid dt, huge/tiny/cancelling bindings, priorities, edges and rebinding.
4. Run focused native Debug and Release suites, retaining all checks, then
   independent review and coordinator-owned integration verification.
5. Stop at a deterministic blocker; retain failed attempts. No source freeze
   or test receipt is a merge or complete-engine acceptance receipt.

Request the coordinator's build slot before configuring, compiling or running
native tests. External multi-config Debug/Release root is
`C:/Users/lucas/Documents/Codex/2026-10-17/files-pasted-by-the-user-you/work/build-finite-input`,
unless the coordinator explicitly assigns another compatible cache. Logs go
to `work/finite-input-evidence`, outside the source worktree.

```powershell
& $cmake -S $worktree -B $build -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
& $cmake --build $build --config Debug --target EngineSystemsTests EngineSampleTests --parallel 2
& "$build/Debug/EngineSampleTests.exe" HostExtremeFiniteAxisBindingsRemainDirectional
& $ctest --test-dir $build -C Debug --output-on-failure -V -R '^Engine(Systems|Sample)Tests$' --no-tests=error
& $cmake --build $build --config Release --target EngineSystemsTests EngineSampleTests --parallel 2
& $ctest --test-dir $build -C Release --output-on-failure -V -R '^Engine(Systems|Sample)Tests$' --no-tests=error
git diff --check
git diff --name-only
& $python Scripts/test_test_safety.py
& $python Scripts/verify_milestone1.py
& $python Scripts/verify_milestone2.py
& $python Scripts/verify_milestone3.py
```

CMake/CTest are from the installed VS2022 Community CMake bin directory;
resolve variables to literal paths and retain exact commands/results in QA.
The coordinator owns broader full-all builds, native desktop and GPU tests.
This owner commits scoped work only; no push or merge.
