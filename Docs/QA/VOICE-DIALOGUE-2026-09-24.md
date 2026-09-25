# VD-001 author verification receipt

September 24, 2026. Baseline inspected with the GitHub connector:
`2958741188279a0b438dd489ad00cac546012c25`.
Dedicated local worktree/branch: `feature/voices-dialogue-20260924`.

## What ran
Python 3.13.5 on Linux. All verification commands below exited 0:
```
python Tools/DialoguePresentation/validate_contract.py Content/Narrative/voice-dialogue-contract.v1.json
python -m unittest discover -s Tests/DialoguePresentation -p 'test_*.py' -v
python Tools/DialoguePresentation/generate_advance_cue.py --output ../dialogue-audio-preview
python Tools/DialoguePresentation/generate_advance_cue.py --output ../dialogue-audio-preview --check
python -m py_compile Tools/DialoguePresentation/*.py Tests/DialoguePresentation/*.py
```
35/35 standard-library tests passed: 26 authoring-contract tests and 9 source-audio
tests. This verifies fixture invariants and original PCM generation, NOT in-game
single-character control, voiced conversations, facial animation or audio quality.
No tests were skipped in this Linux run. Native tests and root CI registration are
not changed; ordinary green root CI does not mean it executed these new tests.

The cue generated and exact --check passed. It is 48 kHz mono PCM16, 5,280 frames,
110 ms, 10,604 bytes. SHA-256:
`5e59dc027766133335bfcce8f1625d167631f038d2849ed3304a863866340834`.
The source requires no network, third-party samples or TTS. Tests check pinned
bytes, amplitude/DC/onset/tail bounds, corruption, overwrite refusal, symlink
checks, source-root output refusal and CRLF pin handling. A listening preview was
created; no perceptual comparison or human approval is claimed.

## Source provenance and scope verification
Direct HTTPS git access failed DNS. The workspace is an isolated source slice,
not a full repository checkout. Connector-fetched AGENTS.md and
GAME_DEVELOPMENT_CONTROL.md baseline bytes were reconstructed and matched their
Git blobs before appending instructions:
- AGENTS.md: `8c78105ab8f78ec220df072fce0155f851756794`
- GAME_DEVELOPMENT_CONTROL.md: `3f0982bd53f453a5a78647c6cdedc54cec655580`
Every old byte is preserved as a prefix. Newly added local Markdown links resolve.
One initial task-display command used the base directory rather than the worktree
and failed to find the file; displaying its absolute worktree path succeeded
before implementation. This was not a test-suite failure.

14 scoped files total: two append-only control updates and twelve new files.
No live Engine/Scene implementation, root CMake, CI, renderer/audio backend,
existing hourly records, dependency, deployment or other PR is changed. The WAV
is intentionally generated outside the source tree; its generator and exact
manifest pin, not duplicate derived audio, are the proposed repository content.

## Unverified and pending
All six character/locale voice slots are unassigned. No voice model was installed
or executed, no API was called, and no character takes or phoneme tracks were
created. Japanese sample text is a draft awaiting native review. This is not a
final cast, commercial-rights approval, native Windows/full-regression result,
listening approval, runtime feature or proof of Genshin/Pokemon sound similarity.
The JSON validator is deliberately limited to this source fixture; a shipping
voice manifest needs a separately reviewed schema and evidence gate. It cannot
infer acoustic distinctness or prove licensing from metadata.
Independent review is pending, not replaced by this author verification. Publish
as a draft PR, request exact-head review, and queue engine/game integration. Do
not merge or claim playable acceptance under VD-001.

## Exact authored Git blob identities
The receipt itself is excluded from this list to avoid a self-hash loop.
- `AGENTS.md`: `15905c0cd634f3323dc99b6e99950c2888fe43db`
- `Content/Narrative/voice-dialogue-contract.v1.json`: `c1fd20761e4acec18007c77eb05c8020a9edd979`
- `Content/Starter/Audio/UI/README.md`: `f18a1ae7642c136262afb8b35686d404318d8782`
- `Content/Starter/Audio/UI/expected-manifest.json`: `6911211ae7db516a639ad4de41a9b0ff31c0301b`
- `Docs/Engine/DIALOGUE-AUDIO-EXPRESSION-CONTRACT.md`: `9ec5e51147e814581fcb9137455130bf09254eee`
- `Docs/Game/CHARACTERS-VOICES-DIALOGUE.md`: `7dfbd0f453c53006e1fb010d45ee75718abc9bab`
- `Docs/Research/CHARACTER-VOICE-TOOLS-2026-09-24.md`: `b900e0a65d92de8ad4a210ff281c9a7901f84394`
- `GAME_DEVELOPMENT_CONTROL.md`: `32b1a1b6e5590fe794d1ba587a22b2aa6634fdd4`
- `Tasks/VOICE-DIALOGUE-2026-09-24.md`: `b54c6903f85ac95a5ac0b715f4c67585092a1894`
- `Tests/DialoguePresentation/test_advance_cue.py`: `a19e31535f50597538324e70cdb4e7106b0d8747`
- `Tests/DialoguePresentation/test_contract.py`: `636bd03e2852b9f9e7ece36264b1601d255e8280`
- `Tools/DialoguePresentation/generate_advance_cue.py`: `6196ae3ce9569eebbce55b1933e31269e55d142d`
- `Tools/DialoguePresentation/validate_contract.py`: `281fa77c4eefeb05e15533a5cdb0a06f210b9507`
