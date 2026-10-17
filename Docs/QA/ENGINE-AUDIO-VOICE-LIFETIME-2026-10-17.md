# One-shot audio lifetime evidence

Owner `/root`; independent reviewer `/root/candidate_review`.
Task: `Tasks/ENGINE-AUDIO-VOICE-LIFETIME-2026-10-17.md`.
Deliberate base964a8c46b8469e549dd57ebeaadd7b224a350c30.

## Reproduced old failure

On DESKTOP-AKPLL81, October17 19:09:15–19:09:50 UTC, a fresh VS2022 x64 configure
and two-job Debug EngineWorldTests build exited0 (3.625/31.375s).
The neutral WorldTeardownStopsOwnedOneShots filter then failed the expected
`!mixer.IsPlaying(witness.voice)` assertion with child exit3 (0.032s). The
originating behavior's weak clip witness had already expired. Only IsPlaying
was called after teardown; Mix never dereferenced the released clip. Production
was unchanged; the old Engine/Tests test-only patchSHA256 is
cbe325f9f0bd4c327840d17e7cfb10e07bae03cd89f17c80ec7a2d72211a9d64.
Logs and exact argv/source/time/exits remain in
`work/audio-lifetime-evidence/old-{configure,debug-build,teardown}.*`.

## Implemented candidate and fixed checks

Mixer voices can now retain shared ownership of immutable clips. Stop,
completion and replacement release the owner under the same mutex used by Mix.
The old low-level borrowed Play overload explicitly requires its caller to
retain immutable storage until the voice stops. World PlaySoundAt offers an
owning overload; its legacy raw-pointer overload takes a safe snapshot before
enqueuing. AudioSource and the actual reusable Playground chime forward their
existing owners. The sample's sound/content and behavior remain unchanged.

World one-shot IDs are reaped before subsequent enqueue; capacity is reserved
before Play so allocation failure cannot strand an untracked voice. Destruction
stops remaining generation-bearing IDs after all OnDestroy callbacks. Other
worlds and external replacement voices sharing the mixer remain independent.
The externally supplied mixer must still outlive its GameWorld, as before.

The executed regressions cover the old teardown witness, playback after behavior
removal, raw snapshot content after original mutation/release, callback-created
sounds, shared-world isolation and generation reuse, AudioSource clip ownership,
completion/Stop/steal/rejected-play release, and actual worker-thread Mix calls
concurrent with owning Play/Stop. Independent review found that an initial
all-finite threaded fixture could pass on silence if mixing finished too soon.
It was strengthened before execution: every one of128 play phases requires
an acknowledged audible worker Mix before Stop, with finite/audible/count checks.
The worker keeps mixing while control releases/replaces voices between phases.
Independent source/test pre-review was then clean. There was no production
correctness finding and no post-fix deterministic failure.

## Actual native validation

One exclusive coordinator slot ran October17 19:14:06–19:16:37 UTC. All receipt
source hashes include Engine, Game and Tests and equal
`9c56368e1535d7cb5e8417e40c0651a9db743fcac1972f2de15f21c97e30b772`.
No source/test edits occurred while the matrix executed.

| Gate | Actual result | External log prefix |
|---|---|---|
| Debug affected build | exit0,23.016s, two jobs | fixed-Debug-build |
| Debug four registered suites | 4/4,63.86s;81 cases | fixed-Debug-test |
| Release affected build | exit0,55.578s, two jobs | fixed-Release-build |
| Release four registered suites | 4/4,1.21s;81 cases | fixed-Release-test |
| Unchanged test safety | 3/3,6.205s; exit0 | fixed-safety |
| M1/M2/M3 static verifiers | exit0 each | fixed-milestone1/2/3 |

Each configuration ran World27+Systems20+Framework14+Sample20 cases. All six
new World and three new Systems cases pass, including the original witness and
actual Mix consumers. Existing lifecycle/Rigidbody/input/reload and reusable
sample play/restart tests ran unchanged. Assertions and safety registration are
unchanged; Release retains the expected /UNDEBUG override. Exact commands are
in the task and each JSON argv/time/SHA/exit receipt under the external evidence
directory. The enclosing session49620 exited0. No actual historical R0 ran.

Final committed source/docs/log independent binding and combined/hosted gates
remain pending. No current device-thread/native audio, performance,
package/GPU/soak or complete development-workflow acceptance claim.

The shared path does not duplicate PCM. Legacy raw World calls copy their clip
once per play; frequent callers should pass existing shared ownership. The
actual validation sample now does so. This is a deliberate compatibility cost,
not an unmeasured performance improvement. No mixer/clip format redesign.

Primary language contract consulted: [shared ownership semantics](https://eel.is/c++draft/util.smartptr.shared).
Shared ownership protects lifetime; callers must separately honor immutable
sample data during playback. No post-C++17 language feature is used.
