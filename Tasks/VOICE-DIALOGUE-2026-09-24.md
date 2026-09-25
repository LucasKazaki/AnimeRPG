# VD-001: voices, protagonist scope, dialogue presentation and original UI audio

Date: September 24, 2026 (America/New_York).
Observed upstream: LucasKazaki/AnimeRPG main at
`2958741188279a0b438dd489ad00cac546012c25`.
Dedicated branch/worktree: `feature/voices-dialogue-20260924`.
This is an isolated source slice, not a full production checkout. Direct GitHub
clone failed DNS; use connector-fetched source and exact blob checks instead.

## Operator request
Lucas requests AI-generated character voices in English and Japanese initially,
distinct personalities and voices for every character, only the protagonist
playable, optional armor/tools/weapons/cosmetics gacha, a Genshin/Pokemon-like
advance cue, shorter conversations and story-directed facial expressions.
The exact reference recording and reuse permission were not supplied. Produce an
original short synthetic cue with a similar interaction role, not ripped audio.

## Allowed scope
Append the new explicit direction to AGENTS.md and GAME_DEVELOPMENT_CONTROL.md;
preserve every previous byte. Add only:
- Docs/Game/CHARACTERS-VOICES-DIALOGUE.md
- Docs/Engine/DIALOGUE-AUDIO-EXPRESSION-CONTRACT.md
- Docs/Research/CHARACTER-VOICE-TOOLS-2026-09-24.md
- Content/Narrative/voice-dialogue-contract.v1.json
- Content/Starter/Audio/UI/README.md
- Content/Starter/Audio/UI/expected-manifest.json
- Tools/DialoguePresentation/generate_advance_cue.py
- Tools/DialoguePresentation/validate_contract.py
- Tests/DialoguePresentation/test_contract.py
- Tests/DialoguePresentation/test_advance_cue.py
- Docs/QA/VOICE-DIALOGUE-2026-09-24.md
- this task.

Do not edit live Engine/Scene dialogue/combat, renderer, audio backend, root CMake,
workflows, hourly state, other PRs, dependencies or the UE5 experiment. Do not
install models, purchase API credits, clone an actor, launch a local worker,
change monetization, deploy, release or merge under this packet.

## Acceptance and stop
Publish instructions, strict authoring-fixture validation, original audio source
and exact output pin. Generated WAV is a build artifact, not a second tracked
copy. Publish a listening sample in this conversation. Tests verify the source
fixture and PCM contract only, not voice quality, playback or gameplay.
Stop at a draft PR and coordinated engine/game issues plus independent review
request. Keep runtime acceptance and model auditions explicitly pending.

## Commands, from worktree root
```
python Tools/DialoguePresentation/validate_contract.py Content/Narrative/voice-dialogue-contract.v1.json
python -m unittest discover -s Tests/DialoguePresentation -p 'test_*.py' -v
python Tools/DialoguePresentation/generate_advance_cue.py --output ../dialogue-audio-preview
python Tools/DialoguePresentation/generate_advance_cue.py --output ../dialogue-audio-preview --check
python -m py_compile Tools/DialoguePresentation/*.py Tests/DialoguePresentation/*.py
```
Output directory must be outside the worktree. No overwrite by default. Exact
manifest matching is required; do not repin silently after a failed check.
Native follow-ups must retain Windows Debug/Release regression, actual audio
playback, per-locale lip sync, facial blending, skip/choice/reward correctness,
controller/mouse/accessibility and timing evidence. No native tests run here.
