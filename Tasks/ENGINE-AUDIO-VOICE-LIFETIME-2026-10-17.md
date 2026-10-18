# Keep one-shot audio valid through teardown and restart

Owner `/root`; independent reviewer `/root/candidate_review`.
Dedicated worktree `work/astral-audio-lifetime`, branch
`engine/audio-voice-lifetime-20261017`, deliberate stabilized base
`964a8c46b8469e549dd57ebeaadd7b224a350c30`.
Lucas's October17 engine-only implementation authority applies; independent
review, native checks and integration gates remain required.

## Problem and safe reproduction

Integration source review found that the reusable Playground validation sample
owns its chime as a behavior shared_ptr but passes chime_.get() to PlaySoundAt.
The world forwards that borrowed pointer to a mixer voice and does not retain
or stop one-shot voices when its behavior/world is removed. The mixer audio
thread subsequently dereferences the clip. This can outlive the clip during an
immediate restart or behavior removal. This is a source finding until the
bounded old predicate below executes; do not intentionally mix freed storage.

Prepare a small neutral test behavior that owns a procedural clip, plays it
through existing World::PlaySoundAt in OnCreate, and exposes its VoiceId plus
weak clip witness. Destroy the world before playback completes, then require
that the shared external mixer no longer plays its voice. Read IsPlaying only;
never Mix after the old clip is freed. Retain the expected old failure before
production edits. The same principle covers scene restart without a real device.

## Ownership and allowed paths

Own only `Engine/Audio/AudioMixer.h`, `Engine/Audio/AudioMixer.cpp`,
`Engine/Framework/GameWorld.h`, `Engine/Framework/GameWorld.cpp`,
`Game/Samples/Playground/PlaygroundBehaviours.cpp` (one shared-owner call),
`Tests/EngineWorldTests.cpp`, `Tests/EngineSystemsTests.cpp`, this task and
`Docs/QA/ENGINE-AUDIO-VOICE-LIFETIME-2026-10-17.md`.
Current lifecycle/Rigidbody source is reviewed and frozen. No other agent writes
these paths. Preserve all other worktrees and current input/reload/editor guards.
No game content or sound-design change, new dependency, policy or CMake change.

## Contract and approach

Mixer voices may explicitly own shared_ptr<const AudioClip>; add an owning Play
overload and retain the low-level borrowed overload with a clear lifetime
contract. Hold/release the owner under the existing mixer mutex, including Stop,
completion and voice stealing; preserve generation-safe VoiceIds and behavior.
The shared clip must remain immutable during playback, as with existing samples.

World PlaySoundAt gets a shared-owner overload used by the actual reusable
sample. Preserve the existing raw-pointer world API through a safe clip snapshot
before enqueueing. Track this world's one-shot VoiceIds, reap inactive/stolen
IDs, and stop remaining voices after all teardown callbacks (which may themselves
play a final sound). Other worlds/external voices on the same mixer survive.
AudioSource must pass its existing owner into the owning mixer overload.
Do not stop all mixer voices or silently retain dead clip owners indefinitely.

## Acceptance and exact scheduled commands

Use coordinator's single compiler/test slot, installed VS2022 x64 CMake/CTest,
two jobs and external `work/build-audio-lifetime`. Retain logs under
`work/audio-lifetime-evidence`. First configure/build only the old World test
filter and retain its assertion exit. After the old witness, implement and run
affected Systems/World/Framework/Sample full suites in Debug and Release, with
assertions active, unchanged safety3/3 and M1-3. Stop on unexpected deterministic
failure and diagnose before a materially changed retry.

```powershell
& $cmake -S $source -B $build -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON
& $cmake --build $build --config Debug --target EngineWorldTests --parallel 2
& "$build/Debug/EngineWorldTests.exe" WorldTeardownStopsOwnedOneShots
# Only after expected old failure and independent review of the approach:
& $cmake --build $build --config Debug --target EngineSystemsTests EngineWorldTests EngineFrameworkTests EngineSampleTests --parallel 2
& $ctest --test-dir $build -C Debug -R '^Engine(Systems|World|Framework|Sample)Tests$' --output-on-failure -V --no-tests=error
& $cmake --build $build --config Release --target EngineSystemsTests EngineWorldTests EngineFrameworkTests EngineSampleTests --parallel 2
& $ctest --test-dir $build -C Release -R '^Engine(Systems|World|Framework|Sample)Tests$' --output-on-failure -V --no-tests=error
& $python "$source/Scripts/test_test_safety.py"
& $python "$source/Scripts/verify_milestone1.py"
& $python "$source/Scripts/verify_milestone2.py"
& $python "$source/Scripts/verify_milestone3.py"
git diff --check
git diff --name-only
```

Fixed regressions exercise actual sample-compatible shared and legacy raw world
paths, release of originating behavior, actual Mix only with repaired lifetime,
Stop/end/steal/reject ownership release, destruction callbacks, generation reuse,
and isolation across worlds sharing one mixer. Existing AudioSource behavior
must remain valid. Obtain independent final source/docs/log/commit review before
publishing and integrated main adoption. Hosted sanitizers supplement local
consumer tests; this packet does not claim actual device-thread/native audio,
GPU, performance, packaging, soak or full workflow acceptance.
